#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <asm/switch_to.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>

#include "mastermind_ioctl.h"

#define MASTERMIND_MAJOR 0
#define MASTERMIND_MINOR 0
#define MASTERMIND_NR_DEVS 4
#define MASTERMIND_PREDICTION_NUMBER 1000
#define MASTERMIND_GUESS_LIMIT 20

int mastermind_major = MASTERMIND_MAJOR;
int mastermind_minor = MASTERMIND_MINOR;
int mastermind_nr_devs = MASTERMIND_NR_DEVS;
int mastermind_prediction_number = MASTERMIND_PREDICTION_NUMBER;
int mastermind_guess_limit = MASTERMIND_GUESS_LIMIT;
char *number_to_guess= "0000";

int32_t mastermind_rem_guess = MASTERMIND_GUESS_LIMIT;
int32_t value = 0;

module_param(mastermind_major, int, S_IRUGO);
module_param(mastermind_minor, int, S_IRUGO);
module_param(mastermind_nr_devs, int, S_IRUGO);
module_param(mastermind_prediction_number, int, S_IRUGO);
module_param(mastermind_guess_limit, int, S_IRUGO);
module_param(number_to_guess,charp, 0);

MODULE_AUTHOR("Ihsan SOYDEMIR, Okan DONMEZ, Hasan Huseyin KACMAZ");
MODULE_LICENSE("Dual BSD/GPL");

struct prediction
{
    char *prediction;
    int len;
};

struct mastermind_dev
{
    struct prediction **data;
    int prediction_number;
    unsigned long size;
    struct semaphore sem;
    struct cdev cdev;
};

struct mastermind_dev *mastermind_devices;

int mastermind_trim(struct mastermind_dev *dev)
{
    int i;
    if (dev->data) {
        for (i = 0; i < dev->prediction_number; i++) {
            if (dev->data[i])
            kfree(dev->data[i]);
        }
        kfree(dev->data);
    }
    dev->data = NULL;
    dev->prediction_number = 0;
    dev->size = mastermind_prediction_number;
    return 0;
}

void delete_guesses(struct file *filp)
{
    struct mastermind_dev *dev = filp->private_data;
    struct prediction *pred;
    int ret = mastermind_trim(dev);

    dev -> prediction_number = 0;
    mastermind_guess_limit = MASTERMIND_GUESS_LIMIT;
    mastermind_rem_guess = MASTERMIND_GUESS_LIMIT;
}

void start_new_game(struct file *filp)
{
    delete_guesses(filp);
}

int mastermind_open(struct inode *inode, struct file *filp)
{
    struct mastermind_dev *dev;

    dev = container_of(inode->i_cdev, struct mastermind_dev, cdev);
    filp->private_data = dev;

    if ((filp->f_flags & O_APPEND) == 0 && (filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
        mastermind_trim(dev);
        up(&dev->sem);
    }
    return 0;
}

int mastermind_release(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t mastermind_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos)
{
    struct mastermind_dev *dev = filp->private_data;
    struct prediction *pred;
    int prediction_pos = (long)*f_pos;
    ssize_t retval = 0;
    int pos, line = 0, buf_pos = 0, prev = 0;

    char *local_buffer;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    if (*f_pos >= dev->prediction_number)
        goto out;

    if (dev->data == NULL || !dev->data[prediction_pos])
        goto out;

    local_buffer = kmalloc((count + 50) * sizeof(char), GFP_KERNEL);
    memset(local_buffer, 0, (count + 50) * sizeof(char));

    for(pos = prediction_pos; pos < dev->prediction_number; pos++) {

        pred = dev->data[pos];
        prev = buf_pos;
        line = -1;
        while (pred->prediction[++line])
            local_buffer[buf_pos++] = pred->prediction[line];

        if (buf_pos > count) {
            buf_pos = prev;
            break;
        }
    }
    if (copy_to_user(buf, local_buffer, buf_pos)) {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += buf_pos;
    retval = buf_pos;

  out:
    up(&dev->sem);
    return retval;
}

ssize_t mastermind_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
    struct mastermind_dev *dev = filp->private_data;
    ssize_t retval = -ENOMEM;

    int prediction_pos = dev->prediction_number;
    char *local_buffer;
    char *prediction;

    int pos = 0;
    int m_pos = 0;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    if (prediction_pos >= dev->size) {
        goto out;
    }

    if (!dev->data) {
        dev->data = kmalloc(dev->size * sizeof(struct prediction *), GFP_KERNEL);
        if (!dev->data)
            goto out;
        memset(dev->data, 0, dev->size * sizeof(struct prediction *));
    }
    if (!dev->data[prediction_pos]) {
        dev->data[prediction_pos] = kmalloc(sizeof(struct prediction), GFP_KERNEL);
        if (!dev->data[prediction_pos])
            goto out;
    }

    local_buffer = kmalloc(count * sizeof(char), GFP_KERNEL);

    if (copy_from_user(local_buffer, buf, count)) {
        retval = -EFAULT;
        goto out;
    }

    prediction = kmalloc(100 * sizeof(char), GFP_KERNEL);

    if(dev->prediction_number >= mastermind_guess_limit){
        retval = -EDQUOT;
          goto out;
    }

    pos--;
    while(++pos < count-1)
        prediction[m_pos++] = local_buffer[pos];

    char temp_number[4];
    int i = 0;
    for (i = 0; i < 4; i++){
      temp_number[i] = number_to_guess[i];
    }
    int j = 0;
    int m = 0;
    int n = 0;
    for(i = 0; i < 4; i++){
      for(j=0; j < 4; j++){
        if(local_buffer[i] == temp_number[j] && local_buffer[i]!= 'X' && temp_number[j] != 'X'){
            if(i == j)
              m++;
            else
              n++;
            local_buffer[i] = 'X';
            temp_number[j] = 'X';
        }
      }
    }
    int digit = 0;
    int temp_digit = 0;
    int coefficient = 1000;
    int temp_pred = dev->prediction_number+1;
    while( temp_pred != 0 ){
      temp_pred/=10;
      digit++;
    }
    temp_digit = digit;
    temp_pred = dev->prediction_number+1;
    prediction[m_pos++] = ' ';
    prediction[m_pos++] = m+ '0';
    prediction[m_pos++] = '+';
    prediction[m_pos++] = ' ';
    prediction[m_pos++] = n+ '0';
    prediction[m_pos++] = '-';
    prediction[m_pos++] = ' ';
    i = 4;
    while(i > 0){
		prediction[m_pos++] = temp_pred / coefficient + '0';
		if (temp_pred >= coefficient)
			temp_pred -= (temp_pred / coefficient) * coefficient;
		coefficient /= 10;
		i--;
	 }
    prediction[m_pos++] = '\n';
    prediction[m_pos] = 0;
    dev->data[prediction_pos]->prediction = prediction;
    dev->prediction_number++;

    (*f_pos)++;
    mastermind_rem_guess = mastermind_guess_limit - dev -> prediction_number;
    //printk(KERN_INFO "remaining = %d", mastermind_rem_guess);
    retval = count;
    kfree(local_buffer);
  out:
    up(&dev->sem);
    return retval;
}
long mastermind_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	int err = 0;
	int retval = 0;

  //printk(KERN_INFO "cmd: %d", cmd);
  //printk(KERN_INFO "mastermaind_remaining_guess: %d", MASTERMIND_REMAINING_GUESS);

	switch(cmd) {
		case MASTERMIND_SET_GUESS_LIMIT:
			if (! capable (CAP_SYS_ADMIN))
				return -EPERM;
			mastermind_guess_limit = arg;
			break;
    case MASTERMIND_REMAINING_GUESS:
      copy_to_user((int32_t*) arg, &mastermind_rem_guess, sizeof(mastermind_rem_guess));
      break;
    case MASTERMIND_ENDGAME:
  		delete_guesses(filp);
  		break;
    case MASTERMIND_NEWGAME:
      copy_from_user(&value ,(int32_t *) arg, sizeof(value));
      printk(KERN_INFO "Value = %d\n", value);
      int coefficient = 1000;
      int j = 4;
      int m_pos = 0;
      while(j > 0){
  		number_to_guess[m_pos++] = value / coefficient + '0';
  		if (value >= coefficient)
  			value -= (value / coefficient) * coefficient;
  		coefficient /= 10;
  		j--;
  	  }
      start_new_game(filp);
  		break;
		default:
			return -ENOTTY;
	}
	return retval;
}


