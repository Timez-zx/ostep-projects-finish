#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "fs.h"
#include "param.h"

#define MAX_LEN 128

char* my_mmap_read(char *path, off_t *len);

void rsect(char* mmapped, uint sec, void *buf);

void rinode(char* mmapped, uint inum, struct dinode *ip, struct superblock *sb_input);

ushort bitmap_exist(int addr, char* bitmap);

void bitmap_delete(int addr, char* bitmap);

void inode_detect_1(char* mmapped);

void inodeaddr_detect_2(char* mmapped);

void root_check_3(char* mmapped);

void directory_check_4(char* mmapped);

void bitmap_check_5(char* mmapped);

void bitmap_check_6(char* mmapped);

void address_check_7_8(char* mmapped);

void inode_check_9(char* mmapped, uint inode_num);


int main(int argc, char *argv[]){ //argv[0]无论何时就是“”空字符串，argc也就是1，命令行输入的第一个参数为argv[1]
    if(argc==1)
    {
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    };

    char *mmapped = NULL;
    off_t *file_len;
    mmapped = my_mmap_read(argv[1], file_len);

    // bitmap_check_6(mmapped);
    inode_check_9(mmapped, 1);


    return 0;


    

}


char* my_mmap_read(char *path, off_t *len)
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
    *len = sb.st_size; 
    if((mmapped = mmap(NULL, sb.st_size, PROT_READ, 
        MAP_SHARED, fd, 0)) == (void*)-1)  perror("mmap\n");
    close(fd);
    return mmapped;
}

void rsect(char* mmapped, uint sec, void *buf)
{ 
    memcpy(buf, (mmapped+BSIZE*sec), BSIZE);
}

void rinode(char* mmapped, uint inum, struct dinode *ip, struct superblock *sb_input)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;
  struct superblock *sb;
  if(sb_input == NULL){
    char sb_buf[BSIZE];
    rsect(mmapped, 1, sb_buf);
    sb = (struct superblock*) sb_buf;
  }
  else{
    sb = sb_input;
  }
  bn = inum/IPB + sb->inodestart;
  rsect(mmapped, bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}

void inode_detect_1(char* mmapped)
{
  struct superblock *sb;
  struct dinode *ip = (struct dinode*) malloc(sizeof(struct dinode));
  char sb_buf[BSIZE];
  rsect(mmapped, 1, sb_buf);
  sb = (struct superblock*) sb_buf;
  for(int i = 1; i <= sb->ninodes; i++){
      rinode(mmapped, i, ip, sb);
      if(ip->type != 0 && ip->type != 1 && ip->type != 2 && ip->type != 3){
          printf("Inode:%d type allocate error\n", i);
          exit(1);
      }
  }
}

void inodeaddr_detect_2(char* mmapped)
{
  struct superblock *sb;
  struct dinode *ip = (struct dinode*) malloc(sizeof(struct dinode));
  char sb_buf[BSIZE];
  rsect(mmapped, 1, sb_buf);
  sb = (struct superblock*) sb_buf;
  uint block_num;
  uint indirect[NINDIRECT];

  for(int i = 1; i <= sb->ninodes; i++){
      rinode(mmapped, i, ip, sb);
      printf("number:%d %d\n", i, ip->type);
      if(ip->size == 0){
        continue;
      }
      else{
        if(ip->size%BSIZE == 0)
            block_num = ip->size/BSIZE;
        else
            block_num = ip->size/BSIZE + 1;
        printf("block_num:%d\n", block_num);
        if(block_num <= NDIRECT){
          for(int j = 0; j < block_num; j++){
            printf("DADDR:%d\n", ip->addrs[j]);
            if(ip->addrs[j] >= FSSIZE || ip->addrs[j] < FSSIZE - sb->nblocks){
              printf("ERROR: bad direct address in inode:%d direct:%d\n", i, j);
              exit(1);
            }
          }
        }
        else{
          for(int j = 0; j <= NDIRECT; j++){
            printf("DADDR:%d\n", ip->addrs[j]);
            if(ip->addrs[j] >= FSSIZE || ip->addrs[j] < FSSIZE - sb->nblocks){
              printf("ERROR: bad direct address in inode:%d direct:%d\n", i, j);
              exit(1);
            }
          }
          rsect(mmapped, ip->addrs[NDIRECT], (char*)indirect);
          for(int j = 0; j < block_num - NDIRECT; j++){
            printf("IADDR:%d\n", indirect[j]);
            if(indirect[j] >= FSSIZE  || indirect[j] < FSSIZE - sb->nblocks){
              printf("ERROR: bad indirect address in inode:%d indirect:%d\n", i, j+NDIRECT);
              exit(1);
            }
          }
        }
      }
  }
}

