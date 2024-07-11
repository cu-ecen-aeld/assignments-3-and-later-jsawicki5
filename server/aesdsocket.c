#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef struct {
    bool sig_rec;
    int s_fd;
    int recv_s_fd;
}AESDSOC_DATA_t;

/* Global Variables*/
static AESDSOC_DATA_t aesdsocketData = {
    .sig_rec = false,
    .s_fd = 0
};




/*********** LOCAL FUNCTIONS ***********/
static void _aesdsocket_handler (int signum)
{
    if(signum == SIGINT || signum == SIGTERM )
    {
        aesdsocketData.sig_rec = true;
    }   

    return;
}

static void _aesdsocket_shutdownHelper(int recv_s_fd, int fd)
{
    if(recv_s_fd > 0)
    {
        shutdown(recv_s_fd, SHUT_RD);
        close(recv_s_fd);
    }
    else
    {
        /* Continue */
    }

    if(aesdsocketData.s_fd > 0)
    {
        shutdown(aesdsocketData.s_fd, SHUT_RDWR);
        close(aesdsocketData.s_fd);
    }
    else
    {
        /* Continue */
    }

    if(fd > 0)
    {
        close(fd);
        unlink("/var/tmp/aesdsocketdata");
    }
    else
    {
        /* Continue */
    }

    return;
}

