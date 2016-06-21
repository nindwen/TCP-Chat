#include "signal.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define LOG_FILE "usvs.log"

void handleConnection(int, int*, int);
void serverLoop(int*);
void sendToClient(int);

void error(const char *msg)
{
      perror(msg);
      exit(1);
}

int main(int argc, char *argv[])
{
      //Ignore child signals
      signal(SIGCHLD,SIG_IGN);

      //Socket init
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
      int maxClientId = 0;
      while (1) {
            newsockfd = accept(sockfd, 
                        (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0) 
                  error("ERROR on accept");
            maxClientId++;
            pid = fork();
            if (pid < 0)
                  error("ERROR on fork");
            if (pid == 0)  {
                  close(sockfd);
                  handleConnection(newsockfd, fildes, maxClientId);
                  exit(0);
            }
            else close(newsockfd);
      } /* end of while */
      close(sockfd);
      return 0; /* we never get here */
}

//Forks to sendToClient. Reads messages from children and
//sends them to serverLoop through pipe
void handleConnection (int sock, int* pipe, int maxClientId)
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
      char with_nick[256];
      bzero(buffer,256);
      bzero(with_nick,256);

      while(read(sock,buffer,255-strlen(with_nick))) {
            sprintf(with_nick, "[%5d]: ", maxClientId);
            strcat(with_nick, buffer);
            n = write(pipe[1],with_nick,strlen(with_nick));
            bzero(buffer,256);
            bzero(with_nick,256);
      }
}

//Reads from file, writes to clients
void sendToClient(int sock) {
      FILE* handle = NULL;
      handle = fopen(LOG_FILE,"r");
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

//Listens to children, writes to file
void serverLoop(int* pipe) {
      close(pipe[1]);
      int n;
      char buffer[256];
      FILE* handle = NULL;
      handle = fopen(LOG_FILE,"a");
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