void root_check_3(char* mmapped){
  struct superblock *sb;
  struct dinode *ip = (struct dinode*) malloc(sizeof(struct dinode));
  char sb_buf[BSIZE];
  rsect(mmapped, 1, sb_buf);
  sb = (struct superblock*) sb_buf;
  struct dirent* direct = (struct dirent*) malloc(sizeof(struct dirent));
  char buf[BSIZE];

  rinode(mmapped, 1, ip, sb);
  rsect(mmapped, ip->addrs[0], buf);
  memcpy(direct, buf+sizeof(struct dirent), sizeof(struct dirent));
  if(direct->inum != 1){
    printf("ERROR: root directory does not exist\n");
    exit(1);
  }
  printf("Valid root: root directory exists\n");
}

void directory_check_4(char* mmapped){
  struct superblock *sb;
  struct dinode *ip = (struct dinode*) malloc(sizeof(struct dinode));
  char sb_buf[BSIZE];
  rsect(mmapped, 1, sb_buf);
  sb = (struct superblock*) sb_buf;
  struct dirent* direct = (struct dirent*) malloc(sizeof(struct dirent));
  struct dirent* father_direct = (struct dirent*) malloc(sizeof(struct dirent));
  char buf[BSIZE];

  for(int i = 1; i <= sb->ninodes; i++){
    rinode(mmapped, i, ip, sb);
    if(ip->type == 0){
      continue;
    }
    else{
      if(ip->type == 1){
        rsect(mmapped, ip->addrs[0], buf);
        memcpy(direct, buf, sizeof(struct dirent));
        memcpy(father_direct, buf + sizeof(struct dirent), sizeof(struct dirent));
        if( !strcmp(direct->name, ".") &&  !strcmp(father_direct->name, "..") && direct->inum == i){
          continue;
        }
        else{
          printf("ERROR: directory not properly formatted\n");
          exit(1);
        }
      }
    }
  }
}

ushort bitmap_exist(int addr, char* bitmap){
  ushort char_index;
  ushort char_loca;
  uchar bit;
  if((addr+1)%8 == 0){
    char_index = (addr+1)/8 - 1;
  }
  else{
    char_index = (addr+1)/8;
  }
  char_loca = addr%8;
  bit = bitmap[char_index];
  // bit = bit & (1 << char_loca);
  bit = (bit >> char_loca) & (0x1);
  return !!bit;
}

void bitmap_check_5(char* mmapped){
  struct superblock *sb;
  struct dinode *ip = (struct dinode*) malloc(sizeof(struct dinode));
  char sb_buf[BSIZE];
  rsect(mmapped, 1, sb_buf);
  sb = (struct superblock*) sb_buf;
  char buf[BSIZE];
  uint block_num;
  uint indirect[NINDIRECT];

  rsect(mmapped, sb->bmapstart, buf);
  for(int i = 1; i <= sb->ninodes; i++){
    rinode(mmapped, i, ip, sb);
    if(ip->type == 0){
      continue;
    }
    else{
      if(ip->size == 0){
        continue;
      }
      else{
        if(ip->size%BSIZE == 0)
            block_num = ip->size/BSIZE;
        else
            block_num = ip->size/BSIZE + 1;

        if(block_num <= NDIRECT){
          for(int j = 0; j < block_num; j++){
            if(bitmap_exist(ip->addrs[j],buf)){
              printf("DADDR:%d %d\n", ip->addrs[j],bitmap_exist(ip->addrs[j], buf));
              continue;
            }
            else{
              printf("ERROR: address used by inode but marked free by bitmap\n");
            }
            // printf("DADDR:%d %d\n", ip->addrs[j],bitmap_exist(ip->addrs[j], buf));
          }
        }
        else{
          for(int j = 0; j <= NDIRECT; j++){
            if(bitmap_exist(ip->addrs[j],buf)){
              printf("DADDR:%d %d\n", ip->addrs[j],bitmap_exist(ip->addrs[j], buf));
              continue;
            }
            else{
              printf("ERROR: address used by inode but marked free by bitmap\n");
            }
            // printf("DADDR:%d %d\n", ip->addrs[j],bitmap_exist(ip->addrs[j], buf));
          }
          rsect(mmapped, ip->addrs[NDIRECT], (char*)indirect);
          for(int j = 0; j < block_num - NDIRECT; j++){
            if(bitmap_exist(indirect[j],buf)){
              printf("IADDR:%d %d\n", indirect[j], bitmap_exist(indirect[j], buf));
              continue;
            }
            else{
              printf("ERROR: address used by inode but marked free by bitmap\n");
            }
            // printf("IADDR:%d %d\n", indirect[j], bitmap_exist(indirect[j], buf));
          }
        }
     }
    }

  }
}

