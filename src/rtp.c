#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>

#include "rtp.h"
#include "util.h"


void rcb_init(uint32_t window_size) {
    if (rcb == NULL) {
        rcb = (rcb_t *) calloc(1, sizeof(rcb_t));
    } else {
        perror("The current version of the rtp protocol only supports a single connection");
        exit(EXIT_FAILURE);
    }
    rcb->window_size = window_size;
    printf("In rcb_init: Done\n");
    // TODO: you can initialize your RTP-related fields here
}

/*********************** Note ************************/
/* RTP in Assignment 2 only supports single connection.
/* Therefore, we can initialize the related fields of RTP when creating the socket.
/* rcb is a global varialble, you can directly use it in your implementatyion.
/*****************************************************/
int rtp_socket(uint32_t window_size) {
    rcb_init(window_size); 
    // create UDP socket
    printf("In rtp_socket: Done\n");
    return socket(AF_INET, SOCK_DGRAM, 0);  
}


int rtp_bind(int sockfd, struct sockaddr *addr, socklen_t addrlen) {
    printf("In rtp_bind: Done\n");
    return bind(sockfd, addr, addrlen);
}


int rtp_listen(int sockfd, int backlog) {
    // TODO: listen for the START message from sender and send back ACK
    // In standard POSIX API, backlog is the number of connections allowed on the incoming queue.
    // For RTP, backlog is always 1
    char buffer[5000];
    struct sockaddr_in sender;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    rtp_header_t *rtp;
    int recv_bytes;

    while(1)
    {
        memset(&buffer,0,sizeof(buffer));
        memset(&sender, 0, sizeof(sender));
        addr_len = sizeof(struct sockaddr_in);
        

        if ((recv_bytes = rtp_recvfrom(sockfd, (void *)&buffer, sizeof(buffer), 0, 
        (struct sockaddr*)&sender, &addr_len)) < 0) {
            perror("In rtp_listen: receive error");
        }

        buffer[recv_bytes] = '\0';
        // extract header
        rtp = (rtp_header_t *)buffer;
        if (rtp->type == RTP_START)//START
        {
            printf("In rtp_listen: Received START!, seq_num:%d\n",rtp->seq_num);
            break;
        }
    }

    char buffer_start[2048];
    rtp_sendto(sockfd, "", 0, 0, 
      (struct sockaddr*)&sender, addr_len,rtp->seq_num,RTP_ACK);

    printf("In rtp_listen: Send ACK for start!\n");
    return 1;
}


int rtp_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    // Since RTP in Assignment 2 only supports one connection,
    // there is no need to implement accpet function.
    // You don’t need to make any changes to this function.
    printf("In rtp_accept: Done\n");
    return 1;
}

int rtp_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    // TODO: send START message and wait for its ACK
    rtp_sendto(sockfd,(void *)"", 0, 0, addr, addrlen, SEND_START, RTP_START);
    printf("In rtp_connect: send START!\n");

    char buffer[5000];
    struct sockaddr_in sender;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    rtp_header_t* rtp;
    int recv_bytes;

    printf("In rtp_connect: for ACK START\n");
    struct timeval timeout = {0,500000};
    fd_set thetime;

    while(1)
    {
        memset(&buffer,0,sizeof(buffer));
        memset(&sender,0,sizeof(sender));
        addr_len = sizeof(struct sockaddr_in);

        printf("start select\n");
        while(1)
        {
            FD_ZERO(&thetime);
            FD_SET(sockfd, &thetime);
            int done = 0;
            switch(select(sockfd+1, &thetime, NULL,NULL, &timeout)) {
                case -1: 
                {
                    printf("error\n"); 
                    done = 1; 
                    break;
                }
                case 0: 
                {
                    printf("gonna return\n");
                    return 2;
                }
                default: 
                {
                // printf("Available!\n");
                    if(FD_ISSET(sockfd, &thetime))
                    {
                        done = 1;
                        // printf("ISSET\n");
                        break;
                    }
            }

        }
        if(done == 1)
        {
            break;
        }
        }
        printf("end sleect if\n");


        if ((recv_bytes = rtp_recvfrom(sockfd, (void *)&buffer, sizeof(buffer), 0, 
            (struct sockaddr*)&sender, &addr_len)) < 0) {
            perror("In rtp_connect: receive error");
        }
        
        buffer[recv_bytes] = '\0';
        rtp = (rtp_header_t *)buffer;
        if (rtp->type == RTP_ACK)//ACK START
        {
            printf("In rtp_connect: receive ACK START\n");
            break;
        }
        printf("In rtp_connect: waiting ACK START but received : %d/%d, seqnum=%d\n",rtp->type,RTP_ACK, rtp->seq_num);
    }
    return 1;
}

int rtp_close(int sockfd) {
    printf("In rtp_close: Done\n");
    return close(sockfd);
}


int rtp_sendto(int sockfd, const void *msg, int len, int flags, const struct sockaddr *to, socklen_t tolen, 
    int seqnum, int header_type) {
    // TODO: send message

    // Send the first data message sample
    char buffer[BUFFER_SIZE];
    rtp_header_t* rtp = (rtp_header_t*)buffer;
    rtp->length = len;
    rtp->checksum = 0;
    rtp->seq_num = seqnum;
    rtp->type = header_type;
    printf("Type: %d, seqnum: %d, length: %d\n", rtp->type, rtp->seq_num, rtp->length);
    memcpy((void *)buffer+sizeof(rtp_header_t), msg, len);
    rtp->checksum = compute_checksum((void *)buffer, sizeof(rtp_header_t) + len);

    int sent_bytes = sendto(sockfd, (void*)buffer, sizeof(rtp_header_t) + len, flags, to, tolen);
    if (sent_bytes != (sizeof(struct RTP_header) + len)) {
        perror("send error");
        exit(EXIT_FAILURE);
    }
    printf("In rtp_sendto: Done\n");
    return 1;

}

int rtp_recvfrom(int sockfd, void *buf, int len, int flags, 
                 struct sockaddr *from, socklen_t *fromlen) {
    // TODO: recv message

    char buffer[2048];
    int recv_bytes = recvfrom(sockfd, buffer, 2048, flags, from, fromlen);
    if (recv_bytes < 0) {
        perror("*In rtp_recvfrom: receive error");
        exit(EXIT_FAILURE);
    }
    buffer[recv_bytes] = '\0';

    // extract header
    rtp_header_t *rtp = (rtp_header_t *)buffer;

    // verify checksum
    uint32_t pkt_checksum = rtp->checksum;
    rtp->checksum = 0;
    uint32_t computed_checksum = compute_checksum(buffer, recv_bytes);
    if (pkt_checksum != computed_checksum) {
        perror("checksums not match");
        return -1;
    }

    memcpy(buf, buffer, sizeof(rtp_header_t)+(rtp->length));
    rtp_header_t* tmp_rtp = (rtp_header_t *)buf;
    printf("In rtp_recvfrom: type:%d/%d, seq_num:%d/%d, len: %d\n",
            tmp_rtp->type,rtp->type,tmp_rtp->seq_num, rtp->seq_num, rtp->length);

    return sizeof(rtp_header_t)+(rtp->length);
}
