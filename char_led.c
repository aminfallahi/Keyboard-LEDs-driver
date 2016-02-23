/****************************************************************************
 * (C) 2015 - Amin Fallahi- IUST.
 ****************************************************************************
 *
 *	   
 *	  Author:  Amin Fallahi
 *
 * Description: A driver that gets a string of characters from users and displays its corresponding binary values on keyboard LEDs
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/tty.h>		
#include <linux/kd.h>		
#include <linux/vt.h>
#include <linux/console_struct.h>	
#include <linux/vt_kern.h>
#include <linux/slab.h>

MODULE_DESCRIPTION("Example module illustrating the use of Keyboard LEDs.");
MODULE_AUTHOR("Daniele Paolo Scarpazza");
MODULE_LICENSE("GPL");

struct timer_list my_timer;
struct tty_driver *my_driver;
char kbledstatus = 0;
int step=1;
int *p= &step; 
int wirting_enable=0;


#define BLINK_DELAY   200
#define ALL_LEDS_ON   0x07
#define RESTORE_LEDS  0xFF

/*

 * 
 */



#define BUF_MAX_SIZE		1024

struct node
{
	char c;
	struct list_head  l;

};


struct list_head list;

struct list_head *temp;

static char buff[BUF_MAX_SIZE];
static dev_t mydev;			 // (major,minor) value
struct cdev my_cdev;


ssize_t my_write (struct file *flip, const char __user *buf, size_t count, loff_t *f_ops)
{


	struct node *aNewNode;
	int i;

	//copy_from_user(&output[*f_ops], buf, 1);
	if(count >  BUF_MAX_SIZE){
		copy_from_user(buff, buf, BUF_MAX_SIZE);

		return 1024;
	}
	else
		copy_from_user(buff, buf, count);


	INIT_LIST_HEAD(&list);


	for( i=0; i< count  ; i++){
		aNewNode = kmalloc(sizeof(*aNewNode), GFP_KERNEL);
		aNewNode->c= buff[i];
		INIT_LIST_HEAD(&aNewNode->l);
		list_add_tail(&list, &(aNewNode->l));
	}

	temp=&list;
	wirting_enable=1;

	return count;
}

ssize_t my_read(struct file *flip, char __user *buf, size_t count, loff_t *f_ops){

	if(buff[*f_ops]=='\0')
		return 0;

	copy_to_user(buf, &buff[*f_ops],1);
	*f_ops+=1;
	return 1;

}

struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.read = my_read,
	.write=my_write,
};


int * char_bin(char c)
{
	static int a[3];
	a[0]= (((int)c & 7) & 2) | (((int)c & 7) & 4)| (((int)c & 7) & 1);
	a[1]= ((((int)c >> 3 ) & 7) & 2 ) | ((((int)c >> 3 ) & 7) & 4) | ((((int)c >> 3 ) & 7) & 1);
	a[2]= ((((int)c >> 6) & 7) & 2)|((((int)c >> 6) & 7) & 4)| ((((int)c >> 6) & 7) & 1) ;

	return a;
}


static void my_timer_func(unsigned long ptr)
{


	int *pstatus = (int *)ptr;

	if( wirting_enable==0)
	goto out;



	if (step==1 ){


		// get the next character from the list

		temp= temp->next;

		struct node *nodo_ptr = list_entry(temp, struct node, l);

		p=char_bin(nodo_ptr->c);//  call char_bin function to convert a character to three parts binary values

		if(p)

			(my_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, *p);

		step++;
	}
	else if( step==2){
		if(p)
			(my_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, *(p+1));

		step++;
	}
	else if( step==3){
		if(p)
			(my_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, *(p+2));

		step++;
	}
	else if( step==4){

		(my_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, 7);
		step++;

	}
	else if( step==5){

		(my_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, 0);
		step++;

	}

	else if( step==6){

		(my_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, 7);

		step =1;

	}


	if (*pstatus == ALL_LEDS_ON)
		*pstatus = RESTORE_LEDS;
	else
		*pstatus = ALL_LEDS_ON;


	out:
		my_timer.expires = jiffies + BLINK_DELAY;
		add_timer(&my_timer);
}

static int __init kbleds_init(void)
{
	int i;
	alloc_chrdev_region(&mydev, 0, 1, "eadriver");
	cdev_init(&my_cdev, &my_fops);
	my_cdev.owner = THIS_MODULE;
	cdev_add(&my_cdev, mydev, 1);
	
	for (i = 0; i < MAX_NR_CONSOLES; i++){
		if (!vc_cons[i].d)
			break;
		printk(KERN_INFO "poet_atkm: console[%i/%i] #%i, tty %lx\n", i,
		MAX_NR_CONSOLES, vc_cons[i].d->vc_num,
			(unsigned long)vc_cons[i].d->port.tty);
	}
	
	my_driver = vc_cons[fg_console].d->port.tty->driver;
	
	/*
	 * Set up the LED blink timer the first time
	 */
	init_timer(&my_timer);
	my_timer.function = my_timer_func;
	my_timer.data = (unsigned long)&kbledstatus;
	my_timer.expires = jiffies + BLINK_DELAY;
	add_timer(&my_timer);

	return 0;
}

static void __exit kbleds_cleanup(void)
{
	
	del_timer(&my_timer);
	(my_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,
			RESTORE_LEDS);

	cdev_del(&my_cdev);
	unregister_chrdev_region(mydev, 1);
}

module_init(kbleds_init);
module_exit(kbleds_cleanup);
