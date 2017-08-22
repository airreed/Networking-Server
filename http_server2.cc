#include "minet_socket.h"
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <string>
#define BUFSIZE 1024
#define FILENAMESIZE 100
int handle_connection(int);
int writenbytes(int,char *,int);
int readnbytes(int,char *,int);

int main(int argc,char *argv[])
{
  int server_port;
  int sock,sock2;
  struct sockaddr_in sa;
  int rc,i;
  fd_set readlist;
  fd_set connections;
  int maxfd;

  /* parse command line args */
  if (argc != 3)
  {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
  }
  server_port = atoi(argv[2]);
  if (server_port < 1500)
  {
    fprintf(stderr,"INVALID PORT NUMBER: %d; can't be < 1500\n",server_port);
    exit(-1);
  }

  /* initialize and make socket */
   if(toupper(*(argv[1]))=='K')
        minet_init(MINET_KERNEL);
   else if(toupper(*(argv[1]))=='U')
        minet_init(MINET_USER);
   else
   {
        fprintf(stderr,"Input Format Error!\n");
        exit(-1);
   }
   sock=minet_socket(SOCK_STREAM);
   if(sock==-1)
   {
        minet_perror("Can't create socket!\n");
        exit(-1);
   }


/* set server address*/
   memset(&sa,0,sizeof sa);
   sa.sin_port=htons(server_port);
   sa.sin_addr.s_addr=htonl(INADDR_ANY);
   sa.sin_family=AF_INET;
  /* bind listening socket */
  rc=minet_bind(sock,(struct sockaddr_in *)&sa);
  /* start listening */
  rc= minet_listen(sock,5);
  maxfd=sock;
  FD_ZERO(&readlist);
  FD_SET(sock,&connections);
  /* connection handling loop */
  while(1)
  {
    /* create read list */
  readlist=connections;
    /* do a select */
  minet_select(maxfd+1,&readlist,0,0,0);
    /* process sockets that are ready */
  for(i=0;i<maxfd+1;i++)
  {  if(FD_ISSET(i,&readlist))
    {
      /* for the accept socket, add accepted connection to connections */
      if (i == sock)
      {
        sock2=minet_accept(sock,&sa);
        FD_SET(sock2,&connections);
        if(sock2>maxfd)
        {
                maxfd=sock2;
        }
      }
      else /* for a connection socket, handle the connection */
      {

        rc = handle_connection(i);
        FD_CLR(i,&connections);
      }
    }
  }
  }
}
int handle_connection(int sock2)
{
  char filename[FILENAMESIZE+1];
  int rc;
  int fd;
  struct stat filestat;
  char buf[BUFSIZE+1];
  char *headers;
  char *endheaders;
//  char *bptr;
  int datalen=0;
  char *ok_response_f = "HTTP/1.0 200 OK\r\n"\
                      "Content-type: text/plain\r\n"\
                      "Content-length: %d \r\n\r\n";
  char ok_response[100];
  char *notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"\
                         "Content-type: text/html\r\n\r\n"\
                         "<html><body bgColor=black text=white>\n"\
                         "<h2>404 FILE NOT FOUND</h2>\n"\
                         "</body></html>\n";
  bool ok=true;
std::string buf_str;
endheaders = "\r\n\r\n";
  /* first read loop -- get request and headers*/
  memset(buf, 0, sizeof(buf));
  while((rc=minet_read(sock2,buf,BUFSIZE))>0);//-----------------------???????
  {
        buf[rc]='\0';
        buf_str=buf;
        size_t pos;
        if(( pos=buf_str.find(endheaders))!=std::string::npos)
        {

                std::string temp;
                temp=buf_str.substr(0,pos);
                headers= new char[temp.size()];
                strcpy(headers,temp.c_str());
                break;
        }
  }
  if(rc<0)
  {
        minet_perror("can't read request!\n");
        minet_close(sock2);
        return -1;
  }

  /* parse request to get file name */
  /* Assumption: this is a GET request and filename contains no spaces*/
 std::string name;
  if(( buf_str.substr(0,buf_str.find(" ")))=="GET")
  {
        name=buf_str.substr(buf_str.find(" ")+1);
        size_t pos2=name.find(" ");
        name=name.substr(0,pos2);
  }

    /* try opening the file */
strcpy(filename,name.c_str());
  fd=open(filename,O_RDWR);
  if(fd<0)
  {
        ok=false;
        minet_perror("Can’t open file.\n");
        exit(-1);
 }
  else
    ok=true;

  /* send response */
  if (ok)
  {
    /* send headers */
        rc = stat(filename,&filestat);
        if(rc<0)
        {
          minet_perror("can't get file stat!\n");
          minet_close(sock2);
          return -1;
        }
  sprintf(ok_response,ok_response_f,filestat.st_size);
  rc=writenbytes(sock2,ok_response,strlen(ok_response));
  if(rc<0)
  {
        minet_perror("can't send response!\n");
        minet_close(sock2);
        return -1;
  }
  datalen=0;

    /* send file */
 while((rc = minet_read(fd,buf,BUFSIZE))!=0)
  {
        if(datalen==filestat.st_size)
                break;
        else if(rc<0)
        {
                minet_perror("file read error!");
                ok=false;
                break;
        }
        else
        {
                rc=writenbytes(sock2,buf,rc);
                datalen=datalen+rc;
                if(rc<0)
                {
                        minet_perror("can't send file!\n");
                        minet_close(sock2);
                        return -1;
                }
        }
  }

  }
  else  // send error response
  {
        rc = writenbytes(sock2,notok_response,strlen(notok_response));
        if(rc<0)
        {
                minet_perror("can't send error response.\n");
                minet_close(sock2);
                return -1;
        }

  }

  /* close socket and free space */
  close(fd);
  minet_close(sock2);

  if (ok)
    return 0;
  else
    return -1;
}

int readnbytes(int fd,char *buf,int size)
{
  int rc = 0;
  int totalread = 0;
  while ((rc = minet_read(fd,buf+totalread,size-totalread)) > 0)
    totalread += rc;

  if (rc < 0)
  {
    return -1;
  }
  else
    return totalread;
}

int writenbytes(int fd,char *str,int size)
{
  int rc = 0;
  int totalwritten =0;
  while ((rc = minet_write(fd,str+totalwritten,size-totalwritten)) > 0)
    totalwritten += rc;

  if (rc < 0)
    return -1;
  else
    return totalwritten;
}



