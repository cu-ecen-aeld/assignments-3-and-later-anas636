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
#include <linux/fs.h> // file_operations
#include <linux/string.h>
#include "aesdchar.h"

#include <linux/slab.h>		/* kmalloc() */

#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("anas"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

/*struct aesd_dev aesd_device = { 
    .count_total = 0,
    .add_entry = { .buffptr = NULL, .size = 0 }
};*/
struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    
    struct aesd_dev *dev; /* device information */
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev; /* for other methods */
    
    /**
     * TODO: handle open
     */
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    //PDEBUG("we are here__\n");
    struct aesd_dev *dev = filp->private_data;
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
	
    //PDEBUG("we are here___\n");
    size_t *entry_offset_byte_rtn = kmalloc(sizeof(size_t), GFP_KERNEL);
    //PDEBUG("La valeur de fpos est : %lld \n", *f_pos);
    struct aesd_buffer_entry *my_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, entry_offset_byte_rtn );
    //PDEBUG("we are here_\n");
    if (my_entry == NULL){
        //PDEBUG("we are here_1\n");
        goto out;
    }
    
    if ( *entry_offset_byte_rtn + count > my_entry->size)
        count = my_entry->size - (*entry_offset_byte_rtn);
    //PDEBUG("we are here_2\n");
    //PDEBUG("string to read %s", (my_entry->buffptr)+ (*entry_offset_byte_rtn));
    if (copy_to_user(buf, (my_entry->buffptr)+ (*entry_offset_byte_rtn), count)) {
        //PDEBUG("we are here_3\n");
        retval = -EFAULT;
	    goto out;
    }
    *f_pos += count;
    retval = count;
  out:
    PDEBUG("we are here_4\n");
    kfree(entry_offset_byte_rtn);
	mutex_unlock(&dev->lock);
	return retval;   
 
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	//size_t count_total;
    /**
     * TODO: handle write
     */
    struct aesd_dev *dev = filp->private_data;
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
        
    dev->count_total = dev->count_total + count;
    dev->add_entry.size = dev->count_total;
    
    char *new_data = kmalloc(count+1, GFP_KERNEL);
    if (!new_data){
        retval = -ENOMEM;
        goto out;
    }
    memset(new_data, 0, count+1);

    //PDEBUG("we are here");
	 
    if (copy_from_user(new_data, buf, count)) {
        retval = -EFAULT;
	    goto out;
    }
    retval = count;
  
    //PDEBUG("we are here1\n");
    //PDEBUG("the string to write is (buf) %s \n", buf);
    char *old_data = dev->add_entry.buffptr;
    //PDEBUG("the string to write is (old_data) %s \n", old_data);
    char *all_data = kmalloc((dev->count_total)+1, GFP_KERNEL);
    memset(all_data, 0, (dev->count_total)+1);
    if (!all_data){
        retval = -ENOMEM;
        goto out;
    }    
    //PDEBUG("we are here2\n");
    if (old_data != NULL){
        strcpy(all_data, old_data);
        //PDEBUG("the string to write is (after strcpy) %s \n", all_data);   
    }

    //PDEBUG("we are here3\n");
    strcat(all_data, new_data);
    //PDEBUG("the string to write is (after strcat) %s \n", all_data);

    //PDEBUG("we are here4\n");
    dev->add_entry.buffptr = all_data;
    
    if(*(buf+(count-1))=='\n'){
        if (dev->buffer.full){
            kfree(dev->buffer.entry[dev->buffer.in_offs].buffptr);
        }
        aesd_circular_buffer_add_entry(&dev->buffer, &dev->add_entry);
        dev->count_total = 0;
        dev->add_entry.size = dev->count_total;
        //PDEBUG("the string writed is %s \n", dev->add_entry.buffptr);
        dev->add_entry.buffptr = NULL;
        //PDEBUG("we are here5\n");
    }
    //PDEBUG("we are here6\n");
    kfree(new_data);
    //kfree(all_data);
    
      
    out:
        mutex_unlock(&dev->lock);
	    return retval;
     
         
    //return retval;
}


long aesd_unlocked_ioctl (struct file *filp, unsigned int cmd, unsigned long arg){

    
    long retval = 0;
    struct aesd_dev *dev = filp->private_data;
    


    switch(cmd) {
    case AESDCHAR_IOCSEEKTO:
    struct aesd_seekto seekto;
        if( copy_from_user(&seekto, (const void __user *) arg,
                           sizeof(seekto)) )
            return -EFAULT;

        retval = aesd_adjust_file_offset(filp, seekto.write_cmd, seekto.write_cmd_offset);

        break;
    default:
        return -ENOTTY;
    }


}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .unlocked_ioctl = aesd_unlocked_ioctl,
};


static long aesd_adjust_file_offset(struct file *filp, unsigned int write_cmd, unsigned int write_cmd_offset){

    
    long retval = 0;
    struct aesd_dev *dev = filp->private_data;
    loff_t file_offset = 0;
    if (write_cmd >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        return -EINVAL;
    else if (write_cmd_offset > (dev->buffer.entry[write_cmd].size-1))
        return -EINVAL;
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;


    unsigned int out_off_ = dev->buffer.out_offs;
    while (out_off_ != write_cmd){

        file_offset = (dev->buffer.entry[out_off_].size -1) + file_offset;
        out_off_++; 
        if (out_off_ >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
            if (buffer->full){
                out_off_ = 0;
            }
            else {
                break;
            }
        }
        }
    }

    file_offset = file_offset + write_cmd_offset;

    out:
        mutex_unlock(&dev->lock);
	    return retval;
}


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
    aesd_device.count_total = 0;
    aesd_device.add_entry.buffptr = NULL;
    aesd_device.add_entry.size = 0;

    /**
     * TODO: initialize the AESD specific portion of the device
     i nrrd to init aesd device hier
     */
     
    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
