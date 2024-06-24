#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


int main(int num_args, char * fileinfo[])
{
    int fd = 0;
    size_t content_len = 0;

    openlog(NULL, 0, LOG_USER);

    if(num_args != 3)
    {
        syslog(LOG_ERR, "script should be executed with 2 arguments\r\n ex. writer.c <writefile> <writestr>\r\n writefile = full path to a file, writestr = the string to be written to the file");
        return 1;
    }
    else
    {
        fd = creat(fileinfo[1], 0766);
        if(fd == -1)
        {
            syslog(LOG_ERR, "file %s could not be created or opened", fileinfo[1]);
            return 1;
        }
        else
        {
            /* continue*/
        }
    }

    content_len = strlen(fileinfo[2]);

    if(write(fd, fileinfo[2], content_len) != content_len)
    {
        syslog(LOG_ERR, "writing of file %s failed", fileinfo[1]);
        return 1;
    }
    else
    {
        syslog(LOG_DEBUG, "Writing %s to %s", fileinfo[2], fileinfo[1]);
    }

    close(fd);

    return 0;
}
