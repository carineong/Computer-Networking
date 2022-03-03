#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>

# define BUFF_SIZE 5500
# define MAXNUM_ROUTER 10

#define CMD_DV 0
#define CMD_UPDATE   1
#define CMD_SHOW  2
#define CMD_RESET   3

#define AGENT_RECVFD 10
#define MAIN_RECVFD 11


#define max(a,b) (a>b?a:b)
#define min(a,b) (a<b?a:b)

struct NeighborDV{
    int dv[10];
};

struct sendNeighborDV{
    char dv[10];
};

struct RoutingTable{
    int next_hop;
    int distance;
};
struct wholeRoutingTable{
    struct RoutingTable table[10];
};
struct the_Router{
    int recv_fd[12];
    int send_fd[10];
    int id;
    int port;
    char ip[20];
    int num_con;
    int cost_table[10];
    int counter;
    struct NeighborDV neighbors_dv[10];
    struct RoutingTable routing_table[10];
    struct sockaddr_in address;
};

struct routers_by_agent{
    int fd;
    int id;
    int port;
    char ip[20];
};

struct RouterCMD{
    int cmd_type;
    int data[5];
};

struct Neighbor_DV{

};
int get_index(int ind, int arr[]);
int check_arr(int arr[], int n);
void print_router_info(struct the_Router the_routers[], int n, int all, int the_i);
void agents_router(struct routers_by_agent the_routers[], int n);

