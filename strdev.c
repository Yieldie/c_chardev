#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/device.h>
#include<linux/cdev.h>
#include<linux/fs.h>
#include<linux/irq.h>
#include<linux/delay.h>
#include<linux/poll.h>
#include<linux/slab.h>

#define DEVICE_ID "strdev1"
#define INIT_MESSAGE "none"
#define MSG_PATTERN "Current message: %s\nReads: %d\nWrites: %d\n"
#define MAX_LEN 100
#define USR_LEN 50
#define SUCCESS 0

/* Prototypes */

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

/* Globals */

static int major;
static int read_count = 0;
static int write_count = 0;
static int total_length = 0;
static int patt_length;
static loff_t last_offset = 0;
static atomic_t in_use = ATOMIC_INIT(0);
static char msg[MAX_LEN];
static char usr_msg[USR_LEN] = INIT_MESSAGE;
static struct class *cls;

/**
 * Defines handlers for specific file operations on devices that will be using this driver.
 */
static struct file_operations strdev_fops = {
	.open = device_open,
	.release = device_release,
	.read = device_read,
	.write = device_write,
};

/**
 * Called on insmod.
 */
static int __init strdev_init(void)
{
	char *tmp;
	major = register_chrdev(0, DEVICE_ID, &strdev_fops);
	if(major < 0)
		return major;
	cls = class_create(THIS_MODULE, DEVICE_ID);
	device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_ID);
	sprintf(msg, MSG_PATTERN, usr_msg, read_count, write_count);
	tmp = MSG_PATTERN;
	while(tmp[total_length++] != '\0');
	patt_length = total_length;
	while(msg[total_length] != '\0') {
		total_length++;
	}
	return SUCCESS;
}

/**
 * Called on rmmod.
 */
static void __exit strdev_exit(void)
{
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
	unregister_chrdev(major, DEVICE_ID);
}

/**
 * Function called on open.
 * */
static int device_open(struct inode *inode, struct file *file)
{
	if(atomic_cmpxchg(&in_use, 0, 1))
		return -EBUSY;
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/**
 * Function called on release.
 */
static int device_release(struct inode *inode, struct file *file)
{	
	/* Non-zero offset means that some process did write to the device */
	if(last_offset != 0) {
		usr_msg[last_offset] = '\0';
		write_count++;
		last_offset = 0;
		sprintf(msg, MSG_PATTERN, usr_msg, read_count, write_count);
		total_length = 0;
		while(total_length < MAX_LEN && msg[total_length++] != '\0');
	} else {
		sprintf(msg, MSG_PATTERN, usr_msg, read_count, write_count);
	}

	atomic_set(&in_use, 0);
	module_put(THIS_MODULE);
	return SUCCESS;
}

/**
 * Function called on read from device.
 */
static ssize_t device_read(struct file *filp, char __user *user_buffer, size_t size, loff_t *offset)
{
	ssize_t len = min((size_t)(total_length - *offset), size);

	/* End of reading, update read_count and resize the message if there is more digits now */
	if(len <= 0) {
		read_count++;
		total_length = min(MAX_LEN, total_length + read_count / 10 - (read_count - 1) / 10);
		return 0;
	}

	if(copy_to_user(user_buffer, &msg[*offset], len))
		return -EFAULT;

	*offset += len;
	return len;
}

/**
 * Function called on write to device.
 * */
static ssize_t device_write(struct file *filp, const char __user *user_buffer, size_t size, loff_t *offset)
{
	ssize_t len = min((size_t)(USR_LEN - *offset), size);

	if(len <= 0)
		return 0;

	if(copy_from_user(&usr_msg[*offset], user_buffer, len))
		return -EFAULT;

	*offset += len;
	last_offset += len; // Needed to determine where to put the '\0' char and the new size of the message
	return len;
}

module_init(strdev_init);
module_exit(strdev_exit);

MODULE_LICENSE("GPL");
