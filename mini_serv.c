#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

#define MAX_CLIENT 1024
#define BUFF_SIZE 1025


typedef struct s_client
{
  int id;
  int fd;
  char *msg;
} t_client;

typedef struct s_server
{
  int sockfd;
  int maxfd;
  char w_buff[42], r_buff[BUFF_SIZE];
  t_client *clients[MAX_CLIENT];
  fd_set fds, w_fds, r_fds;
} t_server;

t_server server;

int extract_message(char **buf, char **msg)
{
  char    *newbuf;
  int     i;
  *msg = 0;
  if (*buf == 0)
          return (0);
  i = 0;
  while ((*buf)[i])
  {
    if ((*buf)[i] == '\n')
    {
      newbuf = malloc(sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
      if (newbuf == 0)
              return (-1);
      strcpy(newbuf, *buf + i + 1);
      *msg = *buf;
      (*msg)[i + 1] = 0;
      *buf = newbuf;
      return (1);
    }
    i++;
  }
  return (0);
}

char *str_join(char *buf, char *add)
{
  char    *newbuf;
  int             len;

  if (buf == 0)
          len = 0;
  else
          len = strlen(buf);
  newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
  if (newbuf == 0)
          return (0);
  newbuf[0] = 0;
  if (buf != 0)
          strcat(newbuf, buf);
  free(buf);
  strcat(newbuf, add);
  return (newbuf);
}

int get_port(int argc, char **argv) {
  if (argc != 2) {
    write(2, "Wrong number of arguments\n", 26);
    exit(1);
  }
  return atoi(argv[1]);
}

void fatal() {
  write(2, "Fatal error\n", 12);
  exit(1);
}

int add_client(int sockfd) {
  static int client_id = 0;
  t_client *elm = malloc(sizeof(t_client) * 1);
  elm->fd = sockfd;
  elm->id = client_id++;
  elm->msg = NULL;
  for (int i = 0; i < MAX_CLIENT; i++) {
    if (server.clients[i] == NULL) {
      server.clients[i] = elm;
      break;
    }
  }
  FD_SET(sockfd, &server.fds);
  server.maxfd = ++sockfd >= server.maxfd ? sockfd : server.maxfd;
  return elm->id;
}

int delete_client(int fd) {
  int id = -1;
  for (int i = 0; i < MAX_CLIENT; i++) {
    if (server.clients[i] == 0x00)
      continue;
    if (server.clients[i]->fd == fd) {
      id = server.clients[i]->id;
      close(server.clients[i]->fd);
      free(server.clients[i]->msg);
      free(server.clients[i]);
      server.clients[i] = NULL;
      FD_CLR(fd, &server.fds);
      break;
    }
  }
  return id;
}

void notify(char *msg, int except) {
  for (int fd = 0; fd < server.maxfd; fd++) {
    if (FD_ISSET(fd, &server.w_fds) && fd != except)
      send(fd, msg, strlen(msg), 0x00);
  }
}

void broad_cast(int execpt) {
  for (int i =0; i<MAX_CLIENT;i++) {
    if (server.clients[i] == NULL)
      continue;
    if (server.clients[i]->fd == execpt) {
      char *msg = NULL;
      server.clients[i]->msg = str_join(server.clients[i]->msg, server.r_buff);
      while (extract_message(&server.clients[i]->msg, &msg)) {
        sprintf(server.w_buff, "client %d: ", server.clients[i]->id);
        notify(server.w_buff, execpt);
        notify(msg, execpt);
        free(msg);
      }
      break;
    }
  }
}


int init_socket(int port) {

  int sockfd;
  struct sockaddr_in servaddr;

  // socket create and verification
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
    fatal();
  bzero(&servaddr, sizeof(servaddr));
  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
  servaddr.sin_port = htons(port);
  // Binding newly created socket to given IP and verification
  if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))
    fatal();
  if (listen(sockfd, 10) != 0)
    fatal();
  return sockfd;
}

void init_server(int sockfd) {
  server.sockfd = sockfd;
  server.maxfd = sockfd + 1;
  FD_ZERO(&server.fds);
  FD_SET(server.sockfd, &server.fds);
  for (int i = 0; i < MAX_CLIENT; i++)
    server.clients[i] = 0x00;
}


int main(int argc, char **argv) {

  int sockfd = init_socket(get_port(argc, argv));
  init_server(sockfd);

  for (;;) {
    server.r_fds = server.w_fds = server.fds;
    int set_len = select(server.maxfd, &server.r_fds, &server.w_fds, 0x00, 0x00);
    if (set_len < 0)
      fatal();
    for (int fd = 0; fd < server.maxfd; fd++) {
      //
      if (FD_ISSET(fd, &server.r_fds)) {
        // check if new connection
        if (fd == server.sockfd) {
          // accept new connection
          int connfd = accept(fd, 0x00, 0x00);
          if (connfd < 0)
            fatal();
          // add client && notify all clients
            int id = add_client(connfd);
            sprintf(server.w_buff, "server: client %d just arrived\n", id);
            notify(server.w_buff, fd);
        } else {
          // client is ready to read
          // read data from client
          int ret = recv(fd, server.r_buff, BUFF_SIZE, 0);
          // check if connetiions closed
          if (ret <= 0) {
            // delete client
            int id = delete_client(fd);
            // notify the rest
            sprintf(server.w_buff, "server: client %d just left\n", id);
            notify(server.w_buff, fd);
          } else {
            server.r_buff[ret] = 0x00;
            broad_cast(fd);
          }
        }
        if (--set_len <= 0)
          break;
      }
    }
  }
}
