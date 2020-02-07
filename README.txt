
sudo insmod mastermind.ko number_to_guess = "1234" mastermind_guess_limit=3


./setup	->	     This script will used for compile whole mastermind application.
			     Top of it, there is a Makefile file which execute with "make".
			     After make, it will delete "mastermind" executable file.
			     Then, 3rd line will initialize the module with ".ko" extension and give it some parameters like number_to_guess and
			guess_limit number.
		 	     After that, fourth line will delete /dev/mastermind.
			    In fifth line, it will create an /dev/mastermind device node as an character device driver with 250 limit.
			     Then, end of the file, give permission to /dev/mastermind 777, meaning that; it can be used for execute, read and write
			from all users.


$ echo "4231" >> /dev/mastermind ->    make a guess value to driver.

$ cat /dev/mastermind ->    read the evaluated value by the driver. It is within the format of:  "xxxx m+ n- abcd\n".

$ gcc endgame_test.c -o endgame ->   create and executable file which is called by endgame, from endgame_test.c file.

$ ./endgame ->   it will clear the guess story and guess count.

$ gcc start_new_test.c -o start_new_test ->  create and executable file which is called by start_new_test, from start_new_test.c file.

$ ./start_new_test -> it starts a new game with a new secret value as parameter.

$ gcc remaining_test.c -o remaining_test -> create and executable file which is called by remaining_test, from remaining_test.c file.

$ ./remaining_test -> it returns number of remaining guesses.