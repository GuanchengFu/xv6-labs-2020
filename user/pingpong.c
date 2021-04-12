#include "user/user.h"

void report_error(int status, const char *reason) {
    if (status < 0) {
        fprintf(2, reason);
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    // Create a pipe.
    int status;
    int p[2];
    status = pipe(p);
    report_error(status, "Create pipe failed!");

    int p2[2];
    status = pipe(p2);
    report_error(status, "Create pipe failed!");

    char buf[1];
    int pid = fork();
    report_error(pid, "Fork failed");
    if (pid == 0) {
        //Child process receive first.
        close(p[1]);
        // Try to read one byte from the buffer.
        int bytes_read = read(p[0], buf, 1);
        close(p[0]);
        if (bytes_read == 1) {
            close(p2[0]);
            pid = getpid();
            fprintf(1, "%d: received ping\n", pid);
            int bytes_write = write(p2[1], buf, 1);
            if (bytes_write <= 0) {
                close(p2[1]);
                exit(1);
            }
            close(p2[1]);
        } else {
            close(p2[1]);
            close(p2[0]);
            exit(1);
        }
    } else {
        // write to the pipe.
        buf[0] = 'f';
        write(p[1], buf, 1);
        close(p[0]);
        close(p[1]);
        // Now try to receive from the other pipe.
        close(p2[1]);
        status = read(p2[0], buf, 1);
        if (status < 0) {
            close(p2[0]);
            exit(1);
        }
        pid = getpid();
        fprintf(1, "%d: received pong\n", pid);
    }
    exit(0);
}