static int _aesdsocket_Init(void)
{
    int status;
    struct sigaction l_sig;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    /* Set up signal handler */
    memset(&l_sig, 0, sizeof(l_sig));
    l_sig.sa_handler = _aesdsocket_handler;
    l_sig.sa_flags = 0;
    sigfillset(&l_sig.sa_mask);

    if (sigaction(SIGINT , (const struct sigaction *)&l_sig, NULL) != 0)
    {
        syslog(LOG_ERR, "signal handler for SIGINT failed to set with err, %d", errno);
        return -1;
    }
    else
    {
        /* continue */
    }

    if (sigaction(SIGTERM , (const struct sigaction *)&l_sig, NULL) != 0)
    {
        syslog(LOG_ERR, "signal handler for SIGINT failed to set with err, %d", errno);
        return -1;
    }
    else
    {
        /* continue */
    }



    /* initialize socket at port 9000 */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(NULL, "9000", &hints, &servinfo)) != 0)
    {
        syslog(LOG_ERR, "getaddrinfo error: %s", gai_strerror(status));
        return -1;
    }
    else
    {
        /* continue */
    }

    if((aesdsocketData.s_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        syslog(LOG_ERR, "socket() error");
        freeaddrinfo(servinfo);
        return -1;
    }
    else
    {
        /* continue */
    }

    if(bind(aesdsocketData.s_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        syslog(LOG_ERR, "bind() error: %d", errno);
        freeaddrinfo(servinfo);
        return -1;
    }

    return 0;
}

static int _aesdsocket_Run(void)
{
    int recv_s_fd = 0, fd = 0;
    size_t rd_len = 0;
    static char rd_buff[2048];
    struct sockaddr_storage address;
    socklen_t addrlen;
    ssize_t sent_bytes = 0, written, total_read = 0;

    if(listen(aesdsocketData.s_fd, 8) == -1)
    {
        syslog(LOG_ERR, "listen() error: %d", errno);
        close(aesdsocketData.s_fd);
        return -1;
    }
    else
    {
        /* Open the file and create it if it's not already created */
        fd = open("/var/tmp/aesdsocketdata", O_RDWR|O_CREAT|O_APPEND, 0766);
        if(fd == -1)
        {
            syslog(LOG_ERR, "file could not be created or opened");
            return 1;
        }
        else
        {
            /* continue*/
        }

        while(1)
        {
            memset(&address, 0, sizeof(address));
            addrlen = sizeof(address);
            if((recv_s_fd = accept(aesdsocketData.s_fd, (struct sockaddr *)&address, &addrlen)) == -1)
            {
                if((errno == EINTR) && (aesdsocketData.sig_rec != false))
                {
                    syslog(LOG_INFO, "Caught signal, exiting");
                    _aesdsocket_shutdownHelper(recv_s_fd, fd);
                    return 0;
                }
                else
                {
                    syslog(LOG_ERR, "accept() error: %d", errno);
                    _aesdsocket_shutdownHelper(recv_s_fd, fd);
                    return -1;
                }
                
            }
            else
            {
                void *addr;
                char ipstr[INET6_ADDRSTRLEN];

                if (address.ss_family == AF_INET) 
                { // IPv4
                    struct sockaddr_in *ipv4 = (struct sockaddr_in *)&address;
                    addr = &(ipv4->sin_addr);
                } 
                else 
                { // IPv6
                    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&address;
                    addr = &(ipv6->sin6_addr);
                }

                // convert the IP to a string and print it:
                inet_ntop(address.ss_family, addr, ipstr, sizeof(ipstr));
                syslog(LOG_INFO, "Accepted connection from %s", ipstr);
                memset(rd_buff, 0, sizeof(rd_buff));
                do
                {
                    if((rd_len = recv(recv_s_fd, (void *)&rd_buff[0], 1, 0)) == -1)
                    {
                        if((errno == EINTR) && (aesdsocketData.sig_rec != false))
                        {
                            syslog(LOG_INFO, "Caught signal, exiting");
                            _aesdsocket_shutdownHelper(recv_s_fd, fd);
                            return 0;
                        }
                        else
                        {
                            syslog(LOG_ERR, "recv() error: %d", errno);
                            _aesdsocket_shutdownHelper(recv_s_fd, fd);
                            return -1;
                        }
                    }
                    else if (rd_len == 0)
                    {
                        break;
                    }
                    else
                    {
                        if((written = write(fd, &rd_buff[0], 1)) != 1)
                        {
                            if (written == -1) {
                                syslog(LOG_ERR, "write() error: %d", errno);
                                _aesdsocket_shutdownHelper(recv_s_fd, fd);
                                return -1;
                            }
                        }
                        else
                        {
                            /* Continue */
                        }

                        if(rd_buff[0] == '\n')
                        {
                            fsync(fd);  // Ensure data is written to disk

                            memset(rd_buff, 0, sizeof(rd_buff));
                            
                            lseek(fd, 0, SEEK_SET); // Go to the start of the file
                            while ((total_read = read(fd, rd_buff, sizeof(rd_buff))) > 0) 
                            {
                                sent_bytes = 0;
                                while (sent_bytes < total_read) 
                                {
                                    written = send(recv_s_fd, rd_buff + sent_bytes, total_read - sent_bytes, 0);
                                    if (written == -1) 
                                    {
                                        syslog(LOG_ERR, "send() error: %d", errno);
                                        _aesdsocket_shutdownHelper(recv_s_fd, fd);
                                        return -1;
                                    }
                                    sent_bytes += written;
                                }
                            }
                            if (total_read == -1) {
                                syslog(LOG_ERR, "read() error: %d", errno);
                                _aesdsocket_shutdownHelper(recv_s_fd, fd);
                                return -1;
                            }
                            
                            memset(rd_buff, 0, sizeof(rd_buff));
                        }
                        else
                        {
                            /* continue */
                            rd_buff[0] = 0;
                        }
                    }

                    
                    
                }while(rd_len != 0);

                syslog(LOG_INFO, "Closed connection from %s", ipstr);
            }
        }
    }

    return 0;
}



/********GLOBAL FUNCTIONS *************/
int main(int num_args, char * option[])
{
    bool daemon = false;

    openlog(NULL, 0, LOG_USER);

    if(_aesdsocket_Init() != 0)
    {
        syslog(LOG_ERR, "aesdsocket failed to initialize");
        return -1;
    }
    else
    {
        /* continue */
    }

    if(num_args == 2)
    {
        if(strstr(option[1], "-d") != NULL)
        {
            syslog(LOG_INFO, "starting aesdsocket in daemon mode");
            daemon = true;
        }
        else
        {
            syslog(LOG_ERR, "%s option not recognized use -d to start in daemon mode or leave blank to run normally", option[1]);
            return 1;
        }
        
    }
    else
    {
        syslog(LOG_INFO, "starting aesdsocket");
    }

    if(daemon != false)
    {
        pid_t pid=fork();
        switch(pid)
        {
            case -1:
            {
                perror("fork");
                return -1;
            }
            case 0:
            {
                /* Child code */
                if(_aesdsocket_Run() != 0)
                {
                    return -1;
                }
                else
                {
                    /* continue */
                }
                break;
            }
            default:
            {
                /* Parent code, Continue*/
                break;
            }
        }
    }
    else
    {
        if(_aesdsocket_Run() != 0)
        {
            return -1;
        }
        else
        {
            /* continue */
        }
    }


    return 0;
}
