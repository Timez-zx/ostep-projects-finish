#include <stdio.h>
#include <string.h>
#include <stdlib.h>





int main(int argc, char *argv[]){ //argv[0]无论何时就是“”空字符串，argc也就是1，命令行输入的第一个参数为argv[1]
    if(argc==1)
    {
        printf("wunzip: file1 [file2 ...]\n");
        return 1;
    }

    FILE *fp = stdin;
    int nread;
    int int_read=0;
    char char_read;
    for(int i = 1; i < argc; i++){
        fp = fopen(argv[i],"rb");
        if(fp == NULL)
        {
            printf("wunzip: cannot open file\n");
            return 1;
        }
        while((nread = fread(&int_read,4,1,fp))==1){
            fread(&char_read,1,1,fp);
            for(int j=0; j < int_read; j++){
                printf("%c",char_read);
            }
        }
        fclose(fp);
    }
}