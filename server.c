#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFSIZE 256

struct userinfo
{
  char username[30];
  char password[30];
}users[100];

char connected_users[100][100];

int totalUsers=1;
int totalConnectedUsers=1;

void sendToAll(int j, int i, int sockfd, int nBytes, char *recv_buf, fd_set *master) 
{
	//check if j is set for writing
  if (FD_ISSET(j, master))
  {
    if (j != sockfd)  // && j != i) //nu trimite la el insusi
     { if (send(j, recv_buf, nBytes, 0) == -1) //sending message
        perror("send message");
    }
  }
}

void send_receive(int i, fd_set *master, int sockfd, int fdmax)
{
  int nbytes_recvd, j;
  char recv_buf[BUFSIZE]="", buf[BUFSIZE]="";
  
  //unexpected closing by client
  if ((nbytes_recvd = recv(i, recv_buf, BUFSIZE, 0)) <= 0) 
  {
    if (nbytes_recvd == 0) 
      printf("socket %d hung up\n", i);
    else 
      perror("recv");

    close(i);
    FD_CLR(i, master);
  }
  else 
  { 
  	//quit by client, saying bye
    int len,a,w=0;
    int b;
    char nume[20]="";
    strcpy(buf,recv_buf);
    buf[strlen(buf)-1]='\0';
    len=strlen(buf)-1;

    if(buf[len]=='e' && buf[len-1]=='y' && buf[len-2]=='b')
    {
      while(buf[w]!=':'&& w!=len)
        {
          nume[w]=buf[w];
          w++;
        }

      //removing pending disconected user
      for(b=0;b<totalConnectedUsers-1;b++)
        {
          if(strcmp(nume,connected_users[b])==0)
            break;
        }
      if(b==totalConnectedUsers-1) totalConnectedUsers--;
      else 
        {
          for(a=b;a<totalConnectedUsers-1;a++)
           strcpy(connected_users[a],connected_users[a+1]);
          totalConnectedUsers--;
        }
      printf("%s left the chatroom\n", nume);
      close(i);
      FD_CLR(i, master);
      //
    } 
    else
    	//sendMessage to all
      for(j = 0; j <= fdmax; j++){
        sendToAll(j, i, sockfd, nbytes_recvd, recv_buf, master );
    }
  } 
}

void connection_accept(fd_set *master, int *fdmax, int sockfd, struct sockaddr_in *client_addr)
{
  socklen_t addrlen;
  int newsockfd;
  char username[30]="",password[30]="";
  int i,j;
  int existing=0;
  int connected=0;
  int wrongPassword=0;
  
  addrlen = sizeof(struct sockaddr_in);
  //accept connection by calling accept function
  if((newsockfd = accept(sockfd, (struct sockaddr *)client_addr, &addrlen)) == -1) 
  {
    perror("accept");
    exit(1);
  }
  else 
  {
  	//set socket for monitoring
    FD_SET(newsockfd, master);
    if(newsockfd > *fdmax)
      *fdmax = newsockfd;
  }
  
  recv(newsockfd, username,20,0);
  recv(newsockfd, password,20,0);

  for(i=0;i<totalUsers-1;i++)
  {
    
    if(strcmp(users[i].username,username)==0 )
    {
      for(j=0;j<totalConnectedUsers-1;j++)
        {
        if(strcmp(connected_users[j],username)==0)
          {
            connected=1;
            break;
          }
        }
      if(connected==1) break;
      else 
       {
          if(strcmp(users[i].password,password)!=0)
            wrongPassword=1;
          existing=1;
          break;
        }
    }
  }
  if(connected==1) send(newsockfd,"Connected already!",strlen("Connected already!"),0);
  else
      if(existing==1 && wrongPassword==1) send(newsockfd,"Wrong password",strlen("Wrong password"),0);
      else 
        if(existing==1 && wrongPassword==0)
        {
           send(newsockfd,"ok!",31,0);
           strcpy(connected_users[totalConnectedUsers-1],username);
           totalConnectedUsers++;
        } 
        else
        {
            send(newsockfd,"ok!",31,0);
            strcpy(users[totalUsers-1].username,username);
            strcpy(users[totalUsers-1].password,password);
            strcpy(connected_users[totalConnectedUsers-1],username);
            totalUsers++;
            totalConnectedUsers++;
        }
    printf("%s is connecting\n", username);
}

void connect_request(int *sockfd, struct sockaddr_in *my_addr, int portNo)
{
  int yes = 1;
    
    //create server socket
  if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    exit(1);
  }
    //set server socket information
  my_addr->sin_family = AF_INET;
  my_addr->sin_port = htons(portNo);
  my_addr->sin_addr.s_addr = INADDR_ANY;
  memset(my_addr->sin_zero, '\0', sizeof my_addr->sin_zero);
    
  if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
    
  if (bind(*sockfd, (struct sockaddr *)my_addr, sizeof(struct sockaddr)) == -1) {
    perror("Unable to bind");
    exit(1);
  }
  if (listen(*sockfd, 10) == -1) {
    perror("listen");
    exit(1);
  }
  printf("\nTCPServer Waiting for client on port %d\n", portNo);
  fflush(stdout);
}

int main (int argc, char *argv[]) 
{

  fd_set master;
  fd_set read_fds;
  int fdmax, i, sockfd=0;
  int portno;
  struct sockaddr_in my_addr, client_addr;

	//check if server is initialized correctly
  if (argc<2) {
    fprintf (stderr,"Port missing!\n");
    exit(1);
	}

  portno = atoi(argv[1]);

	//clear any other fd_sets
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  connect_request(&sockfd,&my_addr, portno);
  //set server socket to monitoring, is now part of the set
  FD_SET(sockfd, &master);

  fdmax= sockfd;

  while(1)
  {
      read_fds = master;
      //select socket
      if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
        perror("select");
        exit(4);
      }
      for (i = 0; i <= fdmax; i++){
        if (FD_ISSET(i, &read_fds)){ //check if is socket is part of the set
          if (i == sockfd)
            connection_accept(&master, &fdmax, sockfd, &client_addr);//accept connection for clients
          else
            send_receive(i, &master, sockfd, fdmax);//send or receive for the current fd ready for R/W operation
        }
      }
  }
  return 0;
}