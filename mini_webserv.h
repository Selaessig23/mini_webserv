#ifndef MINI_WEBSERV
#define MINI_WEBSERV

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <poll.h>
#include <errno.h>

//types
typedef struct client_s{
	int	fd;
	int	id;
	char	*out;
}	client_t;

//functions
int	init_server(int port);
void	ft_err_exit(char err_msg[]);


#endif
