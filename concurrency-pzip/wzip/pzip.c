#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

#define MAX_LEN 128

struct data_zip{
    int data_counter;
    char data;
}__attribute__((packed)) data_zip;

struct pzip
{
    struct data_zip* zip_data;
    off_t data_num;
};


typedef struct para_data{
    char* data;
} para_data;

void diwrite(int n,char c)
{
    fwrite(&n,sizeof(int),1,stdout);
    fwrite(&c,1,1,stdout);
}

char* my_mmap_read(char *path, off_t len[])
{
    char buf[MAX_LEN] = {0}; 
    char *mmapped;
    int fd;
    if( (fd = open(path, O_RDWR)) < 0) {
        perror("open\n");
        return NULL;
    }

    struct stat sb;
    if(fstat(fd, &sb) == -1)  perror("fstat");
    mmapped = (char*) malloc(sizeof(char)*sb.st_size);
    len[0] = sb.st_size; 
    lseek(fd, 200, SEEK_SET);
    if((mmapped = mmap(NULL, sb.st_size, PROT_READ, 
        MAP_SHARED, fd, 0)) == (void*)-1)  perror("mmap\n");
    close(fd);
    return mmapped;
}

void* datazip(void *arg){
    clock_t start, finish;
    start = clock();
    struct para_data *para_arg = (struct para_data*) arg;
    off_t data_len = strlen(para_arg->data);
    struct data_zip* zip_data;
    struct pzip* result;
    zip_data = (struct data_zip*) malloc(sizeof(struct data_zip)*data_len);
    off_t zip_len = 0;
    int count = 1;
    printf("%lld\n",data_len);
    for(int i = 0; i< data_len-1; i++){
        if(para_arg->data[i] == para_arg->data[i+1]){
            count++;
            continue;
        }
        else{
            zip_data[zip_len].data_counter = count;
            zip_data[zip_len].data = para_arg->data[i];
            count = 1;
            zip_len++;
        }

    }
    zip_data[zip_len].data_counter = count;
    zip_data[zip_len].data = para_arg->data[data_len-1];
    result = (struct pzip*) malloc(sizeof(struct pzip));
    result->zip_data = (struct data_zip*) malloc(sizeof(struct data_zip)*data_len);
    result->zip_data = zip_data;
    result->data_num = zip_len+1;
    finish = clock();
    printf( "%f seconds\n", (double)(finish - start) / 1000000);
    return result;
}



int main(int argc, char *argv[]){ //argv[0]无论何时就是“”空字符串，argc也就是1，命令行输入的第一个参数为argv[1]
    if(argc==1)
    {
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    }
    int core_number = sysconf(_SC_NPROCESSORS_ONLN);
    char *mmapped[argc-1];
    off_t file_len[1];
    off_t total_len = 0;
    char *buffer;

    for(int i = 1; i < argc; i++){
        mmapped[i-1] = my_mmap_read(argv[i], file_len);
        total_len += file_len[0];
    }

    buffer = (char*) malloc(sizeof(char)*total_len);
    for(int i = 1; i < argc; i++){
        sprintf(buffer, "%s%s", buffer, mmapped[i-1]);
    }
    if(total_len < 100){
        core_number = 1;
    }
    core_number = 4;
    off_t seperate_len = (off_t) ceil((double) total_len/core_number);
    off_t final_len = total_len-(core_number-1)*seperate_len;
    printf("%lld\n%lld\n",seperate_len,final_len);
    pthread_t thread_id[core_number];
    struct para_data *para[core_number];
    void* zipped_data[core_number];
    struct pzip* zipped[core_number];
    for(int i = 0; i < core_number-1; i++){
        para[i] = (struct para_data*) malloc(sizeof(para_data));
        para[i]->data = (char*) malloc(sizeof(char)*seperate_len);
        strncpy(para[i]->data,buffer+i*seperate_len,seperate_len);
    }
    para[core_number-1] = (struct para_data*) malloc(sizeof(para_data));
    para[core_number-1]->data = (char*) malloc(sizeof(char)*final_len);
    strncpy(para[core_number-1]->data,buffer+(core_number-1)*seperate_len,final_len);
    clock_t start, finish;
    FILE * stream;
    stream=fopen("./a.z", "w");
    double duration;
    start = clock();
    for(int i = 0; i < core_number; i++){
        pthread_create(thread_id+i, NULL, datazip, para[i]);
    }
    for(int i = 0; i < core_number; i++){
        pthread_join(thread_id[i],(void*)(zipped_data+i));
        zipped[i] = (struct pzip*)zipped_data[i];
    }
    // finish = clock();
    for(int i = 0; i < core_number-1; i++){
        if(zipped[i]->zip_data[zipped[i]->data_num-1].data == zipped[i+1]->zip_data[0].data){
            zipped[i+1]->zip_data[0].data_counter += zipped[i]->zip_data[zipped[i]->data_num-1].data_counter;
            zipped[i]->data_num -= 1;
        }
    }
    // finish = clock();
    for(int i = 0; i < core_number; i++){
        fwrite(zipped[i]->zip_data,5,zipped[i]->data_num,stream);
    }
    finish = clock();
    duration = (double)(finish - start) / 1000000;
    printf( "%f seconds\n", duration );

}