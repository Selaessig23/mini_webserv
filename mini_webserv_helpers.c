#include "mini_webserv.h"

/**
 * @brief helper function provide a clean exit in case of an error
 * (1) it returns an error msg
 * (2) it closes the fds of all connected clients
 * (3) it closes the socket fd of the server itself
 */
void	ft_err_exit(char err_msg[], int socket_fd, struct pollfd *fds)
{
	write (2, err_msg, strlen(err_msg));
	while (!fds->fd)
	{
		close(fds->fd);
		fds += 1;
	}
		

	if (socket_fd)
		close(socket_fd);
	exit (1);
}
