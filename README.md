This tiny program waits for the `tty` size to be set correctly before
trying to start your program.

Docker doesn't initialise the terminal before starting a container, instead
the terminal is attached after the container has started. This leads to
a race condition when the program needs to know how big its terminal is.
See docker/docker#25450.

All this program does is waits for up to five seconds for the size of the
terminal to be set correctly, then calls `execvp` with whatever arguments
you pass.
