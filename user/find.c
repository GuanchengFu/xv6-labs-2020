#include "user.h"
#include "kernel/stat.h"
#include "kernel/types.h"
#include "kernel/fs.h"

/**
 * Give a filepath, find the file name.
 * @param path The file path.
 * @return The filename.
 */
char *
fmtname(char *path) {
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}


/**
 * Search a specific path, and find all the file with the name specified.
 * @param path  The path to begin the search.
 * @param filename The name of the file to be searched.
 */
void
find(char *path, char *filename) {
    // Try to open the path.
    int fd;
    struct dirent de;
    struct stat st;
    const char *current_directory = ".";
    const char *previous_directory = "..";
    char buf[512], *p;
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    //Try to get the state of the file.
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // The path should be a directory.
    if (st.type != T_DIR) {
        fprintf(2, "find: please specify a valid directory to search\n");
        close(fd);
        return;
    }

    // Now the path should be a directory.
    // Construct a path.
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
        fprintf(2, "find: path too long\n");
        close(fd);
        return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    // Iterate through the directory.
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        // inode is 0.
        if (de.inum == 0) {
            continue;
        }
        if ((strcmp(de.name, current_directory) == 0)
            || (strcmp(de.name, previous_directory) == 0)) {
            continue;
        }


        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        if (stat(buf, &st) < 0) {
            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }

        // Based on the type, do different things.
        if (st.type == T_FILE) {
            if (strcmp(de.name, filename) == 0) {
                // We find the file.
                fprintf(1, "%s\n", buf);
            }
        } else if (st.type == T_DIR) {
            // recursively does this.
            find(buf, filename);
        }
    }
}

int
main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "Usage: find directory filename\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}
