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

int router_connect(struct the_Router therouter)
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int sender_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sender_fd == 0) {
        perror("create socket failed");
        exit(EXIT_FAILURE);
    }
    
    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(therouter.ip);
    address.sin_port = htons(therouter.port);
    if (connect(sender_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }
    return sender_fd;
}

int router_accept(int main_fd)
{
    struct timeval timeout = {0,100000};
    fd_set thetime;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    memset(&address, 0, sizeof(struct sockaddr_in));
    memset(&address, 0, sizeof(struct sockaddr_in));
        
    FD_ZERO(&thetime);
    FD_SET(main_fd, &thetime);

    int status = select(main_fd+1, &thetime, NULL,NULL, &timeout);
    
    if(status<=0)
    {
        return 0;
    }
    // printf("Available!\n");
    if(FD_ISSET(main_fd, &thetime))
    {
        int new_socket = accept(main_fd,(struct sockaddr *)&address, 
                    (socklen_t*)&addrlen);
        return new_socket;
    }
    return 0;

}

void router_send_dv(struct the_Router therouter, int r_n, int i)
{
    char buff[BUFF_SIZE];
    for(int k = 0; k < r_n; k++)
    {
        if(therouter.send_fd[k] > 0)
        {
            memset(&buff, 0, sizeof(buff));
            struct NeighborDV* send_my_dv = (struct NeighborDV*)buff;
            // printf("Router %d send dv to router %d : ", i, k);
            for(int l = 0; l< r_n; l++)
            {
                send_my_dv->dv[l] = therouter.routing_table[l].distance;
                // printf("%d  ", send_my_dv->dv[l]);
            }
            write(therouter.send_fd[k], (void*)buff, sizeof(struct NeighborDV));
            // printf("\n");
        }
    }  
}

