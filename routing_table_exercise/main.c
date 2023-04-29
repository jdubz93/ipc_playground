/*
 * Routing table entries via process (RTM) Routing Table Manager
 * CUD: Create, Update, Delete
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_LINE                1024
#define MY_SOCKET_MULTI         "/tmp/MySockMulti"
#define MAX_CLIENTS_SUPPORTED    32

int monitored_fd_set[MAX_CLIENTS_SUPPORTED];
int client_result[MAX_CLIENTS_SUPPORTED] = {0};

static void
initialize_monitor_set_fd()
{
	int i = 0;
	for(; i < MAX_CLIENTS_SUPPORTED; i++)
		monitored_fd_set[i] = -1;
}

static void
add_to_monitored_fd_set(int skt_fd)
{
	int i = 0;
	for(; i < MAX_CLIENTS_SUPPORTED; i++)
	{
		if(monitored_fd_set[i] != -1)
			continue;
		
		monitored_fd_set[i] = skt_fd; 
		break;
	}
}

static void
remove_from_monitored_fd_set(int skt_fd)
{
	int i = 0;
	for(; i < MAX_CLIENTS_SUPPORTED; i++)
	{
		if(monitored_fd_set[i] != skt_fd)
			continue;

		monitored_fd_set[i] = -1;
		break;
	}
}

static void
refresh_fd_set(fd_set *fd_set_ptr)
{
	int i = 0;
	FD_ZERO(fd_set_ptr);
	for(; i < MAX_CLIENTS_SUPPORTED; i++)
		if(monitored_fd_set[i] != -1)
			FD_SET(monitored_fd_set[i], fd_set_ptr);
}

static int
get_max_fd()
{
	int max = -1;
	int i = 0;
	for(; i < MAX_CLIENTS_SUPPORTED; i++)
		if(monitored_fd_set[i] > max)
			max = monitored_fd_set[i];

	return max;
}

typedef enum {
    CREATE,
    DELETE,
    UPDATE,
} op_code_t;

typedef struct {
    char dest_ip[16];  /* destination IP address */
    char mask;      /* subnet mask */
    char gw_ip[16]; /* gateway IP address */
    char oif[32];   /* outgoing interface */
} route_t;

typedef struct {
    op_code_t command;
    route_t route;
} message_t;

/* Define the linked list node structure */
typedef struct Node {
    void *data;
    struct Node *next;
} node_t;

// typedef struct dll_{
//     dll_node_t *head;
//     dll_node_t *tail;
// } dll_t;

void display_usage(void)
{
    printf("Usage: rtm <command> <dest_ip> <mask> <gw_ip> <oif>\n");
    printf("   <command> - create, delete, update   \n");
    printf("   <dest_ip> - destination IP address   \n");
    printf("   <mask>    - subnet mask              \n");
    printf("   <gw_ip>   - gateway IP address       \n");
    printf("   <oif>     - outgoing interface       \n");
}

int netmask_to_cidr(const char* netmask) {
    struct in_addr mask;
    int cidr = 0;

    if (inet_pton(AF_INET, netmask, &mask) == 1) {
        uint32_t mask_bits = ntohl(mask.s_addr);
        while (mask_bits) {
            cidr += (mask_bits & 1);
            mask_bits >>= 1;
        }
    }

    return cidr;
}

void display_route(const node_t *node)
{
    printf("========Routing Table=========\n");

    while(node != NULL)
    {
        message_t *message = (message_t*)node->data;
        route_t *route = &message->route;
        printf("dest_ip: %s mask: %d gw_ip: %s oif: %s", route->dest_ip, route->mask, route->gw_ip, route->oif);
        node = node->next;
    }
}

int compare_messages(const message_t *msg1, const message_t *msg2)
{
    //printf("message1: %s %d %s %s\n", msg1->route.dest_ip, msg1->route.mask, msg1->route.gw_ip, msg1->route.oif);
    //printf("message2: %s %d %s %s\n", msg2->route.dest_ip, msg2->route.mask, msg2->route.gw_ip, msg2->route.oif);
    //if(msg1->route.dest_ip != msg2->route.dest_ip)
//	    return -1;

    if(strncmp(msg1->route.dest_ip, msg2->route.dest_ip, sizeof(msg1->route.dest_ip)) != 0)
	    return -1;

    if(msg1->route.mask != msg2->route.mask)
	    return -1;

    if(strncmp(msg1->route.gw_ip, msg2->route.gw_ip, sizeof(msg1->route.gw_ip)) != 0)
        return -1;

    return 0;
}

node_t *create_routing_table(const message_t *message)
{
    node_t *node = (node_t*)malloc(sizeof(node_t));
    if(node == NULL)
    {
        perror("malloc | not enough memory.");
        exit(EXIT_FAILURE);
    }
    // printf("create_routing_table(): does this get called?\n");
    node->data = (message_t*)malloc(sizeof(message_t));
    memcpy(node->data, message, sizeof(message_t));
    // printf("create_routing_table(): does this get called2?\n");
    node->next = NULL;
    return node;
}

