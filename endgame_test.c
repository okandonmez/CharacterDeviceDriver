#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "mastermind_ioctl.h"

int main(int argc, char *argv[]){

	int fd = open("/dev/mastermind", O_RDWR);
	int status = ioctl(fd, MASTERMIND_ENDGAME);

	if(status == -1)
		perror("Cannot change reading mode!");

	return status;
}
