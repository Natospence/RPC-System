
#include "rpcdefs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

//Defined in separate file linked against
int entry(int argc, char** argv);

int sock;

int main(int argc, char** argv)
{

	char write_buffer[1024] = {0};

	char* host;
	char* port_string;
	int port;

	//Ensure that we have at least 2 arguments
	if(argc < 3)
	{
		sprintf(write_buffer, "Usage: %s <hostname> <portnumber> [args]\n", argv[0]);
		write(2, write_buffer, strlen(write_buffer));
		return(-1);
	}

	host = argv[1];
	port_string = argv[2];
	port = atoi(port_string);

	//print the hostname
	sprintf(write_buffer, "Hostname: %s\n", host);	
		write(1, write_buffer, strlen(write_buffer));

	//print the portnumber
	sprintf(write_buffer, "Port number: %s\n", port_string);	
		write(1, write_buffer, strlen(write_buffer));

	int new_arg_c = argc - 2;

	//If arguments weren't passed, ensure a positive array size for the name
	if(new_arg_c < 1) 
		new_arg_c = 1;

	//Make the new argument array
	char* new_arg_array[new_arg_c+1];

	//Set the name argument
	new_arg_array[0] = argv[0];

	//Transfer all the arguments after the second
	for(int i = 1; i < new_arg_c; i++)
	{
		new_arg_array[i] = argv[i+2];
	}

	//Null terminate the new argument list
	new_arg_array[new_arg_c] = 0;




	//Establish the socket
	struct sockaddr_in remote;
	struct hostent* h;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	bzero((char*) &remote, sizeof(remote));
	remote.sin_family = (short) AF_INET;

	h = gethostbyname(host);
	if(h <= 0)
	{
		sprintf(write_buffer, "Invalid hostname. Exiting...\n");
		write(2, write_buffer, strlen(write_buffer));
		exit(-1);
	}
	bcopy((char*)h->h_addr, (char*)&remote.sin_addr, h->h_length);
	remote.sin_port = htons(port);

	connect(sock, (struct sockaddr*) &remote, sizeof(remote));


	//Call the entry function, passing appropriate arguments
	return entry(new_arg_c, new_arg_array);


}

//RPC jacket function to make the "Open" syscall
int rp_open(const char *pathname, int flags, int mode)
{

	//Build the size of the buffer
	int buf_length = 0;
	
	//Size of opcode char
	buf_length += 1;

	//Size of flags
	buf_length += sizeof(flags);
	
	//Size of mode
	buf_length += sizeof(mode);
	
	//Size of pathname and termination
	buf_length += strlen(pathname) + 1;

	//Create the message buffer
	char* message = malloc(buf_length);

	int buf_ptr = 0;

	//Set the opcode
	message[buf_ptr] = open_call; 
	buf_ptr++;

	//Set the flags
	bcopy(&flags, message + buf_ptr, sizeof(flags));
	buf_ptr += sizeof(flags);

	//Set the mode
	bcopy(&mode, message + buf_ptr, sizeof(mode));
	buf_ptr += sizeof(mode);

	//Set the pathname
	strcpy(&message[buf_ptr], pathname);
	buf_ptr += strlen(pathname)+1;

	int err_ret_val = write(sock, message, buf_length);
	if(err_ret_val < 0)
	{
		perror("Write error");
	}

	//Buffer for received message
	char return_message[1024] = {0};

	err_ret_val = read(sock, return_message, 1024);
	if(err_ret_val < 0)
	{
		perror("Read socket");
	}

	//Return value from the function
	int retval = 0;

	buf_ptr = 0;

	//Grab the return value
	bcopy(return_message + buf_ptr, &retval, sizeof(retval));
	buf_ptr += sizeof(retval);

	//Grab errno
	bcopy(return_message + buf_ptr, &errno, sizeof(errno));
	buf_ptr += sizeof(errno);

	free(message);

	return retval;
	

}

//RPC jacket function to make the "Close" syscall
int rp_close(int fd)
{

	//Build the size of the buffer
	int buf_length = 0;
	
	//Size of opcode char
	buf_length += 1;

	//Size of fd
	buf_length += sizeof(fd);

	char* message = malloc(buf_length);

	int buf_ptr = 0;

	//Write the opcode
	message[buf_ptr] = close_call;
	buf_ptr++;

	//Write the fd
	bcopy(&fd, &message[buf_ptr], sizeof(fd));
	buf_ptr += sizeof(fd);


	int err_ret_val = write(sock, message, buf_length);
	if(err_ret_val < 0)
		perror("Write");

	char return_message[1024] = {0};

	err_ret_val = read(sock, return_message, 1024);
	if(err_ret_val < 0)
		perror("Socket Read");

	buf_ptr = 0;

	int retval = 0;

	//Grab the return value
	bcopy(return_message + buf_ptr, &retval, sizeof(retval));
	buf_ptr += sizeof(retval);

	//Grab errno
	bcopy(return_message + buf_ptr, &errno, sizeof(errno));
	buf_ptr += sizeof(errno);

	free(message);
	
	return retval;

}

