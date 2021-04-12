#include "user.h"

#define MAXARGS 10

static char buf[512];

/**
 * Run a specific task as specified in the run_task.
 * @param argc The number of arguments within the argv.
 * @param argv The arguments.
 * @return The result.
 */
int run_task(int argc, char *argv[]) {
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(2, "Usage: xargs args...\n");
        exit(1);
    }

    // Bulid the new args.
    char *newargv[MAXARGS];
    int newargc = 0;
    for (int i = 1; i < argc; ++i) {
        newargc += 1;
        newargv[i - 1] = argv[i];
    }

    // Build the original one.
    while (readline(0, buf, 512) > 0) {
        // The line is read into buf.
        // Handle buf.
        int pid = fork();
        if (pid == 0) {
            // handle the buf.
            // find the args.
            char *begin = buf;
            char *p = buf;
            while (1) {
                if (*p == ' ') {
                    *p = '\0';
                    newargv[newargc] = begin;
                    begin = p + 1;
                    newargc += 1;
                } else if (*p == '\n') {
                    *p = '\0';
                    newargv[newargc] = begin;
                    begin = p + 1;
                    newargc += 1;
                    break;
                }
                p = p + 1;
            }

            // Build args done.
            exec(newargv[0], newargv);
            fprintf(2, "Executed failed:%s\n", newargv[0]);
            exit(1);
        } else {
            wait(0);
        }
    }
    exit(0);
}
