#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main()
{
    int fd1[2], fd2[2]; // fd1 from parent to child, fd2 from child to parent.
    if (pipe(fd1) == -1 || pipe(fd2) == -1) {
        fprintf(2, "Creating pipes failed\n");
        exit(1);
    }

    if (fork() != 0) {
        // parent does
        close(fd1[0]);
        close(fd2[1]);
        write(fd1[1], "ping", strlen("ping"));
        close(fd1[1]);
        char str1[5];
        read(fd2[0], str1, sizeof(str1));
        close(fd2[0]);
        wait((int *) 0);
        printf("%d: received %s\n", getpid(), str1);
        exit(0);
    } else {
        // child does
        close(fd2[0]);
        close(fd1[1]);
        write(fd2[1], "pong", strlen("pong"));
        close(fd2[1]);
        char str2[5];
        read(fd1[0], str2, sizeof(str2));
        close(fd1[0]);
        printf("%d: received %s\n", getpid(), str2);
        exit(0);
    }

    return 0;
}