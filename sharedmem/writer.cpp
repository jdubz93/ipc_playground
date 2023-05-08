#include "shared_mem.hpp"
#include <iostream>
#include <iterator>
#include <sys/mman.h>
#include <algorithm>

int main(void)
{
    const char *shared_mem_path = "/my_shared_mem";

    /* Create the shared meomry object */
    int shm_fd = shm_open(shared_mem_path, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        std::cerr << "Failed to create shared memory" << std::endl;
        return 1;
    }

    /* set the size of shared memory */
    ftruncate(shm_fd, sizeof(shmem_t));

    /* map the shared memory into the process address space */
    shmem_t *shared_data = (shmem_t *)mmap(nullptr, sizeof(shmem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shared_data == MAP_FAILED)
    {
        std::cerr << "Failed to map shared memory" << std::endl;
        return 1;
    }

    /* Note this will not work ( shared_data->message = "Hello World from shared memory"; ) */
    shared_data->num = 1234;
    std::string str = "Hello World from Shared Memory - this is a new message";
    std::copy(str.begin(), str.end(), shared_data->message);
    shared_data->message[str.size()] = '\0';

    /* unmap the shared memory */
    munmap(shared_data, sizeof(shmem_t));
    close(shm_fd);

    exit(EXIT_SUCCESS);
}
