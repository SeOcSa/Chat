#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define message_bufferSIZE 256

struct userinfo
{
  char username[30];
  char password[30];
}users[100]; // lista de useri inregistrati

char connected_users[100][100]; //lista de useri conectati

int totalUsers=1;
int totalConnectedUsers=1;

void sendMessageToAllClients(int j, int i, int sockfd, int nBytes, char *message_received, fd_set *master) 
{
  if (FD_ISSET(j, master))
  {
    if (j != sockfd) //mesajul se trimite doar clientilor nu si serverului
     { 
        if (send(j, message_received, nBytes, 0) == -1) 
        perror("send message");
    }
  }
}

void send_receive_to_from_client(int i, fd_set *master, int sockfd, int fdmax)
{
  int nBytes_received, j;
  char message_received[message_bufferSIZE]="", message_buffer[message_bufferSIZE]="";
  
   //se astepta un mesaj din partea clientului
  if ((nBytes_received = recv(i, message_received, message_bufferSIZE, 0)) <= 0)
  {
    if (nBytes_received == 0) //clientul nu a trimis nimic => este deconectat
      printf("socket %d hung up\n", i);
    else 
      perror("recv");
    close(i);
    FD_CLR(i, master);
  }
  else //clientul a transmis un mesaj
  { 
    int len,a,w=0;
    int b;
    char nume[20]="";

    strcpy(message_buffer,message_received);
    message_buffer[strlen(message_buffer)-1]='\0';
    len=strlen(message_buffer)-1;
    //tolower(message_buffer);

    //daca clientul trimite mesajul bye => se deconecteaza
    if(tolower(message_buffer[len])=='e' && tolower(message_buffer[len-1])=='y' && tolower(message_buffer[len-2])=='b') 
    {
      //formam numele userului ce a trimis mesajul
      while(message_buffer[w]!=':'&& w!=len) 
        {
          nume[w]=message_buffer[w];
          w++;
        }

      //cautam userul in lista de useri conectati
      for(b=0;b<totalConnectedUsers-1;b++)
        {
          if(strcmp(nume,connected_users[b])==0)
            break;
        }
      //eliminam userul din lista de useri conectati
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
      char leaving[100]="";
      strcat(leaving,nume);
      strcat(leaving," is leaving the chatroom...\n");
      for(j = 0; j <= fdmax; j++)
      {
          if (FD_ISSET(j, master))
        {
          if (j != sockfd && j!=i)  // && j != i) //nu trimite la el insusi
          { 
            if (send(j, leaving, strlen(leaving), 0) == -1) //sending message
               perror("send message");
          }
        }
      }
    }
    //mesajul este diferit de bye => serverul il trimite tuturor clientilor conectati
    else
      for(j = 0; j <= fdmax; j++){
        sendMessageToAllClients(j, i, sockfd, nBytes_received, message_received, master );
    }
  } 
}

void accepting_connection_server(fd_set *master, int *fdmax, int sockfd, struct sockaddr_in *client_addr)
{
  socklen_t addr_length;
  int newsockfd;
  char username[30]="",password[30]="";
  int i,j;
  int existing=0;
  int connected=0;
  int wrongPassword=0;
  
  addr_length = sizeof(struct sockaddr_in);
  if((newsockfd = accept(sockfd, (struct sockaddr *)client_addr, &addr_length)) == -1) 
  {
    perror("accept");
    exit(1);
  }
  else 
  {
    FD_SET(newsockfd, master);
    if(newsockfd > *fdmax)
      *fdmax = newsockfd;
  }
  
  //se primesc username-ul si parola de la client
  recv(newsockfd, username,20,0);
  recv(newsockfd, password,20,0);

  //se face verificarea daca clientul este inregistrat deja, in caz afirmativ se verifica daca este logat.
  //In acest caz se va trasnmite un mesaj clientului si se va intrerupe conexiunea.
  //Daca userul nu este logat deja, se va verifica daca parola coicide cu cea asociata userului.
  //Daca parola difera, un mesaj este afisat clientului si conexiunea se intrerupe
  //Daca parola coincide, clientul se va putea loga si va putea participa la chat
  /*Daca userul nu este inregistrat, datele sale se salveaza in lista de useri inregistrati
    se va loga si va putea participa la chat*/
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
    printf("%s is connecting\n", username); // in server va aparea cine incearca sa se conecteze
}

void connecting_server(int *sockfd, struct sockaddr_in *server_addr, int portNo)
{
  int yes = 1;
    
  if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    exit(1);
  }
    
  server_addr->sin_family = AF_INET;
  server_addr->sin_port = htons(portNo);
  server_addr->sin_addr.s_addr = INADDR_ANY;
  memset(server_addr->sin_zero, '\0', sizeof server_addr->sin_zero);
    
  if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
    
  if (bind(*sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr)) == -1) {
    perror("Unable to bind");
    exit(1);
  }
  if (listen(*sockfd, 10) == -1) {
    perror("listen");
    exit(1);
  }
  
  printf("\nTCPServer is working on port %d\n", portNo);
  fflush(stdout);
}


int main (int argc, char *argv[]) 
{

  fd_set master;
  fd_set read_fds;
  int fdmax, i, sockfd=0;
  int portNo;
  struct sockaddr_in server_addr, client_addr;

  if (argc<2) {
    fprintf (stderr,"Port missing!\n");
    exit(1);
    }

  portNo = atoi(argv[1]);

  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  connecting_server(&sockfd,&server_addr, portNo);
  FD_SET(sockfd, &master);

  fdmax= sockfd;

  while(1)
  {
      read_fds = master;
      if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
        perror("select");
        exit(4);
      }
      for (i = 0; i <= fdmax; i++){
        if (FD_ISSET(i, &read_fds)){
          if (i == sockfd)
            accepting_connection_server(&master, &fdmax, sockfd, &client_addr); // clientul se conecteaza la server
          else
            send_receive_to_from_client(i, &master, sockfd, fdmax); //clientul este conectat si incepe "discutia" cu serverul
        }
      }
  }
  close(sockfd);
  return 0;
}