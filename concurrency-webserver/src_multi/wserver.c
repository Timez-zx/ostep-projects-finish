#include <stdio.h>
#include "request.h"
#include "io_helper.h"
#include <pthread.h>

char default_root[] = ".";
char default_schedule[] = "FIFO";
int buffer_num = 16;
int fill = 0;
int use = 0;
int count = 0;

pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//
// ./wserver [-d <basedir>] [-p <portnum>] 
// 
typedef struct thread_arg
{
	int port;
	int *buf;
	struct request_infor **infor;
	
} thread_arg;

void put(int value, int buffer[]){
	buffer[fill] = value;
	fill = (fill+1)%buffer_num;
	count++;
}

void put_sff(struct request_infor *infor, struct request_infor *infor_buf[]){
	count++;
	infor_buf[count-1] = infor;
	struct request_infor *temp;
	for(int i = count-1; i > 0 ; i--){
		if(infor_buf[i]->sbuf.st_size < infor_buf[i-1]->sbuf.st_size){
			temp = infor_buf[i-1];
			infor_buf[i-1] = infor_buf[i];
			infor_buf[i] = temp;
		}
	}
}

struct request_infor * get_sff(struct request_infor *infor_buf[]){
	struct request_infor * infor;
	infor = (struct request_infor *)malloc(8192*5);
	infor = infor_buf[0];
	for(int i = 1; i < count; i++){
		infor_buf[i-1] = infor_buf[i];
	}
	count--;
	return infor;
}

int get(int buffer[]){
	int temp = buffer[use];
	use = (use+1)%buffer_num;
	count--;
	return temp;
}

void *thread_consumer(void *arg){
	struct thread_arg *temp = (struct thread_arg*) arg;	
	while(1){
		pthread_mutex_lock(&mutex);
		while (count == 0){
			pthread_cond_wait(&full, &mutex);
		}
		int tmp = get(temp->buf);
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&mutex);
		printf("service:%d begin\n", tmp);
		// struct request_infor * infor = file_size(tmp);
		// request_handle_infor(infor);
		request_handle(tmp);
		close_or_die(tmp);
		printf("service:%d finish\n", tmp);
	}
	return NULL;
}

void *thread_consumer_sff(void *arg){
	struct thread_arg *temp = (struct thread_arg*) arg;	
	while(1){
		pthread_mutex_lock(&mutex);
		while (count == 0){
			pthread_cond_wait(&full, &mutex);
		}
		struct request_infor *infor= get_sff(temp->infor);
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&mutex);
		printf("service:%d begin\n", infor->fd);
		// struct request_infor * infor = file_size(tmp);
		request_handle_infor(infor);
		// request_handle(tmp);
		close_or_die(infor->fd);
		printf("service_size:%lld finish\n", infor->sbuf.st_size);
	}
	return NULL;
}


int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
	char *schedule = default_schedule;
    int port = 10000;
	int thread_num = 8;
    
    while ((c = getopt(argc, argv, "d:p:b:t:s:")) != -1)
	switch (c) {
	case 'd':
	    root_dir = optarg;
	    break;
	case 'p':
	    port = atoi(optarg);
	    break;
	case 'b':
	    buffer_num = atoi(optarg);
	    break;
	case 't':
	    thread_num = atoi(optarg);
	    break;
	case 's':
	     schedule = optarg;
	    break;
	default:
	    fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-b buffer_size] [-t thread_num] [-s schedule_way]\n");
	    exit(1);
	}

	int buffer[buffer_num];
	struct request_infor * infor_buffer[buffer_num];

    // run out of this directory
    chdir_or_die(root_dir);
	pthread_t consumer_thread_id[thread_num];

	struct thread_arg arg_con;
	arg_con.buf = buffer;
	arg_con.infor = infor_buffer;
	for(int i = 0; i < thread_num ; i++){
		if(!strcmp(schedule,"SFF")){
			pthread_create(consumer_thread_id+i, NULL, thread_consumer_sff, &arg_con);
		}
		else{
			pthread_create(consumer_thread_id+i, NULL, thread_consumer, &arg_con);
		}
	}

	int listen_fd; 
	listen_fd = open_listen_fd_or_die(port);
	while(1){
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		if(!strcmp(schedule,"SFF")){
			int conn_fd = accept_or_die(listen_fd , (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
			pthread_mutex_lock(&mutex);
			while (count == buffer_num-1){
				pthread_cond_wait(&empty, &mutex);
			}
			struct request_infor * infor = file_size(conn_fd);
			put_sff(infor, infor_buffer);
			pthread_cond_signal(&full);
			pthread_mutex_unlock(&mutex);
		}
		else{
			int conn_fd = accept_or_die(listen_fd , (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
			pthread_mutex_lock(&mutex);
			while (count == buffer_num){
				pthread_cond_wait(&empty, &mutex);
			}
			put(conn_fd, buffer);
			pthread_cond_signal(&full);
			pthread_mutex_unlock(&mutex);
		}
	}

    return 0;
}


    


 
