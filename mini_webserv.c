#include "mini_webserv.h"
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

/**
 * @brief sends a message to all existing clients that informs about the new client
 * and a welcome message to the new client
 *
 * @param fds a pointer to the poll struct array
 * @param clients a pointer to the client struct array
 * @param len the lenght of the pollstruct array, the new client is already considered
 */
void ft_welcome(struct pollfd *fds, client_t *clients, int len, int client_id)
{
	// struct pollfd	*fds = *fds_pointer;
	// client_t	*clients = *clients_pointer;
	int i = 1;
	char info[1024];
	int new_client_index = len - 1;

	//bzero(info, sizeof(info));
	memset(info, 0, sizeof(info));
	if (sprintf(info, "A new member has entered the room, Id: %d\n", client_id) < 0)
		ft_err_exit("Error with sprintf", fds[0].fd, fds);
	while (i < (len)) // -1 since the newest client should not receive the message
	{
		strcat(clients[i - 1].out, info); // check for size of clients[i].out before
		fds[i].events |= POLLOUT;
		i += 1;
	}
	memset(info, 0, sizeof(info));
	if (sprintf(info, "Welcome new member, you got the id: %d\n", client_id) < 0)
		ft_err_exit("Error with sprintf", fds[0].fd, fds);
	strcpy(clients[new_client_index].out, info);
	// strcat(clients[i].out, inf); //check for size of clients[i].out before
	fds[len].events |= POLLOUT;
}

/**
 * @brief function to remove a client
 * from pollstruct array
 * and from client array
 * by using swap-removal:
 * it ovewrites the client to remove with the last client of the array.
 * If there is only one client, it changes the length of the array.
 *
 * @param fds struct pollfd array to remove the element aand close the fd
 * @param clients struct client array to remove the element from the array
 * @param len length of struct pollfd (== length of struct clients + 1)
 * @param  xthe index of the client
 * @param pollfd_index index of elemnt to remove from pollfd struct (&struct clients)
 */
static void ft_remove_client(struct pollfd *fds, client_t *clients, int *len, int pollfd_index)
{
	if (pollfd_index <= 0 || pollfd_index < *len)
		return ;
	close(fds[pollfd_index].fd);
	if (*len > 1)
	{
		fds[pollfd_index] = fds[*len - 1];
		// bzero(&fds[*len - 1], sizeof(fds[*len - 1]));
		memset(&fds[*len - 1], 0, sizeof(fds[*len - 1]));
		clients[pollfd_index - 1] = clients[*len - 2];
		// bzero(&clients[*len - 2], sizeof(clients[*len - 2]));
		memset(&clients[*len - 2], 0, sizeof(clients[*len - 2]));
	}
	*len -= 1;
}

/**
 * @brief this function runs the main poll loop,
 * it checks for events of the socket 
 * (fds[0]0) and the connected clients(fds[i] with i != 0)
 *
 * it manages the struct pollfd as well as the struct client_t (= client array)
 *
 * @param fds array of pollfd struct, requied for poll-fct
 * @param len length of the pollfd struct array
 */
