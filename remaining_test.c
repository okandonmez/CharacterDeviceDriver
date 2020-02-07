#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MASTERMIND_REMAINING_GUESS _IOR('a','b',int32_t*)

int main()
{
        int fd;
        int32_t mastermind_remaining_guess;

        printf("\nOpening Driver\n");
        fd = open("/dev/mastermind", O_RDWR);
        if(fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
        }

        printf("Reading remaining number of guess from Driver\n");
        ioctl(fd, MASTERMIND_REMAINING_GUESS, (int32_t*) &mastermind_remaining_guess);
        printf("Remaning mastermind_remaining_guess number of guess is %d\n", mastermind_remaining_guess);

        printf("Closing Driver\n");
        close(fd);
}
