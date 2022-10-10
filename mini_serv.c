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
#define BUFF_SIZE 1024


typedef struct s_client
{
  int id;
  int sock_fd;
} t_client;

typedef struct s_data
{
  int sockfd;
  int maxc;
  int maxfdp1;
  t_client *client_list[MAX_CLIENT];
  fd_set socket_set;
} t_data;

t_client *new_elem(int sockfd) {
  static int client_id = 0;
  t_client *elm = malloc(sizeof(t_client) * 1);
  elm->sock_fd = sockfd;
  elm->id = client_id++;
  return elm;
}

void delete_client(t_client **client) {
  close((*client)->sock_fd);
  free(*client);
  *client = NULL;
}

int ft_strlen(char *msg) {
  int i = 0;
  while (*msg) {
    msg++;
    i++;
  }
  return i;
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = malloc(sizeof(*newbuf) * (ft_strlen(*buf + i + 1) + 1));
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
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = ft_strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + ft_strlen(add) + 1));
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
    write(2, "Wrong number of arguments\n", 27);
    exit(1);
  }
  return atoi(argv[1]);
}

int init_socket(int port) {

  int sockfd;
	struct sockaddr_in servaddr;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		write(2, "Fatal error\n", 13);
		exit(1);
	}
	bzero(&servaddr, sizeof(servaddr));
  // setsockopt(sockfd, SO_REUSEADDR, 1, 0, 0);
	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
		write(2, "Fatal error\n", 13);
		exit(1);
	}

	if (listen(sockfd, 10) != 0) {
		write(2, "Fatal error\n", 13);
		exit(1);
	}
  return sockfd;
}

int FD_IS_ANY_SET(fd_set const *fdset)
{
    static fd_set empty;     // initialized to 0 -> empty
    return memcmp(fdset, &empty, sizeof(fd_set)) != 0;
}

void broad_cast(t_data data, char *msg, int exept) {
  fd_set clients = data.socket_set;
  FD_CLR(data.sockfd, &clients);
  if (exept >= 0)
    FD_CLR(exept, &clients);
  if (!FD_IS_ANY_SET(&clients)) {
    return;
  }
  int set_len = select(data.maxfdp1, NULL, &clients, NULL, NULL);
  printf("set_len %d\n", set_len);
  for (int i = 0; i < data.maxc; i ++) {
    if (data.client_list[i] == NULL || data.client_list[i]->id == exept)
      continue;
    t_client *client = data.client_list[i];
    if (FD_ISSET(client->sock_fd, &clients)) {
      printf("client %d is ready to Write\n",client->id);
      int ret = send(client->sock_fd, msg, ft_strlen(msg), 0);
      if (ret <= 0) {
        printf("can\'t send\n");
      }
      if (--set_len <= 0) // no more readable data
        break;
    }
  }
}



char *generate_message(char *buffer, int client_id) {
  char *msg = NULL;
  char *messages = NULL;
  int isMsg = 1;
  while (isMsg) {
    isMsg = extract_message(&buffer, &msg);
    if (isMsg == -1) {
      printf("malloc failed\n");
      exit(1);
    } else if (isMsg == 1) {
      char g_msg[BUFF_SIZE + 1];
      sprintf(g_msg, "client %d: %s", client_id, msg);
      free(msg);
      messages = str_join(messages, g_msg);
    }
  }
  return messages;
}


void init_data(t_data *data) {
  data->maxc = 0;
  data->maxfdp1 = 0;
  FD_ZERO(&(data->socket_set));
  FD_SET(data->sockfd, &(data->socket_set));
  data->maxfdp1 = data->sockfd + 1;
  for (int i = 0; i < MAX_CLIENT; i++)
    data->client_list[i] = NULL;
}


int main(int argc, char **argv) {

  t_data data;
	data.sockfd  = init_socket(get_port(argc, argv));
  init_data(&data);

  for (;;) {

    // init fdset to listen to
    fd_set read_set = data.socket_set;
    int set_len = select(data.maxfdp1, &read_set, NULL, NULL, NULL);
    // accept new connections
    if (FD_ISSET(data.sockfd, &read_set)) {
      // accept new client
	    int connfd = accept(data.sockfd, 0, 0);
	    if (connfd < 0) {
        	write(2, "Fatal error\n", 13);
          exit(1);
      }
      printf("fd : %d\n", connfd);
      // add client to socket_set and clients collection with a new id
      t_client *new_client = new_elem(connfd);
      if (!new_client) {
        // error
        printf("malloc failed\n");
        exit(-1);
      }
      printf("new client joinded : %d\n", new_client->id);
      char msg[BUFF_SIZE + 1];
      sprintf(msg, "server: client %d just arrived\n", new_client->id);
      broad_cast(data, msg, -1);
      // add client to clinet_list and socket_set
      FD_SET(connfd, &data.socket_set);
      int i = 0;
      for (i = 0; i < MAX_CLIENT; i++) {
        if (data.client_list[i] == NULL) {
          data.client_list[i] = new_client;
          break;
        }
      }
      data.maxc = i >= data.maxc ? i + 1 : data.maxc;
      data.maxfdp1 = connfd >= data.maxfdp1 ? connfd + 1 : data.maxfdp1;
    }

    // loop over clients and check if they are ready to read
    for (int i = 0; i < data.maxc; i ++) {
      // if client is ready to read
      if (data.client_list[i] == NULL)
        continue;
      t_client *client = data.client_list[i];
      if (FD_ISSET(client->sock_fd, &read_set)) {
        printf("client %d is ready to Read\n",client->id);
        char *msg = NULL;
        char *buffer = malloc(sizeof(char) * BUFF_SIZE + 1);
        if (!buffer) {
          printf("malloc failed\n");
          exit(-1);
        }
        int ret = recv(client->sock_fd, buffer, BUFF_SIZE + 1, 0);
        // ret == 0 -> the connection is closed
        if (ret <= 0) {
          // client left should be sent to all clients
          msg = malloc(sizeof(char) * BUFF_SIZE + 1);
          if (!msg) {
            printf("malloc failed\n");
            exit(1);
          }
          sprintf(msg, "server: client %d just left\n", client->id);
          broad_cast(data, msg, client->id);
          FD_CLR(client->sock_fd, &data.socket_set);
          delete_client(data.client_list + i);
          free(msg);
          free(buffer);
        } else {
          buffer[ret] = 0x00;
          buffer = generate_message(buffer, client->id);
          broad_cast(data, buffer, client->id);
          free(buffer);
          // printf("client %d : %s\n", client->id, buffer);
        }
        if (--set_len <= 0) // no more readable data
          break;
      }
    }
  }

}
