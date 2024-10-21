#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>


int main(int argc, char **argv)
{
    int rc=0;
    openlog(NULL,0,LOG_USER);
    if( argc != 3 ) {
        //printf("Usage: %s filename string \n", argv[0]);
        syslog(LOG_ERR,"Invalid Number of arguments: %d",argc);
        
        rc=1;
        
    } else {
    
       
        const char *filename = argv[1];
        const char *string = argv[2];
        
        syslog(LOG_DEBUG,"writing %s to %s",string,filename);
        
        int fd=open(filename, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO);
        
        if( fd == -1 ) {
        
          syslog(LOG_ERR,"Error with open(), erno is %d (%s)\n",errno,strerror(errno));
        
          rc=1;
          
            
        } else {
        
            ssize_t byteswritten=write(fd, string, strlen(string));
            
            if( byteswritten == -1 ){
              
              syslog(LOG_ERR,"Error with write(), erno is %d (%s)\n",errno,strerror(errno));
            
              rc=1;
            
            }
            
            close(fd);
        
            }
        
        }
        closelog();
        return rc;
                             
    }

   
