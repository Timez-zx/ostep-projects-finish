#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void diwrite(int n,char c, FILE* stream)
{
    fwrite(&n,sizeof(int),1,stream);
    fwrite(&c,1,1,stream);
}


int main(int argc, char *argv[]){ //argv[0]无论何时就是“”空字符串，argc也就是1，命令行输入的第一个参数为argv[1]
    if(argc==1)
    {
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    }

    FILE *fp = stdin;
    int nread;
    char char_read;
    char last_read = 0;
    int count = 0;
    clock_t start, finish;
    double duration;
    FILE * stream;
    stream=fopen("./a.z", "w");
    for(int i = 1; i < argc; i++){
        fp = fopen(argv[i],"rb");
        if(fp == NULL)
        {
            printf("wzip: cannot open file\n");
            return 1;
        }
        start = clock();
        while((nread = fread(&char_read,1,1,fp))==1){
            if(char_read == last_read){
                count++;
                continue;
            }
            else
            {
                if (count > 0)
                {
                    diwrite(count,last_read,stream);
                }
                count = 1;
            }
            last_read = char_read;
        }
        finish = clock();
        fclose(fp);
    }
    diwrite(count,last_read,stream);
    duration = (double)(finish - start) / 1000000;
    printf( "%f seconds\n", duration );
}