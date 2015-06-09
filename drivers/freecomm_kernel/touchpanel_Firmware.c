#include <generated/compile.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/list.h>
#include <mach/tpfirmware_v.h>

#define MAX_ID_NUM             16
#define MAX_DEVICES_NUM        16

#define MAX_NAME_LEN   32

struct freecomm_id {
       char manuf_name[MAX_NAME_LEN];
       u32 id;
       u32 flag;
};

struct freecomm_devices_version {
       char device_name[MAX_NAME_LEN];
       struct freecomm_id freecomm_device_id[MAX_ID_NUM];
       struct list_head    node;
};

static DEFINE_MUTEX(fdv_list_mutex);
static LIST_HEAD(fdv_list);

//static struct freecomm_devices_version fdv[MAX_DEVICES_NUM]; 


int register_tp_firmware(char *device_name, char *manuf_name, u32 id)
{
       struct freecomm_devices_version *fdv = NULL;
       int i = 0;

       mutex_lock(&fdv_list_mutex);
       list_for_each_entry(fdv, &fdv_list, node) {
               if (!strcmp(fdv->device_name, device_name)) {
                       break;
               }
       }

       if(&(fdv->node) != &fdv_list)   //device name alread in
       {
               //device already exsit, add id
               printk("device already exsit, add id\n");
               //check if id is already in.
               for(i = 0; i < MAX_ID_NUM; i++)
               {
                       if(fdv->freecomm_device_id[i].id == id)
                               break;
               }
               if(i < MAX_ID_NUM)      //id already in
               {
                       printk("devices id alread register: %s %s 0x%08x\n", 
                                       fdv->device_name, fdv->freecomm_device_id[i].manuf_name, fdv->freecomm_device_id[i].id);
                       goto err;
               }
               else //id not in, insert
               {
                       i = 0;
                       while(fdv->freecomm_device_id[i].id)
                       {
                               i++;
                       }
               
                       if(strlen(manuf_name) >= MAX_NAME_LEN)
                       {
                               printk("Error, manuf name is too long, max is 32\n");
                               goto err;
                       }
                       strcpy(fdv->freecomm_device_id[i].manuf_name, manuf_name);
                       
                       fdv->freecomm_device_id[i].id = id;
                       
                       //spin_unlock(&fdv_list_lock);
                       mutex_unlock(&fdv_list_mutex);
                       return 0;
               }
       }
       else
       {
               // device is NOT in, insert it.
               printk("device is NOT in, insert it.\n");

               
               fdv = kzalloc(sizeof(struct freecomm_devices_version), GFP_KERNEL);
               if (!fdv)
               {
                       printk("malloc for fdv Error\n");
                       goto err;
               }

               //device name
               if(strlen(device_name) >= MAX_NAME_LEN)
               {
                       printk("Error, device name is too long, max is 32\n");
                       goto err;
               }
               strcpy(fdv->device_name, device_name);
               
               //manuf name
               if(strlen(manuf_name) >= MAX_NAME_LEN)
               {
                       printk("Error, manuf name is too long, max is 32\n");
                       goto err;
               }
               strcpy(fdv->freecomm_device_id[0].manuf_name, manuf_name);

               //id
               fdv->freecomm_device_id[0].id = id;

               list_add_tail(&fdv->node, &fdv_list);

               mutex_unlock(&fdv_list_mutex);
               return 0;
       }

err:
       mutex_unlock(&fdv_list_mutex);
       return -1;
}
EXPORT_SYMBOL_GPL(register_tp_firmware);


int confirm_tp_firmware (char *device_name, char *manuf_name, u32 id )
{
     struct freecomm_devices_version *fdv = NULL;       
        int i = 0;
     
        mutex_lock(&fdv_list_mutex);
     list_for_each_entry(fdv, &fdv_list, node) {
             if (!strcmp(fdv->device_name, device_name)) {
                                    break;
              }
     }
     
        for(i = 0;i < MAX_ID_NUM;i++)
        {
               if(fdv->freecomm_device_id[i].id == id)
                  {
                  fdv->freecomm_device_id[i].flag = 1;
                      break;
                  }
         }
         
        if(i < MAX_ID_NUM)
         {
              printk("device uses the id of manuf: %s %s 0x%8x\n",
                 fdv->device_name,fdv->freecomm_device_id[i].manuf_name,fdv->freecomm_device_id[i].id);
         }
         else
         {
                       printk(KERN_INFO "Error, the id 0x%08x, is not registered!\n", id);
         }
      
         mutex_unlock(&fdv_list_mutex);
 
      return 0;          
                       
}
EXPORT_SYMBOL_GPL(confirm_tp_firmware);

static inline int fdv_proc_info(char *buf, struct freecomm_devices_version *this)
{
       int i;
       int len = 0;
       char *ptr = buf;

       for(i = 0; i < MAX_ID_NUM; i++)
       {
               if(this->freecomm_device_id[i].id != 0)
               {
                       len += sprintf(ptr, "%s \t%s \t0x%08x \t%d\n",
                                               this->device_name, 
                                               this->freecomm_device_id[i].manuf_name,
                                               this->freecomm_device_id[i].id,
                                               this->freecomm_device_id[i].flag);
                                   
                       ptr = buf + len;
                       //printk("%s %s %d\n",
                       //                      this->device_name, 
                       //                      this->freecomm_device_id[i].manuf_name,
                       //                      this->freecomm_device_id[i].id);
               } 
       }

       return len;
}

static ssize_t fdv_read_proc(char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
       struct freecomm_devices_version *fdv = NULL;
       int len = 0, l = 0;
       off_t   begin = 0;

       mutex_lock(&fdv_list_mutex);
       list_for_each_entry(fdv, &fdv_list, node) {
               l = fdv_proc_info(page + len, fdv);
               len     += l;
        if (len+begin > off+count)
            goto done;
        if (len+begin < off) {
               begin += len;
               len = 0;
        }
       }
    *eof = 1;

done:
       mutex_unlock(&fdv_list_mutex);
    if (off >= len+begin)
            return 0;
    *start = page + (off-begin);
    return ((count < begin+len-off) ? count : begin+len-off);
}


static void create_fdv_proc_file(void)
{
       struct proc_dir_entry *fdv_proc_file =
               create_proc_entry("TP_firmware", 0644, NULL);

       if (fdv_proc_file) {
               fdv_proc_file->read_proc = fdv_read_proc;
       } else
               printk(KERN_INFO "TP_firmware proc file create failed!\n");
}

static int __init fdv_init(void)
{
       printk(KERN_INFO "qiang.lius:--------fdv_init\n");
       create_fdv_proc_file();
       
       return 1;
}

static void __exit fdv_exit(void)
{
}

module_init(fdv_init);
module_exit(fdv_exit);

MODULE_DESCRIPTION("Tp firmware version");
MODULE_AUTHOR("qiang.lius <qiang.lius@feixun.com.cn>");
MODULE_LICENSE("GPL");
