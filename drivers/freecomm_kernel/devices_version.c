#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <mach/fdv.h>
#include <linux/uaccess.h>


//gxh add for phicomm pdata codec start, 2013.03.08

#define MAX_FDV_NUMS	32

struct phicomm_pdata_fdv_info
{
	unsigned char name[16];
	unsigned int id;
	unsigned char part_number[12];
};

static struct phicomm_pdata_fdv_info ppfi[MAX_FDV_NUMS];
static int ppfi_index = 0;

unsigned int get_fdv_id_by_name(char *devname)
{
	int i = 0;

	for(i = 0; i < MAX_FDV_NUMS; i++)
	{
		if(!strcmp(ppfi[i].name, devname))
		{
			break;
		}
	}
	
	if(i == MAX_FDV_NUMS)
	{
		printk("get %s id error\n", devname);
		return 0;
	}
	else 
	{
		return ppfi[i].id;
	}
}

static unsigned char *part_number_err = "FFFFFFFF";
unsigned char *get_fdv_part_number_by_name(char *devname)
{
	int i = 0;

	for(i = 0; i < MAX_FDV_NUMS; i++)
	{
		if(!strcmp(ppfi[i].name, devname))
		{
			break;
		}
	}
	
	if(i == MAX_FDV_NUMS)
	{
		printk("get %s part number error\n", devname);
		return part_number_err;
	}
	else 
	{
		return ppfi[i].part_number;
	}
}
EXPORT_SYMBOL_GPL(get_fdv_part_number_by_name);

