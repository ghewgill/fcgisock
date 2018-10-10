#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#define UID_WWW 67
#define GID_WWW 67

int debug = 0;

void usage(void)
{
    errx(1, "usage: fcgisock socket root cgi");
}

int main(int argc, char *argv[])
{
    int a = 1;
    while (a < argc) {
        if (argv[a][0] != '-') {
            break;
        }
        if (strcmp(argv[a], "-d") == 0) {
            debug = 1;
        } else {
            usage();
        }
        a++;
    }
    if (argc-a < 3) {
        usage();
    }
    if (geteuid() != 0) {
        errx(1, "need root privileges");
    }
    if (!debug && daemon(0, 0) < 0) {
        err(1, "daemon");
    }
    //if (!debug) {
    //    openlog(__progname, LOG_PID|LOG_NDELAY, LOG_DAEMON);
    //}
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        err(1, "socket");
    }
    struct sockaddr_un sun;
    sun.sun_len = sizeof(sun);
    sun.sun_family = AF_UNIX;
    strlcpy(sun.sun_path, argv[a], sizeof(sun.sun_path));
    if (unlink(sun.sun_path) < 0) {
        err(1, "unlink");
    }
    mode_t old_umask = umask(S_IXUSR|S_IXGRP|S_IWOTH|S_IROTH|S_IXOTH);
    if (bind(sock, (struct sockaddr *)&sun, sizeof(sun)) < 0) {
        err(1, "bind");
    }
    umask(old_umask);
    if (chown(sun.sun_path, UID_WWW, GID_WWW) < 0) {
        err(1, "chown");
    }
    if (listen(sock, 5) < 0) {
        err(1, "listen");
    }
    if (chroot(argv[a+1]) < 0) {
        err(1, "chroot");
    }
    if (chdir("/") < 0) {
        err(1, "chdir");
    }
    if (setresuid(UID_WWW, UID_WWW, UID_WWW) != 0) {
        err(1, "setresuid");
    }
    for (;;) {
        pid_t child = fork();
        if (child == 0) {
            close(0);
            close(1);
            close(2);
            dup2(sock, 0);
            close(sock);
            execl(argv[a+2], argv[a+2], NULL);
            err(1, "exec");
        }
        int status;
        waitpid(child, &status, 0);
    }
}