loff_t mastermind_llseek(struct file *filp, loff_t off, int whence)
{
    struct mastermind_dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence) {
        case 0:
            newpos = off;
            break;

        case 1:
            newpos = filp->f_pos + off;
            break;

        case 2:
            newpos = dev->prediction_number + off;
            break;

        default:
            return -EINVAL;
    }
    if (newpos < 0)
        return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}


struct file_operations mastermind_fops = {
    .owner =    THIS_MODULE,
    .llseek =   mastermind_llseek,
    .read =     mastermind_read,
    .write =    mastermind_write,
    .unlocked_ioctl =  mastermind_ioctl,
    .open =     mastermind_open,
    .release =  mastermind_release,
};


void mastermind_cleanup_module(void)
{
    int i;
    dev_t devno = MKDEV(mastermind_major, mastermind_minor);

    if (mastermind_devices) {
        for (i = 0; i < mastermind_nr_devs; i++) {
            mastermind_trim(mastermind_devices + i);
            cdev_del(&mastermind_devices[i].cdev);
        }
    kfree(mastermind_devices);
    }

    unregister_chrdev_region(devno, mastermind_nr_devs);
}


int mastermind_init_module(void)
{
    int result, i;
    int err;
    dev_t devno = 0;
    struct mastermind_dev *dev;

    if (mastermind_major) {
        devno = MKDEV(mastermind_major, mastermind_minor);
        result = register_chrdev_region(devno, mastermind_nr_devs, "mastermind");
    } else {
        result = alloc_chrdev_region(&devno, mastermind_minor, mastermind_nr_devs,
                                     "mastermind");
        mastermind_major = MAJOR(devno);
    }
    if (result < 0) {
        printk(KERN_WARNING "mastermind: can't get major %d\n", mastermind_major);
        return result;
    }

    mastermind_devices = kmalloc(mastermind_nr_devs * sizeof(struct mastermind_dev),
                            GFP_KERNEL);
    if (!mastermind_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(mastermind_devices, 0, mastermind_nr_devs * sizeof(struct mastermind_dev));


    for (i = 0; i < mastermind_nr_devs; i++) {
        dev = &mastermind_devices[i];
        dev->size = mastermind_prediction_number;
        dev->prediction_number = 0;
        sema_init(&dev->sem,1);
        devno = MKDEV(mastermind_major, mastermind_minor + i);
        cdev_init(&dev->cdev, &mastermind_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &mastermind_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        if (err)
            printk(KERN_NOTICE "Error %d adding mastermind%d", err, i);
    }

    return 0;

  fail:
    mastermind_cleanup_module();
    return result;
}

module_init(mastermind_init_module);
module_exit(mastermind_cleanup_module);
