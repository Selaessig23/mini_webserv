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

/**
 * @brief fuction that tries to imitate the memmove function
 * it copies n bytes from memory area src to memory area dest, mem area may
 * overlap
 *
 * in contrast to the libc-like version, it is protected against wrong NULL-pointers
 * for dest and src
 *
 * @return a pointer to dest
 */
void *ft_memmove(void *dst, const void *src, size_t n)
{
	char *dst_cpy;
	const char *src_cpy;

	if (n == 0)
		return (dst);
	if (!dst_cpy || !src_cpy)
		return (NULL);
	dst_cpy = dst;
	src_cpy = src;
	while (n > 0)
	{
		dst_cpy[n - 1] = src_cpy[n - 1];
		n -= 1;
	}
	return (dst);
}

/**
 * @brief helper function to close all fds in the pollfd array
 */
void ft_close_poll_fds(struct pollfd *fds, int len)
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

/**
 * @brief extracts one complete line/message from a buffered stream.
 *
 * This helper splits @buf at the first '\n', returns the extracted part in
 * @msg, and keeps the remaining bytes in @buf. In mini_irc_webserv, this is
 * useful for handling TCP input incrementally, where a client message may
 * arrive in fragments or several commands may arrive in a single read.
 * The function lets the server process one IRC-style line at a time while
 * preserving any unfinished data for the next poll/read cycle.
 *
 * @param buf pointer to the buffer containing the incoming data stream. This buffer is modified in-place to remove the extracted message.
 * @param msg pointer to a char pointer where the extracted message will be stored. The caller is responsible for freeing this memory after use.
 * @return 1 if a complete message was successfully extracted, 0 if no complete message was found (i.e., no '\n' in the
 */
int extract_message(char **buf, char **msg)
{
	char *newbuf;
	int i;

	*msg = 0;
	if (!*buf)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (!newbuf)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i += 1;
	}
	return (0);
}

/**
 * @brief extracts one complete line/message from a buffered stream.
 *
 * This helper splits @buf at the first '\n', returns the extracted part in
 * @msg, and keeps the remaining bytes in @buf. In mini_irc_webserv, this is
 * useful for handling TCP input incrementally, where a client message may
 * arrive in fragments or several commands may arrive in a single read.
 * The function lets the server process one IRC-style line at a time while
 * preserving any unfinished data for the next poll/read cycle.
 */
int ft_extract_message(char *buf, char *msg)
{
	int i;

	*msg = 0;
	if (!*buf)
		return (0);
	i = 0;
	while (buf[i])
	{
		if (buf[i] == '\n')
		{
			ft_memmove(msg, buf, i + 1);
			msg[i + 1] = 0;
			return (1);
		}
		i += 1;
	}
	return (0);
}