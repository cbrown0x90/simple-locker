#pragma once

#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>

typedef struct
{
    dbus_uint32_t inhibit_last_cookie;
    GHashTable *inhibit_list;
} Listener;

typedef struct
{
    gchar* owner;
    gchar* name;
} HashValue;

extern int inhibited;
extern Listener listener;
extern pthread_mutex_t listener_mutex;

DBusConnection *init_dbus(void);