int fdv_pn_mach(char *devname, char *partno)
{
    if(!strcmp(partno, get_fdv_part_number_by_name(devname)))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
EXPORT_SYMBOL_GPL(fdv_pn_mach);

FDV_SETUP(Camera_main)
FDV_SETUP(Camera_second)
FDV_SETUP(L_Psensor)
FDV_SETUP(Geomagnetic)
FDV_SETUP(G_sensor)
FDV_SETUP(LCD)
FDV_SETUP(NAND)
FDV_SETUP(TP)

//gxh add for phicomm pdata codec end, 2013.03.08

#define MAX_ID_NUM		16
#define MAX_DEVICES_NUM	16

#define MAX_NAME_LEN	32
#define MAX_DESC_LEN	32

struct freecomm_id {
	char manuf_name[MAX_NAME_LEN];
	u32 id;
	unsigned char part_number[12];
	u32 flag;
	char desc[MAX_DESC_LEN];
};

struct freecomm_devices_version {
	char device_name[MAX_NAME_LEN];
	struct freecomm_id freecomm_device_id[MAX_ID_NUM];
	struct list_head    node;
};

static DEFINE_MUTEX(fdv_list_mutex);
static LIST_HEAD(fdv_list);

//static struct freecomm_devices_version fdv[MAX_DEVICES_NUM]; 

/*************************************************************
 *  Example:
 *
 *  struct freecomm_devices_version fdv = {
 *  	.device_name = "PMIC",
 *  	.freecomm_device_id[0] = {
 *  		.manuf_name = "Marvell",
 *  		.id = 0x8606,
 *  	},
 *  	.freecomm_device_id[1] = {
 *  		.manuf_name = "Marvell",
 *  		.id = 0x8607,
 *  	},
 *  };
 *
 ************************************************************/


int register_fdv(char *device_name, char *manuf_name, u32 id)
{
	struct freecomm_devices_version *fdv = NULL;
	int i = 0;

	mutex_lock(&fdv_list_mutex);
	list_for_each_entry(fdv, &fdv_list, node) {
		if (!strcmp(fdv->device_name, device_name)) {
			break;
		}
	}

	if(&(fdv->node) != &fdv_list)	//device name alread in
	{
		//device already exsit, add id
		printk("device already exsit, add id\n");
		//check if id is already in.
		for(i = 0; i < MAX_ID_NUM; i++)
		{
			if(fdv->freecomm_device_id[i].id == id)
				break;
		}
		if(i < MAX_ID_NUM)	//id already in
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
EXPORT_SYMBOL_GPL(register_fdv);

int register_fdv_with_desc(char *device_name, char *manuf_name, u32 id, char *desc)
{
	struct freecomm_devices_version *fdv = NULL;
	int i = 0;

	mutex_lock(&fdv_list_mutex);
	list_for_each_entry(fdv, &fdv_list, node) {
		if (!strcmp(fdv->device_name, device_name)) {
			break;
		}
	}

	if(&(fdv->node) != &fdv_list)	//device name alread in
	{
		//device already exsit, add id
		printk("device already exsit, add id\n");
		//check if id is already in.
		for(i = 0; i < MAX_ID_NUM; i++)
		{
			if(fdv->freecomm_device_id[i].id == id)
				break;
		}
		if(i < MAX_ID_NUM)	//id already in
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
			
			if(strlen(desc) >= MAX_DESC_LEN)
			{
				printk("Error, manuf description is too long, max is %d\n", MAX_DESC_LEN);
				goto err;
			}

			strcpy(fdv->freecomm_device_id[i].manuf_name, manuf_name);
			
			fdv->freecomm_device_id[i].id = id;
			
			strcpy(fdv->freecomm_device_id[i].desc, desc);
			
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

		if(strlen(desc) >= MAX_DESC_LEN)
		{
			printk("Error, manuf description is too long, max is %d\n", MAX_DESC_LEN);
			goto err;
		}
		strcpy(fdv->freecomm_device_id[0].desc, desc);

		list_add_tail(&fdv->node, &fdv_list);

		mutex_unlock(&fdv_list_mutex);
		return 0;
	}

err:
	mutex_unlock(&fdv_list_mutex);
	return -1;
}
EXPORT_SYMBOL_GPL(register_fdv_with_desc);

int register_fdv_with_partno(char *device_name, char *manuf_name, u32 id, char *partno, char *desc)
{
	struct freecomm_devices_version *fdv = NULL;
	int i = 0;

	mutex_lock(&fdv_list_mutex);
	list_for_each_entry(fdv, &fdv_list, node) {
		if (!strcmp(fdv->device_name, device_name)) {
			break;
		}
	}

	if(&(fdv->node) != &fdv_list)	//device name alread in
	{
		//device already exsit, add id
		printk("device already exsit, add id\n");
		//check if id is already in.
		for(i = 0; i < MAX_ID_NUM; i++)
		{
			//if(fdv->freecomm_device_id[i].id == id)
			if(!strcmp(partno, fdv->freecomm_device_id[i].part_number))
				break;
		}
		if(i < MAX_ID_NUM)	//id already in
		{
			//printk("devices id alread register: %s %s 0x%08x\n", 
			//		fdv->device_name, fdv->freecomm_device_id[i].manuf_name, fdv->freecomm_device_id[i].id);
			printk("devices part number alread register: %s %s %s\n", 
					fdv->device_name, fdv->freecomm_device_id[i].manuf_name, fdv->freecomm_device_id[i].part_number);
			goto err;
		}
		else //id not in, insert
		{
			i = 0;
			//while(fdv->freecomm_device_id[i].id)
			while(0 != strlen(fdv->freecomm_device_id[i].part_number))
			{
				i++;
			}
		
			if(strlen(manuf_name) >= MAX_NAME_LEN)
			{
				printk("Error, manuf name is too long, max is 32\n");
				goto err;
			}
			
			if(strlen(desc) >= MAX_DESC_LEN)
			{
				printk("Error, manuf description is too long, max is %d\n", MAX_DESC_LEN);
				goto err;
			}

            if(strlen(partno) >= 12)
			{
				printk("Error, part number is too long, max is 11\n");
				goto err;
			}

			strcpy(fdv->freecomm_device_id[i].manuf_name, manuf_name);
			
			fdv->freecomm_device_id[i].id = id;
			strcpy(fdv->freecomm_device_id[i].part_number, partno);
			
			strcpy(fdv->freecomm_device_id[i].desc, desc);
			
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
		
        if(strlen(partno) >= 12)
		{
			printk("Error, part number is too long, max is 11\n");
			goto err;
		}

		//id
		fdv->freecomm_device_id[0].id = id;
		strcpy(fdv->freecomm_device_id[i].part_number, partno);

		if(strlen(desc) >= MAX_DESC_LEN)
		{
			printk("Error, manuf description is too long, max is %d\n", MAX_DESC_LEN);
			goto err;
		}
		strcpy(fdv->freecomm_device_id[0].desc, desc);

		list_add_tail(&fdv->node, &fdv_list);

		mutex_unlock(&fdv_list_mutex);
		return 0;
	}

err:
	mutex_unlock(&fdv_list_mutex);
	return -1;
}
EXPORT_SYMBOL_GPL(register_fdv_with_partno);
//add by zhll 2012.9.4,start

int confirm_fdv (char *device_name, char *manuf_name, u32 id )
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
EXPORT_SYMBOL_GPL(confirm_fdv);

int confirm_fdv_partno (char *device_name, char *manuf_name, char *partno )
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
	    if(!strcmp(partno, fdv->freecomm_device_id[i].part_number))
	    {
 	        fdv->freecomm_device_id[i].flag = 1;
	           break;
	    }
    } 
	 
	if(i < MAX_ID_NUM)
	{
	     printk("device uses the partno of manuf: %s %s %s\n",
               fdv->device_name,fdv->freecomm_device_id[i].manuf_name,fdv->freecomm_device_id[i].part_number);
	}
	else
	{
	 		printk(KERN_INFO "Error, the part number %s, is not registered!\n", fdv->freecomm_device_id[i].part_number);
	}
    
	mutex_unlock(&fdv_list_mutex);
 
    return 0;	  
	 		
}
EXPORT_SYMBOL_GPL(confirm_fdv_partno);
//add by zhll 2012.9.4,end

static int show_fdv_with_part_number = 0;
static inline int fdv_proc_info(char *buf, struct freecomm_devices_version *this)
{
	int i;
	int len = 0;
	char *ptr = buf;

    if(show_fdv_with_part_number)
    {
        //print dev with part_number first.
	    for(i = 0; i < MAX_ID_NUM; i++)
	    {
	    	if(this->freecomm_device_id[i].id != 0)
	    	{
                if(strlen(this->freecomm_device_id[i].part_number))
                {
	    		    len += sprintf(ptr, "%-15s  %-16s  %-10s  %-2d  %-32s  0x%08x\n",
	    					this->device_name, 
	    					this->freecomm_device_id[i].manuf_name,
	    					this->freecomm_device_id[i].part_number,
	    					this->freecomm_device_id[i].flag,
	    					this->freecomm_device_id[i].desc,
	    					this->freecomm_device_id[i].id);
                }
	    		            
	    		ptr = buf + len;
	    		//printk("%s %s %d\n",
	    		//			this->device_name, 
	    		//			this->freecomm_device_id[i].manuf_name,
	    		//			this->freecomm_device_id[i].id);
	     	} 
	    }
    }
    else
    {
        //then, print dev without part_number
	    for(i = 0; i < MAX_ID_NUM; i++)
	    {
	    	if(this->freecomm_device_id[i].id != 0)
	    	{
                if(!strlen(this->freecomm_device_id[i].part_number))
                {
                    len += sprintf(ptr, "%-15s  %-16s  0x%08x  %-2d  %-32s  0x%08x\n",
	    					this->device_name, 
	    					this->freecomm_device_id[i].manuf_name,
	    					this->freecomm_device_id[i].id,
	    					this->freecomm_device_id[i].flag,
	    					this->freecomm_device_id[i].desc,
	    					this->freecomm_device_id[i].id);
                }
	    		            
	    		ptr = buf + len;
	    		//printk("%s %s %d\n",
	    		//			this->device_name, 
	    		//			this->freecomm_device_id[i].manuf_name,
	    		//			this->freecomm_device_id[i].id);
	     	} 
	    }
    }

	return len;
}

//gxh add
static unsigned int read_done;
static ssize_t fdv_read_proc(struct file *f, char __user *_buf, size_t count, loff_t *off)
//static ssize_t fdv_read_proc(char *page, char **start, off_t off,
//		int count, int *eof, void *data)
{
	struct freecomm_devices_version *fdv = NULL;
	int len = 0, l = 0;
    //off_t   begin = 0;
	//char temp_buf[4096];
	//void *pbuf = &temp_buf[0];
	void *pbuf;

	if(read_done)
		return 0;

	read_done = 1;
	pbuf = kzalloc(4096, GFP_KERNEL);
	if(pbuf == NULL)
	{
		printk("%s: kzalloc failed!\r\n", __func__);
		return 0;
	}

	mutex_lock(&fdv_list_mutex);
    
    //print dev with part_number first.
    show_fdv_with_part_number = 1;
	list_for_each_entry(fdv, &fdv_list, node) {
		l = fdv_proc_info(pbuf + len, fdv);
		len	+= l;
        /*if (len+begin > off+count)
            goto done;
        if (len+begin < off) {
        	begin += len;
        	len = 0;
        }
	*/
	}
    
    //print dev without part_number.
    show_fdv_with_part_number = 0;
	list_for_each_entry(fdv, &fdv_list, node) {
		l = fdv_proc_info(pbuf + len, fdv);
		len	+= l;
        /*if (len+begin > off+count)
            goto done;
        if (len+begin < off) {
        	begin += len;
        	len = 0;
        }*/
	}
    //*eof = 1;

     if(copy_to_user(_buf, pbuf, len) != 0)
	{
		printk("copy_to_user error in %s\r\n", __func__);
	}
	mutex_unlock(&fdv_list_mutex);
	kfree(pbuf);
    return len;
}

static int fdv_open_proc(struct inode *inode, struct file *file)
{
	read_done = 0;
	return 0;
}

static const struct file_operations fdv_proc_fops = {
	 .owner  	 = THIS_MODULE,
	 .open		= fdv_open_proc,
	 .read           = fdv_read_proc,
	};

static void create_fdv_proc_file(void)
{
	struct proc_dir_entry *fdv_proc_file =
//		create_proc_entry("fdv_info", 0644, NULL);
		proc_create("fdv_info", 0644, NULL, &fdv_proc_fops);

	if (!fdv_proc_file) 
//	{
//		fdv_proc_file->read_proc = fdv_read_proc;
//	} else
		printk(KERN_INFO "fdv proc file create failed!\n");
}
//gxh add end

static int __init fdv_init(void)
{
	printk(KERN_INFO "gxh:--------fdv_init\n");
	create_fdv_proc_file();
	
	return 1;
}

static void __exit fdv_exit(void)
{
}

module_init(fdv_init);
module_exit(fdv_exit);

MODULE_DESCRIPTION("freecomm device version");
MODULE_AUTHOR("xinghuan.geng <xinghuan.geng@phicomm.com.cn>");
MODULE_LICENSE("GPL");
