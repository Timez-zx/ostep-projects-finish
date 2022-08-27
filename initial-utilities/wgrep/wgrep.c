#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void wgrep(char* path, char* search_word, int16_t status)
{   

    FILE *fp = fopen(path, "r");
    if(status){
        fp = stdin;
    }
    char *str = NULL;
    size_t len = 0;
    ssize_t read;
    if (fp == NULL) {
        printf("%s","wgrep: cannot open file\n");
        exit(1);
    }

    while((read = getline(&str, &len, fp)) != -1){  //getline 中间参数为0时，自动调用malloc函数分配内存，
                                                     //行多长分配对应长度内存，但是fgets只能读取等长的字符
        if(strstr(str,search_word) != NULL){
            printf("%s",str);
        }
    }
    fclose(fp);
    free(str);
}

int main(int argc, char *argv[]){ //argv[0]无论何时就是“”空字符串，argc也就是1，命令行输入的第一个参数为argv[1]
    if(argc == 1){
        printf("%s\n","wgrep: searchterm [file ...]");
        exit(1);
    }
    else if (argc == 2)
    {
        wgrep("",argv[1],1);
    }
    
    for(int i = 2; i < argc ; i++){
        wgrep(argv[i],argv[1],0);
    }
    return 0;
}