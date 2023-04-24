#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h> // Use for 

#define MY_SOCK_NAME    "/tmp/MySockTest"
#define BUFFER_SIZE     128

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
    int res;
    char buf[BUFFER_SIZE];

    /* incase the program exited inadvertently on the last run,
     * remove the socket.
     */
    unlink(MY_SOCK_NAME);

    /* Create Master Socket 
     * SOCK_DGRAM for Datagram based communication
     * SOCK_STREAM for Stream based communication
     */
    conn_sock = socket(AF_UNIX, SOCK_STREAM, 0);

    /* initialize */
    memset(&addr, 0, sizeof(struct sockaddr_un));

    /* specify the socket Cridentials */
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, MY_SOCK_NAME, sizeof(addr.sun_path) - 1); // Copy the string to the sun_path, the -1 is to leave room for the null terminator

    /* Bind socket to socket name. */
    ret = bind(conn_sock, (const struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (ret < 0) { /* If the return value is less than 0, then there was an error */
        perror("bind");
        exit(EXIT_FAILURE);
    }
    printf("bind successful\n");

    /* Listen on the socket */
    ret = listen(conn_sock, 20); // 20 is the backlog num of connections that can be waiting while processing request
    if (ret < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("listen successful\n");

    while(1)
    {
        /* Accept a connection */
        data_socket = accept(conn_sock, NULL, NULL);
        if (data_socket < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("Connection accepted from client\n");

        res = 0;
        for(;;)
        {
            memset(buf, 0, BUFFER_SIZE); // Clear the buffer, so that you don't send any left over garbage data
            /* Wait for next data packet. */
            /* This read() function can be used to read the data sent by the client */
            ret = read(data_socket, buf, BUFFER_SIZE);
            if (ret < 0)
            {
                perror("read error");
                exit(EXIT_FAILURE);
            }

            memcpy(&data, buf, sizeof(int));
            if (data == 0) break;
            res += data;
        }

        memset(buf, 0, BUFFER_SIZE);
        sprintf(buf, "Result = %d\n", res);

        printf("Sending final result back to client\n");
        ret = write(data_socket, buf, BUFFER_SIZE);
        if (ret < 0)
        {
            perror("write error");
            exit(EXIT_FAILURE);
        }

        close(data_socket);
    }

    close(conn_sock);
    unlink(MY_SOCK_NAME);

    return 0;
}