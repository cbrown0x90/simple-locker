#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
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
        int base = 0;
        int errbase = 0;
        dpy = XOpenDisplay(0);
        XScreenSaverQueryExtension(dpy, &base, &errbase);
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

gboolean removeDead(gpointer key, gpointer value, gpointer user_data)
{
    HashValue* hashValue = (HashValue*)value;
    gchar* name = hashValue->name;

    DIR* proc = opendir("/proc");
    struct dirent* cur_dir;
    while ((cur_dir = readdir(proc)) != NULL)
    {
        // Check if the first character is a number
        if (!isdigit(*cur_dir->d_name))
        {
            continue;
        }

        char comm_path[268];
        snprintf(comm_path, sizeof(comm_path), "/proc/%s/comm", cur_dir->d_name);
        FILE* comm = fopen(comm_path, "r");
        if (comm != NULL)
        {
            char comm_val[1024];
            size_t num_read = fread(comm_val, 1, sizeof(comm_val), comm);
            // -1 to not compare the new line character
            if (strncmp(name, comm_val, num_read - 1) == 0)
            {
                closedir(proc);
                fclose(comm);
                return FALSE;
            }
            fclose(comm);
        }
    }

    closedir(proc);
    printf("Removing: %s\n", name);
    return TRUE;
}

gboolean removeDeadMap(gpointer user_data)
{
    pthread_mutex_lock(&listener_mutex);
    g_hash_table_foreach_remove(listener.inhibit_list, removeDead, NULL);
    if (g_hash_table_size(listener.inhibit_list) == 0 && inhibited == 1)
    {
        Display *dpy = getDpy();
        XScreenSaverSuspend(dpy, 0);
        inhibited = 0;
        puts("Removed all dead\n");
    }
    pthread_mutex_unlock(&listener_mutex);

    return TRUE;
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
                pthread_mutex_lock(&listener_mutex);
                if (inhibited == 0)
                {
                    pthread_mutex_unlock(&listener_mutex);
                    runLock();
                }
                else
                {
                    pthread_mutex_unlock(&listener_mutex);
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
