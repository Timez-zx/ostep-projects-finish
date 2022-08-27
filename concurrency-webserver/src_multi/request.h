#include "io_helper.h"
#ifndef __REQUEST_H__
#define MAXBUF (8192)
struct request_infor{
    struct stat sbuf;
    char buf[MAXBUF]; 
    char filename[MAXBUF];
    char cgiargs[MAXBUF];
    int is_static;
    int fd;
};
void request_handle_infor(struct request_infor* infor);
void request_handle(int fd);
struct request_infor *file_size(int fd);
#endif // __REQUEST_H__
