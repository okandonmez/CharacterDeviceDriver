#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MASTERMIND_NEWGAME _IOR('c','d',int32_t*)

int main()
{
        int fd;
        int32_t mastermind_new_to_guess;

        printf("\nOpening Driver\n");
        fd = open("/dev/mastermind", O_RDWR);
        if(fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
        }

        printf("Enter the Value to send\n");
        scanf("%d",&mastermind_new_to_guess);
        printf("Writing Value to Driver\n");
        ioctl(fd, MASTERMIND_NEWGAME, (int32_t*) &mastermind_new_to_guess);

        printf("Closing Driver\n");
        close(fd);
}
