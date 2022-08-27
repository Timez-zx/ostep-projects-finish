#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void wcat(char* path)
{
    FILE *fp = fopen(path, "r");
    char *str;
    int filesize;
    char buffer[4096];
    if (fp == NULL) {
        printf("%s","wcat: cannot open file\n");
        exit(1);
    }
    fseek(fp,0,SEEK_END); //fp指针指向文件末尾,0为偏移量，第三个参数为从哪里开始偏移
    filesize = ftell(fp); //返回文件位置指针当前位置相对于文件首的偏移字节数，也就是文件字节数
    str=(char *) malloc(filesize+1); //按照文件分配内存块
    memset(str,0,filesize+1);  //将内存块全部初始化为0
    rewind(fp);  //将fp指针重置到开头位置
    int bufsize=sizeof(buffer);
    while((fgets(buffer,bufsize,fp))!=NULL){ //每次最多读取b每行的bufsize-1个字符
        strcat(str,buffer);
    }
    printf("%s",str);
    fclose(fp);
    free(str);
}

int main(int argc, char *argv[]){ //argv[0]无论何时就是“”空字符串，argc也就是1，命令行输入的第一个参数为argv[1]
    for(int i = 1; i < argc ; i++){
        wcat(argv[i]);
    }
    return 0;
}