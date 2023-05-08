#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

typedef struct SharedMemData {
    int num;
    char message[100];
}shmem_t;


