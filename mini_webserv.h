#ifndef MINI_WEBSERV
#define MINI_WEBSERV

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>

#define MAX_CLIENTS 10

// Define the debug print mode as a C preprocessor macro.
// If DEBUG is defined, the DEBUG_PRINT macro will print the provided message (variadic macro). Otherwise, it will do nothing.
#ifdef DEBUG
#define DEBUG_PRINT(...)                    \
	do                                      \
	{                                       \
		printf("DEBUG_INFO: " __VA_ARGS__); \
	} while (0)
#else
#define DEBUG_PRINT(...) \
	do                   \
	{                    \
	} while (0)
#endif

// types
typedef struct client_s
{
	int fd;
	int id;
	char in[1024];
	char out[1024];
} client_t;

// functions
int init_server(int port);
void ft_err_exit(char err_msg[], int socket_fd, struct pollfd *fds);

#endif
