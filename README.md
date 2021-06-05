# simple-locker
This is a simple daemon that locks your screen after a period of inactivity. It works by running a progam as configured in `config.h` when the the Xorg inactivity timer runs out. The Xorg timer can set by running the command `xset s $NUMSECONDS`. The locker also has dbus support, so the watching a video on Firefox or Google Chrome will pause the lock timer.

## Building
Xorg and dbus libraries are required for building

## TODO
* Add dbus introspection
* Add comments
* Remove all the TODOs
