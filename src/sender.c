#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "util.h"
#include "rtp.h"



int sender(char *receiver_ip, char* receiver_port, int window_size, char* message){
  
  // create socket
  int sock = 0;
  if ((sock = rtp_socket(window_size)) < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // create receiver address
  struct sockaddr_in receiver_addr;
  memset(&receiver_addr, 0, sizeof(receiver_addr));
  receiver_addr.sin_family = AF_INET;
  receiver_addr.sin_port = htons(atoi(receiver_port));

  // convert IPv4 or IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, receiver_ip, &receiver_addr.sin_addr)<=0) {
    perror("address failed");
    exit(EXIT_FAILURE);
  }

  // connect to server
  rtp_connect(sock, (struct sockaddr *)&receiver_addr, sizeof(struct sockaddr));


  char buffer[MAX_MSG_SIZE];
  socklen_t addr_len = sizeof(struct sockaddr_in);
  int recv_bytes;
  rtp_header_t* rtp;
  
  
  

  // send data
  //char test_data[] = "Hello, world!\n";
  // TODO: if message is filename, open the file and send its content
  // rtp_sendto(sock, (void *)test_data, strlen(test_data), 0, 
  //     (struct sockaddr*)&receiver_addr, sizeof(struct sockaddr));

  printf("Message Size: %ld\n",strlen(message));
  int finish = 0;
  int seq_num = 0;
  while(1)
  {
    memset(&buffer,0,sizeof(buffer));
    recv_bytes = 0;

    int msg_len = strlen(message);
    char* cur_msg;
    int cur_len;

    // for(int i = 0; i<window_size; i++)
    // {

    // }
    if (msg_len >= MAX_MSG_SIZE-1)
    {
      cur_len = MAX_MSG_SIZE - 1;
    }
    else
    {
      cur_len = msg_len;
      finish = 1;
    }

    cur_msg = malloc(cur_len);
    memcpy(cur_msg, message, cur_len);

    rtp_sendto(sock, (void *)cur_msg, cur_len, 0, 
      (struct sockaddr*)&receiver_addr, sizeof(struct sockaddr),seq_num,RTP_DATA);
    printf("sender send packet %d\n",seq_num);
    
    if ((recv_bytes = rtp_recvfrom(sock, (void *)buffer, sizeof(buffer), 0, 
      (struct sockaddr*)&receiver_addr, &addr_len)) < 0) {
         perror("sender: receive error");
    }
    
    buffer[recv_bytes] = '\0';
    rtp = (rtp_header_t *)buffer;

    if (rtp->type == RTP_ACK)//ACK DATA
    {
        printf("sender receive ACK DATA : %s\n", buffer);
        
    }
    else
    {
        printf("sender receive %d: %s\n",rtp->type, buffer);
    }
    

    if(finish)
    {
      seq_num = 0;
      rtp_sendto(sock, (void *)"", 0, 0, 
      (struct sockaddr*)&receiver_addr, sizeof(struct sockaddr),seq_num, RTP_END);
      break;
    }
    seq_num++;
  }
  

  // close rtp socket
  rtp_close(sock);

  return 0;
}



/*
 * main()
 * Parse command-line arguments and call sender function
*/
int main(int argc, char **argv) {
  char *receiver_ip;
  char *receiver_port;
  int window_size;
  char *message;

  if (argc != 5) {
    fprintf(stderr, "Usage: ./sender [Receiver IP] [Receiver Port] [Window Size] [message]");
    exit(EXIT_FAILURE);
  }

  receiver_ip = argv[1];
  receiver_port = argv[2];
  window_size = atoi(argv[3]);
  message = argv[4];
  return sender(receiver_ip, receiver_port, window_size, message);
}