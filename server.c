#include <errno.h>
#include "rpcdefs.h"
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>


//Function to establish a connection with a client
int establish_socket();

int port;

int main()
{

	//Buffer to write to stdout
	char write_buffer[1024] = {0};

	//Socket FD for current connection
	int conn;

	//Loop for parent
	while(1)
	{
		
		//Establish a socket with the client
		conn = establish_socket();

		//Create a child process
		int cpid = fork();

		//Allow the child process to break out of the loop
		if(cpid == 0)
			break;
		
		//Close the socket for the parent
		close(conn);

	}

	//While the socket is active, continue
	while(1)
	{

		//Set the opcode to a no-op
		char opcode = -1;

		//Read the opcode
		int opcode_ret = read(conn, &opcode, 1);

		//If the pipe was empty and returns
		if(opcode_ret <= 0)
		{
			sprintf(write_buffer, "Socket %d closed.\n", port);
			write(1, write_buffer, strlen(write_buffer));
			exit(0);
		}

		//Variable for return value from the RPC syscall
		int result = 0;

		//Variables for fixed arguments from the RPC
		unsigned int arg1 = 0;
		unsigned int arg2 = 0;

		//Variable arguments
		char* variable_argument_buffer = 0;

		switch(opcode)
		{

		case open_call:

			//Set the flags
			read(conn, &arg1, sizeof(arg1));

			//Set the mode
			read(conn, &arg2, sizeof(arg2));

			//Buffer to read the directory name
			char dir_name[1024] = {0};

			//Read the directory name
			int err_ret_val = read(conn, dir_name, 1023);
			if(err_ret_val < 0)
			{
				sprintf(write_buffer, "Marshalling error: %s\n", strerror(errno));
				write(1, write_buffer, strlen(write_buffer));
				exit(-1);
			}

			//No need to copy the string, just pass it
			result = open(dir_name, arg1, arg2);


			break;

		case close_call:

			//Set the fd
			read(conn, &arg1, sizeof(arg1));
	
			result = close(arg1);

			break;

		case read_call:

			//Set the fd
			read(conn, &arg1, sizeof(arg1));

			//Set the count
			read(conn, &arg2, sizeof(arg2));

			//Create an appropriately sized read buffer
			variable_argument_buffer = malloc(arg2);
	
			result = read(arg1, variable_argument_buffer, arg2);

			break;

		case write_call:

			//Set the fd
			read(conn, &arg1, sizeof(arg1));

			//Set the count
			read(conn, &arg2, sizeof(arg2));

			//Make a buffer to store the write data
			variable_argument_buffer = malloc(arg2);
			bzero(variable_argument_buffer, arg2);

			//Read the write data into the buffer
			int read_ret_val = read(conn, variable_argument_buffer, arg2);
			if(read_ret_val < 0)
			{
				sprintf(write_buffer, "Marshalling error: %s\n", strerror(errno));
				write(2, write_buffer, strlen(write_buffer));
				exit(-1);
			}

			result = write(arg1, variable_argument_buffer, read_ret_val);

			break;

		case seek_call:

			//Set the fd
			read(conn, &arg1, sizeof(arg1));

			//Set the offset
			read(conn, &arg2, sizeof(arg2));

			int arg3;

			//Set the whence
			read(conn, &arg3, sizeof(arg3));

			result = lseek(arg1, arg2, arg3);

			break;

		case checksum_call:

			//Set the fd	
			read(conn, &arg1, sizeof(arg1));

			//Seek the beginning of the file
			lseek(arg1, 0, SEEK_SET);

			unsigned char checksum_result = 0;

			unsigned char checksum_buffer[4096];


				int read_val = read(arg1, checksum_buffer, 4096);

				while(read_val)
				{
					for(int i = 0; i < read_val; i++)
					{
						checksum_result = checksum_result ^ checksum_buffer[i];
					}
					read_val = read(arg1, checksum_buffer, 4096);
				}

				//If checksum failure, return -1
				if(read_val < 0)					
				{
					result = -1;
					break;
				}
				else
				{
				result = (int)checksum_result;
				}

			break;
	
		default:
			exit(0);
			break;

		}
	
		//Calculate the length of the message
		unsigned int out_len = 0;

		//Add the retval	
		out_len += sizeof(result);

		//Add errno
		out_len += sizeof(errno);

		//Add the length from a read
		if(opcode == read_call)
		{
			out_len += result;
		}

		//Create the buffer for the message
		char* output = malloc(out_len);
		bzero(output, out_len);

		unsigned int out_ptr = 0;

		//Write the result data to the message
		bcopy(&result, output + out_ptr, sizeof(result));
		out_ptr += sizeof(result);

		//Write the errno data to the message
		bcopy(&errno, output + out_ptr, sizeof(errno));
		out_ptr += sizeof(errno);

		//Write the data returned to the message
		if(opcode == read_call)
		{
			bcopy(variable_argument_buffer, output + out_ptr, result);
		}

		if(opcode == read_call || opcode == write_call)
		{
			free(variable_argument_buffer);
		}

		//Write the message back to the client
		write(conn, output,out_len);

		//Free the message sent to the client
		free(output);

	}

	return 0;


}


int establish_socket()
{

	int listener, conn, length; 
	struct sockaddr_in s1, s2;
	
	listener = socket( AF_INET, SOCK_STREAM, 0 );

	bzero((char *) &s1, sizeof(s1));
	s1.sin_family = (short) AF_INET;
	s1.sin_addr.s_addr = htonl(INADDR_ANY);
	s1.sin_port = htons(0); /* bind() will gimme unique port. */

	bind(listener, (struct sockaddr *)&s1, sizeof(s1));
	length = sizeof(s1);

	getsockname(listener, (struct sockaddr *)&s1, (socklen_t*) &length);

	port = ntohs(s1.sin_port);
	
	char msg[256];
	sprintf(msg, "Assigned port number %d\n", port);
	write(2, msg, strlen(msg));

	listen(listener,1);

	length = sizeof(s2);


	conn=accept(listener, (struct sockaddr *)&s2, (socklen_t*) &length);


	return conn;
}

