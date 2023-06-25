#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

    int fd;
    char string_to_write[64] = { 0 };
    char file[64] = { 0 };
    ssize_t nr;

    openlog(NULL, 0, LOG_USER);
    if (argc != 3) {
        syslog(LOG_ERR, "Invalid Number of arguments: %d", argc);
        return 1;
    }

    strcpy(file, argv[1]);
    strcpy(string_to_write, argv[2]);

    fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if (fd < 0) {
        syslog(LOG_ERR, "File %s cannot be created", file);
        return 1;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", string_to_write, file);

    nr = write(fd, string_to_write, strlen(string_to_write));
    if (nr < 0) {
        syslog(LOG_ERR, "Writing failed");
        return 1;
    }

    syslog(LOG_DEBUG, "Written %ld bytes to %s", nr, file);

    if(close(fd) == -1) {
        syslog(LOG_ERR, "Closing file %s failed", file);
        return 1;
    }

    return 0;
}
