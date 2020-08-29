#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(2, "Usage: sleep times...\n");
        exit(0);
    }
    int ti = atoi(argv[1]);
    sleep(ti);
    exit(0);
}