#pragma once
#include <X11/extensions/scrnsaver.h>

extern guint timeout;

Display *getDpy();
gboolean lock(gpointer data);
gboolean removeDeadMap(gpointer user_data);
