#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  if(argc < 2) {
    fprintf(2, "Usage: xargs <commands>\n");
    exit();
  }

  int i, myargc;
  int pid, ok;
  char *p;
  static char *myargv[MAXARG];
  static char buf[101];

  for(myargc = 1; myargc < argc; ++myargc) {
    myargv[myargc - 1] = argv[myargc];
  }

  while(gets(buf, sizeof buf) && strlen(buf) > 0) {
    ok = 0;
    myargc = argc - 1;
    for(i = 0, p = buf; *(p + i); ) {
      switch(*(p + i)) {
      case '\n':
        ok = 1;
      case '\t':
      case ' ':
        *(p + i) = 0;
        myargv[myargc++] = p;
        p += i + 1;
        i = 0;
        if(ok)  goto endline;
        break;
      default:
        i++;
      }
    }
endline:
    myargv[myargc] = 0;
    pid = fork();
    if(pid < 0) {
      fprintf(2, "fork err\n");
      exit();
    }
    if(pid == 0) {
      exec(myargv[0], myargv);
    } else {
      wait();
    }
  }
  exit();
}