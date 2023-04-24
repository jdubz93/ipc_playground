#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h> // Use for

// #define MY_SOCK_NAME    "/tmp/MySockTest"
#define BUFFER_SIZE     128

// #undef MY_SOCK_NAME
#define MY_SOCK_NAME    "/tmp/MySockMulti"

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
    int i;
    int result;
    char buffer[BUFFER_SIZE];

    data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if(data_socket < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* initialize client socket */
    memset(&addr, 0, sizeof(struct sockaddr_un));

    /* specify the socket credentials */
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, MY_SOCK_NAME, sizeof(addr.sun_path) - 1); //

    /* Connect the client socket to the server socket */
    ret = connect(data_socket, (const struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if(ret < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    do{
        printf("enter a number to send to server: ");
        scanf("%d", &i);
        /* Send data */
        ret = write(data_socket, &i, sizeof(int));
        if(ret < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
        printf("no of bytes sent: %d, data sent: %d\n", ret, i);
    } while(i);

    memset(buffer, 0, BUFFER_SIZE);
    strncpy(buffer, "RES", strlen("RES"));
    buffer[strlen(buffer)] = '\0';
    printf("buffer: %s\n", buffer);

    ret = write(data_socket, buffer, strlen(buffer));
    if(ret < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    /* Receive result */
    memset(buffer, 0, BUFFER_SIZE); // ptr to memory, value to set, size of memory block.
    ret = read(data_socket, buffer, BUFFER_SIZE);
    if(ret < 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    buffer[BUFFER_SIZE - 1] = '\0'; // Null terminate the buffer
    printf("%s\n", buffer);

    /* Close socket */
    close(data_socket);


    // // /* Send arguments */
    // // printf("Sending arguments to server...\n");
    // // strcpy(buffer, "Hello");
    // // ret = write(data_socket, buffer, strlen(buffer) + 1);
    // // if(ret < 0)
    // // {
    // //     perror("write");
    // //     exit(EXIT_FAILURE);
    // // }
    // // printf("waiting for response...\n");

    // /* Receive result */
    // ret = read(data_socket, buffer, BUFFER_SIZE);
    // if(ret < 0)
    // {
    //     perror("read");
    //     exit(EXIT_FAILURE);
    // }
    // printf("Result = %s\n", buffer);

    // /* Close socket */
    // close(data_socket);

    return 0;
}