// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019 NXP
 * Copyright 2022 Technology Advancement Group
 */

#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/virtio.h>
#include <linux/rpmsg.h>
#include <linux/slab.h>
#include <linux/imx_rpmsg.h>

#include "rpmsg-auxdisplay.h"

#define SRTM_DISPLAY_CATEGORY (0xBU)
#define SRTM_DISPLAY_VERSION (0x0100U)

struct aux_dev {
	struct cdev		 cdev;
	struct device		*device;
        struct rpmsg_device	*rpdev;
};

static struct class *aux_class;
static int aux_major;
static dev_t devnum;

#define DRIVER_NAME "aux-display"

static int aux_open(struct inode *inode, struct file *file)
{
	struct aux_dev *dev;

	dev = container_of(inode->i_cdev, struct aux_dev, cdev);
	file->private_data = dev;

	return 0;
}

#ifdef WANT_CKSUM
static unsigned img_cksum;

static unsigned addem(uint8_t const *data, unsigned count)
{
	unsigned sum = 0;
	while (0 < count--) {
		sum += *data++ ;
	}
	return sum;
}
#endif

static ssize_t aux_write (struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	struct aux_dev *dev = (struct aux_dev *)file->private_data;
	struct auxdisplay_rpmsg_msg msg;
        struct rpmsg_device *rpdev = dev->rpdev;
	loff_t offs = *ppos;
	int err;

#ifdef WANT_CKSUM
	if (0 == offs)
		img_cksum = 0;
#endif

	/* display overrun */
	if (offs >= AUX_DISPBYTES)
		return -ENOSPC;

	/* clamp size */
	if (offs + count > AUX_DISPBYTES)
		count = AUX_DISPBYTES - offs;

	if (count > sizeof(msg.data))
		count = sizeof(msg.data);

	memset(&msg, 0, sizeof(msg));
	msg.hdr.cate = SRTM_DISPLAY_CATEGORY;
	msg.hdr.major = SRTM_DISPLAY_VERSION >> 8;
	msg.hdr.minor = SRTM_DISPLAY_VERSION & 0xFF;
	msg.hdr.type = 0;
	msg.hdr.cmd = AUX_UPDATE_DISPLAY;
	msg.y = offs / AUX_BPL;
	msg.x = (offs % AUX_BPL) * 8;

	if(copy_from_user(&msg.data, buf, count)) 
		return -EFAULT;

	err = rpmsg_sendto(rpdev->ept, &msg, sizeof(msg)-sizeof(msg.data)+count, rpdev->dst);
	if (err)
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", err);
	else {
		dev_info(&rpdev->dev, "[%d:%d] -> %lu bytes\n", msg.x, msg.y, count);
		*ppos += count;
#ifdef WANT_CKSUM
		img_cksum += addem(msg.data, count);
		if (AUX_DISPBYTES == *ppos)
			dev_info(&rpdev->dev, "cksum 0x%x\n", img_cksum);
#endif
		err = count;
	}

	return err;
}

static const struct file_operations aux_fops = {
	.owner = THIS_MODULE,
	.open  = aux_open,
	.write = aux_write,
};

static int rpmsg_auxdisplay_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	pr_info("%s: len %d from %d\n", __func__, len, src);
	return 0;
}

static int rpmsg_auxdisplay_probe(struct rpmsg_device *rpdev)
{
	struct auxdisplay_rpmsg_msg msg;
	struct aux_dev *dev; 
	int err;

	dev_info(&rpdev->dev, "new channel: 0x%x -> 0x%x!\n",
			rpdev->src, rpdev->dst);

	dev = kzalloc(sizeof(struct aux_dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&rpdev->dev, "Unable to alloc dev\n");
		return -ENOMEM;
	}

	aux_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(aux_class)) {
		kfree(dev);
		dev_err(&rpdev->dev, "class_create() failed for aux_class\n");
		return PTR_ERR(aux_class);
	}

	err = alloc_chrdev_region(&devnum, 0, 1, DRIVER_NAME);
	if (err < 0) {
		kfree(dev);
		printk(KERN_ERR "no chrdevs %d\n", err);
		return err;
	}

	aux_major = MAJOR(devnum);

	cdev_init(&dev->cdev, &aux_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aux_fops;

	err = cdev_add(&dev->cdev, devnum, 1);
	if (err) {
		kfree(dev);
		return err;
	}

	dev->device = device_create(aux_class, NULL, devnum, dev, "%s", "aux-display");
	if (IS_ERR(dev->device)) {
		dev_err(&rpdev->dev, "device_create failed for aux-display\n");
		cdev_del(&dev->cdev);
		kfree(dev);
		return PTR_ERR(dev->device);
	}

	dev->rpdev = rpdev;
	dev_info(&rpdev->dev, "%s: created device with priv %px:%px\n", __func__, dev, rpdev);

	memset(&msg, 0, sizeof(msg));
	msg.hdr.cate = SRTM_DISPLAY_CATEGORY;
	msg.hdr.major = SRTM_DISPLAY_VERSION >> 8;
	msg.hdr.minor = SRTM_DISPLAY_VERSION & 0xFF;
	msg.hdr.type = 0;
	msg.hdr.cmd = 0;

	err = rpmsg_sendto(rpdev->ept, &msg, sizeof(msg) - sizeof(msg.data), rpdev->dst);
	if (err) {
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", err);
		return err;
	}

	dev_info(&rpdev->dev, "%s: sent hello and msg\n", __func__);

	return 0;
}

static void rpmsg_auxdisplay_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "rpmsg auxdisplay driver is removed\n");
}

static struct rpmsg_device_id rpmsg_driver_auxdisplay_id_table[] = {
	{ .name	= "rpmsg-auxdisplay" },
	{ },
};

static struct rpmsg_driver rpmsg_auxdisplay_driver = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= rpmsg_driver_auxdisplay_id_table,
	.probe		= rpmsg_auxdisplay_probe,
	.callback	= rpmsg_auxdisplay_cb,
	.remove		= rpmsg_auxdisplay_remove,
};

static int __init init(void)
{
	return register_rpmsg_driver(&rpmsg_auxdisplay_driver);
}

static void __exit fini(void)
{
	unregister_rpmsg_driver(&rpmsg_auxdisplay_driver);
}
module_init(init);
module_exit(fini);

MODULE_AUTHOR("Technology Advancement Group (Tag).");
MODULE_DESCRIPTION("Aux display driver");
MODULE_LICENSE("GPL v2");
