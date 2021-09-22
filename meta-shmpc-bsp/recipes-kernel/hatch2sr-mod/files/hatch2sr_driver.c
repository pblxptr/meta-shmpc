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

#include <linux/input.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>

#include "sensor.h"
#include "engine.h"
#include "hatch2sr_ctrl.h"

#define DEV_BASE_MINOR (0)
#define DEV_COUNT      (1)

/*
** Function prototypes for attributes
*/
ssize_t hatch2sr_show_attr_status(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t hatch2sr_store_attr_change_positon(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
ssize_t hatch2sr_show_attr_slow_start(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t hatch2sr_store_attr_slow_start(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

/*
**	Function prototypes for file operations
*/
static int     hatch2sr_fop_open(struct inode*, struct file*);
static int     hatch2sr_fop_release(struct inode*, struct file*);
static ssize_t hatch2sr_fop_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t hatch2sr_fop_write(struct file*, const char __user*, size_t, loff_t*);

/*
** Driver struct
*/
static struct hatch2sr_device {
  dev_t num;
  struct cdev cdev;
  struct device* dev;
} hatch2sr_dev;

/*
** File operations
*/
static struct file_operations fops = {
  .owner =    THIS_MODULE,
  .open =     hatch2sr_fop_open,
  .release =  hatch2sr_fop_release,
  .read =     hatch2sr_fop_read,
  .write =    hatch2sr_fop_write
};

/*
** Attributtes
*/
static DEVICE_ATTR(status, S_IRUGO,
  hatch2sr_show_attr_status,
  NULL
);
static DEVICE_ATTR(change_positon, S_IWUSR,
  NULL,
  hatch2sr_store_attr_change_positon
);
static DEVICE_ATTR(slow_start, S_IRUGO | S_IWUSR,
  hatch2sr_show_attr_slow_start,
  hatch2sr_store_attr_slow_start
);

static struct attribute* hatch2sr_attrs[] = {
  &dev_attr_status.attr,
  &dev_attr_change_positon.attr,
  &dev_attr_slow_start.attr,
  NULL
};

static const struct attribute_group hatch2sr_group = {
  .attrs = hatch2sr_attrs,
};

static const struct attribute_group* hatch2sr_groups[] = {
  &hatch2sr_group,
  NULL
};

/* Function definitions for attributes
** This function is called when somebody reads the status attribute.
*/
ssize_t hatch2sr_show_attr_status(struct device *dev, struct device_attribute *attr, char *buf)
{
  const hatch_status status = hatch2sr_get_status();
  const char* pattern = "%s\n";

  pr_info("%s\n", __FUNCTION__);

  //sysfs_emit is aware of PAGE_SIZE
  if (status == HATCH_STATUS_OPEN) {
    return sysfs_emit(buf, pattern, "open");
  } else if (status == HATCH_STATUS_CLOSED) {
    return sysfs_emit(buf, pattern, "closed");
  } else if (status == HATCH_STATUS_CHANGING_POSITION) {
    return sysfs_emit(buf, pattern, "changing_position");
  } else if (status == HATCH_STATUS_FAULTY) {
    return sysfs_emit(buf, pattern, "faulty");
  } else {
    return sysfs_emit(buf, pattern, "undefined");
  }
}

/*
** This function is called while reading the open attribute.
*/
ssize_t hatch2sr_store_attr_change_positon(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
  if (sysfs_streq(buf, "open")) {
    hatch2sr_open();
  } else if (sysfs_streq(buf, "close")) {
    hatch2sr_close();
  } else {
    return -EINVAL;
  }

  return count;
}

/*
** This function is called while reading the slow_start attribute.
*/
ssize_t hatch2sr_show_attr_slow_start(struct device *dev, struct device_attribute *attr, char *buf)
{
  bool slow_start;

  slow_start = hatch2sr_engine_get_slow_start();

  if (slow_start) {
    return sysfs_emit(buf, "%s\n", "1");
  } else {
    return sysfs_emit(buf, "%s\n", "0");
  }
}

/*
** This function is called while writing to the slow_start attribute.
*/
ssize_t hatch2sr_store_attr_slow_start(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
  if (sysfs_streq(buf, "1")) {
    hatch2sr_engine_set_slow_start(true);
  } else if (sysfs_streq(buf, "0")) {
    hatch2sr_engine_set_slow_start(false);
  } else {
    return -EINVAL;
  }

  return count;
}


/* Function definitions for file operations
** This function is called when somebody has called open driver file.
*/
static int	hatch2sr_fop_open(struct inode* inode, struct file* file)
{
  pr_info("%s\n", __FUNCTION__);

  return 0;
}

/*
** This function is called when somebody has called close driver file.
*/
static int hatch2sr_fop_release(struct inode* inode, struct file* file)
{
  printk("%s\n", __FUNCTION__);

  return 0;
}

/*
** This function is called when somebody has called read driver file.
*/
static ssize_t hatch2sr_fop_read(struct file* file, char __user* buf, size_t len, loff_t* off)
{
  printk("%s\n", __FUNCTION__);

  return 0;
}

/*
** This function is called when somebody has called write driver file.
*/
static ssize_t hatch2sr_fop_write(struct file* file, const char __user* buf, size_t len, loff_t* off)
{
  printk("%s\n", __FUNCTION__);

  return len;
}

static int hatch2sr_driver_probe(struct platform_device* pdev)
{
  struct pwm_device* pwm_dev;
  struct gpio_desc* gpio_sensor_open;
  struct gpio_desc* gpio_sensor_closed;
  struct gpio_desc* gpio_relay;

  hatch2sr_dev.dev = &pdev->dev;

  pr_info("Hello driver loaded.\n");

  // Allocate Major number
  if (alloc_chrdev_region(&hatch2sr_dev.num, DEV_BASE_MINOR, DEV_COUNT, "hatch2sr") < 0) {
    dev_err(hatch2sr_dev.dev, "Cannot allocate major number for device.\n");
    return -1;
  }
  pr_info("Major: %d, Minor: %d \n", MAJOR(hatch2sr_dev.num), MINOR(hatch2sr_dev.num));

  //Initializing cdev for driver
  cdev_init(&hatch2sr_dev.cdev, &fops);

  //Add character device
  if ((cdev_add(&hatch2sr_dev.cdev, hatch2sr_dev.num, DEV_COUNT)) < 0){
      dev_err(hatch2sr_dev.dev, "Cannot add the device to the system\n");
      goto r_cdev_err;
    }

  //Create device class
  if ((hatch2sr_dev.dev->class = class_create(THIS_MODULE, "hatch2sr")) == NULL){ // unregister_chrdev_region + cdev_del
      dev_err(hatch2sr_dev.dev, "Cannot create the struct class for device\n");
      goto r_class_err;
  }
  hatch2sr_dev.dev->class->dev_groups = hatch2sr_groups;

  //Create device nodes
  if ((device_create(hatch2sr_dev.dev->class, NULL, hatch2sr_dev.num, NULL, "hatch2sr")) == NULL) {  // unregister_chrdev_region + cdev_del + class_destroy
      dev_err(hatch2sr_dev.dev, "Cannot create the Device\n");
      goto r_device_err;
  }

  //Configure peripherals
  pwm_dev = pwm_get(hatch2sr_dev.dev, "motor1");

  if (IS_ERR(pwm_dev)) {
    dev_err(hatch2sr_dev.dev, "Cannot get pwm dev for engine.\n");
    goto r_pwm_err;
  }

  gpio_sensor_open = gpiod_get(hatch2sr_dev.dev, "openpossensor", GPIOD_IN);
  if (IS_ERR(gpio_sensor_open)) {
    dev_err(hatch2sr_dev.dev, "Cannot get gpio dev for open position sensor.\n");
    goto r_openpos_sensor_err;
  }

  gpio_sensor_closed = gpiod_get(hatch2sr_dev.dev, "closepossensor", GPIOD_IN);
  if (IS_ERR(gpio_sensor_closed)) {
    dev_err(hatch2sr_dev.dev, "Cannot get gpio dev for closed position sensor.\n");
    goto r_closedpos_sensor_err;
  }

  gpio_relay = gpiod_get(hatch2sr_dev.dev, "relay", GPIOD_OUT_HIGH); //TODO: Should it be out_low?????
  if (IS_ERR(gpio_relay)) {
    dev_err(hatch2sr_dev.dev, "Cannot get gpio dev for relayr.\n");
    goto r_relay_err;
  }

  if (hatch2sr_init(pwm_dev, gpio_sensor_open, gpio_sensor_closed, gpio_relay)) {
    dev_err(hatch2sr_dev.dev, "Cannot initialize logic for hatch2sr driver.\n");
    goto r_hatch2sr_init_err;
  }

  //Initialize driver logic
  pr_info("Hatch2sr Kernel Module probed successfully test...\n");

  return 0;

  r_hatch2sr_init_err:
    gpiod_put(gpio_relay);
  r_relay_err:
    gpiod_put(gpio_sensor_closed);
  r_closedpos_sensor_err:
    gpiod_put(gpio_sensor_open);
  r_openpos_sensor_err:
    pwm_put(pwm_dev);
  r_pwm_err:
    device_destroy(hatch2sr_dev.dev->class, hatch2sr_dev.num);
  r_device_err:
    class_destroy(hatch2sr_dev.dev->class);
  r_class_err:
    cdev_del(&hatch2sr_dev.cdev);
  r_cdev_err:
    unregister_chrdev_region(hatch2sr_dev.num, DEV_COUNT);
    return -1;
}

static int hatch2sr_driver_remove(struct platform_device *pdev)
{
  printk("%s\n", __FUNCTION__);

  hatch2sr_deinit();

  device_destroy(hatch2sr_dev.dev->class, hatch2sr_dev.num);
  class_destroy(hatch2sr_dev.dev->class);
  cdev_del(&hatch2sr_dev.cdev);
  unregister_chrdev_region(hatch2sr_dev.num, DEV_COUNT);

  pr_info("Hatch2sr Kernel Module removed successfully...\n");

  return 0;
}


static const struct of_device_id hatch2sr_match[] = {
  { .compatible = "hatch2sr", },
  { },
};

MODULE_DEVICE_TABLE(of, hatch2sr_match);

static struct platform_driver hatch2sr_driver = {
  .probe	= hatch2sr_driver_probe,
  .remove = hatch2sr_driver_remove,
  .driver = {
    .name	= "hatch2sr",
    .owner = THIS_MODULE,
    .of_match_table = of_match_ptr(hatch2sr_match),
  },
};

module_platform_driver(hatch2sr_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Patryk Biel");
MODULE_DESCRIPTION("Driver used to control hatch through 8-pin relay.");
MODULE_VERSION("1:0.1");
