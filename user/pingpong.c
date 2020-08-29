#include "kernel/types.h"
#include "user/user.h"

int main(void) {
    int parent_fd[2], child_fd[2];
    char buf[10];

    pipe(parent_fd);
    pipe(child_fd);

    int spid = fork();
    if(spid < 0) {
        fprintf(2, "fork err\n");
        exit(0);
    }
    if(spid == 0) {
        read(parent_fd[0], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
        write(child_fd[1], "pong", 4);
    } else {
        write(parent_fd[1], "ping", 4);
        read(child_fd[0], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
    }
    exit(0);
}