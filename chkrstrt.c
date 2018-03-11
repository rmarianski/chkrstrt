#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <ram-def.h>

int main() {
    size_t maps_size_inc = 4096;
    size_t maps_buffer_size = maps_size_inc;
    char *maps_buffer = malloc(maps_buffer_size);
    assert(maps_buffer);

    struct dirent *ep;
    DIR *dp = opendir("/proc");
    if (dp) {
        while ((ep = readdir(dp))) {
            char *name = ep->d_name;
            if (name[0] == '.') {
                continue;
            }
            char *endptr = name;
            u32 pid = strtol(name, &endptr, 10);
            if (endptr == name) {
                continue;
            }
            char maps_fname[32];
            assert(snprintf(maps_fname, sizeof(maps_fname), "/proc/%d/maps", pid) < sizeof(maps_fname));
            FILE *mapsf = fopen(maps_fname, "r");
            if (!mapsf) {
                if (errno == EACCES) {
                    continue;
                }
                perror("fopen");
                fprintf(stderr, "Error opening maps file: %s\n", maps_fname);
                continue;
            }
            bool maps_read_err = false;
            size_t maps_buf_size_rem = maps_buffer_size - 1;
            size_t maps_buf_read = 0;
            char *maps_buf_pos = maps_buffer;
            while (!feof(mapsf)) {
                if (maps_buf_size_rem == 0) {
                    maps_buffer_size += maps_size_inc;
                    maps_buf_size_rem = maps_size_inc;
                    maps_buffer = realloc(maps_buffer, maps_buffer_size);
                    assert(maps_buffer);
                    maps_buf_pos = maps_buffer + maps_buf_read;
                }
                size_t n_read = fread(maps_buf_pos, 1, maps_buf_size_rem, mapsf);
                if (ferror(mapsf)) {
                    perror("fread");
                    maps_read_err = true;
                    break;
                }
                if (n_read == 0) {
                    break;
                }
                maps_buf_read += n_read;
                maps_buf_pos += n_read;
                maps_buf_size_rem -= n_read;
            }
            fclose(mapsf);
            if (maps_read_err) {
                continue;
            }
            maps_buffer[maps_buf_read] = 0;
            if (strstr(maps_buffer, ".so (deleted)")) {
                char comm_fname[32];
                assert(snprintf(comm_fname, sizeof(comm_fname), "/proc/%d/comm", pid) < sizeof(comm_fname));
                FILE *commf = fopen(comm_fname, "r");
                if (!commf) {
                    perror("fopen");
                    fprintf(stderr, "Failure opening: %s\n", comm_fname);
                } else {
                    char comm[128];
                    size_t n_read = fread(comm, 1, sizeof(comm) - 1, commf);
                    if (ferror(commf)) {
                        perror("fread");
                        fprintf(stderr, "Error reading from %s\n", comm_fname);
                    } else {
                        u32 idx;
                        for (idx = n_read-1; idx >= 0; idx--) {
                            if (!isspace(comm[idx])) {
                                break;
                            }
                        }
                        comm[idx + 1] = 0;
                        printf("%s: %d\n", comm, pid);
                    }
                }
            }
        }
        closedir(dp);
    }
    free(maps_buffer);
}
