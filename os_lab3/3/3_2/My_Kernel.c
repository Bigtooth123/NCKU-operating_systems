#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <asm/current.h>

#define procfs_name "Mythread_info"
#define BUFSIZE  1024
char buf[BUFSIZE]; //kernel buffer
static int buf_len; 
static ssize_t Mywrite(struct file *fileptr, const char __user *ubuf, size_t buffer_len, loff_t *offset){
    ssize_t len;
    
    if(*offset > 0){
        return 0;
    }

    copy_from_user(buf, ubuf, buffer_len);
    
    len = sprintf(buf+buffer_len, "PID: %d, TID: %d, time: %lld\n", 
                        current->tgid, current->pid, 
                        current->utime/100/1000);
    *offset += buffer_len;
    *offset += len;
    buf_len = buffer_len+ len;
    return len;
}

static ssize_t Myread(struct file *fileptr, char __user *ubuf, size_t buffer_len, loff_t *offset){
    if(*offset > 0){
        return 0;
    }
    copy_to_user(ubuf, buf, buf_len);
    *offset = *offset+ buf_len;
    return buf_len;
}

static struct proc_ops Myops = {
    .proc_read = Myread,
    .proc_write = Mywrite,
};

static int My_Kernel_Init(void){
    proc_create(procfs_name, 0644, NULL, &Myops);   
    pr_info("My kernel says Hi");
    return 0;
}

static void My_Kernel_Exit(void){
    pr_info("My kernel says GOODBYE");
}

module_init(My_Kernel_Init);
module_exit(My_Kernel_Exit);

MODULE_LICENSE("GPL");