#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>
#include <stdio.h>
#include <X11/extensions/scrnsaver.h>

#include "dbus-listener.h"
#include "lock.h"

int inhibited = 0;

static Listener listener;

void free_value(void* value)
{
    if (value != NULL)
    {
        HashValue* hashValue = (HashValue*)value;
        g_free(hashValue->owner);
        g_free(hashValue->name);
        free(hashValue);
    }
}

void gs_listener_init(Listener *listener)
{
    listener->inhibit_list = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, free_value);
}

/*
 * This is the XML string describing the org.freedesktop.ScreenSaver
 * interfaces, methods and implemented. It's used by the
 * 'Introspect' method of 'org.freedesktop.DBus.Introspectable'
 * interface.
 */
const char *server_introspection_xml = DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
    "<node>\n"

    "  <interface name='org.freedesktop.DBus.Introspectable'>\n"
    "    <method name='Introspect'>\n"
    "      <arg name='data' type='s' direction='out' />\n"
    "    </method>\n"
    "  </interface>\n"

    "  <interface name='org.freedesktop.ScreenSaver'>\n"
    "    <method name=\"Inhibit\">\n"
    "      <arg name=\"cookie\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"application_name\" direction=\"in\" type=\"s\"/>\n"
    "      <arg name=\"reason_for_inhibit\" direction=\"in\" type=\"s\"/>\n"
    "    </method>\n"
    "    <method name=\"UnInhibit\">\n"
    "      <arg name=\"cookie\" direction=\"in\" type=\"u\"/>\n"
    "    </method>\n"
    "  </interface>\n"

    "</node>\n";

void raise_property_type_error(DBusConnection *connection, DBusMessage *in_reply_to, const char *device_id)
{
    char buf[512];
    DBusMessage *reply;

    snprintf(buf, 511, "Type mismatch setting property with id %s", device_id);

    reply = dbus_message_new_error(
        in_reply_to, "org.freedesktop.ScreenSaver.TypeMismatch", buf);
    if (reply == NULL)
    {
        g_error("No memory");
    }
    if (!dbus_connection_send(connection, reply, NULL))
    {
        g_error("No memory");
    }

    dbus_message_unref(reply);
}

dbus_uint32_t gs_listener_add_inhibit(Listener *listener, const char *owner, const char* application)
{
    dbus_uint32_t *cookie;

    cookie = g_new(dbus_uint32_t, 1);
    if (cookie == NULL)
    {
        g_error("No memory");
    }

    *cookie = ++listener->inhibit_last_cookie;

    if (g_hash_table_size(listener->inhibit_list) == 0)
    {
        Display *dpy = getDpy();
        XScreenSaverSuspend(dpy, 1);
        inhibited = 1;
        puts("Inhibit");
    }

    HashValue* value = (HashValue*)malloc(sizeof(HashValue));
    value->owner = g_strdup(owner);
    value->name = g_strdup(application);

    // TODO struct that contains the owner and application
    g_hash_table_insert(listener->inhibit_list, cookie, value);

    return *cookie;
}

void gs_listener_remove_inhibit(Listener *listener, dbus_uint32_t cookie, const char *owner)
{
    const HashValue* value = g_hash_table_lookup(listener->inhibit_list, &cookie);

    if (value->owner == NULL)
    {
        return;
    }

    if (strcmp(owner, value->owner) != 0)
    {
        return;
    }

    g_hash_table_remove(listener->inhibit_list, &cookie);

    if (g_hash_table_size(listener->inhibit_list) == 0)
    {
        Display *dpy = getDpy();
        XScreenSaverSuspend(dpy, 0);
        inhibited = 0;
        puts("UnInhibit");
    }
}

