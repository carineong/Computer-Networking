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
  int status_connect = rtp_connect(sock, (struct sockaddr *)&receiver_addr, sizeof(struct sockaddr));


  char buffer[MAX_MSG_SIZE];
  socklen_t addr_len = sizeof(struct sockaddr_in);
  int recv_bytes;
  
  char* filename = malloc(sizeof(message));
  strcpy(filename, message);

  // TODO: if message is filename, open the file and send its content
  if( access(filename, F_OK ) == 0 ) {
    
    memset(message,0,sizeof(message));
    printf("FILE %s EXISTS!\n",filename);
   
    long length;
    FILE * f = fopen (filename, "rb");

    if (f)
    {
      fseek (f, 0, SEEK_END);
      length = ftell (f);
      printf("Total Length: %ld\n",length);
      fseek (f, 0, SEEK_SET);
      message = malloc(length);
      if (message)
      {
        fread (message, 1, length, f);
      }
      fclose (f);
    }
  } 
  else {
    printf("FILE NOT FOUND!\n");
      // file doesn't exist
  }
  
  // for data
  printf("Message Size: %ld\n",strlen(message));
  int finish = (status_connect == 2? 1:0);
  int now_send_seqnum = 0;
  int receive_require_seqnum = 0;
  int largest_seqnum = 0;
  char* timeout_buf;
  int timeout_buf_len;
  
  //send DATA
  while(1)
  {
    if(finish)
    {
      printf("-------------------\nsender send END\n");
      rtp_sendto(sock, (void *)"", 0, 0, 
      (struct sockaddr*)&receiver_addr, sizeof(struct sockaddr),largest_seqnum+1, RTP_END);
      break;
    }

    printf("-----------------------------\n");
    memset(&buffer,0,sizeof(buffer));
    recv_bytes = 0;

    for(int i = 0; i< window_size;i++)
    {
      char* cur_msg;
      int cur_len;
      int cur_position = (now_send_seqnum)*(MAX_MSG_SIZE-1);
      if(now_send_seqnum == 0)
      {
        cur_position=0;
      }
      int msg_len = strlen(message) - cur_position;
      
      if (msg_len >= MAX_MSG_SIZE-1)
      {
        cur_len = MAX_MSG_SIZE - 1;
      }
      else
      {
        cur_len = msg_len;
        finish = 1;
      }
      
      printf("msg_len:%d, cur_position:%d, cur_len:%d\n",msg_len,cur_position,cur_len);
      cur_msg = malloc(cur_len);
      memcpy(cur_msg, message + cur_position, cur_len);
      
      rtp_sendto(sock, (void *)cur_msg, cur_len, 0, 
        (struct sockaddr*)&receiver_addr, sizeof(struct sockaddr),now_send_seqnum,RTP_DATA);
        
      printf("sender send packet %d\n",now_send_seqnum);

      largest_seqnum = max(largest_seqnum, now_send_seqnum);
      
      now_send_seqnum++;
      if(finish == 1)
      {
        break;
      }
    }
    
    // set timeout timer
    struct timeval timeout = {0,500000};
    fd_set thetime;

    // printf("start select\n");
    while(1)
    {
      FD_ZERO(&thetime);
      FD_SET(sock, &thetime);
      int done = 0;
      switch(select(sock+1, &thetime, NULL,NULL, &timeout)) {
        case -1: 
        {
          printf("error\n"); 
          done = 1; 
          break;
        }
        case 0: 
        {
          int timeout_seqnum = min(receive_require_seqnum,now_send_seqnum-1);
          printf("TIMEOUT! Resend packet %d\n",timeout_seqnum);
          char* cur_msg;
          int cur_len;
          int cur_position = (timeout_seqnum)*MAX_MSG_SIZE-1;
          if(timeout_seqnum == 0)
          {
            cur_position=0;
          }
          int msg_len = strlen(message) - cur_position;

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
          memcpy(cur_msg, message + cur_position, cur_len);
          
          rtp_sendto(sock, (void *)cur_msg, cur_len, 0, 
            (struct sockaddr*)&receiver_addr, sizeof(struct sockaddr),timeout_seqnum,RTP_DATA);
          done = 1;
          break;
        }
        default: 
        {
          // printf("Available!\n");
          if(FD_ISSET(sock, &thetime))
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
    // printf("end sleect if\n");

    if ((recv_bytes = rtp_recvfrom(sock, (void *)buffer, sizeof(buffer), 0, 
      (struct sockaddr*)&receiver_addr, &addr_len)) < 0) {
         perror("sender: receive error");
    }
    
    buffer[recv_bytes] = '\0';
    rtp_header_t* rtp = (rtp_header_t *)buffer;

    if (rtp->type == RTP_ACK)//ACK DATA
    {
      receive_require_seqnum = rtp->seq_num;
      now_send_seqnum = rtp->seq_num;
      printf("sender receive ACK DATA : %d\n", rtp->seq_num);
    }
    else
    {
      printf("sender receive %d: %d\n",rtp->type, rtp->seq_num);
    }
  }
  
  // for END
  int close_connect = 0;
  struct timeval timeout = {0,500000};
  fd_set thetime;

  //wait for ACK END
  while(1)
  {
    memset(&buffer,0,sizeof(buffer));
    recv_bytes = 0;

    // printf("start select\n");
    while(1)
    {
      FD_ZERO(&thetime);
      FD_SET(sock, &thetime);
      int done = 0;
      switch(select(sock+1, &thetime, NULL,NULL, &timeout)) {
          case -1: 
          {
              printf("error\n"); 
              done = 1; 
              break;
          }
          case 0: 
          {
              done = 1;
              break;
          }
          default: 
          {
          // printf("Available!\n");
              if(FD_ISSET(sock, &thetime))
              {
                  done = 2;
                  // printf("ISSET\n");
                  break;
              }
          }

      }
      if(done > 0)
      {
        if(done == 1)
        {
          close_connect = 1;
        }
          break;
      }
    }

    // printf("end sleect if\n");
    if(close_connect==1)
    {
      break;
    }
    if ((recv_bytes = rtp_recvfrom(sock, (void *)buffer, sizeof(buffer), 0, 
      (struct sockaddr*)&receiver_addr, &addr_len)) < 0) {
         perror("sender: receive error");
    }

    buffer[recv_bytes] = '\0';
    rtp_header_t* rtp = (rtp_header_t *)buffer;

    if (rtp->type == RTP_ACK)//ACK DATA
    {
      printf("sender received ACK END!\n");
      break;
    }
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