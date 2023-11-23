#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

char buf[4];

// 管道是一个小的内核缓冲区，它以文件描述符对的形式提供给进程，一个用于写操作，一个用于读操作。
// 从管道的一端写的数据可以从管道的另一端读取。管道提供了一种进程间交互的方式。
int main(int argc, char *argv[]){
    int p2c[2];
    int c2p[2];

    pipe(p2c);
    pipe(c2p);
    int pid = fork();
    // 子进程逻辑
    // 由于管道会等待输入，因此无需等待子进程退出。
    if (pid == 0){
        read(p2c[0],buf,sizeof buf);
        fprintf(2,"%d: received ping\n",pid);
        write(c2p[1],"pong",4);
    }else{
        write(p2c[1],"ping",4);
        read(c2p[0],buf,sizeof buf);
        fprintf(2,"%d: received pong\n",pid);
    }
    close(p2c[0]);
    close(p2c[1]);
    close(c2p[0]);
    close(c2p[1]);
    exit(0);
}