static int ft_poll_loop(struct pollfd fds[10], int len, int client_id)
{
	int pollret = 0;
	int pollloop = 0;
	client_t clients[10];
	// struct pollfd	new_con;
	// client_t	new_client;

	memset(clients, 0, sizeof(clients));
	DEBUG_PRINT("Debug: poll loop starts here\n");
	while (1)
	{
		pollret = 0;
		pollloop = 0;
		pollret = poll(fds, len, -1);
		DEBUG_PRINT("Debug: poll event\n");
		if (pollret < 0)
		{
			if (errno == EINTR)
				continue;
			// ft_poll_loop(fds, len);
			// ft_err_exit("A signal ocurred before any requested event\n");
			else
				ft_err_exit("Error: Issue during poll loop\n", fds[0].fd, fds);
		}
		while (pollloop < len)
		{
			printf("Debug: poll event loop\n");
			if (fds[pollloop].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				DEBUG_PRINT("Debug: poll event error\n");
				if (pollloop == 0)
					ft_err_exit("Error with socket during poll-loop\n", fds[0].fd, fds);
				// handle farewll message to all existing clients?
				// if (clients[pollloop - 1])
				ft_remove_client(fds, clients, &len, pollloop);
				break;
			}
			else if (pollloop == 0 && fds[0].revents & POLLIN)
			{
				// handle accept
				DEBUG_PRINT("DEBUG: New incoming connection\n");
				fds[len].fd = accept(fds[0].fd, NULL, NULL);
				if (fds[len].fd < 0)
					ft_err_exit("Error with accept during poll loop\n", fds[0].fd, fds); //maybe specify the number of the client
				fds[len].events = POLLIN;
				fds[len].revents = 0;
				// fds[len] = new_con;
				// new_client.fd = new_con.fd;
				// new_client.id = len - 1;
				// memset(new_client.in, 0, sizeof(new_client.in));
				// memset(new_client.out, 0, sizeof(new_client.out));
				client_id += 1;
				clients[len - 1].fd = fds[len].fd;
				clients[len - 1].id = client_id;
				memset(clients[len - 1].in, 0, sizeof(clients[len - 1].in));
				memset(clients[len - 1].out, 0, sizeof(clients[len - 1].out));
				// clients[len - 1] = new_client;
				// handle welcome message to all existing clients
				ft_welcome(fds, clients, len, client_id);
				len += 1;
				break;
			}
			else if (pollloop != 0 && fds[pollloop].revents & POLLOUT)
			{
				DEBUG_PRINT("Debug: poll event pollout\n");
				// handle outputstream
				int send_ret = 0;
				size_t message_len = strlen(clients[pollloop - 1].out);

				send_ret = send(fds[pollloop].fd, clients[pollloop - 1].out, message_len, 0);
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
				memmove(clients[pollloop - 1].out,
						clients[pollloop - 1].out + send_ret,
						message_len - send_ret + 1);
				if (strlen(clients[pollloop - 1].out) == 0)
					fds[pollloop].events &= ~POLLOUT;
				// char	new_out[1024];
				// memset(new_out, 0, sizeof(new_out));
				// strcpy(new_out,&clients[pollloop].out[send_ret]);
				// memset(clients[pollloop].out, 0, sizeof(clients[pollloop].out));
				// strcpy(clients[pollloop].out, new_out);
			}
			else if (pollloop != 0 && fds[pollloop].revents & POLLIN)
			{
				DEBUG_PRINT("Debug: poll event pollin\n");
				// handle inputstream
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
				strcpy(clients[pollloop - 1].in, buf);
				// input of client never gets executed
			}
			pollloop += 1;
		}
	}
	return (0);
}

/**
 * @brief sets the first element of pollstruct array
 * to be the socket fd (= server)
 * and calls the poll-loop-fct
 */
void ft_prepare_run_polling(int socket_fd)
{
	struct pollfd fds[10];
	struct pollfd socket_poll;

	//bzero(fds, sizeof(fds));
	memset(&fds, 0, sizeof(fds));
	socket_poll.fd = socket_fd;
	socket_poll.events = POLLIN;
	socket_poll.revents = 0;
	fds[0] = socket_poll;
	ft_poll_loop(fds, 1, 0);
}

/**
 * @brief inits the server socket
 * (1) it creates a socket
	use htonl to transform IP
	use htons to transform PORT
 * (2) it assigns a name to this socket by using bind
 * (3) it starts listening for incoming connections of this socket
 * (4) it calls the function that prepares the pollfd struct and calls the pall loop
 *
 * It would also be possible to use getaddrinfo(NULL, port, struct addrinfo hints, &res) and use
 * res->ai_addr for binding
 */
int init_server(int port)
{
	int socket_fd = 0;
	// assign a sockaddr variable for bind()
	struct sockaddr_in my_addr;
	// struct in_addr		i_addr;
	// int			var;
	// socklen_t	addrlen = 0;

	//bzero(addr, sizeof(my_addr));
	memset(&my_addr, 0, sizeof(my_addr));
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		printf("socket: %d\n", socket_fd);
		ft_err_exit("Error. Problems with socket function.\n", socket_fd, NULL);
	}
	// if (inet_pton(AF_INET, "172.0.0.1", &i_addr.s_addr) <= 0) //converts IP address into an integer
	//	ft_err_exit("Error. Problems with network address of address family.\n");
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	// my_addr.sin_addr = i_addr;
	my_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(socket_fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
	{
		printf("errno: %s\n", strerror(errno));
		ft_err_exit("Error. Problems with bind functions.\n", socket_fd, NULL);
	}
	if (listen(socket_fd, 10) < 0)
		ft_err_exit("Error. Problems with listen function.\n", socket_fd, NULL);
	ft_prepare_run_polling(socket_fd);
	return (0);
}
