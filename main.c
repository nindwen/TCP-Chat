#include "signal.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void handleConnection(int, int*);
void serverLoop(int*);
void sendToClient(int);
void error(const char *msg)
{
      perror(msg);
      exit(1);
}

int main(int argc, char *argv[])
{
      signal(SIGCHLD,SIG_IGN);

      int sockfd, newsockfd, portno, pid;
      socklen_t clilen;
      struct sockaddr_in serv_addr, cli_addr;

      sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0) 
            error("ERROR opening socket");
      bzero((char *) &serv_addr, sizeof(serv_addr));
      portno = 5650;
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = INADDR_ANY;
      serv_addr.sin_port = htons(portno);
      if (bind(sockfd, (struct sockaddr *) &serv_addr,
                        sizeof(serv_addr)) < 0) 
            error("ERROR on binding");

      //Pipes
      int fildes[2];
      const int PIPE_BUFFER_SIZE = 100;
      char buf[PIPE_BUFFER_SIZE];
      ssize_t nbytes;
      int status;
      status = pipe(fildes);

      switch(fork()) {
            case -1:
                  error("ERROR on fork");
                  break;
            case 0:
                  serverLoop(fildes);
                  exit(0);
      }


      listen(sockfd,5);
      clilen = sizeof(cli_addr);
      while (1) {
            newsockfd = accept(sockfd, 
                        (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0) 
                  error("ERROR on accept");
            pid = fork();
            if (pid < 0)
                  error("ERROR on fork");
            if (pid == 0)  {
                  close(sockfd);
                  handleConnection(newsockfd, fildes);
                  exit(0);
            }
            else close(newsockfd);
      } /* end of while */
      close(sockfd);
      return 0; /* we never get here */
}

/******** handleConnection() *********************
  There is a separate instance of this function 
  for each connection.  It handles all communication
  once a connnection has been established.
 *****************************************/
void handleConnection (int sock, int* pipe)
{
      switch(fork()) {
            case -1:
                  error("Sporks");
            case 0:
                  sendToClient(sock);
                  exit(0);
      }

      close(pipe[0]);
      int n;
      char buffer[256];

      while(1) {
            bzero(buffer,256);
            n = read(sock,buffer,255);
            if (n < 0) error("ERROR reading from socket");
            n = write(pipe[1],buffer,strlen(buffer));
            if (n < 0) error("ERROR writing to pipe");
      }
}

void sendToClient(int sock) {
      FILE* handle = NULL;
      handle = fopen("usvs.log","r");
      if(handle == NULL) {
            perror("Cannot open log (r)");
      }
      fseek(handle,0, SEEK_END);
      char buffer[256];
      while(1) {
            bzero(buffer,256);
            fread(buffer,1,255,handle);
            write(sock,buffer,strlen(buffer));
      }
}

void serverLoop(int* pipe) {
      close(pipe[1]);
      int n;
      char buffer[256];
      FILE* handle = NULL;
      handle = fopen("usvs.log","a");
      if(handle == NULL) {
            perror("Cannot open log (r)");
      }
      while(1) {
            bzero(buffer,256);
            n = read(pipe[0], buffer, 255);
            n = fwrite(buffer,1,strlen(buffer),handle);
            fflush(handle);
      }
}