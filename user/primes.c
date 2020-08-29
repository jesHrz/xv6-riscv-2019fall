#include "kernel/types.h"
#include "user/user.h"

void solve(int input_fd) {
    int fd[2];
    int p, n;
    int pid;

    if(!read(input_fd, &p, sizeof(int))) {
        fprintf(2, "pipe exit\n");
        exit(0);
    }

    printf("prime %d\n", p);

    if(pipe(fd) < 0) {
        fprintf(2, "pipe err\n");
        exit(0);
    }

    pid = fork();
    if(pid < 0) {
        fprintf(2, "fork err\n");
        exit(0);
    }
    if(pid == 0) {
        close(fd[1]);
        solve(fd[0]);
    } else {
        close(fd[0]);
        while(read(input_fd, &n, sizeof(int))) {
            if(n % p) {
                write(fd[1], &n, sizeof(int));
            }
        }
        close(fd[1]);
        close(input_fd);
    }
}

int main(void) {
    int fd[2];
    int pid;
    int i;

    if(pipe(fd) < 0) {
        fprintf(2, "pipe err\n");
        exit(0);
    }

    pid = fork();
    if(pid < 0) {
        fprintf(2, "fork err\n");
        exit(0);
    }
    if(pid == 0) {
        close(fd[1]);
        solve(fd[0]);
    } else {
        close(fd[0]);
        for(i = 2; i <= 35; ++i) {
            if(write(fd[1], &i, 4) != 4) {
                fprintf(2, "write err\n");
                exit(0);
            }
        }
    }
    
    exit(0);
}