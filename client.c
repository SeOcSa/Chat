#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define BUFSIZE 256

void send_receive(int i, int sockfd, char username[100]) {
  char send_buf[BUFSIZE];
  char recv_buf[BUFSIZE];
  int recvByte;

  if(i==0)
  {
    fgets(send_buf,BUFSIZE,stdin);
    if (strcmp(send_buf , "bye\n") == 0)
    {
      send(sockfd,username,strlen(username),0);
      send(sockfd, ":", 1, 0);
      send(sockfd, send_buf, strlen(send_buf), 0);
      send(sockfd,username,strlen(username), 0);
      send(sockfd,"leaving chat room...",20, 0);
      exit(0);
    }
  else{
        send(sockfd,username,strlen(username),0);
        send(sockfd, ":", 1, 0);
        send(sockfd, send_buf, strlen(send_buf), 0);
    }
  }
  else
  {
    recvByte= recv(sockfd, recv_buf, BUFSIZE, 0);

    //if server stop unespected
    if(recvByte==0)
    {
    	printf("Server is down, reconnect later!\n"); //show message and deconnect user
    	exit(0);
    }
    else
    {
    	recv_buf[recvByte]='\0'; //keep sending
    	printf("--%s",recv_buf);
    	fflush(stdout);
	}
  }
}

void connect_request(int *sockfd, struct sockaddr_in *server_addr, int portNo, char addr[],char username[], char password[])
 {
  int n;
  char buffer[256]="";

//creat socket for client
  if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    exit(1);
  }
  //set connection information for client socket
  server_addr->sin_family = AF_INET;
  server_addr->sin_port = htons(portNo);
  server_addr->sin_addr.s_addr = INADDR_ANY;
  memset(server_addr->sin_zero, '\0', sizeof server_addr->sin_zero);
  
  //connect
  if(connect(*sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr)) == -1) {
    perror("connect");
    exit(1);
  }

  //after connect send username and pass to server for log in checking
  n=send(*sockfd, username, strlen(username), 0);
  n=send(*sockfd, password, strlen(password), 0);

  //receving log in report
  n=recv(*sockfd,buffer,255,0);

  if( (strncmp(buffer,"Wrong password",strlen("Wrong password"))==0) ||
    (strncmp(buffer,"Connected already!",strlen("Connected already!"))==0))
  {
    printf("%s\n",buffer);
    exit(1);
  }
}

int main (int argc, char *argv[]){
  
  int sockfd, portNo, i, fdmax;
  struct sockaddr_in serv_addr;
  fd_set master;
  fd_set read_fds;

  if (argc<5) 
  {
      fprintf (stderr,"Usage %s Host-Name Port UserName Password\n", argv[0]);
      exit(0);
  }
   
  portNo=atoi(argv[2]);

  connect_request(&sockfd, &serv_addr,portNo,argv[1],argv[3],argv[4]);

  //clear any other setfd
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  //set fd for monitoring
  FD_SET(0, &master);
  FD_SET(sockfd, &master);

  fdmax = sockfd;

  while(1) {
    fflush(stdout);
     read_fds = master;
     //select fd
     if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
     {
        perror("select");
        exit(4);
     }

    for(i=0; i <= fdmax; i++ )
      if(FD_ISSET(i, &read_fds))//check if fd has setfd
        send_receive(i, sockfd, argv[3]);///send or receive for the current fd ready for R/W operation
    }

  close(sockfd); 
  return 0;

}