void add_route(node_t **head, const message_t *message)
{
    // node_t *new_node = (node_t*)malloc(sizeof(node_t));
    node_t *new_node = create_routing_table(message);
    if(new_node == NULL)
    {
        perror("malloc | not enough memory.");
        exit(EXIT_FAILURE);
    }
    memcpy(new_node->data, message, sizeof(message_t));
    new_node->next = *head;
    *head = new_node;

    display_route(*head);
}

void delete_route(node_t **head, const message_t *message)
{
    node_t *temp = *head;
    node_t *prev = NULL;

    //int test = memcmp((message_t*)temp->data, message, sizeof(message_t));
    //int test = compare_messages((message_t *)temp->data, message); 
    //printf("delete_route(): test = %d\n", test);

    if ((temp != NULL) && (compare_messages((message_t *)temp->data, message) == 0)) 
    {
        *head = temp->next;
        free(temp);
        return;
    }

    while((temp != NULL) && (compare_messages((message_t *)temp->data, message) != 0))
    {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) return;

    prev->next = temp->next;
    free(temp);
}

void update_route(node_t **head, const message_t *message)
{
    node_t *temp = *head;

    while((temp != NULL) && (compare_messages((message_t *)temp->data, message) != 0))
    {
        temp = temp->next;
    }

    if (temp == NULL) return;

    memcpy(temp->data, message, sizeof(message_t));
}

int main(void)
{

	struct sockaddr_un name;
#if 0
	struct sockaddr_un {
		sa_family_t sun_family;         /* AF_UNIX */
		char        sun_path[108];      /* pathname */
	};
#endif
	int ret;
	int connection_socket;
	int data_socket;
	int result;
	char buffer[BUFFER_SIZE];
	fd_set readfds, testfds;
	int i, maxfd, comm_socket_fd;
	int nbytes;
	initialize_monitor_fd_set();
	add_monitor_fd_set(0);
	maxfd = -1;

	/* Remove the socket file if it already exists */
	unlink(SOCKET_NAME);

	/* Create Master Socket */
	connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (connection_socket == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	printf("Master Socket Created\n");

	/* Initialize */
	memset(&name, 0, sizeof(struct sockaddr_un));

	/* Specify the socket Credentials */
	name.sun_family = AF_UNIX;
	strncpy(name.sun_path, MY_SOCKET_MULTI, sizeof(name.sun_path) - 1);

	/* Bind socket to socket name */
	ret = bind(connection_socket, (const struct sockaddr *) &name, sizeof(struct sockaddr_un));
	if (ret == -1)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}
	printf("bind() succeeded\n");

	/* Listen on the socket for connections */
	ret = listen(connection_socket, 20);
	if (ret == -1)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	printf("listen() succeeded\n");

	/* Initialize the master fd_set */
	FD_ZERO(&readfds);
	FD_SET(connection_socket, &readfds);
	maxfd = connection_socket;


    char line[MAX_LINE];
    char *token;
    char *delim = " ";
    message_t message;
    node_t *head = NULL;

    printf("Routing Table Manager (RTM) \n");
    printf("Enter 'help' for usage \n");

    while(fgets(line, MAX_LINE, stdin) != NULL)
    {
        /*
         * Note: strtok() is not thread safe. If using multi-threading then use strtok_r();
         */
        token = strtok(line, delim);
        if(token == NULL)
        {
            display_usage();
            continue;
        }

        if(strcmp(token, "create") == 0) 
        {
            message.command = CREATE;
            printf("cmd received: create\n");
        }
        else if(strcmp(token, "delete") == 0)
        {
            message.command = DELETE;
            printf("cmd received: delete\n");
        }
        else if(strcmp(token, "update") == 0)
        {
            message.command = UPDATE;
            printf("cmd received: update\n");
        }
        else
        {
            display_usage();
            continue;
        }

        token = strtok(NULL, delim);
        if(token == NULL)
        {
            display_usage();
            continue;
        }
        strcpy(message.route.dest_ip, token);
        printf("dest_ip: %s\n", message.route.dest_ip);

        token = strtok(NULL, delim);
        if(token == NULL)
        {
            display_usage();
            continue;
        }
        // message.route.mask = atoi(token);
        // printf("mask: %d\n", message.route.mask);
        message.route.mask = netmask_to_cidr(token);
        printf("mask: %s -> CIDR: /%d\n", token, message.route.mask);

        token = strtok(NULL, delim);
        if(token == NULL)
        {
            display_usage();
            continue;
        }
        strcpy(message.route.gw_ip, token);
        printf("gw_ip: %s\n", message.route.gw_ip);

        token = strtok(NULL, delim);
        if(token == NULL)
        {
            display_usage();
            continue;
        }
        strcpy(message.route.oif, token);
        printf("oif: %s\n", message.route.oif);

        switch(message.command)
        {
            case CREATE:
                // printf("adding route to routing table\n");
                add_route(&head, &message);
                break;
            case DELETE:
                // printf("deleting route from routing table\n");
                delete_route(&head, &message);
                display_route(head);
                break;
            case UPDATE:
                // printf("updating route in routing table\n");
                update_route(&head, &message);
                display_route(head);
                break;
            default:
                printf("invalid command\n");
                // display_route(head);
                break;
        }
    }

    return 0;
}