//RPC jacket function to make the "Read" syscall
int rp_read(int fd, void* buf, int count)
{

	//Build the size of the buffer
	int buf_length = 0;
	
	//Size of opcode char
	buf_length++;

	//Size of fd
	buf_length += sizeof(fd);

	//Size of count
	buf_length += sizeof(count);

	char* message = malloc(buf_length);

	int buf_ptr = 0;

	//Write the opcode
	message[buf_ptr] = read_call;
	buf_ptr++;

	//Write the fd
	bcopy(&fd, message + buf_ptr, sizeof(fd));
	buf_ptr += sizeof(fd);
	
	//Write the count
	bcopy(&count, message + buf_ptr, sizeof(count));
	buf_ptr += sizeof(count);

	int err_ret_val = write(sock, message, buf_length);
	if(err_ret_val < 0)
		perror("Socket send");

	//Make the return message buffer wide enough for two ints and the message
	char* return_message = malloc(8 + count);
	bzero(return_message, 8 + count);

	err_ret_val = read(sock, return_message, count + 8);

	buf_ptr = 0;
	int retval = 0;

	//Grab the return value
	bcopy(return_message + buf_ptr, &retval, sizeof(retval));
	buf_ptr += sizeof(retval);

	//Grab errno
	bcopy(return_message + buf_ptr, &errno, sizeof(errno));
	buf_ptr += sizeof(errno);

	//Get the data from the message
	if(retval)
	{
	bcopy(return_message + buf_ptr, buf, retval);
	buf_ptr += retval;
	}
	
	free(return_message);
	free(message);
	
	return retval;
}

//RPC jacket function to make the "Write" syscall
int rp_write(int fd, void* buf, int count)
{

	//Build the size of the buffer
	int buf_length = 0;
	
	//Size of opcode char
	buf_length++;

	//Size of fd argument
	buf_length += sizeof(fd);

	//Size of count argument
	buf_length += sizeof(count);

	//Size of buf data to be sent
	buf_length += count;

	char* message = malloc(buf_length);
	bzero(message, buf_length);

	int buf_ptr = 0;

	//Write the opcode
	message[buf_ptr] = write_call;
	buf_ptr++;

	//Write the fd
	bcopy(&fd, message + buf_ptr, sizeof(fd));
	buf_ptr += sizeof(fd);
	
	//Write the count
	bcopy(&count, message + buf_ptr, sizeof(count));
	buf_ptr += sizeof(count);

	//Write the text
	bcopy(buf, message + buf_ptr, count);
	buf_ptr += count;


	int err_ret_val = write(sock, message, buf_length);
	if(err_ret_val < 0)
		perror("Socket send");

	char return_message[1024] = {0};

	err_ret_val = read(sock, return_message, 1024);
	if(err_ret_val < 0)
		perror("Socket receive");

	buf_ptr = 0;
	int retval = 0;

	//Grab the return value
	bcopy(return_message + buf_ptr, &retval, sizeof(retval));
	buf_ptr += sizeof(retval);

	//Grab errno
	bcopy(return_message + buf_ptr, &errno, sizeof(errno));
	buf_ptr += sizeof(errno);
	
	free(message);
	
	return retval;
}

//RPC jacket function to make the "Lseek" syscall
int rp_seek(int fd, int offset, int whence)
{

	//Build the size of the buffer
	int buf_length = 0;
	
	//Size of opcode char
	buf_length += 1;

	//Size of fd
	buf_length += sizeof(fd);
	
	//Size of offset
	buf_length += sizeof(offset);

	//Size of whence
	buf_length += sizeof(whence);

	char* message = malloc(buf_length);

	int buf_ptr = 0;

	//Write the opcode
	message[buf_ptr] = seek_call;
	buf_ptr++;

	//Write the fd
	bcopy(&fd, message + buf_ptr, sizeof(fd));
	buf_ptr += sizeof(fd);

	//Write the offset
	bcopy(&offset, message + buf_ptr, sizeof(offset));
	buf_ptr += sizeof(offset);
	
	//Write the whence
	bcopy(&whence, message + buf_ptr, sizeof(whence));
	buf_ptr += sizeof(whence);

	int err_ret_val = write(sock, message, buf_length);
	if(err_ret_val < 0)
		perror("Socket send");

	char return_message[1024];

	err_ret_val = read(sock, return_message, 1024);
	if(err_ret_val < 0)
		perror("Socket read");

	int retval = 0;
	buf_ptr = 0;

	//Grab the return value
	bcopy(return_message + buf_ptr, &retval, sizeof(retval));
	buf_ptr += sizeof(retval);

	//Grab errno
	bcopy(return_message + buf_ptr, &errno, sizeof(errno));
	buf_ptr += sizeof(errno);

	free(message);
	
	return retval;
}

//RPC Jacket function to perform the checksum calculation
int rp_checksum(int fd)
{

	//Build the size of the buffer
	int buf_length = 0;
	
	//Size of opcode char
	buf_length++;

	//Size of fd
	buf_length += sizeof(fd);

	char* message = malloc(buf_length);

	int buf_ptr = 0;

	//Write the opcode
	message[buf_ptr] = checksum_call;
	buf_ptr++;

	//Write the fd
	bcopy(&fd, message + buf_ptr, sizeof(fd));
	buf_ptr += sizeof(fd);

	int err_ret_val = write(sock, message, buf_length);
	if(err_ret_val < 0)
		perror("Socket send");

	char return_message[1024];

	err_ret_val = read(sock, return_message, 1024);
	if(err_ret_val < 0)
		perror("Socket read");

	int retval = 0;
	buf_ptr = 0;

	//Grab the return value
	bcopy(return_message + buf_ptr, &retval, sizeof(retval));
	buf_ptr += sizeof(retval);

	//Grab errno
	bcopy(return_message + buf_ptr, &errno, sizeof(errno));
	buf_ptr += sizeof(errno);

	free(message);
	
	return retval;
}
