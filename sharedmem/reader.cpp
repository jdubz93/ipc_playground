#include "shared_mem.hpp"
#include <cstdlib>
#include <sys/mman.h>

int main(void)
{
    // Open the shared memory object.
    const char *shm_path = "/my_shared_mem";
    int shm_fd = shm_open(shm_path, O_RDONLY, 0666);
    if(shm_fd == -1)
    {
        std::cerr << "Error opening shared memory object." << std::endl;
        exit(EXIT_FAILURE);
    }

    // Map the shared memory object to the address space of this process
    // and get a pointer to the beginning of the block of shared memory.
    const int SIZE = sizeof(shmem_t);
    shmem_t *shmemptr = (shmem_t *)mmap(nullptr, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if(shmemptr == MAP_FAILED)
    {
        std::cerr << "Error mapping shared memory object." << std::endl;
        exit(EXIT_FAILURE);
    }

    // Read the shared memory object.
    std::cout << "Reading from shared memory: " << std::endl;
    std::cout << "  " << shmemptr->num << std::endl;
    std::cout << "  " << shmemptr->message << std::endl;

    // Unmap the shared memory object.
    // Note: The shared memory object remains in the system after the process
    // terminates. To remove the shared memory object, call shm_unlink().
    // shm_unlink(shm_path);
    munmap(shmemptr, SIZE);
    close(shm_fd);
    shm_unlink(shm_path);

    exit(EXIT_SUCCESS);
}
