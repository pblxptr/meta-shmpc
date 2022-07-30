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

/*
** Function prototypes for attributes
*/
ssize_t hatch2sr_show_attr_status(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t hatch2sr_store_attr_change_position(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
ssize_t hatch2sr_show_attr_engine_slow_start(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t hatch2sr_store_attr_engine_slow_start(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
ssize_t hatch2sr_show_attr_engine_max_speed(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t hatch2sr_store_attr_engine_max_speed(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

/*
**	Function prototypes for file operations
*/
static int     hatch2sr_fop_release(struct inode*, struct file*);
static ssize_t hatch2sr_fop_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t hatch2sr_fop_write(struct file*, const char __user*, size_t, loff_t*);

/*
** Driver struct
*/
struct hatch2sr_device {
  struct miscdevice misc;
};

/*
** File operations
*/
static struct file_operations fops = {
  .owner =    THIS_MODULE,
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
static DEVICE_ATTR(change_position, S_IWUSR,
  NULL,
  hatch2sr_store_attr_change_position
);
static DEVICE_ATTR(slow_start, S_IRUGO | S_IWUSR,
  hatch2sr_show_attr_engine_slow_start,
  hatch2sr_store_attr_engine_slow_start
);

static DEVICE_ATTR(engine_max_speed, S_IRUGO | S_IWUSR,
  hatch2sr_show_attr_engine_max_speed,
  hatch2sr_store_attr_engine_max_speed
);


static struct attribute* hatch2sr_attrs[] = {
  &dev_attr_status.attr,
  &dev_attr_change_position.attr,
  &dev_attr_slow_start.attr,
  &dev_attr_engine_max_speed.attr,
  NULL
};

static const struct attribute_group hatch2sr_group = {
  .attrs = hatch2sr_attrs,
};

static const struct attribute_group* hatch2sr_groups[] = {
  &hatch2sr_group,
  NULL
};

static struct class hatch2sr_class = {
  .name = "hatch2sr",
  .owner = THIS_MODULE,
  .dev_groups = hatch2sr_groups
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
ssize_t hatch2sr_store_attr_change_position(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
  pr_info("%s\n", __FUNCTION__);

  if (sysfs_streq(buf, "open")) {
    hatch2sr_open();
  } else if (sysfs_streq(buf, "close")) {
    hatch2sr_close();
  } else if (sysfs_streq(buf, "stop")) {
    hatch2sr_stop();
  } else {
    return -EINVAL;
  }

  return count;
}

/*
** This function is called while reading the slow_start attribute.
*/
ssize_t hatch2sr_show_attr_engine_slow_start(struct device *dev, struct device_attribute *attr, char *buf)
{
  bool slow_start;

  pr_info("%s\n", __FUNCTION__);

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
ssize_t hatch2sr_store_attr_engine_slow_start(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
  pr_info("%s\n", __FUNCTION__);

  if (sysfs_streq(buf, "1")) {
    hatch2sr_engine_set_slow_start(true);
  } else if (sysfs_streq(buf, "0")) {
    hatch2sr_engine_set_slow_start(false);
  } else {
    return -EINVAL;
  }

  return count;
}

/*
** This function is called while reading engine's max spped.
*/
ssize_t hatch2sr_show_attr_engine_max_speed(struct device *dev, struct device_attribute *attr, char *buf)
{
  int max_speed;

  pr_info("%s\n", __FUNCTION__);

  max_speed = hatch2sr_engine_get_max_speed_pct();

  return sysfs_emit(buf, "%d\n", max_speed);
}

/*
** This function is called while setting engine's max spped.
*/
ssize_t hatch2sr_store_attr_engine_max_speed(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
  int max_speed;
  int ret;

  pr_info("%s\n", __FUNCTION__);

  ret = kstrtouint(buf, 0, &max_speed); //0 means auto-detected base
  if (ret) {
    return ret;
  } else {
    ret = hatch2sr_engine_set_max_speed_pct(max_speed);
    if (ret) {
      return ret;
    } else {
      return count;
    }
  }
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
  struct device* dev;
  struct hatch2sr_device* priv;
  struct pwm_device* pwm_dev;
  struct gpio_desc* gpio_sensor_open;
  struct gpio_desc* gpio_sensor_closed;
  struct gpio_desc* gpio_relay;

  dev = &pdev->dev;

  pr_info("Loading hatch2sr Kernel Module\n");

  priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
  if (!priv)
    return -ENOMEM;

  dev_set_drvdata(dev, priv);

  //Register msic device
  priv->misc.name = "hatch2sr";
  priv->misc.minor = MISC_DYNAMIC_MINOR;
  priv->misc.fops = &fops;

  if (misc_register(&priv->misc) != 0) {
    dev_err(dev, "Cannot register misc device\n");
    return -1;
  }

  //TODO: Class not needed, use misc class, is it possible?
  //Create device class
  if (class_register(&hatch2sr_class)) {
    dev_err(dev, "class");
    goto r_class_err;
  }

  //Create device nodes
  if ((device_create(&hatch2sr_class, dev, dev->devt, NULL, "hatch2sr")) == NULL) {  // unregister_chrdev_region + cdev_del + class_destroy
      dev_err(dev, "Cannot create the Device\n");
      goto r_device_err;
  }

  //Configure peripherals
  pwm_dev = devm_pwm_get(dev, "motor1");

  if (IS_ERR(pwm_dev)) {
    dev_err(dev, "Cannot get pwm dev for engine.\n");
    goto r_cleanup;
  }

  gpio_sensor_open = devm_gpiod_get(dev, "openpossensor", GPIOD_IN);
  if (IS_ERR(gpio_sensor_open)) {
    dev_err(dev, "Cannot get gpio dev for open position sensor.\n");
    goto r_cleanup;
  }

  gpio_sensor_closed = devm_gpiod_get(dev, "closepossensor", GPIOD_IN);
  if (IS_ERR(gpio_sensor_closed)) {
    dev_err(dev, "Cannot get gpio dev for closed position sensor.\n");
    goto r_cleanup;
  }

  gpio_relay = devm_gpiod_get(dev, "relay", GPIOD_OUT_HIGH); //TODO: Should it be out_low?????
  if (IS_ERR(gpio_relay)) {
    dev_err(dev, "Cannot get gpio dev for relayr.\n");
    goto r_cleanup;
  }

  //Initialize driver logic
  if (hatch2sr_init(pwm_dev, gpio_sensor_open, gpio_sensor_closed, gpio_relay)) {
    dev_err(dev, "Cannot initialize logic for hatch2sr driver.\n");
    goto r_cleanup;
  }

  pr_info("Hatch2sr Kernel Module probed successfully test...\n");

  return 0;

  r_cleanup:
    device_destroy(&hatch2sr_class, dev->devt);
  r_device_err:
    class_unregister(&hatch2sr_class);
  r_class_err:
    misc_deregister(&priv->misc);
    //TODO: Deallocate priv?? 
    return -1;
}

static int hatch2sr_driver_remove(struct platform_device *pdev)
{
  struct device* dev = &pdev->dev;
  struct hatch2sr_device* priv = dev_get_drvdata(dev);

  printk("%s\n", __FUNCTION__);

  hatch2sr_deinit();

  device_destroy(&hatch2sr_class, dev->devt);
  class_unregister(&hatch2sr_class);
  misc_deregister(&priv->misc);
  // kfree(priv);

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
