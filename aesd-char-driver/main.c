/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h> // file_operations
#include <linux/string.h>
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Jacob Sawicki"); 
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev = NULL;

    PDEBUG("open");
    if((inode != NULL) && (filp != NULL))
    {
        dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
        filp->private_data = dev;
    }
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *l_devp = NULL;
    struct aesd_buffer_entry *k_entry = NULL;
    size_t buf_pos = 0;
    size_t read_len = 0;

    ssize_t retval = -EFAULT;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    if((filp != NULL) && (f_pos != NULL) && (buf != NULL))
    {
        l_devp = (struct aesd_dev *)filp->private_data;
        if(l_devp != NULL)
        {
            k_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&l_devp->buffer, (size_t)*f_pos, &buf_pos);  
            if ((k_entry != NULL) && (k_entry->buffptr != NULL))
            {
				PDEBUG("entry size: %zu, buffer pos: %zu", k_entry->size, buf_pos);
                read_len = ((k_entry->size - buf_pos) < count) ? (k_entry->size - buf_pos) : count;
                read_len -= copy_to_user(buf, &k_entry->buffptr[buf_pos], read_len);
                *f_pos += read_len;
                retval = (ssize_t)read_len;
            } 
            else
            {
                PDEBUG("no entry found");
                retval = 0;
            }
        }  
        else
        {
            PDEBUG("filp->private_data is NULL");
        }      
    }

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_buffer_entry k_entry;
    struct aesd_dev *l_devp = NULL;
    char * k_buf = NULL;
    char *old_entry = NULL;
    ssize_t retval = -ENOMEM;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    k_buf = kmalloc((count * sizeof(char)), GFP_KERNEL);
    if(k_buf != NULL) 
    {
		PDEBUG("Allocated entry, %lu", (uintptr_t)k_buf);
        if ((buf != NULL) && (filp != NULL))
        {
            if(copy_from_user(k_buf, buf, count) != 0)
            {
                PDEBUG("copy_from_user failed");
                goto cleanup;
            }
            else
            {
                k_entry.size = count;  
                PDEBUG("entry size: %zu", k_entry.size);
                k_entry.buffptr = k_buf; 
                l_devp = (struct aesd_dev *)filp->private_data;
                if(l_devp != NULL)
                {
                    PDEBUG("Adding buffer %lu to circular buffer", (uintptr_t)k_buf);
					old_entry = aesd_circular_buffer_add_entry(&l_devp->buffer, &k_entry);   
					if(old_entry != NULL)
					{
						PDEBUG("Freeing entry, %lu", (uintptr_t)old_entry);
						kfree(old_entry);
					}
					(void)k_buf;
					(void)k_entry;
					retval = (ssize_t)count;
				}
				else
				{
                    PDEBUG("filp->private_data is NULL");
					goto cleanup;
				}
            }
        }

        return retval;
    }
    else
    {
        return retval;
    }

    cleanup:
    {
		PDEBUG("Cleanup: Freeing entry, %lu", (uintptr_t)k_buf);
        kfree(k_buf);
    }
    
    return retval;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    aesd_circular_buffer_init(&aesd_device.buffer);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
	uint8_t i;
	
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    for (i = 0; i < (sizeof(aesd_device.buffer.entry)/sizeof(struct aesd_buffer_entry)); i++)
    {
		if (aesd_device.buffer.entry[i].buffptr != NULL)
		{
			PDEBUG("Freeing entry, %lu", (uintptr_t)aesd_device.buffer.entry[i].buffptr);
			kfree(aesd_device.buffer.entry[i].buffptr);
		}
		else
		{
			/* no entry */
		}
	}

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
