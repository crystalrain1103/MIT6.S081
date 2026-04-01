#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

__attribute__((noreturn))
void 
prime_sieve(int read_fd) {
    int prime;
    int n;
    int fd[2];
    
    if (read(read_fd, &prime, sizeof(prime)) == 0) {
        close(read_fd);
        exit(0);
    }
    
    printf("prime %d\n", prime);
    
    if (pipe(fd) == -1) {
        fprintf(2, "Creating pipe failed\n");
        exit(1);
    }
    
    if (fork() == 0) {
        // child process
        close(fd[1]);
        prime_sieve(fd[0]);
        exit(0);
    } else {
        // parent process
        close(fd[0]);
        while (read(read_fd, &n, sizeof(n)) > 0) {
            if (n % prime != 0) {
                write(fd[1], &n, sizeof(n));
            }
        }
        close(fd[1]);
        wait((int *) 0);
        close(read_fd);
        exit(0);
    }
}

int 
main() {
    int fd[2];

    if (pipe(fd) == -1) {
        fprintf(2, "Creating pipe failed\n");
        exit(1);
    }
    
    if (fork() == 0) {
        // child process
        close(fd[1]);
        prime_sieve(fd[0]);
        exit(0);
    } else {
        // parent process
        close(fd[0]);
        for (int i = 2; i <= 35; i++) {
            write(fd[1], &i, sizeof(i));
        }
        close(fd[1]);
        wait((int *) 0);
        exit(0);
    }
    
    return 0;
}