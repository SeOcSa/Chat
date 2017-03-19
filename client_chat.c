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

#define BUFSIZE 256

void send_receive_to_from_server(int i, int sockfd, char username[100]) {
  char message_sent[BUFSIZE]="";
  char message_received[BUFSIZE]="";
  int nBytes_received;

  if(i==0) // catre server
  {
    fgets(message_sent,BUFSIZE,stdin);
    //daca clientul tasteaza bye, va trimite mesajul corespunzator serverului si se va incheia conexiunea
    if (strcmp(message_sent , "bye\n") == 0)
    {
      send(sockfd,username,strlen(username),0);
      send(sockfd, ":", 1, 0);
      send(sockfd, message_sent, strlen(message_sent), 0);
      exit(0);
    }
    //mesajul difera de bye => se trimite doar mesajul corespunzator serverului
    else
    {
        send(sockfd,username,strlen(username),0);
        send(sockfd, ":", 1, 0);
        send(sockfd, message_sent, strlen(message_sent), 0);
    }
  }
  else // de la server
  {
    nBytes_received= recv(sockfd, message_received, BUFSIZE, 0);
    if(nBytes_received==0) // daca nu primeste nimic de la server => serverul a picat => clientul isi incheie conexiunea 
    {
      printf("Server is down, reconnect later\n");
      exit(0);
    }
    else //serverul trimite mesajul primit de la un user tuturor clientilor conectati 
    {    
        message_received[nBytes_received]='\0';
        printf("--%s",message_received); // fiecarui client ii va aparea in fereastra un mesaj de tipul "--USER:MESSAGE"
        fflush(stdout);
    }
  }
}

void connecting_client_to_server(int *sockfd, struct sockaddr_in *server_addr, int portNo, char addr[],char username[], char password[])
 {
  int n;
  char buffer[256]="";

  if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    exit(1);
  }
  server_addr->sin_family = AF_INET;
  server_addr->sin_port = htons(portNo);
  server_addr->sin_addr.s_addr = INADDR_ANY;
  memset(server_addr->sin_zero, '\0', sizeof server_addr->sin_zero);
  
  if(connect(*sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr)) == -1) {
    perror("connect");
    exit(1);
  }

  //clientul trimite username-ul si parola prin care a incercat sa se conecteze
  n=send(*sockfd, username, strlen(username), 0);
  n=send(*sockfd, password, strlen(password), 0);

  //primeste de la server un mesaj corespunzator daca logarea nu s-a facut cu succes si se va incheia conexiunea cu serverul
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
      fprintf (stderr,"Usage %s Host-Name Port Username Password\n", argv[0]);
      exit(0);
  }
   
  portNo=atoi(argv[2]);

  //clientul incearca sa se conecteze la server prin hostname, port username si parola
  connecting_client_to_server(&sockfd, &serv_addr,portNo,argv[1],argv[3],argv[4]);

  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  FD_SET(0, &master);
  FD_SET(sockfd, &master);
  fdmax = sockfd;

  while(1) {
     fflush(stdout);
     read_fds = master;
     if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
     {
        perror("select");
        exit(4);
     }

    for(i=0; i <= fdmax; i++ )
      if(FD_ISSET(i, &read_fds))
        send_receive_to_from_server(i, sockfd, argv[3]);
    }

  close(sockfd); 
  return 0;

}