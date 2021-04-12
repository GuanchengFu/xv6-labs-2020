#include "user.h"

void task(int *pipes) {
    // Keep reads from the left pipe.
    close(pipes[1]);
    int first_number_flag = 1;
    int pipe_created_flag = 1;
    int first_number = 0;
    int p[2];
    int num_received;
    int bytes_read;
    while ((bytes_read = read(pipes[0], &num_received, 4)) != 0) {
        if (first_number_flag == 1) {
            first_number = num_received;
            fprintf(1, "prime %d\n", first_number);
            first_number_flag = 0;
        } else {
            int number_read = num_received;
            //printf("first_number: %d\n", first_number);
            if (number_read % first_number == 0) {
                continue;
            } else {
                // pass it to the next child.
                if (pipe_created_flag == 1) {
                    // Create the child and the pipe.
                    pipe_created_flag = 0;
                    pipe(p);
                    int pid = fork();
                    if (pid == 0) {
                        // child process
                        task(p);
                    } else {
                        // send to it.
                        close(p[0]);
                        write(p[1], &number_read, 4);
                    }
                } else {
                    // pipe already created.
                    write(p[1], &number_read, 4);
                }
            }
        }
    }
    close(pipes[0]);
    if (pipe_created_flag == 0) {
        close(p[1]);
        wait((int *) 0);
        exit(0);
    } else {
        exit(0);
    }
}

int main(int argc, char * argv[]) {
    int p[2];
    pipe(p);
    int pid = fork();
    if (pid == 0) {
        task(p);
    } else {
        close(p[0]);
        for (int i = 2; i <= 35; ++i) {
            if (i == 2) {
                fprintf(1, "prime %d\n", i);
            } else {
                if (i % 2 != 0) {
                    write(p[1], &i, 4);
                }
            }
        }
        close(p[1]);
        wait((int *) 0);
    }
    exit(0);
}
