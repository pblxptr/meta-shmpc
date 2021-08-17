/******************************************************************************
 *
 *   Copyright (C) 2011  Intel Corporation. All rights reserved.
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *****************************************************************************/

/**
*	- Device Driver 1 - Introduction 					-> done
*	- Device Driver 2 - First Driver 					-> done
* - Device Driver 3 - Passing Arguments 		-> TODO!!!
*	- Device Driver 4 - Major & Minor Number 	-> done
*	- Device Driver 5 - Creating Device File 	-> done
*	- Device Driver 6 - File Operations 			-> done
* - Device Driver 7 - Real Device Driver 		-> done
* - Device Driver 8 - IOCTL Tutorial 				-> TODO!!!
* - Device Driver 9 - Procfs Tutorial 			-> TODO!!!		
*/



#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define BASE_MINOR 0
#define DEV_COUNT  1
#define BUFFER_SIZE 1024

/*
**	Function prototypes
*/
static int 		 __init example_driver_init(void);
static void 	 __exit example_driver_exit(void);
static int 		 example_open(struct inode*, struct file*);
static int  	 example_release(struct inode*, struct file*);
static ssize_t example_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t example_write(struct file*, const char __user*, size_t, loff_t*);

static struct file_operations fops = {
	.owner = 		THIS_MODULE,
	.open = 		example_open,
	.release = 	example_release,
	.read = 		example_read,
	.write = 		example_write
};


dev_t dev = 0;
static struct class* dev_class;
static struct cdev example_cdev;
uint8_t* kernel_buffer;


/*
** This function is called when somebody has called open driver file.
*/
static int	example_open(struct inode* inode, struct file* file)
{
	pr_info("%s\n", __FUNCTION__);

	if ((kernel_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL)) == 0) {
		pr_err("Cannot allocate memory in kernel.\n");

		return -1;
	}

	return 0;
}

/*
** This function is called when somebody has called close driver file.
*/
static int example_release(struct inode* inode, struct file* file)
{
	pr_info("%s\n", __FUNCTION__);
	kfree(kernel_buffer);

	return 0;
}

/*
** This function is called when somebody has called read driver file.
*/
static ssize_t example_read(struct file* file, char __user* buf, size_t len, loff_t* off)
{
	pr_info("%s\n", __FUNCTION__);
	copy_to_user(buf, kernel_buffer, len);

	return 0;
}

/*
** This function is called when somebody has called write driver file.
*/
static ssize_t example_write(struct file* file, const char __user* buf, size_t len, loff_t* off)
{
	pr_info("%s\n", __FUNCTION__);
	copy_from_user(kernel_buffer, buf, len);

	return len;
}

/*
** Module init function
*/
int static __init example_driver_init(void)
{
	pr_info("%s\n", __FUNCTION__);

	if (alloc_chrdev_region(&dev, BASE_MINOR, DEV_COUNT, "example") < 0) {
		pr_info("Cannot allocate major number for device.\n");

		return -1;
	}

	/* Initializing cdev structutre */
	cdev_init(&example_cdev, &fops);
	
	/* Adding character device to the system */
	if (cdev_add(&example_cdev, dev, DEV_COUNT) < 0) {
		pr_err("Cannot add the cdev to the system.\n");

		goto r_class_fail;
	}

	/* Creating class */
	if ((dev_class = class_create(THIS_MODULE, "example_class")) == NULL) {
		pr_err("Cannot create the struct class for device.\n");
		
		goto r_class_fail;
	}

	/* Creating device */
	if (device_create(dev_class, NULL, dev, NULL, "example_device") == NULL) {
		pr_err("Cannot create the device.\n");

		goto r_device_fail;
	}

	pr_info("Major: %d, Minor: %d", MAJOR(dev), MINOR(dev));
	pr_info("Kernel Module inserted successfully...\n");

	return 0;

	r_device_fail:
		class_destroy(dev_class);
	r_class_fail:
		unregister_chrdev_region(dev, DEV_COUNT);
		return -1;
}

/*
** Module exit function
*/
static void __exit example_driver_exit(void)
{
	pr_info("%s\n", __FUNCTION__);

	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&example_cdev);
	unregister_chrdev_region(dev, DEV_COUNT);
	pr_info("Kernel Module removed successfully...\n");

}

module_init(example_driver_init);
module_exit(example_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Patryk Biel");
MODULE_DESCRIPTION("Driver used to control hatch through 8-pin relay.");
MODULE_VERSION("1:0.0");