#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void findPrime(int*);
int status;
// 素数筛
int main(int argc, char *argv[]){
    int p[2];
    pipe(p);
    int gid = fork();
    if (gid == 0){
        close(p[0]);
        for(int i = 2;i <= 35;i++){
            write(p[1],&i,4);
        }
        close(p[1]);
    }else{
        findPrime(p);
    }
    exit(0);
}


void findPrime(int* p){
    close(p[1]);

    // 第一个输入的作为质数
    int prime;
    int len = read(p[0],&prime,4);
    if (len != 0){
        fprintf(1,"prime %d\n",prime);
        int nxt_p[2];
        pipe(nxt_p);
        int gid = fork();
        if (gid == 0){
            close(nxt_p[0]);
            int num;
            while (1){
                len = read(p[0],&num,4);
                if (len == 0){
                    break;
                }
                if (num % prime != 0){
                    write(nxt_p[1],&num,sizeof(num));
                }
            }
            close(nxt_p[1]);
        }else{
            findPrime(nxt_p);
        }
    }


    close(p[0]);
}