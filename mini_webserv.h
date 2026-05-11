#ifndef MINI_WEBSERV
#define MINI_WEBSERV

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>

//types
typedef struct client_s{
	int	fd;
	int	id;
	char	in[1024];
	char	out[1024];
}	client_t;

//functions
int	init_server(int port);
void	ft_err_exit(char err_msg[], int socket_fd);


#endif
