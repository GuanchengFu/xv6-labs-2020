#include "user.h"
#include "kernel/stat.h"
#include "kernel/types.h"

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        // No arguments provided.
        fprintf(2, "Usage: sleep ticks \n");
        exit(1);
    }
    // find the first argument.
    int valid = 0;
    int ticks = atoi_safe(argv[1], &valid);
    if (ticks < 0 || valid == 0) {
        fprintf(2, "Error: ticks must be a valid non-negative integer \n");
        exit(1);
    }
    // Invoke the syscall.
    int result = sleep(ticks);
    if (result < 0) {
        fprintf(2, "Bad call");
        exit(1);
    }
    exit(0);
}
