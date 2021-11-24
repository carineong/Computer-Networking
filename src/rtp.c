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
    char buffer[2048];
    struct sockaddr_in sender;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    rtp_header_t *rtp;

    while(1)
    {
        memset(&buffer,0,sizeof(buffer));
        memset(&sender, 0, sizeof(sender));
        addr_len = sizeof(struct sockaddr_in);

        int recv_bytes = recvfrom(sockfd, buffer, 2048, 0, 
                    (struct sockaddr*)&sender, &addr_len);
        if (recv_bytes < 0) {
            perror("*In receive error");
            exit(EXIT_FAILURE);
        }
        buffer[recv_bytes] = '\0';

        // extract header
        rtp = (rtp_header_t *)buffer;

        // verify checksum
        uint32_t pkt_checksum = rtp->checksum;
        rtp->checksum = 0;
        uint32_t computed_checksum = compute_checksum(buffer, recv_bytes);
        if (pkt_checksum != computed_checksum) {
            perror("checksums not match");
            return -1;
        }
        if (rtp->type == RTP_START)//START
        {
            printf("In rtp_listen: Received START!\n");
            break;
        }
    }
    char buffer_start[2048];
    rtp_sendto(sockfd, (void *)buffer_start, sizeof(rtp_header_t), 0, 
      (struct sockaddr*)&sender, sizeof(struct sockaddr),rtp->seq_num,RTP_ACK);
    
    // rtp_header_t* rtp_start = (rtp_header_t*)buffer_start;
    // rtp_start->length = 0;
    // rtp_start->checksum = 0;
    // rtp_start->seq_num = rtp->seq_num;
    // rtp_start->type = RTP_ACK; //ACK
    // rtp_start->checksum = compute_checksum((void *)buffer_start, sizeof(rtp_header_t));

    // int sent_bytes = sendto(sockfd, (void*)buffer_start, sizeof(rtp_header_t), 0,
    //                  (struct sockaddr*)&sender, addr_len);

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
    rtp_sendto(sockfd,(void *)"", sizeof(rtp_header_t), 0, addr, addrlen, 0, RTP_START);
    printf("In rtp_connect: send START!\n");

    char buffer[MAX_MSG_SIZE];
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int recv_bytes;
    rtp_header_t* rtp;

    printf("In rtp_connect: for ACK START\n");
    while(1)
    {
        memset(&addr,0,sizeof(addr));
        if ((recv_bytes = rtp_recvfrom(sockfd, (void *)buffer, sizeof(buffer), 0, 
            (struct sockaddr*)&addr, &addr_len)) < 0) {
            perror("In rtp_connect: receive error");
        }
        printf("In rtp_connect: in loop for ACK START\n");
        buffer[recv_bytes] = '\0';
        rtp = (rtp_header_t *)buffer;
        if (rtp->type == RTP_ACK)//ACK START
        {
            printf("In rtp_connect: receive ACK START : %s\n", buffer);
            break;
        }
        printf("In rtp_connect: waiting ACK START but received : %d  %s\n, %s",rtp->type, buffer);
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

int rtp_recvfrom(int sockfd, void *buf, int len, int flags,  struct sockaddr *from, socklen_t *fromlen) {
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

    memcpy(buf, buffer+sizeof(rtp_header_t), rtp->length);
    printf("In rtp_recvfrom: Done\n");
    return rtp->length;
}
