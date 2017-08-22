#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <string>
#define BUFSIZE 1024

int write_n_bytes(int fd, char * buf, int count);
char *USERAGENT="Mozilla 5.0";
int main(int argc, char * argv[]) {
    char * server_name = NULL;
    int server_port = 0;
    char * server_path = NULL;

    int sock = 0;
    int rc = -1;
    int datalen = 0;
    bool ok = true;
    struct sockaddr_in sa;
    FILE * wheretoprint = stdout;
    struct hostent * site = NULL;
    char * req = NULL;

    char buf[BUFSIZE + 1];
   // char * bptr = NULL;
   // char * bptr2 = NULL;
    char * endheaders = NULL;
    std::string response="";
    std::string header="";
    struct timeval timeout;
    fd_set set;

    /*parse args */
    if (argc != 5) {
        fprintf(stderr, "usage: http_client k|u server port path\n");
        exit(-1);
    }
    server_name = argv[2];
    server_port = atoi(argv[3]);
    server_path = argv[4];



    /* initialize minet */
    if (toupper(*(argv[1])) == 'K') {
        minet_init(MINET_KERNEL);
    } else if (toupper(*(argv[1])) == 'U') {
        minet_init(MINET_USER);
    } else {
        fprintf(stderr, "First argument must be k or u\n");
        exit(-1);
    }

    /* create socket */
        sock=minet_socket(SOCK_STREAM);
        if(sock==-1)
        {
                fprintf(stderr, "Create minet socket Error.");
                exit(-1);
        }
    // Do DNS lookup
    /* Hint: use gethostbyname() */
       site=gethostbyname(server_name);
        if(site==NULL)
        {
                fprintf(stderr, "Host name Error!");
                minet_close(sock);
                exit(-1);
        }
    /* set address */
        memset(&sa,0,sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(server_port);
        sa.sin_addr.s_addr = * (unsigned long *)site->h_addr_list[0];//--------------------------
 /* connect socket */
        int i=minet_connect(sock,(struct sockaddr_in *) &sa);
        if(i!=0)
        {
                fprintf(stderr, "Can't connect to socket!\n");
                minet_close(sock);
                exit(-1);
        }
    /* send request */
        if(server_path[0]=='/')
        {
                server_path=server_path+1;
        }
        char* temp = "GET /%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\nUser-Agent: %s\r\n\r\n";
        req=(char *)malloc(strlen(server_name)+strlen(server_path)+strlen(USERAGENT)+strlen(temp)-5);
        sprintf(req,temp,server_path,server_name,USERAGENT);
        datalen = strlen(req) + 1;
        rc=minet_write(sock,req,datalen);
        if(rc<0)
        {
                fprintf(stderr,"Can't send request!\n");
                minet_close(sock);
                exit(-1);
        }
/* wait till socket can be read */
    /* Hint: use select(), and ignore timeout for now. */
        FD_ZERO(&set);
        FD_SET(sock,&set);
        if(FD_ISSET(sock,&set)==0)
        {
                fprintf(stderr,"Can't add sock to the set");
                minet_close(sock);
                exit(-1);
        }
        rc=minet_select(sock+1,&set,0,0,&timeout);
        if(rc<0)
        {
                fprintf(stderr,"select error!");
                minet_close(sock);
                exit(-1);
        }
    /* first read loop -- read headers */
        memset(buf,0,sizeof(buf));
        endheaders="\r\n\r\n";
        size_t pos;
        while((rc=minet_read(sock,buf,BUFSIZE))>0)
        {
                buf[rc]='\0';
                response=response+(std::string)buf;
                if((pos=response.find(endheaders,0))!=std::string::npos)
                {
                        header=response.substr(0,pos);
                        response=response.substr(pos+4);//the entity
                        break;
                }
        }
    /* examine return code */
    //Skip "HTTP/1.0"
    //remove the '\0'
        std::string text=header.substr(header.find(" ")+1);
        text=text.substr(0,text.find(" "));
    // Normal reply has return code 200
        if(text=="200")
        {
                ok=true;
        }else{
                ok=false;
        }


    /* print first part of response */

        fprintf(wheretoprint,header.c_str());
        fprintf(wheretoprint,endheaders);
        fprintf(wheretoprint,response.c_str());
    /* second read loop -- print out the rest of the response */
        while((rc=minet_read(sock,buf,BUFSIZE))>0)
        {
                buf[rc]='\0';
                if(ok)
                {
                        fprintf(wheretoprint,buf);
                }else{
                        fprintf(stderr,buf);
                }
        }
    /*close socket and deinitialize */
        minet_close(sock);
        minet_deinit();

    if (ok) {
        return 0;
    } else {
        return -1;
    }
}
int write_n_bytes(int fd, char * buf, int count) {
    int rc = 0;
    int totalwritten = 0;

    while ((rc = minet_write(fd, buf + totalwritten, count - totalwritten)) > 0) {
        totalwritten += rc;
    }

    if (rc < 0) {
        return -1;
    } else {
        return totalwritten;
    }
}


