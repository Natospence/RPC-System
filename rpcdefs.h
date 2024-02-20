#ifndef RPCTEST_H
#define RPCTEST_H

#define	open_call	1
#define close_call	2
#define read_call	3
#define write_call	4
#define seek_call	5
#define checksum_call	6

int rp_open(const char *pathname, int flags, int mode);
int rp_close(int fd);
int rp_read(int fd, void* buf, int count);
int rp_write(int fd, void* buf, int count);
int rp_seek(int fd, int offset, int whence);
int rp_checksum(int fd);

#endif
