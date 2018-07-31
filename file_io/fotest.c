/* The below code, can be used to create a file and write data to a file using kernel_write and kernel_read.
*/ 

#include <linux/module.h>
#include <linux/fs.h>
#include <asm-generic/segment.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/string.h>

#define dump_filename "/tmp/mario"
#define my_data "HEY ITS ME MARIO"

MODULE_LICENSE("GPL");

struct file * driver_file_open(const char *path, int flags, int mode)
{
        struct file *filp = NULL;
        mm_segment_t oldfs;
        oldfs   = get_fs();
        set_fs(get_ds());
        filp = filp_open(path, O_CREAT|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
        set_fs(oldfs);
        return (filp);
}

void driver_file_close(struct file *filp)
{
        filp_close(filp, NULL);
}

int driver_file_write(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
        int ret;
        mm_segment_t oldfs;
        loff_t  pos = offset;
        oldfs   = get_fs();
        set_fs(get_ds());

        //vfs_setpos(file, pos, pos + PAGE_SIZE);
        //Workaround for vfs_setpos, not implemented on my version of linux.
        spin_lock(&file->f_lock);
        file->f_pos = pos;
        //file->f_version = 0;
        printk(KERN_INFO "set position to  %llx\n", pos);
        spin_unlock(&file->f_lock);


        ret = kernel_write(file, data, size, &pos);
        //vfs_fsync(file, 0);
        set_fs(oldfs);
        return (ret);
}

int driver_file_read(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
        int ret;
        mm_segment_t oldfs;
        loff_t  pos = offset;
        oldfs   = get_fs();
        set_fs(get_ds());

        //vfs_setpos(file, pos, pos + PAGE_SIZE);
        //Workaround for vfs_setpos, not implemented on my version of linux.
        spin_lock(&file->f_lock);
        file->f_pos = pos;
        //file->f_version = 0;
        printk(KERN_INFO "set position to read %llx\n", pos);
        spin_unlock(&file->f_lock);

        ret = kernel_read(file, data, size, &pos);
        //vfs_fsync(file, 0);
        set_fs(oldfs);
        return (ret);
}

static int __init my_init(void)
{
	struct file *file;
        printk(KERN_ALERT "IT WORKS");
	file = driver_file_open(dump_filename, O_WRONLY|O_CREAT, 0644);
	driver_file_write(file,0,my_data,strlen(my_data));
	driver_file_close(file);
        return 0;       /* everything is ok */
}

static void __exit my_exit(void)
{
        //remove_proc_entry(procfs_name, NULL);
        printk(KERN_ALERT "ITS DONE");
}

module_init(my_init);
module_exit(my_exit);
