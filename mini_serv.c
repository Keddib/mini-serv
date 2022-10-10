#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>


typedef struct s_client
{
  int id;
  int sock_fd;
  char *buff;
  struct s_client *next;

} t_client;

t_client *new_elem(int sockfd) {
  static int client_id = 0;
  t_client *elm = malloc(sizeof(t_client) * 1);
  elm->sock_fd = sockfd;
  elm->id = client_id++;
  elm->buff = 0x00;
  elm->next = NULL;
  return elm;
}

void addback_elem(t_client **head, t_client *elem) {

  if (*head == NULL) {
    *head = elem;
    return;
  }
  t_client *tmp = *head;
  while (tmp->next) {
    tmp = tmp->next;
  }
  tmp->next = elem;
}

t_client *get_client(t_client *list, int sockfd) {
  while (list->next) {
    if (list->sock_fd == sockfd)
      return list;
    list = list->next;
  }
  return NULL;
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
	char	*newbuf;
	int		len;

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

int main(int argc, char **argv) {

  int port = get_port(argc, argv);

	int sockfd  = init_socket(port);

  t_client *client_list = NULL;

  fd_set socket_set;
  FD_ZERO(&socket_set);
  FD_SET(sockfd, &socket_set);
  int maxfdp1 = sockfd + 1;

  for (;;) {

    // init fdset to listen to
    fd_set read_set = socket_set;
    fd_set write_set = socket_set;
    int set_len = select(maxfdp1, &read_set, &write_set, NULL, NULL);

    // accept new connections
    if (FD_ISSET(sockfd, &read_set)) {
      // accept new client
	    int connfd = accept(sockfd, 0, 0);
	    if (connfd < 0) {
        	write(2, "Fatal error\n", 13);
          exit(1);
      }
      printf("fd : %d\n", connfd);
      // add client to socket_set and clients collection with a new id
      FD_SET(connfd, &socket_set);
      t_client *new_client = new_elem(connfd);
      addback_elem(&client_list, new_client);
      printf("new client joinded : %d\n", new_client->id);
      maxfdp1 = connfd >= maxfdp1 ? connfd + 1 : maxfdp1;
    }

    // loop over clients and check if they are ready to read
    t_client *client = client_list;
    while (client) {
      // client ready to read
      if (FD_ISSET(client->sock_fd, &read_set)) {
        // read data
        // if ret == 0, connections closed
        printf("client ready to read %d\n", client->id);
      }
      client = client->next;
    }
    // loop over clients and check if they are ready to write
    client = client_list;
    while (client) {
      // client ready to read
      if (FD_ISSET(client->sock_fd, &write_set)) {
        // read data
        // if ret == 0, connections closed
        printf("client ready to write %d\n", client->id);
      }
      client = client->next;
    }
  }

}
