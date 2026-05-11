#include "mini_webserv.h"

void	ft_err_exit(char err_msg[])
{
	write (2, err_msg, strlen(err_msg));
	exit (1);
}
