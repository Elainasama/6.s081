#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

char buf[512];
int status;

int getLine(){
    int i = 0;
    char c;
    while (read(0,&c,1) != 0){
        if (c == '\n'){
            buf[i] = 0;
            return 1;
        }
        buf[i++] = c;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    while (getLine()) {
        int pid = fork();
        if (pid == 0) {
            char *newArgv[MAXARG];
            for (int i = 1; i < argc; ++i) {
                newArgv[i - 1] = argv[i];
            }
            newArgv[argc - 1] = buf;
            newArgv[argc] = 0;
            exec(newArgv[0], newArgv);
            exit(1);
        } else {
            wait(&status);
        }
    }
    exit(0);
}
