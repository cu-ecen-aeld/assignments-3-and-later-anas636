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
#include <pthread.h>
#include <time.h>
#include<sys/queue.h>

#define LISTEN_BACKLOG 50
#define MAX_THREAD 50
#define BUF_SIZE 512

//Global Varibles
bool caught_signal = false;
char *filename = "/var/tmp/aesdsocketdata";
int it = 1;
//pthread_t thread_id[MAX_THREAD];
//int counter=0;
pthread_mutex_t lock;

/*struct args {

    int clientsocket; 
    char* ip;
};*/

struct slist_data_s {
    int clientsocket; 
    char* ip;
    int thread_flag;
    pthread_t thread_id;
    SLIST_ENTRY(slist_data_s) entries;
};
typedef struct slist_data_s slist_data_t;


int become_daemon ()
{   
    int x = fork();
    switch(x)                    
  {
    case -1:
        //sleep(50); 
        return -1;
    case 0: 
        //sleep(50); 
        break;                  
    default:
        //sleep(50); 
        _exit(EXIT_SUCCESS);
  }

  if(setsid() == -1)
  {               
    return -1;
  }
    int y = fork();
    switch(y)
    {
    case -1:
        //sleep(50);
        return -1;
    case 0: 
        //sleep(50);
        break;                  
    default:
        //sleep(50); 
        _exit(EXIT_SUCCESS);
    }

  return 0;

}


static void signal_handler(int signal_number){

    caught_signal = true;

}

void timer_funct(union sigval timer_data)
{
    char outstr[200];
    time_t t = time(NULL);
    struct tm *tmp;
    tmp = localtime(&t);
    size_t len = strftime(outstr, sizeof(outstr),"timestamp:%F %T \n",tmp);
    
    const char *filename = "/var/tmp/aesdsocketdata";

    pthread_mutex_lock(&lock);
    int fd_f = open(filename, O_APPEND | O_CREAT | O_RDWR, 0644);
    if (fd_f<0){
        pthread_exit((void *)1);	
    }

    ssize_t n_wr = write(fd_f, outstr, len);
    close(fd_f);
    pthread_mutex_unlock(&lock);
    if (n_wr < 0){
        close(fd_f);
        pthread_mutex_unlock(&lock);
        pthread_exit((void *)1);	
    }

}



/*void thread_funct_cleanUp (struct args* thread_func_args) {

    close(thread_func_args->clientsocket);
    free(thread_func_args->ip);
    thread_func_args->ip = NULL;
    free(thread_func_args);
    thread_func_args = NULL;
    

}*/



void* thread_function(void* arguments)
{

    slist_data_t* thread_func_args = (slist_data_t*) arguments;
    //const char *filename = "/var/tmp/aesdsocketdata";
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE); //clear the variable
    ssize_t nread;
    ssize_t byread;


    //printf("Connection done with client IP address: %s number: %d \n", thread_func_args->ip, it);
    it++;

    while((nread = recv(thread_func_args->clientsocket, buf, BUF_SIZE, 0))>0){

        pthread_mutex_lock(&lock);
        int fd_f = open(filename, O_APPEND | O_CREAT | O_RDWR, 0644);
        //printf("\n rcv: %s \n", buf);

        if (fd_f<0){
            pthread_mutex_unlock(&lock);
            //thread_funct_cleanUp(thread_func_args);
            thread_func_args->thread_flag = 1;
            pthread_exit((void *)1);	
        }

        ssize_t n_wr = write(fd_f, buf, nread);
        close(fd_f);
        pthread_mutex_unlock(&lock);
        
        if (n_wr != nread){
            //thread_funct_cleanUp(thread_func_args);
            thread_func_args->thread_flag = 1;
            pthread_exit((void *)1);	
        }

        if(buf[nread-1]=='\n')
        {
            break;
        }

        memset(buf, 0, BUF_SIZE); //clear the variable

    }

    if(nread < 0){
        //thread_funct_cleanUp(thread_func_args);
        thread_func_args->thread_flag = 1;
        pthread_exit((void *)1);
    }

    //send to client
    
    int fd_f1 = open(filename, O_RDONLY);

    if (fd_f1<1){
            //thread_funct_cleanUp(thread_func_args);
            thread_func_args->thread_flag = 1;
            pthread_exit((void *)1);	
        }


    while((byread = read(fd_f1, buf, BUF_SIZE))>0){

        //printf("\n sendinf: %s \n", buf);

        ssize_t bysent = send(thread_func_args->clientsocket, buf, byread, 0);

        if (bysent != byread){
            close(fd_f1);
            //thread_funct_cleanUp(thread_func_args);
            thread_func_args->thread_flag = 1;
            pthread_exit((void *)1);	
        }

    
        memset(buf, 0, BUF_SIZE); //clear the variable
    }

    close(fd_f1);
    syslog(LOG_DEBUG,"Closed connection from %s \n", thread_func_args->ip);
    thread_func_args->thread_flag = 1;
    //thread_funct_cleanUp(thread_func_args);
   
    return NULL;
    
}


