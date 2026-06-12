#include "mini_webserv.h"

/**
 * @brief signal handler function that sets the global variable g_signalnum to the received signal number
 * this variable is used in the poll loop to break the loop and exit the program in a clean way when a signal is received
 * the signal handler is set in main.c for the signals SIGINT, SIGTERM, SIGQUIT and SIGPIPE
 * (SIGPIPE is ignored since it can occur when trying to write to a closed socket)
 *
 * @param sig the signal number received
 */
void signal_handler(int sig)
{
	(void)sig;
	DEBUG_PRINT("Debug: signal handler called\n");
	g_signalnum = sig;
}

static void ft_close_poll_fds(struct pollfd *fds, int len)
{
	int i;

	i = 0;
	while (i < len)
	{
		if (fds[i].fd >= 0)
			close(fds[i].fd);
		i += 1;
	}
}

/**
 * @brief helper function provide a clean exit in case of an error
 * (1) it returns an error msg
 * (2) it closes the fds of all connected clients
 * (3) it closes the socket fd of the server itself
 */
void ft_err_exit(char err_msg[], int socket_fd, struct pollfd *fds)
{
	write(2, err_msg, strlen(err_msg));
	if (fds)
		ft_close_poll_fds(fds, MAX_CLIENTS + 1);
	if (socket_fd >= 0)
		close(socket_fd);
	exit(1);
}