DBusHandlerResult listener_inhibit(Listener *listener, DBusConnection *connection, DBusMessage *message, DBusMessage *reply)
{
    const char *path;
    int type;
    DBusMessageIter iter;
    dbus_uint32_t cookie;
    const char *application;

    path = dbus_message_get_path(message);

    dbus_message_iter_init(message, &iter);
    type = dbus_message_iter_get_arg_type(&iter);

    if (type != DBUS_TYPE_STRING)
    {
        raise_property_type_error(connection, message, path);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    dbus_message_iter_get_basic(&iter, &application);
    printf("%s\n", application);

    dbus_message_iter_next(&iter);
    type = dbus_message_iter_get_arg_type(&iter);

    if (type != DBUS_TYPE_STRING)
    {
        raise_property_type_error(connection, message, path);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    cookie = gs_listener_add_inhibit(listener, dbus_message_get_sender(message), application);

    dbus_message_append_args(reply, DBUS_TYPE_UINT32, &cookie, DBUS_TYPE_INVALID);

    return DBUS_HANDLER_RESULT_HANDLED;
}

DBusHandlerResult listener_uninhibit(Listener *listener, DBusConnection *connection, DBusMessage *message)
{
    const char *path;
    int type;
    DBusMessageIter iter;
    dbus_uint32_t cookie = 0;

    path = dbus_message_get_path(message);

    dbus_message_iter_init(message, &iter);
    type = dbus_message_iter_get_arg_type(&iter);

    if (type != DBUS_TYPE_UINT32)
    {
        raise_property_type_error(connection, message, path);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    dbus_message_iter_get_basic(&iter, &cookie);

    gs_listener_remove_inhibit(listener, cookie, dbus_message_get_sender(message));

    if (dbus_message_get_no_reply(message))
    {
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

/*
 * This function implements the 'org.freedesktop.ScreenSaver' interface for the
 * 'Server' DBus object.
 *
 * It also implements 'Introspect' method of
 * 'org.freedesktop.DBus.Introspectable' interface which returns the
 * XML string describing the interfaces, methods, and signals
 * implemented by 'Server' object. This also can be used by tools such
 * as d-feet(1) and can be queried by:
 *
 * $ gdbus introspect --session --dest org.freedesktop.ScreenSaver --object-path /ScreenSaver
 * /org/example/TestObject
 */
DBusHandlerResult server_message_handler(DBusConnection *conn, DBusMessage *message, void *data)
{
    DBusHandlerResult res = DBUS_HANDLER_RESULT_HANDLED;

    DBusError err;
    dbus_error_init(&err);

    DBusMessage *reply = dbus_message_new_method_return(message);
    if (reply == NULL) goto fail;

    if (dbus_message_is_method_call(message, "org.freedesktop.ScreenSaver", "Inhibit"))
    {
        res = listener_inhibit(&listener, conn, message, reply);
    }
    else if (dbus_message_is_method_call(message, "org.freedesktop.ScreenSaver", "UnInhibit"))
    {
        res = listener_uninhibit(&listener, conn, message);
    }
    else if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect"))
    {

        dbus_message_append_args(reply,
                     DBUS_TYPE_STRING, &server_introspection_xml,
                     DBUS_TYPE_INVALID);
    }
    else
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

fail:
    if (reply == NULL)
    {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    if (res == DBUS_HANDLER_RESULT_HANDLED)
    {
        if (!dbus_connection_send(conn, reply, NULL))
        {
            g_error("No memory");
            res = DBUS_HANDLER_RESULT_NEED_MEMORY;
        }
    }

    dbus_message_unref(reply);

    return res;
}

const DBusObjectPathVTable server_vtable = {
    .message_function = server_message_handler
};

DBusConnection *init_dbus()
{
    gs_listener_init(&listener);

    DBusConnection *conn;
    DBusError err;
    int rv;

    dbus_error_init(&err);

    /* connect to the daemon bus */
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (!conn)
    {
        fprintf(stderr, "Failed to get a session DBus connection: %s\n", err.message);
        goto fail;
    }

    rv = dbus_bus_request_name(conn, "org.freedesktop.ScreenSaver",
                               DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (rv != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        fprintf(stderr, "Failed to request name on bus: %s\n", err.message);
        goto fail;
    }

    if (!dbus_connection_register_object_path(conn, "/ScreenSaver",
                                              &server_vtable, NULL))
    {
        fprintf(stderr, "Failed to register a object path for 'ScreenSaver'\n");
        goto fail;
    }

    return conn;

fail:
    dbus_error_free(&err);
    return NULL;
}
