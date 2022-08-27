#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXPATH 128
#define PATH_NUM 2
#define MAX_COMMAND 10
#define MAX_PARAMETER 6


char* get_near_fold(char* buf,int16_t bufsize,int16_t num){
    getcwd(buf,bufsize);
    int16_t count=0;
    int16_t buf_size = bufsize-1;
    while(count < num && buf_size >= 0){
        if(buf[buf_size] == '/'){
            count++;
        }
        buf_size--;
    }
    return (buf+buf_size+1);
}


int path_zx(char* token,char *command_path[], int max_command)
{
    int command_num=0;
    char *token_part=NULL;
    char *token_temp=NULL;
    int command_length=0;
    for(int i=0; i < max_command; i++){
        // command_path[i] = (char *) malloc(1 + strlen(token_part));
        command_path[i] = NULL;
    }
    if(!strcmp(token, "")){
        return 0;
    }
    for(token_part=strsep(&token," "); token_part != NULL; token_part=strsep(&token," ")){
        command_length=strlen(token_part);
        if(token_part[command_length-1] == '/'){
            command_path[command_num]=token_part;
        }
        else{
            token_temp = (char *) malloc((1 + strlen(token_part))*sizeof(char*));
            strcpy(token_temp, token_part);
            strcat(token_temp, "/");
            command_path[command_num]=token_temp;
        }
        // printf("%s\n",command_path[command_num]);
        command_num++;
    }
    return command_num;    

}

char *input_fix(char *input){
    int input_len = strlen(input);
    char *fix_final;
    char *fix_input = (char *)malloc((input_len+10)*sizeof(char));
    int fix_index = 0;
    int symbol_flag = 0;
    int symbol_his = 0;

    for(int i = 0; i < input_len; i++){
        if(input[i] == 32){
            symbol_flag = 0;
        }
        else{
            symbol_flag = 1;
        }
        if(symbol_flag == 0 && symbol_his == 0)
            continue;
        else {
            fix_input[fix_index++] = input[i];
        }
        symbol_his = symbol_flag;
    }
    if(fix_input[fix_index-2] == 32){
        fix_final = (char *)malloc((fix_index+1)*sizeof(char));
        strncpy(fix_final, fix_input, fix_index-2);
        fix_final[fix_index-2] = '\n';
    }
    else
    {
        fix_final = (char *)malloc((fix_index+1)*sizeof(char));
        strncpy(fix_final, fix_input, fix_index);
        free(fix_input);
    }
    return fix_final;
}

char** exec_zx(char* command, char* para, char *command_path[], int command_num, char *final_command){
    char * real_para=NULL;
    int para_num=1;
    char **parameter = (char**)malloc(MAX_PARAMETER * sizeof(char*));
    for (int i =0; i<MAX_PARAMETER; i++){
        parameter[i] = NULL;
    } 

    for(int i=0; i < command_num; i++){
        strcpy(final_command, command_path[i]);
        strcat(final_command, command);
        if(!access(final_command, F_OK)){
            break;
        }
        final_command=NULL;
    }

    parameter[0] = command;
    if(strcmp(para, "")){
        for(real_para=strsep(&para," "); real_para != NULL; real_para=strsep(&para," ")){
            parameter[para_num]=real_para;
            para_num++;
        }
    }
    return parameter;
}



