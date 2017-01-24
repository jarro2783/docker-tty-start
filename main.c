// This code waits until a valid terminal size is set before starting
// the program requested.
// This should not be necessary once docker starts containers with the
// correct terminal size. See https://github.com/docker/docker/issues/25450

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static volatile int size_changed = 0;

void handler(int unused) {
    size_changed = 1;
}

int main(int argc, char** argv) {

    if (argc < 2) {
        fprintf(stderr, "You must input a program to run\n");
        exit(1);
    }

    // here we need to block SIGWINCH, setup the signal handler,
    // then call pselect with SIGWINCH unblocked so that we can
    // get the signal while we are waiting.
    // If we don't use pselect we get a race condition between unblocking
    // the signal and entering select.
    sigset_t empty_mask;
    sigset_t mask;
    sigemptyset(&mask);
    sigemptyset(&empty_mask);
    sigaddset(&mask, SIGWINCH);
    sigprocmask(SIG_BLOCK, &mask, 0);

    //we then need to get the screen size first to know if we even need
    //to wait.
    struct winsize size;
    int result = ioctl(0, TIOCGWINSZ, &size);

    if (result == -1) {
        perror("ioctl");
        exit(1);
    }

    if (size.ws_row == 0 && size.ws_col == 0) {

        struct timespec timeout = {
            5, 0
        };

        signal(SIGWINCH, handler);

        int result = pselect(0, 0, 0, 0, &timeout, &empty_mask);

        switch (result) {
            case 0:
            fprintf(stderr, "Didn't get a screen size change in time");
            exit(1);
            break;

            case -1:
            if (!size_changed) {
                fprintf(stderr, "The size wasn't changed.");
                exit(1);
            }
            break;
        }

        // reset to the default handling before we exec
        signal(SIGWINCH, SIG_DFL);
        sigprocmask(SIG_UNBLOCK, &empty_mask, 0);
    }

    execvp(argv[1], &argv[1]);

    return 1;
}
