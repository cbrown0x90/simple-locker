#ifndef GRANDPARENT_H
#define GRANDPARENT_H

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

DBusConnection *init_dbus(void);

#endif
