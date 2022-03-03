
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>

#include "common.h"

int get_index(int ind, int arr[])
{
    for(int i = 0; i < MAXNUM_ROUTER;i++)
    {
        if(arr[i]==ind)
        {
            return i;
        }
    }
    return -1;
    
}

int check_arr(int arr[], int n)
{
    for (int i = 0; i < n; i++)
    {
        if(arr[i] == 0)
        {
            return 0;
        }
    }
    return 1;
}

void print_router_info(struct the_Router the_routers[], int n, int all, int the_i)
{
    if(all > 0)
    {
        for(int i = 0; i < n; i++)
        {
            printf("IP: %s, Port: %d, ID: %d\n",the_routers[i].ip, the_routers[i].port, the_routers[i].id);
            printf("send | ");
            for(int j = 0;j < 10; j++)
            {
                if(the_routers[i].send_fd[j] > 0)
                {
                    printf("%d:%d, ", j, the_routers[i].send_fd[j]);
                }
                
            }
            printf("\nrecv | ");
            for(int j = 0;j < 12; j++)
            {
                if(the_routers[i].recv_fd[j] > 0)
                {
                    printf("%d:%d, ", j, the_routers[i].recv_fd[j]);
                }
            }
        printf("\n\n");
        }
        
    }
    
    if(all>=0)
    {
        for (int i = 0; i < n; i++)
        {
            printf("\nCost Table for router %d (%d)\n", 
                the_routers[i].id, i);

            for(int j = 0; j < n; j++)
            {
                if(i == j || the_routers[i].cost_table[j] == -1)
                {
                    continue;
                }
                printf("To router %d (%d), Cost: %d\n", 
                    the_routers[j].id, j, the_routers[i].cost_table[j]);
            }

            printf("\nNeighbors' DV for router %d (%d)\n", 
                the_routers[i].id, i);

            for(int j = 0; j < n; j++)
            {
                if(i == j || the_routers[i].cost_table[j] == -1)
                {
                    continue;
                }
                printf("For router %d (%d): ", 
                    the_routers[j].id, j);
                for(int k = 0; k < n; k++)
                {
                    printf("%d, ",the_routers[i].neighbors_dv[j].dv[k]);
                }
                printf("\n");
            }
        
                    printf("\nRouting Table for router %d (%d)\n", 
                the_routers[i].id, i);

            for(int j = 0; j < n; j++)
            {
                int next_hop_real_id = (the_routers[i].routing_table[j].next_hop == -1? -1 : the_routers[the_routers[i].routing_table[j].next_hop].id);
                printf("Dest: %d (%d), Next Hop: %d (%d), distance: %d\n", 
                    the_routers[j].id, j, 
                    next_hop_real_id, the_routers[i].routing_table[j].next_hop,  
                    the_routers[i].routing_table[j].distance);            
            }
        
        }
    }
    if(all==-1)
    {
        printf("\nCost Table for router %d \n", the_i);

        for(int j = 0; j < n; j++)
        {
            // if(the_i == j || the_routers[the_i].cost_table[j] == -1)
            // {
            //     continue;
            // }
            printf("To router %d, Cost: %d\n", j, the_routers[the_i].cost_table[j]);
        }

        printf("\nNeighbors' DV for router %d\n", the_i);

        for(int j = 0; j < n; j++)
        {
            // if(the_i == j || the_routers[the_i].cost_table[j] == -1)
            // {
            //     continue;
            // }
            printf("For router %d: ", j);
            for(int k = 0; k < n; k++)
            {
                printf("%d, ",the_routers[the_i].neighbors_dv[j].dv[k]);
            }
            printf("\n");
        }
    
        printf("\nRouting Table for router %d\n", the_i);

        for(int j = 0; j < n; j++)
        {
            int next_hop_real_id = (the_routers[the_i].routing_table[j].next_hop == -1? -1 : the_routers[the_routers[the_i].routing_table[j].next_hop].id);
            printf("Dest: %d, Next Hop: %d, distance: %d\n", 
                j, 
                the_routers[the_i].routing_table[j].next_hop,  
                the_routers[the_i].routing_table[j].distance);            
        }
    }
    printf("-------------------------\n\n");
}

void agents_router(struct routers_by_agent the_routers[], int n)
{
    for(int i = 0; i < n; i++)
    {
        printf("IP: %s, Port: %d, ID: %d, fd:%d\n", 
                the_routers[i].ip, the_routers[i].port, the_routers[i].id,  the_routers[i].fd);
    }
}