int main(int argc, char *argv[]){
    FILE *fp;
    char *str = NULL;
    char *fix_str=NULL;
    char *file_str=NULL;
    char *final_str=NULL;
    char *final_file_str=NULL;
    char *fix_str_origin=NULL;
    char *temp;
    int temp_mr=-1; 
    size_t len = 0;
    char *fold = NULL; 
    char *token;
    char buf[MAXPATH];
    char error_message[30] = "An error has occurred\n";
    int command_pathnum=0;
    char *final_command=NULL;
    char **parameter;
    char *command_part="";
    char *command_part_new;
    char *fix_command_part;
    char *command_path[MAX_COMMAND];
    int redirect = 0;
    int count_file=0;
    int count_file_pro=0;
    command_pathnum = path_zx("/bin", command_path, MAX_COMMAND);
    
    char *redi_char;
    if(argc == 1){
        fp = stdin;
        fold = get_near_fold(buf,MAXPATH,PATH_NUM);
        printf("%s %s ",fold,"wish>");
        while((getline(&str, &len, fp)) != -1){
            command_delt:
                fix_str_origin = input_fix(str);
                for(int i=0; i < strlen(fix_str_origin); i++){
                    if(fix_str_origin[i] == '>')
                        redirect = 1;
                }
                redi_char="";
                while(redi_char != NULL){
                    if(fix_str_origin != NULL && fix_str_origin[0] == '>'){
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        break;
                    }
                    if(strcmp(redi_char,"\0") && strcmp(redi_char," ")){
                        // printf("%s\n", redi_char);
                        if(count_file == 0){
                            fix_str = redi_char;
                            final_str = (char *)malloc((strlen(fix_str)+1)*sizeof(char));
                            strcpy(final_str,fix_str);
                            final_str[strlen(fix_str)]='\n';
                            temp = final_str;
                        }
                        else{
                            final_file_str = input_fix(redi_char);
                            printf("%s\n", final_file_str);
                            for(int i = 0; i < strlen(final_file_str); i++){
                                if(final_file_str[i] == ' '){
                                    temp_mr = i;
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                    break;
                                }
                            }
                            if(temp_mr >= 0){
                                temp_mr = -1;
                                break;
                            }
                        } 
                        int fd,sfd;
                        if(count_file > 0){
                            sfd=dup(STDOUT_FILENO);
                            fd = open(final_file_str, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
                            dup2(fd,STDOUT_FILENO);
                        }
                        if(redirect == 0){
                            final_str = temp;
                            command_part="";
                            while(command_part != NULL){
                                if(strcmp(command_part ,"\0") && strcmp(command_part ," ")){
                                    // printf("zhangxiao\n");
                                    command_part_new = (char *)malloc((strlen(command_part)+5)*sizeof(char));
                                    strcpy(command_part_new,command_part);
                                    command_part_new[strlen(command_part)]='\n';
                                    fix_command_part = input_fix(command_part_new);
                                    // printf("%s\n",fix_command_part);
                                    token = strsep(&fix_command_part," \n");

                                    if(!(strcmp(token,"exit"))){
                                        if(strlen(fix_command_part) == 0)
                                            exit(0);
                                        else{
                                            write(STDERR_FILENO, error_message, strlen(error_message));
                                            break;
                                        }
                                    }
                                    if(!(strcmp(token,"cd"))) {
                                        int state = chdir(strsep(&fix_command_part," \n"));
                                        if(state){
                                            write(STDERR_FILENO, error_message, strlen(error_message)); 
                                        }
                                        memset(buf, 0, MAXPATH);
                                        fold = get_near_fold(buf,MAXPATH,PATH_NUM);
                                    }
                                    else if(!(strcmp(token,"path"))){
                                        command_pathnum  = path_zx(strsep(&fix_command_part,"\n"), command_path, MAX_COMMAND);
                                    }
                                    else{
                                        //父进程的入口是main，子进程结束返回时需要找父进程入口地址继续运行
                                        //，如果子进程在一个函数中创建，子进程运行结束无法找到main入口，因此父进程无法回收
                                        //或者说父进程不能找到子进程并将它回收
                                        final_command = (char *) malloc(200);
                                        memset(final_command,0,200);
                                        parameter = exec_zx(token, strsep(&fix_command_part,"\n"), command_path, command_pathnum,final_command);
                                        if(access(final_command, F_OK)){
                                            // printf("command path:%s not exist\n",token);
                                            write(STDERR_FILENO, error_message, strlen(error_message));
                                        }
                                        else{
                                            pid_t pid = fork(); 
                                            if(pid == 0){
                                                execve(final_command,parameter,NULL);
                                                exit(0);
                                            }
                                            wait(NULL);
                                        }

                                    }
                                }
                                command_part = strsep(&final_str,"&\n");


                            }
                        }
                        redirect = 0;
                        if(count_file > 0){
                            dup2(sfd,STDOUT_FILENO);
                            close(fd);
                        }
                        count_file++;
                    }
                    else{
                        
                        if(count_file > 0 && count_file_pro == count_file){
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        }
                        count_file_pro = count_file;
                    }
 
                    redi_char = strsep(&fix_str_origin,">\n");

                }
                redirect = 0;
                count_file = 0;
                // count_sum=0;
                if(argc > 1)
                    goto next_line;
                printf("%s %s ",fold,"wish>");
        }
    }
    else{
        if(argc > 2){
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            exit(1);  
        }
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            exit(1);
        }
        int count=0;
        while((getline(&str, &len, fp)) != -1){
            goto command_delt;
            next_line:count++;
        }
    }


    return 0;
}