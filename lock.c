#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <X11/extensions/scrnsaver.h>
#include <glib.h>

#include "config.h"
#include "dbus-listener.h"

guint timeout;

Display *getDpy()
{
    static Display *dpy;
    if (dpy == NULL)
    {
        if (!(dpy = XOpenDisplay(0)))
        {
            // TODO error
        }

        int base, errbase;
        if (!XScreenSaverQueryExtension(dpy, &base, &errbase))
        {
            // TODO error
        }
    }

    return dpy;
}

void runLock()
{
    int pid = fork();
    if (pid == 0)
    {
        execv(PROGRAM[0], PROGRAM);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
}

gboolean lock(gpointer data)
{
    Display *dpy = getDpy();
    XScreenSaverInfo *info = XScreenSaverAllocInfo();
    XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), info);

    switch (info->state)
    {
        case ScreenSaverOff:
        {
            unsigned long time = info->til_or_since;
            printf("time remaining: %lu\n", time);
            if (time / 1000 <= 0)
            {
                if (inhibited == 0)
                {
                    runLock();
                }
                timeout = g_timeout_add(TIMEOUT * 1000, (GSourceFunc)lock, NULL);
            }
            else
            {
                printf("Sleeping for %lu\n", time);
                timeout = g_timeout_add(time, (GSourceFunc)lock, NULL);
            }
            break;
        }
        case ScreenSaverOn:
            runLock();
            // Fall through
        case ScreenSaverDisabled:
            printf("Sleeping for %d\n", TIMEOUT * 1000);
            timeout = g_timeout_add(TIMEOUT * 1000, (GSourceFunc)lock, NULL);
            break;
    }

    XFree(info);
    return FALSE;
}
