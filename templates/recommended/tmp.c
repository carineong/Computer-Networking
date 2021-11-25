#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void f()
{
// 	  if(update ==1)
//   {
//     int cur_size = cur_seqnum*(MAX_MSG_SIZE-1);
//     int lower = (cur_size == 0? 0:cur_size/RECV_BUFFER_SIZE);
//     int upper = (cur_size+rtp->length)/RECV_BUFFER_SIZE;
    
//     printf("UpdateBuffer, lower=%d, upper=%d\n",lower,upper);
//     if(lower!=upper)
//     {
//       if(lower == 0)
//       {
//         strcpy(typeofwrite,"w+");
//       }
//       else
//       {
//         strcpy(typeofwrite,"w");
//       }
//       printf("lower!=upper, %s\n",typeofwrite);
      
//       memset(&buffer,0,sizeof(buffer));
//       printf("memset buffer\n");
//     }
//     else
//     {
//       int tmp = (cur_seqnum+1) / MAX_PACKET_IN_ONE_BUFFER;
//       printf("Continue in buffer, pos: %d\n",tmp);
//       memcpy(buffer+(tmp*(MAX_MSG_SIZE-1)), buf_withheader+sizeof(rtp_header_t), rtp->length);
//       printf("Finish cpy!");
//     }
        
//   }
  
}

int main()
{
	fd_set rd;
	struct timeval tv;
	int err;
	

	// FD_ZERO(&rd);
	// FD_SET(0,&rd);
	
	// tv.tv_sec = 5;
	// tv.tv_usec = 0;
	// err = select(1,&rd,NULL,NULL,&tv);
	
    // printf("%d\n",FD_ISSET(1,&rd));
    // if(FD_ISSET(1,&rd))
    // {
    //     printf("is\n");
    // }
	// if(err == 0) //超时
	// {
	// 	printf("select time out!\n");
	// }
	// else if(err == -1)  //失败
	// {
	// 	printf("fail to select!\n");
	// }
	// else  //成功
	// {
	// 	printf("data is available!\n");
	// }

    // printf("hi\n");
	
 

//     fd_set rd1;
//     struct timeval timeout={3,0}; //select等待3秒，3秒轮询，要非阻塞就置0  

//     while(1)  
//    {  
//         FD_ZERO(&rd1); //每次循环都要清空集合，否则不能检测描述符变化  
//         FD_SET(0,&rd1); //添加描述符  

//         int done = 0;

//         switch(select(1,&rd1,NULL,NULL,&timeout))   //select使用  
//         {  
//             case -1: printf("error\n");break; //select错误，退出程序  
//             case 0: printf("timeout\n");break; //再次轮询  
//             default:  
//                 printf("hey! %d\n",FD_ISSET(1,&rd1));
//                         done = 1;
//                   break; 
//         }// end switch 
//         if(done == 1)
//         {
//             break;
//         }  
//      }

    char tmp[3];
	
	strcpy(tmp,"hi");

	FILE *f1 = fopen("../../data/result2.txt", "w");


	// fputs(tmp,f1);
    fclose(f1);

	// f1 = fopen("../../data/result2.txt", "w");
	// for(int i = 0; i<5;i++)
	// {
		
	// 	char t[3+i];
	// 	sprintf(t,"%d",i);
	// 	strcpy(tmp,t);

	// 	fputs(t,f1);
    
	// }
	// fclose(f1);
    printf("Close File\n");
	return 0;
}