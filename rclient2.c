#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rpcdefs.h"
#include <unistd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int entry(int argc, char** argv)
{

	char* infile = argv[1];
	char* outfile = argv[2];

	char write_buffer[1024] = {0};

	//Check for arguments
	if (argc < 3)
	{
		sprintf(write_buffer, "Usage: ./rclient2 <hostname> <port> [infile] [outfile]");
		write(2, write_buffer, strlen(write_buffer));
		exit(-1);
	}


	//Open infile at server
	int infile_fd = rp_open(infile, O_RDWR, 0);
	if(infile_fd < 0)
	{
		perror("Infile open");
		exit(-1);
	}

	int err_ret_val = rp_seek(infile_fd, 10, SEEK_SET);
	if(err_ret_val < 0)
	{
		perror("Seek");
		exit(-1);
	}
	
	//Open outfile on client
	int outfile_fd = open(outfile, O_RDWR | O_CREAT, 0600);
	if(outfile_fd < 0)
	{
		perror("Outfile open");
		exit(-1);
	}

	err_ret_val = 0;
	
	//Read from infile
	int read_retval = rp_read(infile_fd, write_buffer, 1024);
	while(read_retval > 0)
	{

		err_ret_val = write(outfile_fd, write_buffer, read_retval);

		read_retval = rp_read(infile_fd, write_buffer, 1024);

		if(read_retval < 0 || err_ret_val < 0)
		{
			perror("Read/Write Cycle");
			exit(-1);
		}
	}

	return 0;
}
