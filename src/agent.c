#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"

int agent(char* router_location_file_name) {
    // printf("%ld\n",sizeof(struct NeighborDV));
    FILE *router_file = fopen(router_location_file_name, "r");
    
    if (router_file == NULL)
    {
        // printf("ROUTER LOCATION FILE NOT FOUND!\n");
        return -1;
    }
    else
    {
        // printf("ROUTER LOCATION FILE EXISTS!\n");
    }

    int map_router[10];
    struct routers_by_agent my_routers[10];
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buff[BUFF_SIZE];

    memset(map_router,0,sizeof(map_router));
    memset(my_routers,0,sizeof(my_routers));
    memset(&address, 0, sizeof(struct sockaddr_in));
    memset(&buff, 0, sizeof(buff));

    // printf("Start reading router location file\n");
    
    char* router_num = fgets(buff, BUFF_SIZE, router_file);
    int r_n = atoi(router_num);
    // printf("%d\n",r_n);
    for(int i = 0; i < r_n; i++)
    {
        memset(&buff,0,sizeof(buff));
        fgets(buff, BUFF_SIZE, router_file);
        char* token[3];
        token[0] = strtok(buff, ",");
        for(int j=1;j<3;j++)
        {
            token[j] = strtok(NULL, ",");
        }
        token[2][strlen(token[2])-1] = 0;

        strncpy(my_routers[i].ip, token[0],strlen(token[0]));
        my_routers[i].port = atoi(token[1]);
        my_routers[i].id = atoi(token[2]); 
        my_routers[i].fd = 0;
        map_router[i] = my_routers[i].id;

        int sender_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sender_fd == 0) {
            perror("create socket failed");
            exit(EXIT_FAILURE);
        }
        int option = 1;
        setsockopt(sender_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
        my_routers[i].fd = sender_fd;
        // create socket address
        // forcefully attach socket to the port
        memset(&address, 0, sizeof(struct sockaddr_in));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(my_routers[i].ip);
        address.sin_port = htons(my_routers[i].port);

        // if(inet_pton(AF_INET, my_routers[i].ip, &address.sin_addr)<=0) {
        //     perror("address failed");
        //     exit(EXIT_FAILURE);
        // }
        // connect 
        if (connect(sender_fd, (struct sockaddr *)&address, sizeof(address))<0) {
            perror("connect failed");
            exit(EXIT_FAILURE);
        }
        // printf("Connected! fd: %d\n", sender_fd);

        //test

        // printf("IP: %s, port: %d, ID: %d\n", my_routers[i].ip, my_routers[i].port, map_router[i]);
        
    }
    // printf("Finish reading router location file\n\n");

    
    // agents_router(my_routers, r_n);
    char cmd[BUFF_SIZE];
    char cmd_buff[BUFF_SIZE];

    while(1)
    {
        memset(cmd, 0, sizeof(cmd));
        memset(&cmd_buff,0,sizeof(cmd_buff));
        memset(&buff,0,sizeof(buff));
        struct RouterCMD* cmd_to_router = (struct RouterCMD*)buff;

        scanf("%s", cmd); //get cmd
        // printf("You enter: %s\n",cmd);

        if(strcmp("dv",cmd) == 0)
        {
            cmd_to_router->cmd_type = CMD_DV;
            for(int i = 0;i <r_n;i++)
            {
                // printf("DV | ind: %d, fd: %d, cmd: %d\n", i, my_routers[i].fd, cmd_to_router->cmd_type);
                write(my_routers[i].fd, (void*)buff, sizeof(struct RouterCMD));
            }
            continue;
        }

        strncpy(cmd_buff,cmd,7);
        if(strcmp("update:",cmd_buff) == 0)
        {
            // printf("update start \n");
            char* token[3];
            memset(cmd_buff, 0, sizeof(cmd_buff));
            strncpy(cmd_buff, cmd+7, strlen(cmd)-7);
            token[0] = strtok(cmd_buff, ",");
            for(int j=1;j<3;j++)
            {
                token[j] = strtok(NULL, ",");
            }
            // token[2][strlen(token[2])-1] = 0;
            int from = atoi(token[0]);
            int to = atoi(token[1]);
            int weight = atoi(token[2]);
            // printf("update settle \n");

            cmd_to_router->cmd_type = CMD_UPDATE;
            cmd_to_router->data[0] = get_index(from, map_router);
            cmd_to_router->data[1] = get_index(to, map_router);
            
            cmd_to_router->data[2] = weight;
            
            write(my_routers[cmd_to_router->data[0]].fd, (void*)buff, sizeof(struct RouterCMD));
            // printf("Update | %d -> %d, weight: %d. fd:%d cmd: %d\n", 
                    // cmd_to_router->data[0], cmd_to_router->data[1], cmd_to_router->data[2], 
                    // my_routers[cmd_to_router->data[0]].fd, cmd_to_router->cmd_type);

            continue;
        }

        memset(cmd_buff, 0, sizeof(cmd_buff));
        strncpy(cmd_buff,cmd,5);
        if(strcmp("show:",cmd_buff) == 0)
        {
            // printf("show\n");
            memset(cmd_buff, 0, sizeof(cmd_buff));
            strncpy(cmd_buff, cmd+5, strlen(cmd)-5);

            int ind = get_index(atoi(cmd_buff), map_router);

            cmd_to_router->cmd_type = CMD_SHOW;
            
            // printf("SHOW | ind: %d, fd: %d, cmd: %d\n", ind, my_routers[ind].fd, cmd_to_router->cmd_type);
            write(my_routers[ind].fd, (void*)buff, sizeof(struct RouterCMD));
            
            memset(cmd_buff, 0, sizeof(cmd_buff));
            int n = read(my_routers[ind].fd, cmd_buff, BUFF_SIZE-1);
            cmd_buff[n] = 0;
            struct wholeRoutingTable* show_table = (struct wholeRoutingTable*)cmd_buff;
            // printf("Received form router %d, msg: %s\n",ind,cmd_buff);
            for(int i = 0; i<r_n;i++)
            {
                if(show_table->table[i].distance == -1)
                {
                    continue;
                }
                int next = -1;
                if(show_table->table[i].next_hop >= 0)
                {
                    next = map_router[show_table->table[i].next_hop];
                }
                printf("dest: %d, next: %d, cost: %d\n",
                    map_router[i], next, 
                    show_table->table[i].distance);
            }
            // printf("\n");
            continue;
        }

        memset(cmd_buff, 0, sizeof(cmd_buff));
        strncpy(cmd_buff,cmd,6);
        if(strcmp("reset:",cmd_buff) == 0)
        {
            // printf("reset\n");
            memset(cmd_buff, 0, sizeof(cmd_buff));
            strncpy(cmd_buff, cmd+6, strlen(cmd)-6);

            int ind = get_index(atoi(cmd_buff), map_router);

            cmd_to_router->cmd_type = CMD_RESET;
            
            // printf("RESET | ind: %d, fd: %d, cmd: %d\n", ind, my_routers[ind].fd, cmd_to_router->cmd_type);
            write(my_routers[ind].fd, (void*)buff, sizeof(struct RouterCMD));
            continue;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    char *router_location_file_name;

    if (argc != 2) {
        fprintf(stderr, "Usage: ./agent [Router Location File]\n");
        exit(EXIT_FAILURE);
    }

    router_location_file_name = argv[1];
    setvbuf(stdout, NULL,_IONBF, 0);
    return agent(router_location_file_name);
    // sender();
    // return 0;
}