void bitmap_delete(int addr, char* bitmap){
  ushort char_index;
  ushort char_loca;
  uchar temp;
  uchar a = 0xFE; //0x...每一位为hex而不是bit
  if((addr+1)%8 == 0){
    char_index = (addr+1)/8 - 1;
  }
  else{
    char_index = (addr+1)/8;
  }
  char_loca = addr%8;
  temp =  ((a << char_loca) | (a >> (8-char_loca)));
  bitmap[char_index] = bitmap[char_index] & temp;
}

void bitmap_check_6(char* mmapped){
  struct superblock *sb;
  struct dinode *ip = (struct dinode*) malloc(sizeof(struct dinode));
  char sb_buf[BSIZE];
  rsect(mmapped, 1, sb_buf);
  sb = (struct superblock*) sb_buf;
  char buf[BSIZE];
  uint block_num;
  uint indirect[NINDIRECT];

  rsect(mmapped, sb->bmapstart, buf);
  for(int i = 1; i <= sb->ninodes; i++){
    rinode(mmapped, i, ip, sb);
    if(ip->type == 0){
      continue;
    }
    else{
      if(ip->size == 0){
        continue;
      }
      else{
        if(ip->size%BSIZE == 0)
            block_num = ip->size/BSIZE;
        else
            block_num = ip->size/BSIZE + 1;

        if(block_num <= NDIRECT){
          for(int j = 0; j < block_num; j++){
            bitmap_delete(ip->addrs[j],buf);
          }
        }
        else{
          for(int j = 0; j <= NDIRECT; j++){
            bitmap_delete(ip->addrs[j],buf);
          }
          rsect(mmapped, ip->addrs[NDIRECT], (char*)indirect);
          for(int j = 0; j < block_num - NDIRECT; j++){
            bitmap_delete(indirect[j],buf);
          }
        }
     }
    }

  }
  int start_block = sb->bmapstart + 1;
  int startblock_index;
  startblock_index = start_block/8;
  if(buf[startblock_index] ^ (int) (pow(2,start_block%8)-1)){
    printf("ERROR: bitmap marks block in use but it is not in use\n");
    exit(1);
  }

  for(int i = startblock_index+1; i < BSIZE; i++){
    if(buf[i] != 0){
      printf("ERROR: bitmap marks block in use but it is not in use\n");
      exit(1);
    }
  }
}

