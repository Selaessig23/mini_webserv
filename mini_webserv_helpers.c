#include "mini_webserv.h"

void	ft_err_exit(char err_msg[], int socket_fd)
{
	write (2, err_msg, strlen(err_msg));
	if (socket_fd)
		close(socket_fd);
	exit (1);
}
