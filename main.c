#include "mini_webserv.h"

int main (int argc, char *argv[])
{
	int port = 0;

	if (argc != 2)
		ft_err_exit("Error, check number of arguments\n", 0, NULL);
	port = atoi(argv[1]);
	if (!port)
		ft_err_exit("Error, port number incorrect.\n", 0, NULL);
	if (init_server(port))
		return (1);
	return (0);
}

