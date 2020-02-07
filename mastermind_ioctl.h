#ifndef __MASTERMIND_H
#define __MASTERMIND_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

#define MASTERMIND_SET_GUESS_LIMIT	_IO('k',   2)
#define MASTERMIND_REMAINING_GUESS _IOR('a','b',int32_t*)
#define MASTERMIND_NEWGAME _IOR('c','d',int32_t*)
#define MASTERMIND_ENDGAME _IOW('a', 'a', int32_t*)
#endif