int router(char* router_location_file_name, char* topology_conf_file_name, int router_id) {

    FILE *router_file = fopen(router_location_file_name, "r");
    FILE *topology_file = fopen(topology_conf_file_name, "r");
    
    if (router_file == NULL)
    {
        printf("ROUTER LOCATION FILE NOT FOUND!\n");
        return -1;
    }
    else
    {
        // printf("ROUTER LOCATION FILE EXISTS!\n");
    }

    if (topology_file == NULL)
    {
        printf("TOPOLOGY CONF FILE NOT FOUND!\n");
        return -1;
    }
    else
    {
        // printf("TOPOLOGY CONF FILE EXISTS!\n");
    }
    
    int map_router[10];
    struct the_Router routers[10];
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buff[BUFF_SIZE];
    
    memset(map_router,0,sizeof(map_router));
    memset(routers,0,sizeof(routers));
    memset(&address, 0, sizeof(struct sockaddr_in));
    memset(&buff, 0, sizeof(buff));
    // printf("Start reading router location file\n");

    char* router_num = fgets(buff, BUFF_SIZE, router_file);
    int r_n = atoi(router_num);
    // printf("%d\n",r_n);
    for(int i = 0; i < r_n; i++) //read router location file and initialise the routers
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

        strncpy(routers[i].ip, token[0],strlen(token[0]));
        routers[i].port = atoi(token[1]);
        routers[i].id = atoi(token[2]); 
        routers[i].num_con = 0;
        routers[i].counter = 0;
        memset(routers[i].recv_fd, 0, sizeof(routers[i].recv_fd));
        memset(routers[i].send_fd, 0, sizeof(routers[i].send_fd));
        memset(routers[i].cost_table, 0, sizeof(routers[i].cost_table));
        memset(routers[i].neighbors_dv, 0, sizeof(routers[i].neighbors_dv));
        memset(routers[i].routing_table, 0, sizeof(routers[i].routing_table));

        map_router[i] = routers[i].id;

        // initialize cost_table, neighbors' dv & routing table
        for(int j = 0; j < r_n; j++)
        {
            for(int k = 0; k < r_n; k++) // for dv
            {
                if(j==k)
                {
                    routers[i].neighbors_dv[j].dv[k] = 0;
                }
                else
                {
                    routers[i].neighbors_dv[j].dv[k] = -1;
                }
            }
            
            if(i == j)
            {
                routers[i].cost_table[j] = 0;
                routers[i].routing_table[j].next_hop = j;
                routers[i].routing_table[j].distance = 0;
            }
            else
            {
                routers[i].cost_table[j] = -1;
                routers[i].routing_table[j].next_hop = -1;
                routers[i].routing_table[j].distance = -1;
            }
        }
        
        //test
        int receiver_fd = socket(AF_INET, SOCK_STREAM, 0);

        if (receiver_fd == 0) {
            perror("create socket failed");
            exit(EXIT_FAILURE);
        }
        routers[i].recv_fd[MAIN_RECVFD] = receiver_fd;
        int option = 1;
        setsockopt(receiver_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
        memset(&address, 0, sizeof(struct sockaddr_in));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(routers[i].port);

        // bind rtp socket to address
        if (bind(receiver_fd, (struct sockaddr *)&address, sizeof(struct sockaddr))<0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        // printf("Router %d listening fd:%d\n", i,receiver_fd);
        if (listen(routers[i].recv_fd[MAIN_RECVFD], 12) < 0)
        {
            perror("listen failed");
            exit(EXIT_FAILURE);
        }

        
        //printf("IP: %s, port: %d, ID: %d\n", routers[i].ip, routers[i].port, map_router[i]);
        
    }
    // printf("Finish reading router location file\n");

    // printf("Start reading topology location file\n");
    memset(&buff,0,sizeof(buff));
    char* topo_num = fgets(buff, BUFF_SIZE, topology_file);
    int t_n = atoi(topo_num);
    // printf("%d\n",t_n);
    for(int i = 0; i < t_n; i++)
    {
        memset(&buff,0,sizeof(buff));
        fgets(buff, BUFF_SIZE, topology_file);
        char* token[3];
        token[0] = strtok(buff, ",");
        for(int j = 1; j < 3; j++)
        {
            token[j] = strtok(NULL, ",");
        }
        token[2][strlen(token[2])-1] = 0;
        
        int from = get_index(atoi(token[0]), map_router);
        int to = get_index(atoi(token[1]), map_router);
        int cost = atoi(token[2]);

        if(cost > 1000 || cost <= -1)
        {
            routers[from].cost_table[to] = -1;
            
            routers[from].routing_table[to].next_hop = -1;
            routers[from].routing_table[to].distance = -1;
            
        }
        else
        {
            routers[from].num_con++;
            routers[from].cost_table[to] = cost;
            
            routers[from].routing_table[to].next_hop = to;
            routers[from].routing_table[to].distance = cost;
           
        }
        
        //printf("%d -> %d, Cost: %d\n", from, to, cost);      
    }

    // printf("Finish reading topology location file\n");

    //connect between routers
    for(int i = 0; i < r_n; i++)
    {
        for(int j = 0; j < r_n; j++)
        {
            if(i == j || routers[i].cost_table[j] == -1)
            {
                continue;
            }

            int sender_fd = router_connect(routers[j]);
            routers[i].send_fd[j] = sender_fd;
            
            // printf("%d -> %d, Connected! fd: %d\n", i, j, sender_fd);

            while(1)
            {
                int new_socket = router_accept(routers[j].recv_fd[MAIN_RECVFD]);
                if(new_socket < 0)
                {
                    perror("accept failed");
                    exit(EXIT_FAILURE);
                }
                else if(new_socket == 0)
                {
                    continue;
                }
                // printf("%d -> %d, Accept!\n", j, i);
                routers[j].recv_fd[i] = new_socket;
                break;
            } 
        }  
    }
    // printf("Finish connect between routers\n");
    
    //connect with agent
    int all_connect[10];
    int done = 0;
    memset(&all_connect, 0, sizeof(all_connect));   
    while(1)
    {
        for(int i = 0; i < r_n; i++)
        {
            int new_socket = router_accept(routers[i].recv_fd[MAIN_RECVFD]);
            if(new_socket < 0)
            {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }
            else if(new_socket == 0)
            {
                continue;
            }
            // printf("Accept!\n");
            routers[i].recv_fd[AGENT_RECVFD] = new_socket;
            all_connect[i] = 1;
            done = check_arr(all_connect, r_n);
            break;
        }   
        
        if(done == 1)
        {
            break;
        }
    }
    // printf("Finish connect with agent\n");
    
    // print_router_info(routers, r_n, 0,0);
    // receive msg
    struct timeval timeout = {0,100000};
    fd_set thetime;
    while(1)
    {
        for(int i = 0; i < r_n; i++) //router
        {
            int change = 0;
            for(int j = 0; j < 12; j++) //recv_fd
            {
                if(i == j || routers[i].recv_fd < 0 || (j<r_n && routers[i].cost_table[j] == -1))
                {
                    continue;
                }

                memset(&address, 0, sizeof(struct sockaddr_in));
            
                FD_ZERO(&thetime);
                FD_SET(routers[i].recv_fd[j], &thetime);
            
                int status = select(routers[i].recv_fd[j] + 1, &thetime, NULL,NULL, &timeout);
                
                if(status<=0)
                {
                    continue;
                }
                // printf("Available!\n");
                if(FD_ISSET(routers[i].recv_fd[j], &thetime))
                {
                    // printf("Router %d receive \n",i);
                    memset(&buff, 0, sizeof(buff));
                    int n = read(routers[i].recv_fd[j], buff, BUFF_SIZE-1);
                    buff[n] = 0;
                    if(j==AGENT_RECVFD)
                    {
                        struct RouterCMD *cmd_to_router = (struct RouterCMD *)buff;
                        // printf("Router %d receive: %d, len: %d\n", i, cmd_to_router->cmd_type, n);                
                        
                        if(cmd_to_router->cmd_type == CMD_DV)
                        {
                            // printf("For DV\n");
                            router_send_dv(routers[i], r_n, i);
                        }

                        else if(cmd_to_router->cmd_type == CMD_UPDATE)
                        {
                            int cost = cmd_to_router->data[2];
                            int to = cmd_to_router->data[1];
                            int from = cmd_to_router->data[0];
                            if(cost >1000)
                            {
                                cost = -1;
                            }
                            if(cost == -1 && routers[i].cost_table[to] != -1)
                            {
                                close(routers[to].recv_fd[from]);
                                close(routers[from].send_fd[to]);
                                
                            }
                            else if(routers[i].cost_table[to] == -1 && cost > 0)
                            {
                                // TODO
                                int sender_fd = router_connect(routers[to]);
                                routers[i].send_fd[to] = sender_fd;
                                while(1)
                                {
                                    int new_socket = router_accept(routers[to].recv_fd[MAIN_RECVFD]);
                                    if(new_socket < 0)
                                    {
                                        perror("accept failed");
                                        exit(EXIT_FAILURE);
                                    }
                                    else if(new_socket == 0)
                                    {
                                        continue;
                                    }
                                    // printf("%d -> %d, Accept!\n", j, i);
                                    routers[to].recv_fd[i] = new_socket;
                                    break;
                                } 
                            }
                            
                            routers[i].cost_table[to] = cost;
                            // printf("\n\ninitialise routing table %d\n",i);
                            for(int k = 0; k < r_n; k++) //reset routing table (dest)
                            {
                                routers[i].routing_table[k].distance = routers[i].cost_table[k];
                                routers[i].routing_table[k].next_hop = k;
                                if(routers[i].cost_table[k] == -1)
                                {
                                    routers[i].routing_table[k].next_hop = -1;
                                }
                                // printf("dest: %d, next_hop: %d, cost: %d\n", k, 
                                    // routers[i].routing_table[k].next_hop,routers[i].cost_table[k]);
                            }
                            // printf("\n");
                            
                            
                            for(int k = 0; k< r_n; k++)//dest
                            {
                                if(k==i)
                                {
                                    continue;
                                }
                                int x_y_dv = routers[i].routing_table[k].distance;
                                int x_y_hop = routers[i].routing_table[k].next_hop;
                                int tmp = x_y_dv;
                                int new_dv = 1001;
                                int new_hop = -1;
                                if (x_y_dv == -1)
                                {
                                    tmp = 1001;
                                }
                                // printf("-------update----------\nDest %d, my_dv = %d, tmp = %d\n",k, x_y_dv, tmp);
                                for(int l = 0; l < r_n; l++)//intermediate
                                {
                                    
                                    int x_v_cost = routers[i].cost_table[l];
                                    int v_y_dv = routers[i].neighbors_dv[l].dv[k];
                                    // printf("\nnode %d, tmp = %d, x_v_cost = %d, v_y_dv = %d\n",l, tmp,  x_v_cost,v_y_dv);
                                    if(x_v_cost == -1 || v_y_dv == -1)
                                    {
                                        // printf("cont break\n");
                                        continue;
                                    }
                                    
                                    if(tmp > x_v_cost + v_y_dv)
                                    {
                                        // printf(">!\n");
                                        new_dv = x_v_cost + v_y_dv;
                                        new_hop = l;
                                        tmp = new_dv;
                                    }
                                    else if(x_y_dv == x_v_cost + v_y_dv && l == routers[i].routing_table[k].next_hop && new_dv == 1001 )
                                    {
                                        new_dv = x_y_dv;
                                        new_hop = l;
                                    }
                                }
                                if(new_dv == 1001 && x_y_dv == -1|| x_y_dv == -1 && (new_dv == -1))
                                {
                                    // printf("nothing changed cont\n");
                                    continue;
                                }
                                else if (new_dv == x_y_dv && new_hop == x_y_hop)
                                {
                                    // printf("same\n");
                                    continue;
                                }
                                if((new_dv == 1001 || new_dv == -1) && (x_y_dv != -1 && x_y_hop != k))
                                {
                                    routers[i].routing_table[k].distance = -1;
                                    routers[i].routing_table[k].next_hop = -1;
                                    // change = 1;
                                    // printf("EXCEPTION: %d -> %d, cost: %d, next_hop: %d\n",i,k,-1,-1);
                                }
                                else //if(new_dv<x_y_dv|| new_dv > 0 && x_y_dv == -1 )
                                {
                                    routers[i].routing_table[k].distance = new_dv;
                                    routers[i].routing_table[k].next_hop = new_hop;
                                    // change = 1;
                                    // printf("NEW: %d -> %d, cost: %d, next_hop: %d\n",i,k,new_dv,new_hop);
                                }

                            }
                            // printf("-----update------\n");
                            // print_router_info(routers, r_n, -1, i);
                            // printf("\n");
                        }
        
                        else if(cmd_to_router->cmd_type == CMD_SHOW)
                        {
                            memset(&buff, 0, sizeof(buff));
                            struct wholeRoutingTable* send_agent_table = (struct wholeRoutingTable*)buff;
                            for(int k = 0; k<r_n;k++)
                            {
                                send_agent_table->table[k].distance = routers[i].routing_table[k].distance;
                                send_agent_table->table[k].next_hop = routers[i].routing_table[k].next_hop;
                            }
                            write(routers[i].recv_fd[j], (void*)buff, sizeof(struct RoutingTable)*10);
                            // printf("Router %d reply \"%s\" to show\n", i, "received!");   
                            
                        }

                        else if(cmd_to_router->cmd_type ==  CMD_RESET)
                        {
                            routers[i].counter = 0;
                        }   
                    }
                    else if(j< r_n)
                    {
                        // printf("Router %d receive: %s from router %d\n", i, buff, j); 
                        // memset(&buff, 0, sizeof(buff));
                        struct NeighborDV* send_my_dv = (struct NeighborDV*)buff;
                        // printf("Router %d receive dv from router %d : ", i, j);
                        for(int k = 0; k< r_n; k++)
                        {
                            routers[i].neighbors_dv[j].dv[k] = send_my_dv->dv[k];
                            // printf("%d  ",send_my_dv->dv[k]);
                        }
                        // printf("\n");
                        // printf("Router %d start bellman:\n", i);
                        for(int k = 0; k< r_n; k++)//dest
                        {
                            if(k==i)
                            {
                                continue;
                            }
                            int x_y_dv = routers[i].routing_table[k].distance;
                            int x_y_hop = routers[i].routing_table[k].next_hop;
                            int tmp = 1001;
                            int new_dv = 1001;
                            int new_hop = -1;
                            if (x_y_dv == -1)
                            {
                                tmp = 1001;
                            }
                            // printf("------------dv-------\nDest %d, my_dv = %d, tmp = %d\n",k, x_y_dv, tmp);
                            for(int l = 0; l < r_n; l++)//intermediate
                            {
                                
                                int x_v_cost = routers[i].cost_table[l];
                                int v_y_dv = routers[i].neighbors_dv[l].dv[k];
                                // printf("\nnode %d, tmp = %d, x_v_cost = %d, v_y_dv = %d\n",l, tmp, x_v_cost,v_y_dv);
                                if(x_v_cost == -1 || v_y_dv == -1)
                                {
                                    // printf("cont break\n");
                                    continue;
                                }
                                
                                if(tmp > x_v_cost + v_y_dv)
                                {
                                    // printf(">!\n");
                                    new_dv = x_v_cost + v_y_dv;
                                    new_hop = l;
                                    tmp = new_dv;
                                }
                                else if(x_y_dv == x_v_cost + v_y_dv && l == routers[i].routing_table[k].next_hop && new_dv == 1001 )
                                {
                                    new_dv = x_y_dv;
                                    new_hop = l;
                                }
                            }
                        
                            if(new_dv == 1001 && x_y_dv == -1|| x_y_dv == -1 && (new_dv == -1))
                            {
                                // printf("nothing changed cont\n");
                                continue;
                            }
                            else if (new_dv == x_y_dv && new_hop == x_y_hop)
                            {
                                // printf("same\n");
                                continue;
                            }
                            if((new_dv == 1001 || new_dv == -1) && (x_y_dv != -1 && x_y_hop != k))
                            {
                                routers[i].routing_table[k].distance = -1;
                                routers[i].routing_table[k].next_hop = -1;
                                change = 1;
                                // printf("EXCEPTION: %d -> %d, cost: %d, next_hop: %d\n",i,k,-1,-1);
                            }
                            else //if(new_dv<x_y_dv|| new_dv > 0 && x_y_dv == -1 )
                            {
                                routers[i].routing_table[k].distance = new_dv;
                                routers[i].routing_table[k].next_hop = new_hop;
                                change = 1;
                                // printf("NEW: %d -> %d, cost: %d, next_hop: %d\n",i,k,new_dv,new_hop);
                            }

                        }
                        
                        // send(routers[i].send_fd[k], (void*)buff, sizeof(struct NeighborDV), 0);
                        
                    }
                } 
            }  
            if(change > 0)
            {
                // printf("router %d changed after get dv\n", i);
                router_send_dv(routers[i], r_n, i);
            }
        }

    }

    for(int i = 0; i < r_n; i++)
    {        
        for(int j = 0;j < 12; j++)
        {
            if(routers[i].send_fd[j] > 0)
            {
                close(routers[i].recv_fd[j]);
            }
        }
        for(int j = 0;j < 10; j++)
        {
            if(routers[i].send_fd[j] > 0)
            {
                close(routers[i].send_fd[j]);
            }
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    char *router_location_file_name;
    char *topology_conf_file_name;
    int router_id;

    if (argc != 4) {
        fprintf(stderr, "Usage: ./agent [Router Location File] [Topology Conf File] [Router ID]\n");
        exit(EXIT_FAILURE);
    }

    router_location_file_name = argv[1];
    topology_conf_file_name = argv[2];
    router_id = atoi(argv[3]);

    return router(router_location_file_name, topology_conf_file_name, router_id);
    // receiver();
    // return 0;
}

