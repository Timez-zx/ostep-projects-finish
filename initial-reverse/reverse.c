#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void reverse(char* input, char* output, int16_t in_sta, int16_t out_sta)
{
    FILE *fp = stdin;
    FILE *fout = stdout;
    if(in_sta){
        fp = fopen(input, "r");
        if (fp == NULL) {
            fprintf(stderr,"reverse: cannot open file ");
            fprintf(stderr,"'%s'\n",input);
            exit(1);
        }
    }
    if(out_sta){
        fout = fopen(output, "w");
    }
    size_t len = 0;
    ssize_t read;
    int line_count=0;
    char* line=NULL;
    fseek(fp,0,SEEK_END); //fp指针指向文件末尾,0为偏移量，第三个参数为从哪里开始偏移
    int filesize = ftell(fp); //返回文件位置指针当前位置相对于文件首的偏移字节数，也就是文件字节数
    char* str=(char *) malloc(filesize+1); //按照文件分配内存块
    if(str == NULL){
        fprintf(stderr,"malloc failed\n");
        exit(1);
    }
    memset(str,0,filesize+1);  //将内存块全部初始化为0
    rewind(fp);  //将fp指针重置到开头位置
    while((read = getline(&line, &len, fp)) != -1){  //getline 中间参数为0时，自动调用malloc函数分配内存，
                                                     //行多长分配对应长度内存，但是fgets只能读取等长的字符
        strcat(str,line);
    }
    if(str[filesize-1] != '\n'){
        for(int i=filesize-1;i>=0;i--){
            line_count++;
            if(str[i] == '\n'){
                for(int j = 1;j < line_count;j++){
                    fprintf(fout,"%c",str[i+j]);
                }
                fprintf(fout,"%c",'\n');
                line_count = 0;
            }
            if(i == 0){
                for(int j = 0;j < line_count;j++){
                    fprintf(fout,"%c",str[i+j]);
                }
            }
        }
    }
    else{
        for(int i=filesize-2;i>=0;i--){
            line_count++;
            if(str[i] == '\n'){
                for(int j = 1;j < line_count;j++){
                    fprintf(fout,"%c",str[i+j]);
                }
                fprintf(fout,"%c",'\n');
                line_count = 0;
            }
            if(i == 0){
                for(int j = 0;j < line_count;j++){
                    fprintf(fout,"%c",str[i+j]);
                }
            }
        }
        fprintf(fout,"%c",'\n');
    }
    free(str);
    fclose(fp);
}

int main(int argc, char *argv[]){
    if(argc == 1){
        reverse("","",0,0);
    }
    if(argc == 2)
    {
        reverse(argv[1],"",1,0);
    }
    if(argc == 3){
        int in_len = strlen(argv[1]);
        int file_len = 0;
        for(int i = in_len - 1; i >= 0;i--){
            if(argv[1][i] == '/'){
                break;
            }
            file_len++;
        }
        int out_len = strlen(argv[2]);
        if(out_len < file_len){
            reverse(argv[1],argv[2],1,1);
        }
        int count = 0;
        for(int j = out_len - 1; j >= 0;j--){
            if(argv[1][in_len-1-count] == argv[2][j]){
                count++;
                if(count == in_len){
                    fprintf(stderr,"reverse: input and output file must differ\n");
                    exit(1);
                }
                continue;
            }
            else{
                reverse(argv[1],argv[2],1,1);
            }
        }
        reverse(argv[1],argv[2],1,1);
    }
    if(argc > 3){
        fprintf(stderr,"usage: reverse <input> <output>\n");
        exit(1);
    }
    return 0;
}