int main (int argc, char *argv[])
{
	int daem_flag = 0;
    int opt;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct sockaddr client_addr;
    socklen_t client_addr_size;
    //char buf[BUF_SIZE];
    char *ip;
    int fd;
    int clientsocket;
    //ssize_t byread;
    struct sigaction action;
    //char *filename = "/var/tmp/aesdsocketdata";
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

    //initialise mutex
    if (pthread_mutex_init(&lock, NULL) != 0)
            exit(1);
    
    //timer
    int clock_id = CLOCK_MONOTONIC;
    timer_t timerId = 0;
    struct sigevent sev = {0};
    
    
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = &timer_funct;
    //sev.sigev_notify.sival_ptr = 

   
    struct itimerspec its = {   .it_value.tv_sec = 10,
                                .it_value.tv_nsec = 0,
                                .it_interval.tv_sec = 10,
                                .it_interval.tv_nsec = 0,

    };

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

     //create timer
    int res_ = timer_create(clock_id, &sev, &timerId);
    if (res_ != 0)
        exit(1);

    // start timer
    res_ = timer_settime(timerId,0,&its, NULL);
    if (res_ != 0)
    {
        //syslog(LOG_DEBUG, "ailure with %s    Erno : %d \n", strerror(errno), errno); 
        exit(1);
    }

    int res2 = listen(fd, LISTEN_BACKLOG);

    if (res2 == -1){
        //printf("error in listen");
        exit(1);
    }

    //SLIST initialization
    SLIST_HEAD(slisthead, slist_data_s) head;
    SLIST_INIT(&head);

    for( ; ; ) {

        if (caught_signal == true){
            syslog(LOG_DEBUG,"Caught signal, exiting \n");

            if (access(filename, F_OK) == 0) {
                // file exists
                if (remove(filename) == 0) {
                //printf("File %s successfully deleted.\n", filename);
                } else {
                //perror("Error deleting file");
                    exit(1);
                }
            }
            
            break;
        }

        client_addr_size = sizeof(client_addr);
        clientsocket = accept(fd, &client_addr, &client_addr_size);

        if (caught_signal == true){
            syslog(LOG_DEBUG,"Caught signal, exiting \n");
            if (access(filename, F_OK) == 0) {
                // file exists
                if (remove(filename) == 0) {
                //printf("File %s successfully deleted.\n", filename);
                } else {
                //perror("Error deleting file");
                    exit(1);
                }
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


        // running the thread function here 
        /*
        Use of SLIST linked list
        */
  
        SLIST_HEAD(slisthead, slist_data_s) head;
        SLIST_INIT(&head);
        slist_data_t *datap=NULL;
        slist_data_t *datap_temp=NULL;

        datap = malloc(sizeof(slist_data_t));
        if (datap == NULL)
            exit(1);
        
        datap->ip = malloc(strlen(ip)+1);
        if (datap->ip ==NULL)
            exit(1);
        strcpy(datap->ip, ip);
        datap->thread_flag = 0;
        datap->clientsocket = clientsocket;

        SLIST_INSERT_HEAD(&head, datap, entries);


        int rc = pthread_create(&(datap->thread_id), NULL, thread_function, datap);
        //printf("thread ID %ld\n", datap->thread_id);
        //printf("Error creating the thread, errno is %d (%s)\n",errno,strerror(errno));

        if (rc != 0){
            exit(1);
        }

        /*counter++;
        if (counter+1 >= MAX_THREAD)
            exit(1);*/
        
        SLIST_FOREACH_SAFE(datap, &head, entries, datap_temp) {
            //printf("thread ID %ld\n", datap->thread_id);
            if (datap->thread_flag == 1) {
                pthread_join(datap->thread_id, NULL);
                SLIST_REMOVE(&head, datap, slist_data_s, entries);
                free(datap->ip);
                free(datap);
            }
            //printf("Read1: %d\n", datap->value);
        }
        //free(datap_temp);
        //free(datap);
   
    } 

    /*for(int i=0; i<=counter; i++){
        pthread_join(thread_id[i], NULL);
    }*/
    
    pthread_mutex_destroy(&lock);
    close(fd);
    timer_delete(timerId);
    closelog();
    
    return 0;

}





