/bin/bash: line 1: q: command not found

int main (int argc, char *argv[])
{
	char err_msg_arg[] = "Error, check number of arguments\n";
	int port = 0;

	if (argc != 2)
		ft_err_exit("Error, check number of arguments\n");
	port = atoi(argv[1]);
	if (port)
		ft_err_exit("Error, port number incorrect.\n");
	if (init_server(port))
		return (1);
	return (0);
}

