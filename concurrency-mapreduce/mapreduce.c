#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "mapreduce.h"
#include <time.h>
 
typedef struct
{
    char *key;
    char *value;
    unsigned long part_number;
} map_data;

typedef struct 
{
    Reducer reduce;
    int part_number;
}part_para;

typedef struct 
{
    Mapper map;
    char* file;
}map_para;


map_data **all_data;
map_data *** part_data;
int *part_num;
int *part_count;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
long map_count = 0;

// void Map(char *file_name) {
//     FILE *fp = fopen(file_name, "r");
//     assert(fp != NULL);
//     char *line = NULL;
//     size_t size = 0;
//     while (getline(&line, &size, fp) != -1) {
//         char *token, *dummy = line;
//         while ((token = strsep(&dummy, " \t\n\r")) != NULL) {
//             if(strcmp(token, ""))
//                 MR_Emit(token, "1");
//         }
//     }
//     free(line);
//     fclose(fp);
// }

void *MR_map(void*arg){
    map_para* para = (map_para*) arg;
    para->map(para->file);
    return NULL;
}

// void Reduce(char *key, Getter get_next, int partition_number) {
//     int count = 0;
//     char *value;
//     while ((value = get_next(key, partition_number)) != NULL){
//         count++;
//     }
//     printf("%s %d\n", key, count);
// }


unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}


void MR_Emit(char *key, char *value){
    pthread_mutex_lock(&mutex);
    all_data[map_count] = (map_data*) malloc(sizeof(map_data));
    all_data[map_count]->key = (char*) malloc((strlen(key)+1)*sizeof(char));
    strcpy(all_data[map_count]->key, key);
    all_data[map_count]->value = (char*) malloc((strlen(value)+1)*sizeof(char));
    strcpy(all_data[map_count]->value, value);
    map_count++;
    pthread_mutex_unlock(&mutex);
}

char *get_next(char *key, int partition_number){
    if(part_count[partition_number] < part_num[partition_number] && !strcmp(part_data[partition_number][part_count[partition_number]]->key ,key)){
        char * final = part_data[partition_number][part_count[partition_number]]->value;
        part_count[partition_number]++;
        return final;
    }
    else{
        return NULL;
    }  
}

void *MR_reduce(void *arg){
    part_para* part_arg = (part_para*) arg;
    while(part_count[part_arg->part_number] < part_num[part_arg->part_number]){
        part_arg->reduce(part_data[part_arg->part_number][part_count[part_arg->part_number]]->key, get_next, part_arg->part_number);
    }
    return NULL;
}


void MR_Sortadd(map_data** part_list, map_data * new_data, int index_num){
    map_data *temp;
    part_list[index_num] = new_data;
    for(int i = index_num; i > 0; i--){
        int ret = strcmp(part_list[i]->key, part_list[i-1]->key);
        if(ret < 0){
            temp = part_list[i];
            part_list[i] = part_list[i-1];
            part_list[i-1] = temp;
        }
        else{
            break;
        }
    }
}

void MR_Run(int argc, char *argv[], Mapper map, int num_mappers, Reducer reduce, int num_reducers, Partitioner partition){
    int real_num_mappers;
    int num_partition = num_reducers;
    if(argc - 1 <= num_mappers){
        real_num_mappers = argc - 1;
    }
    else{
        printf("Mappers number should be larger than file number:%d\n", argc -1);
        exit(1);
    }
    all_data = (map_data**) malloc(sizeof(map_data*)*1000000);
    pthread_t map_thread_id[real_num_mappers];
    map_para *mpara[real_num_mappers];

    for(int i = 0; i < real_num_mappers; i++){
        mpara[i] = (map_para*) malloc(sizeof(map_para)); 
        mpara[i]->map = map;
        mpara[i]->file = (char*) malloc((strlen(argv[i+1])+1)*sizeof(char));
        strcpy(mpara[i]->file, argv[i+1]);
    }
    for(int i = 0; i < real_num_mappers; i++){
        pthread_create(map_thread_id+i, NULL, MR_map, mpara[i]);
    }
    for(int i = 0; i < real_num_mappers; i++){
        pthread_join(map_thread_id[i],NULL);
    }

    part_data = (map_data***) malloc(sizeof(map_data**)*num_partition);
    part_num = (int*) malloc(sizeof(int)*num_partition);
    part_count = (int*) malloc(sizeof(int)*num_partition);
    
    for(int i = 0; i < num_partition; i++){
        part_data[i] = (map_data**) malloc(sizeof(map_data*)*100000);
        part_num[i] = 0;
        part_count[i] = 0;
    }

    for(int i = 0; i < map_count; i++){
        all_data[i]->part_number = partition(all_data[i]->key, num_partition);
        int part_index = (int) all_data[i]->part_number;
        MR_Sortadd(part_data[part_index], all_data[i], part_num[part_index]);
        part_num[part_index]++;
    }
    part_para *para[num_partition];
    pthread_t reduce_thread_id[num_partition];
    for(int i = 0; i < num_partition; i++){
        para[i] = (part_para*) malloc(sizeof(part_para));
        para[i]->reduce = reduce;
        para[i]->part_number = i;
    }
    for(int i = 0; i < num_partition; i++){
         pthread_create(reduce_thread_id+i, NULL, MR_reduce, para[i]);    
    }
    for(int i = 0; i < num_partition; i++){
        pthread_join(reduce_thread_id[i],NULL);
    }

    free(part_data);
    free(all_data);

}

// int main(int argc, char *argv[]) {
//     if(argc == 1){
//         printf("No file input....\n");
//         exit(1);
//     }
//     clock_t start, finish;
//     double duration;
//     start = clock();
//     MR_Run(argc, argv, Map, 8, Reduce, 8, MR_DefaultHashPartition);
//     finish = clock();
//     duration = (double)(finish - start) / 1000000;
//     printf( "%f seconds\n", duration );
// }