void address_check_7_8(char* mmapped){
  struct superblock *sb;
  struct dinode *ip = (struct dinode*) malloc(sizeof(struct dinode));
  char sb_buf[BSIZE];
  rsect(mmapped, 1, sb_buf);
  sb = (struct superblock*) sb_buf;
  char buf[BSIZE];
  uint block_num;
  uint indirect[NINDIRECT];
  uint address_count[FSSIZE];
  memset(address_count, 0, FSSIZE*sizeof(uint));

  rsect(mmapped, sb->bmapstart, buf);
  for(int i = 1; i <= sb->ninodes; i++){
    rinode(mmapped, i, ip, sb);
    if(ip->type == 0){
      continue;
    }
    else{
      if(ip->size == 0){
        continue;
      }
      else{
        if(ip->size%BSIZE == 0)
            block_num = ip->size/BSIZE;
        else
            block_num = ip->size/BSIZE + 1;

        if(block_num <= NDIRECT){
          for(int j = 0; j < block_num; j++){
            // address_count[ip->addrs[j]]++;
            if(address_count[ip->addrs[j]] > 0){
              if(address_count[ip->addrs[j]] == 1)
                  printf("ERROR: direct address used more than once.\n");
              else
                  printf("ERROR: indirect address used more than once.\n");
              exit(1);
            }
            address_count[ip->addrs[j]]++;
          }
        }
        else{
          for(int j = 0; j <= NDIRECT; j++){
            // address_count[ip->addrs[j]]++;
            if(address_count[ip->addrs[j]] > 0){
              if(address_count[ip->addrs[j]] == 1)
                  printf("ERROR: direct address used more than once.\n");
              else
                  printf("ERROR: indirect address used more than once.\n");
              exit(1);
            }
            address_count[ip->addrs[j]]++;
          }
          rsect(mmapped, ip->addrs[NDIRECT], (char*)indirect);
          for(int j = 0; j < block_num - NDIRECT; j++){
            // address_count[indirect[j]]++;
            if(address_count[indirect[j]] > 0){
              if(address_count[ip->addrs[j]] == 1)
                  printf("ERROR: direct address used more than once.\n");
              else
                  printf("ERROR: indirect address used more than once.\n");
              exit(1);
            }
            address_count[indirect[j]] += 2;
          }
        }
     }
    }

  }
}

void inode_check_9(char* mmapped, uint inode_num){
  struct superblock *sb;
  struct dinode *ip = (struct dinode*) malloc(sizeof(struct dinode));
  struct dinode *inode = (struct dinode*) malloc(sizeof(struct dinode));
  char sb_buf[BSIZE];
  rsect(mmapped, 1, sb_buf);
  sb = (struct superblock*) sb_buf;
  struct dirent* direct = (struct dirent*) malloc(sizeof(struct dirent));
  char buf[BSIZE];
  uint indirect[NINDIRECT];
  uint block_num;

  rinode(mmapped, inode_num, ip, sb);
  if(ip->size%BSIZE == 0)
      block_num = ip->size/BSIZE;
  else
      block_num = ip->size/BSIZE + 1;
  rsect(mmapped, ip->addrs[0], buf);
  if(block_num <= NDIRECT){
    for(int i = 0; i < block_num; i++){
      rsect(mmapped, ip->addrs[i], buf);
      for(int j = 0; j < BSIZE/sizeof(struct dirent); j++){
        memcpy(direct, buf+sizeof(struct dirent)*j, sizeof(struct dirent));
        if(direct->inum > 1){
          rinode(mmapped, direct->inum, inode, sb);
          if(inode->type == 1){
            inode_check_9(mmapped, direct->inum);
          }
          else if(inode->type == 0){
            printf("ERROR: inode marked use but not found in a directory\n");
          }
          else{
            printf("%u %s\n", direct->inum,direct->name);
          }
        }
      }
    }

  }
  else{
    for(int i = 0; i < NDIRECT; i++){
      rsect(mmapped, ip->addrs[i], buf);
      for(int j = 0; j < BSIZE/sizeof(struct dirent); j++){
        memcpy(direct, buf+sizeof(struct dirent)*j, sizeof(struct dirent));
        if(direct->inum > 1){
          rinode(mmapped, direct->inum, inode, sb);
          if(inode->type == 1){
            inode_check_9(mmapped, direct->inum);
          }
          else if(inode->type == 0){
            printf("ERROR: inode marked use but not found in a directory\n");
          }
          else{
            printf("%u %s\n", direct->inum,direct->name);
          }
        }
      }
    }
    rsect(mmapped, ip->addrs[NDIRECT], (char*)indirect);
    for(int i = 0; i < block_num - NDIRECT; i++){
      rsect(mmapped, indirect[i], buf);
      for(int j = 0; j < BSIZE/sizeof(struct dirent); j++){
        memcpy(direct, buf+sizeof(struct dirent)*j, sizeof(struct dirent));
        if(direct->inum > 1){
          rinode(mmapped, direct->inum, inode, sb);
          if(inode->type == 1){
            inode_check_9(mmapped, direct->inum);
          }
          else if(inode->type == 0){
            printf("ERROR: inode marked use but not found in a directory\n");
          }
          else{
            printf("%u %s\n", direct->inum,direct->name);
          }
        }
      }
    }
  }

}