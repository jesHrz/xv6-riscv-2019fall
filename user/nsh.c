#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/fcntl.h"

#define MAXSIZE 100

void redirect(int k, int fd[]) {
  if(!fd)
    return;
  close(k);
  dup(fd[k]);
  close(fd[0]);
  close(fd[1]);
}

static int isspace(char c) { return c == ' ' || c == '\n' || c == '\t'; }

char * parsepipe(char *cmd) {
  char *p = cmd;
  while(*p) p++;
  p--;
  while(p != cmd) {
    if(*p == '|') {
      *p = 0;
      return p + 1;
    }
    p--;
  }
  return cmd;
}

void execmd(char *cmd) {
  int argc = 0;
  char *argv[MAXARG] = {0};
  char *p, *pp;
  int fd = -1;
  for (p = cmd, pp = cmd + strlen(cmd); p < pp; ++p) {
    if(isspace(*p)) {
      *p = 0;
    } else if(*p == '<') {
      close(fd = 0);
    } else if(*p == '>') {
      close(fd = 1);
    } else {
      argv[argc++] = p;
      while(*p && !isspace(*p)) p++;
      if(*p)  *p = 0;
      if(fd == 0) {
        open(argv[--argc], O_RDONLY);
        fd = -1;
      }
      if(fd == 1) {
        open(argv[--argc], O_CREATE | O_WRONLY);
        fd = -1;
      }
    }
  }
  argv[argc] = 0;
  exec(argv[0], argv);
}

void parsecmd(char *cmd) {
  int pid;
  int fd[2];
  char *rcmd = parsepipe(cmd);
  if(rcmd != cmd)  {
    pipe(fd);
    pid = fork();
    if(pid < 0) {
        fprintf(2, "fork failed\n");
        exit(0);
    }
    if(pid == 0) {
      redirect(1, fd);
      parsecmd(cmd);
    } else {
      wait(0);
      redirect(0, fd);
      execmd(rcmd); 
    }
    close(fd[0]);
    close(fd[1]);
  } else {
    execmd(cmd);
  }
}

int main(void) {
  int pid;
  static char buf[MAXSIZE];
  while(1) {
    printf("@ ");
    memset(buf, 0, sizeof buf);
    gets(buf, sizeof buf);
    if(strlen(buf) < 1)
      break;

    pid = fork();
    if(pid < 0) {
      fprintf(2, "fork failed\n");
      exit(0);
    }

    if(pid == 0) {
      parsecmd(buf);
    } else {
      wait(0);
    }
  }
  exit(0);
}