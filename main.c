#include "mini_webserv.h"

volatile sig_atomic_t g_signalnum = 0;

int main(int argc, char *argv[])
{
	int port = 0;

	if (argc != 2)
		ft_err_exit("Error, check number of arguments\n", 0, NULL);
	port = atoi(argv[1]);
	if (!port)
		ft_err_exit("Error, port number incorrect.\n", 0, NULL);
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGPIPE, SIG_IGN);
	if (init_server(port))
		return (1);
	return (0);
}
