CFLAGS = -Wall -Werror -c
LDFLAGS =

CFLAGS += `pkg-config --cflags dbus-1`
CFLAGS += `pkg-config --cflags dbus-glib-1`
CFLAGS += `pkg-config --cflags gio-2.0`

LDFLAGS += `pkg-config --libs dbus-1`
LDFLAGS += `pkg-config --libs dbus-glib-1`
LDFLAGS += `pkg-config --libs gio-2.0`
LDFLAGS += `pkg-config --libs x11`
LDFLAGS += `pkg-config --libs xscrnsaver`

SOURCES = main.c dbus-listener.c lock.c
OBJECTS = $(SOURCES:.c=.o)

EXECUTABLE = simple-locker

all: $(SOURCES) $(EXECUTABLE)

debug: CFLAGS += -g -Og -ggdb
debug: all

$(EXECUTABLE): $(OBJECTS)
	gcc $(OBJECTS) -o $@ $(LDFLAGS)

.c.o:
	gcc $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)
