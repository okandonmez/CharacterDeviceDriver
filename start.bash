make
sudo rmmod mastermind
sudo insmod mastermind.ko number_to_guess="1256" mastermind_guess_limit=20
sudo rm /dev/mastermind
sudo mknod /dev/mastermind c $(grep mastermind /proc/devices | grep [0123456789] | cut -d' ' -f1) 0
sudo chmod 777 /dev/mastermind
