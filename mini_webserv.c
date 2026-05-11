#include "mini_webserv.h"
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

/**
 * @brief send automatic welcome message to all existing clients
 * if a new client connects
 */
void	ft_welcome(struct pollfd *fds, client_t *clients, int len)
{
	//struct pollfd	*fds = *fds_pointer;
	//client_t	*clients = *clients_pointer;
	int		i = 0;
	char		info[1024];

	sprintf(info, "A new member has entered the room, Id: %d\n", len - 1);
	while (i < (len - 1)) // -1 since the newest client should not receive the message
	{
		strcat(clients[i].out, info); //check for size of clients[i].out before
		fds[i].events |= POLLOUT;
		i += 1;
	}
	sprintf(info, "Welcome new member, you got the id: %d\n", len - 1);
	strcpy(clients[i].out, info);
	fds[i].events |= POLLOUT;
}

/**
 * @brief function to remove a client
 * from pollstruct array
 * and from client array
 * by ovewriting the client to remove with the last client of the array. 
 * If there is only one client, it changes the length of the array
 */
static void	ft_remove_client(struct pollfd *fds, client_t *clients, int *len, int i)
{
	//struct pollfd	*fds = *fds_pointer;
	//client_t	*clients = *clients_pointer;

	if (*len > 1)
	{
		//fds[i] = NULL;
		fds[i] = fds[*len];
		//clients[i  - 0] = NULL;
		clients[i - 1] = clients[*len - 1];
	}
	*len -= 1;
}

/**
 * @brief runs the main poll loop
 */
static int	ft_poll_loop(struct pollfd fds[10], int len)
{
	int		pollret = 0;
	int		pollloop = 0;
	client_t	*clients = NULL;

	printf("Debug: poll loop starts here\n"); 
	while (1)
	{
		pollret = 0;
		pollloop = 0;
		pollret = poll(fds, len, -1);
		printf("Debug: poll event\n"); 
		if (pollret < 0)
		{
			if (errno == EINTR)
				continue;
				//ft_poll_loop(fds, len);
				//ft_err_exit("A signal ocurred before any requested event\n");
			else 
				ft_err_exit("Issue during poll loop\n");
		}
		while (pollloop < len)
		{
			printf("Debug: poll event loop\n"); 
			if (fds[pollloop].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				printf("Debug: poll event error\n"); 
				if (pollloop == 0)
					ft_err_exit("Error with socket during poll-loop\n");
				//handle farewll message to all existing clients?
				if (clients)
					ft_remove_client(fds, clients, &len, pollloop);
				break;
			}
			else if (pollloop == 0 && fds[0].revents & POLLIN)
			{
				//handle accept
				printf("DEBUG: New incoming connection");
				struct pollfd	new_con;
				new_con.fd = accept(fds[0].fd, NULL, NULL);
				new_con.events = POLLIN;
				new_con.revents = 0;
				fds[len] = new_con;
				client_t	new_client;
				new_client.fd = new_con.fd;
				new_client.id = len - 1;
				memset(new_client.in, 0, sizeof(new_client.in));
				memset(new_client.out, 0, sizeof(new_client.out));
				clients[len - 1] = new_client;
				len += 1;
				//handle welcome message to all existing clients
				ft_welcome(fds, clients, len);
				break;
			}
			else if (pollloop != 0 && fds[pollloop].revents & POLLOUT)
			{
				printf("Debug: poll event pollout\n"); 
				//handle outputstream
				int	send_ret = 0;

				send_ret = send(fds[pollloop].fd, clients[pollloop - 1].out, sizeof(clients[pollloop - 1].out), 0);
				if (send_ret < 0)
				{
					if (errno == ECONNRESET || errno == ENOTCONN || errno == EPIPE || errno == ETIMEDOUT)
						ft_remove_client(fds, clients, &len, pollloop);
					break;
				}
				if (send_ret == 0)
				{
					ft_remove_client(fds, clients, &len, pollloop);
					break;
				}
				if ((int)strlen(clients[pollloop - 1].out) == send_ret)
				       fds[pollloop].events &= ~POLLOUT;	
				char	new_out[1024];
				memset(new_out, 0, sizeof(new_out));
				strcpy(new_out,&clients[pollloop].out[send_ret]);
				memset(clients[pollloop].out, 0, sizeof(clients[pollloop].out));
				strcpy(clients[pollloop].out, new_out);
			}
			else if (pollloop != 0 && fds[pollloop].revents & POLLIN)
			{
				printf("Debug: poll event pollin\n"); 
				//handle inputstream
				char buf[1024];
				int recv_ret = 0;
				
				recv_ret = recv(fds[pollloop].fd, &buf, (sizeof(buf) - 1), 0);
				if (recv_ret < 0)
				{
					if (errno == ECONNRESET || errno == ENOTCONN)
						ft_remove_client(fds, clients, &len, pollloop);
					break;
				}
				if (recv_ret == 0)
				{
					ft_remove_client(fds, clients, &len, pollloop);
					break;
				}
				buf[recv_ret] = '\0';
				strcpy(clients[len - 1].in, buf);
				//input of client never gets executed
			}
			pollloop += 1;
		}
	}
	return (0);
}

/**
 * @brief sets the first element of pollstruct array 
 * to be the socket fd
 */
void ft_prepare_polling(int socket_fd)
{
	struct pollfd	fds[10];
	struct pollfd	socket_poll;

	socket_poll.fd = socket_fd;
	socket_poll.events = POLLIN;
	socket_poll.revents = 0;
	fds[0] = socket_poll;
	ft_poll_loop(fds, 1);
}


/**
 * @brief inits the server socket
 * (1) it creates a socket
	use htonl to transform IP
	use htons to transform PORT
 * (2) it assigns a name to this socket by using bind
 * (3) it starts listening for incoming connections of this socket
 *
 * It would also be possible to use getaddrinfo(NULL, port, struct addrinfo hints, &res) and use
 * res->ai_addr for binding
 */
int	init_server(int port)
{
	int 		socket_fd = 0;
	//assign a sockaddr variable for bind()
	struct	sockaddr_in	my_addr;
	// struct in_addr		i_addr;
	//int			var;
	//socklen_t	addrlen = 0;
	
	memset(&my_addr, 0, sizeof(my_addr));
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		printf("socket: %d\n", socket_fd);
		ft_err_exit("Error. Problems with socket function.\n");
	}
	//if (inet_pton(AF_INET, "172.0.0.1", &i_addr.s_addr) <= 0) //converts IP address into an integer
	//	ft_err_exit("Error. Problems with network address of address family.\n");
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	// my_addr.sin_addr = i_addr;
	my_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(socket_fd, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0)
	{
		printf("errno: %s\n", strerror(errno));
		ft_err_exit("Error. Problems with bind functions.\n");
	}
	if (listen(socket_fd, 10) < 0)
		ft_err_exit("Error. Problems with listen function.\n");
	ft_prepare_polling(socket_fd);
	//call poll-loop
	return (0);

}
