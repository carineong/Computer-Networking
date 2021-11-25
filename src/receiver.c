#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>

#include "util.h"
#include "rtp.h"

#define RECV_BUFFER_SIZE 32768  // 32KB
#define MAX_PACKET_IN_ONE_BUFFER (RECV_BUFFER_SIZE/(MAX_MSG_SIZE-1))

int receiver(char *receiver_port, int window_size, char* file_name) {

  char buffer[RECV_BUFFER_SIZE];
  char buf_withheader[BUFFER_SIZE];

  // create rtp socket file descriptor
  int receiver_fd = rtp_socket(window_size);
  if (receiver_fd == 0) {
    perror("create rtp socket failed");
    exit(EXIT_FAILURE);
  }

  // create socket address
  // forcefully attach socket to the port
  struct sockaddr_in address;
  memset(&address, 0, sizeof(struct sockaddr_in));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(atoi(receiver_port));

  // bind rtp socket to address
  if (rtp_bind(receiver_fd, (struct sockaddr *)&address, sizeof(struct sockaddr))<0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  int recv_bytes;
  struct sockaddr_in sender;
  socklen_t addr_len = sizeof(struct sockaddr_in);

  // listen to incoming rtp connection
  rtp_listen(receiver_fd, 1);
  // accept the rtp connection
  rtp_accept(receiver_fd, (struct sockaddr*)&sender, &addr_len);


  FILE *f1 = fopen(file_name, "w");
  fclose(f1);

  int expect_next_seqnum = 0;
  int largest_seqnum = 0;
  int finish = 0;
  int smtg_in_buffer = 0;
  char typeofwrite[4] = "a";
  char tmp_buf[BUFFER_SIZE];
  rtp_header_t* rtp;



  while(1)
  {
    printf("-----------------------------\n");
    memset(&sender,0,sizeof(sender));
    memset(&buf_withheader,0,sizeof(buf_withheader));
    memset(&tmp_buf,0,sizeof(tmp_buf));
    if ((recv_bytes = rtp_recvfrom(receiver_fd, (void *)buf_withheader, sizeof(buf_withheader), 0, 
      (struct sockaddr*)&sender, &addr_len)) < 0) {
        perror("receiver: receive error");
    }

    buf_withheader[recv_bytes] = '\0';
    rtp = (rtp_header_t *)buf_withheader;

    
    memcpy(tmp_buf,buf_withheader+sizeof(rtp_header_t),rtp->length);
    
    int cur_seqnum = rtp->seq_num;

    if(rtp->type==RTP_DATA)
    {
      if (cur_seqnum != expect_next_seqnum)
      {
        printf("cur!=expect\n");
        if(cur_seqnum>expect_next_seqnum && cur_seqnum <= (expect_next_seqnum + window_size - 1))
        {
          int tmp = cur_seqnum-expect_next_seqnum - 1;
          memcpy(buffer + (tmp*(MAX_MSG_SIZE)), tmp_buf,rtp->length);
          largest_seqnum = max(largest_seqnum,cur_seqnum);
          printf(" and in window, save in buffer, pos = %d",tmp);
          smtg_in_buffer++;
        }
        printf("\n");
      }
      else
      {
        printf("cur==expect\n");
        printf("Open File, type: %s\n",typeofwrite);
        FILE *f = fopen(file_name, typeofwrite);
        fputs(tmp_buf,f);
        memset(&tmp_buf,0,sizeof(tmp_buf));
        printf("Writing:\n%s\n",tmp_buf);
        fclose(f);
        printf("Close File\n");

        int num_pac_in_buffer = largest_seqnum-cur_seqnum;
        if(num_pac_in_buffer>0 )
        {
          f = fopen(file_name, typeofwrite);
          for(int i = 0; i<num_pac_in_buffer;i++)
          {
            printf("Writing buffer packets\n");

            memset(&tmp_buf,0,sizeof(tmp_buf));
            memcpy(tmp_buf,buffer+(i*(MAX_MSG_SIZE)),MAX_MSG_SIZE);
            printf("write:\n%s\n",tmp_buf);
            fputs(tmp_buf,f);
            memset(&tmp_buf,0,sizeof(tmp_buf));
          }
          fclose(f);
        }
        

        largest_seqnum = max(largest_seqnum,cur_seqnum);
        expect_next_seqnum = largest_seqnum + 1;
        memset(&buffer,0,sizeof(buffer));
        smtg_in_buffer = 0;
      }

    }


    printf("receiver receive seqnum: %d \n", cur_seqnum);
    
    if (rtp->type == RTP_END)//receive end
    {
      printf("receiver receive END and will send ack end\n");
      largest_seqnum = expect_next_seqnum;
      expect_next_seqnum = rtp->seq_num;
      finish = 1;
    }

    rtp_sendto(receiver_fd, (void *)"", 0, 0, 
        (struct sockaddr*)&sender, sizeof(struct sockaddr),expect_next_seqnum, RTP_ACK);
    printf("receiver send ack %d\n",expect_next_seqnum);
    


    if(finish == 1)
    {
      if(smtg_in_buffer>0)
      {
        int num_pac_in_buffer = largest_seqnum - expect_next_seqnum - 1;

        printf("Open File, type: %s\n",typeofwrite);
        FILE *f = fopen(file_name, typeofwrite);

        if(num_pac_in_buffer>0 )
        {
          for(int i = 0; i<num_pac_in_buffer;i++)
          {
            // printf("Writing buffer packets\n");
            // fputs(buffer+(i*(MAX_MSG_SIZE-1)),f);
            memset(&tmp_buf,0,sizeof(tmp_buf));
            memcpy(tmp_buf,buffer+(i*(MAX_MSG_SIZE)),MAX_MSG_SIZE);
            printf("write:\n%s\n",tmp_buf);
            fputs(tmp_buf,f);
          }
        }
        fclose(f);
        printf("Close File\n");
      }
      
      memset(&buffer,0,sizeof(buffer));
      printf("memset buffer\n");
      break;
    }
    
  }

 // receive packet
 

  rtp_close(receiver_fd);

  return 0;
}

/*
 * main():
 * Parse command-line arguments and call receiver function
*/
int main(int argc, char **argv) {
    char *receiver_port;
    int window_size;
    char *file_name;

    if (argc != 4) {
        fprintf(stderr, "Usage: ./receiver [Receiver Port] [Window Size] [File Name]\n");
        exit(EXIT_FAILURE);
    }

    receiver_port = argv[1];
    window_size = atoi(argv[2]);
    file_name = argv[3];
    return receiver(receiver_port, window_size, file_name);
}
