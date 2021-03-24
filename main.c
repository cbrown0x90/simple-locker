#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <X11/extensions/scrnsaver.h>

#include "dbus-listener.h"
#include "lock.h"

GMainLoop *mainloop;

int main(void)
{
    // Setup the dbus connection
    DBusConnection *conn = init_dbus();
    if (conn == NULL)
    {
        return EXIT_FAILURE;
    }

    /*
     * For the sake of simplicity we're using glib event loop to
     * handle DBus messages. This is the only place where glib is
     * used.
     */
    mainloop = g_main_loop_new(NULL, 0);
    /* Set up the DBus connection to work in a GLib event loop */
    dbus_connection_setup_with_g_main(conn, NULL);
    /* Add the lock function to the main loop */
    timeout = g_timeout_add(1, (GSourceFunc)lock, NULL);
    /* Start the glib event loop */
    g_main_loop_run(mainloop);

    return EXIT_SUCCESS;
}
