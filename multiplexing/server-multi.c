#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define MY_SOCK_NAME    "/tmp/MySockMulti"
#define BUFFER_SIZE     128

#define MAX_CLIENT_SUPPORTED   32

int monitored_fd_set[MAX_CLIENT_SUPPORTED];

int client_result[MAX_CLIENT_SUPPORTED] = {0};

static void 
initialize_monitor_fd_set()
{
    int i = 0;
    for(; i < MAX_CLIENT_SUPPORTED; i++)
        monitored_fd_set[i] = -1;
}

static void 
add_to_monitored_fd_set(int skt_fd)
{
    int i = 0;
    for(; i<MAX_CLIENT_SUPPORTED; i++) {
        if (monitored_fd_set[i] != -1)
            continue;
        monitored_fd_set[i] = skt_fd;
        break;
    }
}

static void 
remove_from_monitored_fd_set(int skt_fd)
{
    int i = 0;
    for(; i<MAX_CLIENT_SUPPORTED; i++)
    {
        if (monitored_fd_set[i] != skt_fd)
            continue;

        monitored_fd_set[i] = -1;
        break;
    }
}

static void 
print_monitored_fd_set()
{
    int i = 0;
    printf("Monitored FD set: ");
    for(; i<MAX_CLIENT_SUPPORTED; i++)
    {
        if (monitored_fd_set[i] == -1)
            continue;
        printf("%d ", monitored_fd_set[i]);
    }
    printf("\r");
}

static void
refresh_fd_set(fd_set *fd_set_ptr)
{
    int i = 0;
    FD_ZERO(fd_set_ptr);
    for(; i<MAX_CLIENT_SUPPORTED; i++)
    {
        if(monitored_fd_set[i] != -1)
            FD_SET(monitored_fd_set[i], fd_set_ptr);
    }
}

static int
get_max_fd()
{
    int i = 0;
    int max_fd = -1;

    for(; i<MAX_CLIENT_SUPPORTED; i++)
    {
        if(monitored_fd_set[i] > max_fd)
            max_fd = monitored_fd_set[i];
    }
    return max_fd;
}

int main(void)
{
    struct sockaddr_un addr;

#if 0
    struct sockaddr_un {
        sa_family_t sun_family;               /* AF_UNIX */
        char        sun_path[108];            /* pathname */
    };
#endif

    int ret;
    int data_socket;
    int conn_sock;
    int data;
    int result;
    char buffer[BUFFER_SIZE];
    fd_set read_fd_set;
    int comm_sock_fd, i;
    initialize_monitor_fd_set();
    add_to_monitored_fd_set(0);

    unlink(MY_SOCK_NAME);

    conn_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(conn_sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    printf("Master socket created\n");


    memset(&addr, 0, sizeof(struct sockaddr_un));

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, MY_SOCK_NAME, sizeof(addr.sun_path) -1);

    ret = bind(conn_sock, (const struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if(ret < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    printf("bind() call succeed\n");

    ret = listen(conn_sock, 20);
    if(ret < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    add_to_monitored_fd_set(conn_sock);

    while(1) 
    {
        refresh_fd_set(&read_fd_set);
        print_monitored_fd_set();

        /* Wait for incoming connection. */
        printf("Waiting on select() sys call\n");

        ret = select(get_max_fd() + 1, &read_fd_set, NULL, NULL, NULL);
        if(FD_ISSET(conn_sock, &read_fd_set))
        {
            printf("New connection request received.\n");
            data_socket = accept(conn_sock, NULL, NULL);
            if(data_socket < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            add_to_monitored_fd_set(data_socket);
            printf("New connection accepted.\n");
        }
        else if(FD_ISSET(0, &read_fd_set))
        {
            memset(buffer, 0, BUFFER_SIZE);
            ret = read(0, buffer, BUFFER_SIZE);
            printf("Received command from user: %s\n", buffer);
        }
        else
        {
            // Check for data from client
            i = 0, comm_sock_fd = -1;
            for(; i<MAX_CLIENT_SUPPORTED; i++)
            {
                if (FD_ISSET(monitored_fd_set[i], &read_fd_set))
                {
                    comm_sock_fd = monitored_fd_set[i];
                    memset(buffer, 0, BUFFER_SIZE);
                    printf("Waiting for data from client %d\r", comm_sock_fd);
                    ret = read(comm_sock_fd, buffer, BUFFER_SIZE);
                    if(ret < 0)
                    {
                        perror("read");
                        exit(EXIT_FAILURE);
                    }

                    memcpy(&data, buffer, sizeof(int));
                    printf("Received data from client %d: %d\r", comm_sock_fd, data);
                    if(data == 0)
                    {
                        memset(buffer, 0, BUFFER_SIZE);
                        sprintf(buffer, "Result: %d", client_result[i]);
                        ret = write(comm_sock_fd, buffer, BUFFER_SIZE);
                        if(ret < 0)
                        {
                            perror("write");
                            exit(EXIT_FAILURE);
                        }

                        close(comm_sock_fd);
                        remove_from_monitored_fd_set(comm_sock_fd);
                        client_result[i] = 0;
                        continue; /* go to select() and block*/
                    }
                    client_result[i] += data;
                }
            }
        }
    }
    close(conn_sock);
    remove_from_monitored_fd_set(conn_sock);
    printf("Server exiting...\n");
    unlink(MY_SOCK_NAME);
    exit(EXIT_SUCCESS);
}