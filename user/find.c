#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"



char* fmtname(char *path)
{
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
    return buf;
}

void find(char *path, char *text){
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    // 匹配到结果就输出路径
    if (strcmp(fmtname(path), text) == 0){
        write(1,path, strlen(path));
        write(1, "\n", 1);
    }

    switch(st.type){
        case T_DEVICE:
        case T_FILE:
            break;

        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                fprintf(2,"find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buf,text);
            }
            break;
    }
    close(fd);
}

int main(int argc, char *argv[]){
    if (argc != 3){
        fprintf(2, "Usage: find path text\n");
        exit(1);
    }
    if(strlen(argv[2]) >= DIRSIZ)
        find(argv[1], argv[2]);
    else{
        char buf[DIRSIZ+1];
        memmove(buf,argv[2],strlen(argv[2]));
        memset(buf + strlen(argv[2]),' ',DIRSIZ - strlen(argv[2]));
        find(argv[1], buf);
    }
    exit(0);
}
