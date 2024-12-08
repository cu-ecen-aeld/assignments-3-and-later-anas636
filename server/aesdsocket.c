#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <stdbool.h>

#define LISTEN_BACKLOG 50
#define BUF_SIZE 500

bool caught_signal = false;

int become_daemon ()
{
     switch(fork())                    
  {
    case -1: return -1;
    case 0: break;                  
    default: _exit(EXIT_SUCCESS);
  }

  if(setsid() == -1)
  {               
    return -1;
  }

   switch(fork())
  {
    case -1: return -1;
    case 0: break;                  
    default: _exit(EXIT_SUCCESS);
  }

  return 0;

}


static void signal_handler(int signal_number){

    caught_signal = true;


}


int main (int argc, char *argv[])
{
	int daem_flag = 0;
    int opt;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct sockaddr client_addr;
    socklen_t client_addr_size;
    char buf[BUF_SIZE];
    char *ip;
    int total_size=0;
    int fd;
    int clientsocket;
    ssize_t byread;
    struct sigaction action;
    const char *filename = "/var/tmp/aesdsocketdata";
    memset(&action, 0, sizeof(action));
    action.sa_handler = signal_handler;
    if(sigaction(SIGTERM, &action, NULL) !=0 ){

        exit(1);
    }

    if(sigaction(SIGINT, &action, NULL) !=0 ){
        exit(1);
    }

    while ((opt = getopt(argc, argv, "d")) != -1)
    {
        switch (opt)
        {
            case 'd':
                daem_flag = 1;
            default:
                break;
        }
    }
    

    openlog(NULL,0,LOG_USER);
    memset(&hints, 0, sizeof(hints));   

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int res = getaddrinfo(NULL, "9000", &hints, &servinfo);


    if (res != 0){

        //printf("error with geraddrinfo\n");
        exit(1);
    }


    fd = socket(servinfo->ai_family, servinfo->ai_socktype, 0);


    if (fd == -1){

        //printf("error creating the socket");
        //printf("Error creating the socket, errno is %d (%s)\n",errno,strerror(errno));
        exit(1);    
    }

    const int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        exit(1);
    }
        
    //printf("socket created with sucesss\n");
    

    int res1 = bind(fd, servinfo->ai_addr, servinfo->ai_addrlen);

    if (res1 == -1){
        printf("Error in bind, errno is %d (%s)\n",errno,strerror(errno));
        exit(1); 
    }

    freeaddrinfo(servinfo);

    if (daem_flag==1)
    {
        int ret;
        ret = become_daemon();
        if (ret)
        {
            exit(1);
        }
    } 


    int res2 = listen(fd, LISTEN_BACKLOG);

    if (res2 == -1){
        //printf("error in listen");
        exit(1);
    }


    for( ; ; ) {

        if (caught_signal == true){
            syslog(LOG_DEBUG,"Caught signal, exiting \n");

            if (remove(filename) == 0) {
                //printf("File %s successfully deleted.\n", filename);
            } else {
                //perror("Error deleting file");
                exit(1);
            }

            break;
        }

        client_addr_size = sizeof(client_addr);
        clientsocket = accept(fd, &client_addr, &client_addr_size);

        if (caught_signal == true){
            syslog(LOG_DEBUG,"Caught signal, exiting \n");
            if (remove(filename) == 0) {
                //printf("File %s successfully deleted.\n", filename);
            } else {
                //perror("Error deleting file");
                exit(1);
            }
            break;
        }


        if (clientsocket == -1){
            //printf("Your boolean variable is: %s", caught_signal ? "true" : "false");
            //printf("error with accept\n");
            exit(1);
        }
        if (client_addr.sa_family == AF_INET){
            struct sockaddr_in *sin = (struct sockaddr_in *) &client_addr;
            ip = inet_ntoa(sin->sin_addr);
            //printf("Connection done with client IP address: %s\n", ip);
            syslog(LOG_DEBUG,"Accepted connection from %s \n", ip);
        }


        memset(buf, 0, BUF_SIZE); //clear the variable
        ssize_t nread;

        //printf("we are here\n");

        while((nread = recv(clientsocket, buf, BUF_SIZE, 0))>0){


            int fd_f = open(filename, O_APPEND | O_CREAT | O_RDWR, 0644);
            if (fd_f<0){
                //printf("error in open(use erno)");
                exit(1);	
            }


            //printf("\n The message received is %s \n", buf);
            ssize_t n_wr = write(fd_f, buf, nread);
            close(fd_f);

            if (n_wr != nread){
                //printf("error in write(use erno)");
                exit(1);	
            }

            if(buf[nread-1]=='\n')
            {
                break;
            }

            memset(buf, 0, BUF_SIZE); //clear the variable

        }

        if(nread < 0){
            //printf("Error in recv");
            exit(1);
        }

        //send to client
        
        int fd_f1 = open(filename, O_RDONLY);

        if (fd_f1<1){
                //printf("error in open(use erno)");
                exit(1);	
            }


        while((byread = read(fd_f1, buf, BUF_SIZE))>0){

            //printf("\n sendinf: %s \n", buf);

            ssize_t bysent = send(clientsocket, buf, byread, 0);

            if (bysent != byread){
                //printf("error in send(use erno)");
                exit(1);	
            }

            //printf("\n sendinf: %s \n", buf);

            memset(buf, 0, BUF_SIZE); //clear the variable
        }

        close(fd_f1);
        close(clientsocket);
        syslog(LOG_DEBUG,"Closed connection from %s \n", ip);
  
    }

    close(fd);
    closelog();

    return 0;

}





