/********************************************
 * dpg0100.c dpg0100 device driver
 * 
 * version 2.13-08
 * 
 * Copyright 2002,2007 Interface Corporation
 * 
 *******************************************/
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <asm/delay.h>
// #include <asm/system.h>
#include <linux/hdreg.h>
#include <linux/slab.h>

#include <linux/init.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	#include <linux/iobuf.h>
#else 
#endif

#include "dpg0100.h"

#define DPG0100_DRIVER_NAME "dpg0100"
#define DPG0100_DRIVER_DESC "Common Device Driver"
#define DPG0100_DRIVER_VERSION "2.80.13.00"

MODULE_DESCRIPTION(DPG0100_DRIVER_DESC " Ver" DPG0100_DRIVER_VERSION);
MODULE_AUTHOR("Interface Corporation <http://www.interface.co.jp>");


#if LINUX_VERSION_CODE >= VERSION(2,4,0)
#if LINUX_VERSION_CODE >= VERSION(2,6,0)
#include <linux/interrupt.h>
#include <linux/pagemap.h>
#include <linux/moduleparam.h>
#endif

#include <linux/init.h>

extern int  wrap_init(void);
extern void wrap_cleanup(void);
module_init(wrap_init);
module_exit(wrap_cleanup);

#else

#define wrap_init init_module
#define wrap_cleanup cleanup_module

#endif

static int dpg0100_version_disp = 1;
#if LINUX_VERSION_CODE < VERSION(2,5,0)
MODULE_PARM(dpg0100_version_disp, "i");
#else
module_param(dpg0100_version_disp, int, 0);
#endif


/* RTLinux */
#ifdef __RTL__
#include <pthread.h>

int gpg_rtl_request_irq(
	unsigned int irq, 
	unsigned int (*handler)(unsigned int irq, struct pt_regs *regs))
{
	return rtl_request_irq(irq, handler);
}
int gpg_rtl_free_irq(unsigned int irq)
{
	return rtl_free_irq(irq);
}
void gpg_rtl_no_interrupts(rtl_irqstate_t state)
{
	rtl_no_interrupts(state);
	return;
}
void gpg_rtl_restore_interrupts(rtl_irqstate_t state)
{
	rtl_restore_interrupts(state);
	return;
}
void gpg_rtl_hard_enable_irq(unsigned int irq)
{
	rtl_hard_enable_irq(irq);
	return;
}
void gpg_rtl_global_pend_irq(int irq)
{
	rtl_global_pend_irq(irq);
	return;
}
int gpg_pthread_suspend_np(pthread_t thread)
{
	return pthread_suspend_np(thread);
}
int gpg_pthread_wakeup_np(pthread_t thread)
{
	return pthread_wakeup_np(thread);
}
#if 0
int gpg_pthread_attr_setfp_np(pthread_attr_t *attr, int use_fp)
{
	return pthread_attr_setfp_np(attr, use_fp);
}
#endif
int gpg_pthread_wait_np(void)
{
	return pthread_wait_np();
}
int gpg_pthread_attr_init(pthread_attr_t *thread)
{
	return pthread_attr_init(thread);
}
int gpg_pthread_attr_destroy(pthread_attr_t *thread)
{
	return pthread_attr_destroy(thread);
}
int gpg_usleep(useconds_t useconds)
{
	return usleep(useconds);
}
int gpg_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	return nanosleep(rqtp, rmtp);
}
#endif /* #ifdef __RTL__ */

/* module version display */
void gpg_version_disp_load(char* name, char* desc, char* ver)
{
    printk("%s:Interface %s (Ver%s) loaded.\n", name, desc, ver);
}

void gpg_version_disp_unload(char* name, char* desc, char* ver)
{
    printk("%s:Interface %s (Ver%s) unloaded.\n", name, desc, ver);
}


/* I/O port access */
unsigned char gpg_inb(unsigned long port)
{
	return inb(port);
}
unsigned short gpg_inw(unsigned long port)
{
	return inw(port);
}
unsigned int gpg_inl(unsigned long port)
{
	return inl(port);
}
void gpg_outb(unsigned char b, unsigned long port)
{ 
	outb(b, port);
	return;
}
void gpg_outw(unsigned short w, unsigned long port)
{
	outw(w, port);
	return;
}
void gpg_outl(unsigned long l, unsigned long port)
{
	outl(l, port);
	return;
}
void gpg_insb(unsigned long port, void *dst, unsigned long count)
{
	insb(port, dst, count);
	return;
}
void gpg_insw(unsigned long port, void *dst, unsigned long count)
{
	insw(port, dst, count);
	return;
}
void gpg_insl(unsigned long port, void *dst, unsigned long count)
{
	insl(port, dst, count);
	return;
}
void gpg_outsb(unsigned long port, const void *src, unsigned long count)
{
#ifdef MIPS
	outsb(port, (void *)src, count);
#else
	outsb(port, src, count);
#endif
	return;
}
void gpg_outsw(unsigned long port, const void *src, unsigned long count)
{
#ifdef MIPS
	outsw(port, (void *)src, count);
#else
	outsw(port, src, count);
#endif
	return;
}
void gpg_outsl(unsigned long port, const void *src, unsigned long count)
{
#ifdef MIPS
	outsl(port, (void *)src, count);
#else
	outsl(port, src, count);
#endif
	return;
}


int gpg_MINOR(void *inode)
{
	return MINOR(((struct inode *)inode)->i_rdev);
}
int gpg_MINOR_f(void *file)
{
	return MINOR(file_inode((struct file*)file)->i_rdev);
	// return MINOR(((struct file *)file)->f_dentry->d_inode->i_rdev);
}
void *gpg_get_file_private_data(void *file)
{
	return ((struct file *)file)->private_data;
}
void gpg_set_file_private_data(void *file, void *private_data)
{
	((struct file *)file)->private_data = private_data;
	return;
}
unsigned long gpg_get_file_f_version(void *file)
{
	return ((struct file *)file)->f_version;
}

/* memory */
void *gpg_vmalloc(unsigned long size)
{
	return vmalloc(size);
}
void gpg_vfree(void *addr)
{
	vfree(addr);
	return;
}
void *gpg_kmalloc(unsigned long size, int flags)
{
	return kmalloc(size, flags);
}
void gpg_kfree(void *addr)
{
	kfree(addr);
	return;
}
unsigned long gpg_copy_from_user(void *to, void *from, unsigned long n)
{
	return copy_from_user(to, from, n);
}
unsigned long gpg_copy_to_user(void *to, void *from, unsigned long n)
{
	return copy_to_user(to, from, n);
}
void *gpg_memcpy(void * to, void * from, unsigned int n)
{
	return memcpy(to, from, (size_t)n);
}
void * gpg_memset(void *s, int c, unsigned int count)
{
	return memset(s, c, (size_t)count);
}
void gpg_memmove(void *dest,const void *src, unsigned int n)
{
	memmove(dest, src, n);
	return;
}
unsigned char gpg_readb(unsigned long port)
{
	return readb((void *)port);
}
unsigned short gpg_readw(unsigned long port)
{
	return readw((void *)port);
}
unsigned int gpg_readl(unsigned long port)
{
	return readl((void *)port);
}
void gpg_writeb(unsigned char b, unsigned long port)
{ 
	writeb(b, (void *)port);
	return;
}
void gpg_writew(unsigned short w, unsigned long port)
{
	writew(w, (void *)port);
	return;
}
void gpg_writel(unsigned long l, unsigned long port)
{
	writel(l, (void *)port);
	return;
}


/* resource */
void gpg_request_region(unsigned int from, unsigned int extent, 
						const char *name)
{
	request_region(from, extent, name);
	return;
}
void gpg_release_region(unsigned int from, unsigned int extent)
{
	release_region(from, extent);
	return;
}
int gpg_check_region(unsigned int from, unsigned int extent)
{
#if LINUX_VERSION_CODE < VERSION(2,5,0)
	return check_region(from, extent);
#else
	return 0;
#endif
}
void gpg_request_mem_region(unsigned int from, unsigned int extent, 
							const char *name)
{
#if LINUX_VERSION_CODE >= VERSION(2,4,0)
	request_mem_region(from, extent, name);
#else
#endif
	return;
}
void gpg_release_mem_region(unsigned int from, unsigned int extent)
{
#if LINUX_VERSION_CODE >= VERSION(2,4,0)
	release_mem_region(from, extent);
#else
#endif
	return;
}
int gpg_check_mem_region(unsigned int from, unsigned int extent)
{
#if LINUX_VERSION_CODE >= VERSION(2,6,0)
	return 0;
#elif LINUX_VERSION_CODE >= VERSION(2,4,0)
	return check_mem_region(from, extent);
#else
	return 0;
#endif
}

#if LINUX_VERSION_CODE >= VERSION(2,6,19)
typedef irqreturn_t (*IRQHANDLER)(int, void *);
#elif LINUX_VERSION_CODE >= VERSION(2,5,0)
typedef irqreturn_t (*IRQHANDLER)(int, void *, struct pt_regs *);
#else
typedef void (*IRQHANDLER)(int, void *, struct pt_regs *);
#endif

#if LINUX_VERSION_CODE >= VERSION(2,6,19)
int gpg_request_irq(unsigned int irq,
					void (*handler)(int, void *), 
					unsigned long flags, char *device, void *dev_id)
#else
int gpg_request_irq(unsigned int irq,
					void (*handler)(int, void *, void *), 
					unsigned long flags, char *device, void *dev_id)
#endif
{
	return request_irq(irq, (IRQHANDLER)handler,
					   flags, device, dev_id);
}
void gpg_free_irq(unsigned int irq, void *dev_id)
{
	free_irq(irq, dev_id);
	return;
}

void *gpg_ioremap(unsigned long offset, unsigned long size)
{
	return ioremap(offset, size);
}
void gpg_iounmap(void *addr)
{
	iounmap(addr);
	return;
}
int gpg_get_pci_dev_irq(void *dev)
{
	return ((struct pci_dev *)dev)->irq;
}
void gpg_set_pci_dev_irq(void *dev, unsigned int data)
{
	((struct pci_dev *)dev)->irq = data;
	return;
}


/* timer */
void gpg_udelay(unsigned int n)
{
	/*
	 * udelay on Linux can have problems for
	 * multi-millisecond waits.  Wait at most
	 * 1024us per call.
	 */
	while(1) {
		if(n > 1024) {
			udelay(1024);
			n -= 1024;
		}
		else {
			udelay(n);
			break;
		}
	}
	return;
}
void gpg_init_timer(void *timer)
{
	init_timer((struct timer_list *)timer);
	return;
}
void gpg_add_timer(void *timer)
{
	add_timer((struct timer_list *)timer);
	return;
}
int gpg_del_timer(void *timer)
{
	return del_timer((struct timer_list *)timer);
}
signed long gpg_schedule_timeout(signed long timeout)
{
	return schedule_timeout(timeout);
}
long gpg_get_HZ(void)
{
	return HZ;
}
unsigned long gpg_get_jiffies(void)
{
	return jiffies;
}


/* synchronization */
long gpg_interruptible_sleep_on_timeout(void *q, long timeout)
{
#if LINUX_VERSION_CODE >= VERSION(2,6,0)
	wait_queue_head_t *wq = (wait_queue_head_t *)q;
	return wait_event_interruptible_timeout((*wq), 0, timeout);
#elif LINUX_VERSION_CODE >= VERSION(2,4,0)
	return interruptible_sleep_on_timeout((wait_queue_head_t *)q, timeout);
#else
	return interruptible_sleep_on_timeout((struct wait_queue **)q, timeout);
#endif
}

#if LINUX_VERSION_CODE >= VERSION(2,4,0)
void gpg_init_waitqueue_head(void *q)
{
	init_waitqueue_head(q);
	return;
}
#endif
void gpg_interruptible_sleep_on(void *q)
{
#if LINUX_VERSION_CODE >= VERSION(2,6,0)
	wait_queue_head_t *wq = (wait_queue_head_t *)q;
	wait_event_interruptible((*wq), 0);
#elif LINUX_VERSION_CODE >= VERSION(2,4,0)
	interruptible_sleep_on((wait_queue_head_t *)q);
#else
	interruptible_sleep_on((struct wait_queue **)q);
#endif
	return;
}
void gpg_wake_up_interruptible(void *q)
{
#if LINUX_VERSION_CODE >= VERSION(2,4,0)
	wake_up_interruptible((wait_queue_head_t *)q);
#else
	wake_up_interruptible((struct wait_queue **)q);
#endif
	return;
}
void gpg_save_flags(long *flags)
{
	unsigned long data;

#if LINUX_VERSION_CODE < VERSION(2,5,0)
	save_flags(data);
#else
	local_irq_save(data);
#endif
	*flags = (long)data;
	return;
}
void gpg_save_flags_cli(long *flags)
{
	unsigned long data;

#if LINUX_VERSION_CODE < VERSION(2,5,0)
	save_flags(data); cli();
#else
	local_irq_save(data);
#endif
	*flags = data;
	return;
}
void gpg_restore_flags(long flags)
{
#if LINUX_VERSION_CODE < VERSION(2,5,0)
	restore_flags(((unsigned long)flags));
#else
	local_irq_restore((unsigned long)flags);
#endif
	return;
}
void gpg_cli(void)
{
#if LINUX_VERSION_CODE < VERSION(2,5,0)
	cli();
#else
	local_irq_disable();
#endif
	return;
}

#ifndef __RTL__
#if LINUX_VERSION_CODE >= VERSION(2,6,0)
int gpg_wait_event_interruptible(void *q, unsigned long *flag)
{
	return wait_event_interruptible(*((wait_queue_head_t *)q), (*flag == 0));
}
void gpg_flag_clear_and_wake_up_interruptible(void *q, unsigned long *flag)
{
	*flag = 0;
	wake_up_interruptible((wait_queue_head_t *)q);
}
#endif
#endif

/* spin_lock */
void gpg_spin_lock_init(void *lock)
{
	spin_lock_init((spinlock_t *)lock);
	return;
}
void gpg_spin_lock(void *lock)
{
	spin_lock((spinlock_t *)lock);
	return;
}
void gpg_spin_unlock(void *lock)
{
	spin_unlock((spinlock_t *)lock);
	return;
}
void gpg_spin_unlock_irqrestore(void *lock, long flags)
{
	spin_unlock_irqrestore((spinlock_t *)lock, ((unsigned long)flags));
	return;
}
void gpg_spin_lock_irqsave(void *lock, long *flags)
{
	unsigned long data = 0x0ul;

	spin_lock_irqsave((spinlock_t *)lock, data);
	*flags = (long)data;
	return;
}

int gpg_fasync_helper(int fd, void *filp, int on, 
					  void **fapp)
{
	return fasync_helper(fd, (struct file *)filp, on,
						 (struct fasync_struct **)fapp);
}

#if LINUX_VERSION_CODE == VERSION(2,2,14) || LINUX_VERSION_CODE == VERSION(2,2,15)
#define KILLFASYNCHASTHREEPARAMETERS
#endif

void gpg_kill_fasync(void *async, int sig, ...)
{
#if LINUX_VERSION_CODE < 0x020315 && !defined(KILLFASYNCHASTHREEPARAMETERS)
	/* The extra parameter to kill_fasync was added in 2.3.21, and is
	   _not_ present in _stock_ 2.2.14 and 2.2.15.  However, some
	   distributions patch 2.2.x kernels to add this parameter.  The
	   Makefile.linux attempts to detect this addition and defines
	   KILLFASYNCHASTHREEPARAMETERS if three parameters are found. */
	if(async) kill_fasync((struct fasync_struct *)async, sig);
#else

	/* Parameter added in 2.3.21. */
#if LINUX_VERSION_CODE < 0x020400
	if(async) kill_fasync((struct fasync_struct *)async, sig, POLL_IN);
#else
	/* Type of first parameter changed in
	   Linux 2.4.0-test2... */
	if(async) kill_fasync((struct fasync_struct **)&async, sig, POLL_IN);
#endif
#endif
	return;
}

/* PCI configuration */
#if  LINUX_VERSION_CODE < VERSION(2,4,0)
#define PCI_ANY_ID (~0)
static struct pci_dev *pci_find_subsys(unsigned int vendor,
									   unsigned int device,
									   unsigned int ss_vendor,
									   unsigned int ss_device,
									   struct pci_dev *from)
{
	unsigned short subsystem_vendor, subsystem_device;

	while ((from = pci_find_device(vendor, device, from))) {
		pci_read_config_word(from, PCI_SUBSYSTEM_VENDOR_ID, 
							 &subsystem_vendor);
		pci_read_config_word(from, PCI_SUBSYSTEM_ID, &subsystem_device);
		if ((ss_vendor == PCI_ANY_ID || subsystem_vendor == ss_vendor) &&
		    (ss_device == PCI_ANY_ID || subsystem_device == ss_device))
			return from;
	}
	return NULL;
}
#elif LINUX_VERSION_CODE > VERSION(2,6,10)
#define pci_find_subsys pci_get_subsys
#endif

void *gpg_pci_find_device(unsigned int vendor, unsigned int device, void *from)
{
#if LINUX_VERSION_CODE >= VERSION(2,6,26)
	return (void *)(pci_get_device(vendor, device, (struct pci_dev *)from));
#else
	return (void *)(pci_find_device(vendor, device, (struct pci_dev *)from));
#endif
}
void *gpg_pci_find_subsys(unsigned int vendor, unsigned int device,
						  unsigned int ss_vendor, unsigned int ss_device,
						  void *from)
{
#if LINUX_VERSION_CODE >= VERSION(2,6,26)
	return (void *)(pci_get_subsys(vendor, device, ss_vendor, ss_device, (struct pci_dev *)from));
#else
	return (void *)(pci_find_subsys(vendor, device, ss_vendor, ss_device, (struct pci_dev *)from));
#endif
}
int gpg_pci_read_config_byte(void *dev, int where, u8 *val)
{
	return pci_read_config_byte((struct pci_dev *)dev, where, val);
}
int gpg_pci_read_config_word(void *dev, int where, u16 *val)
{
	return pci_read_config_word((struct pci_dev *)dev, where, val);
}
int gpg_pci_read_config_dword(void *dev, int where, u32 *val)
{
	return pci_read_config_dword((struct pci_dev *)dev, where, val);
}
int gpg_pci_write_config_byte(void *dev, int where, u8 val)
{
	return pci_write_config_byte((struct pci_dev *)dev, where, val);
}
int gpg_pci_write_config_word(void *dev, int where, u16 val)
{
	return pci_write_config_word((struct pci_dev *)dev, where, val);
}
int gpg_pci_write_config_dword(void *dev, int where, u32 val)
{
	return pci_write_config_dword((struct pci_dev *)dev, where, val);
}
void gpg_pci_set_master(void *dev)
{
	pci_set_master((struct pci_dev *)dev);
}

/************************************************/
/* gpg_probe_pci_device                         */
/************************************************/
void *gpg_probe_pci_device(unsigned int vendor, unsigned int device, 
						   unsigned int ss_vendor, unsigned int ss_device, 
						   int rsw, int boardid)
{
	struct pci_dev *dev = NULL;
	unsigned int base_adr;
	int RswNo;
	int baseoffset, portoffset;

	baseoffset = (rsw >> 8) & 0xFF;
	portoffset = rsw & 0xFF;

	printk("search device=%x, boardid=%x\n", device, boardid);

	while((dev = pci_find_subsys(vendor, device, ss_vendor, 
								 ss_device, dev))) {
		pci_read_config_dword(dev, PCI_BASE_ADDRESS_0 + baseoffset, 
							  &base_adr);
		base_adr &= 0xfffe;
		RswNo = inb(base_adr + portoffset) & 0xf;

		printk("found device=%x, RSW=%x\n", device, RswNo);
		if (RswNo==boardid) return dev;
	}

	return NULL;
}

int gpg_get_pci_device_info(struct pci_dev *dev, PPCI_CONFIG ppcidata)
{
	int i;
	unsigned int Data[64];

	for( i=0; i < 64; i++) {
		pci_read_config_dword( dev, i*4, &Data[i]);
	}

	ppcidata->VendorID   = Data[0] & 0x0000FFFF;
	ppcidata->DeviceID   = (Data[0] >> 16) & 0x0000FFFF;
	ppcidata->Command    = Data[1] & 0x0000FFFF;
	ppcidata->StatusReg  = (Data[1] >> 16) & 0x0000FFFF;
	ppcidata->RevisionID = Data[2] & 0x000000FF;
	ppcidata->ProgIf     = (Data[2] >> 8) & 0x000000FF;
	ppcidata->SubClass   = (Data[2] >> 16) & 0x000000FF;
	ppcidata->BaseClass  = (Data[2] >> 24) & 0x000000FF;
	ppcidata->CacheLineSize = Data[3] & 0x000000FF;
	ppcidata->LatencyTimer  = (Data[3] >> 8) & 0x000000FF;
	ppcidata->HeaderType    = (Data[3] >> 16) & 0x000000FF;
	ppcidata->BIST          = (Data[3] >> 24) & 0x000000FF;

	for( i=0; i < 6; i++) {
		ppcidata->BaseAddresses[i] = Data[4+i];
	}

	ppcidata->CIS = Data[10];
	ppcidata->SubVendorID = Data[11] & 0x0000FFFF;
	ppcidata->SubSystemID = (Data[11] >> 16) & 0x0000FFFF;
	ppcidata->ROMBaseAddress = Data[12];
	ppcidata->CapabilitiesPtr = Data[13] & 0x000000FF;
	ppcidata->Reserved1[0] = (Data[13] >> 8) & 0x000000FF;
	ppcidata->Reserved1[1] = (Data[13] >> 16) & 0x000000FF;
	ppcidata->Reserved1[2] = (Data[13] >> 24) & 0x000000FF;
	ppcidata->Reserved2[0] = Data[14];
	ppcidata->InterruptLine = Data[15] & 0x000000FF;
	ppcidata->InterruptPin = (Data[15] >> 8) & 0x000000FF;
	ppcidata->MinimumGrant = (Data[15] >> 16) & 0x000000FF;
	ppcidata->MaximumLatency = (Data[15] >> 24) & 0x000000FF;

	for(i=0; i<48; i++) {
		ppcidata->DeviceSpecific[i*4] = Data[i+16] & 0x000000FF;
		ppcidata->DeviceSpecific[i*4+1] = (Data[i+16] >> 8) & 
			0x000000FF;
		ppcidata->DeviceSpecific[i*4+2] = (Data[i+16] >> 16) & 
			0x000000FF;
		ppcidata->DeviceSpecific[i*4+3] = (Data[i+16] >> 24) & 
			0x000000FF;
	}

	return 0;
}

int gpg_printk(const char *fmt, ...)
{
	char print_buf[992];

	va_list args;

	va_start(args, fmt);
	vsprintf(print_buf, fmt, args);
	va_end(args);

	return printk(print_buf);
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)

#define gpg_page_to_bus(page)      (((page) - mem_map) << PAGE_SHIFT)

/************************************************/
/* gpg_get_bus_address                          */
/************************************************/
unsigned long gpg_get_bus_address(void *iobuf,int index, unsigned long offset)
{
	struct kiobuf *kiobuf = (struct kiobuf *)iobuf;
	struct page *page;
	
	page = kiobuf->maplist[index];
	
	return gpg_page_to_bus(page) + offset;
}

/************************************************/
/* gpg_unmap_free_iobuf                         */
/************************************************/
void gpg_unmap_free_iobuf(int number,void **piobuf)
{
	struct kiobuf **kiobuf = (struct kiobuf **)piobuf;
	struct page *page;
	int pageno;
	
	for(pageno = 0; pageno < (*kiobuf)->nr_pages; pageno++){
		page = (*kiobuf)->maplist[pageno];
	}
	unmap_kiobuf(*kiobuf);
	free_kiovec(number,kiobuf);
}

#else 
#endif


/* ide */
#if LINUX_VERSION_CODE < VERSION(2,5,0)
int gpg_ide_register(int io_port, int ctl_port, int irq)
{
	return ide_register(io_port, ctl_port, irq);
}
void gpg_ide_unregister(unsigned int index)
{
	ide_unregister(index);
	return ;
}
#endif

/* board ID */
static unsigned long pci_inportb(PPCIACCESS_CONFIG Conf)
{
	unsigned long data = 0;
	
	switch(Conf->AccessType){
	case 0:	/* IO */
		data = (unsigned long)inb(Conf->Address);
		break;
	case 1: /* Memory */
		data = (unsigned long)readb((void *)Conf->Address);
		break;
	case 2: /* PCI configuration */
	default:
		pci_read_config_byte((struct pci_dev *)Conf->PciDevice, 0xf8,
							 (u8 *)&data);
		break;
	}

    return data;
}
static void pci_outportb(PPCIACCESS_CONFIG Conf, unsigned long data)
{
	switch(Conf->AccessType) {
	case 0: /* IO */
		 outb(data, Conf->Address);
		 break;
	case 1: /* Memory */
		writeb(data, (void *)Conf->Address);
		break;
	case 2: /* PCI configuration */
	default:
		pci_write_config_byte((struct pci_dev *)Conf->PciDevice, 0xf8,
							 (u8)data);
		break;
	}
}
unsigned long gpg_read_pcidevice_data(PPCIACCESS_CONFIG Conf,
									  unsigned long Offset)
{
	int          i;
	unsigned int ret;
	unsigned long Data;
	unsigned long shift = Conf->BitShift;

	ret = 0x0000;
	/* DTRD */                         /*  CS  SK  DI  */
	pci_outportb(Conf, (0 << shift));  /*  0   0   0   */
	pci_outportb(Conf, (2 << shift));  /*  0   1   0   */
	pci_outportb(Conf, (4 << shift));  /*  1   0   0   */
	pci_outportb(Conf, (4 << shift));  /*  1   0   0   W0  STRAT BIT   */
	pci_outportb(Conf, (6 << shift));  /*  1   1   0                   */
	pci_outportb(Conf, (5 << shift));  /*  1   0   1   W1  */
	pci_outportb(Conf, (7 << shift));  /*  1   1   1       */
	pci_outportb(Conf, (5 << shift));  /*  1   0   1   W1  OP(1) READ */
	pci_outportb(Conf, (7 << shift));  /*  1   1   1       */
	pci_outportb(Conf, (4 << shift));  /*  1   0   0   W0  OP(0) */
	pci_outportb(Conf, (6 << shift));  /*  1   1   0       */

	for(i = 5; i >= 0; i--) {                 /* ADDRESS  WRITE   */
		if(((Offset >> i) & 1) == 0) {
			pci_outportb(Conf, (4 << shift)); /*  1   0   0   W0  */
			pci_outportb(Conf, (6 << shift)); /*  1   1   0       */
		}
		else {
			pci_outportb(Conf, (5 << shift)); /*  1   0   1   W1  */
			pci_outportb(Conf, (7 << shift)); /*  1   1   1       */
		}
	}

	for(i = 7; i >= 0; i--) {                 /* DATA  READ   (D15-D8)    */
		pci_outportb(Conf, (4 << shift));     /*  1   0   0   W0  */
		pci_outportb(Conf, (6 << shift));     /*  1   1   0       */
		Data = (pci_inportb(Conf) >> shift) & 0x01;
		ret <<= 1;
		ret += Data;
	}
	for(i = 7; i >= 0; i--) {                 /* DATA  READ   (D7 -D0)    */
		pci_outportb(Conf, (4 << shift));     /*  1   0   0   W0  */
		pci_outportb(Conf, (6 << shift));     /*  1   1   0       */
		Data = (pci_inportb(Conf) >> shift) & 0x01;
		ret <<= 1;
		ret += Data;
	}
	pci_outportb(Conf, (0 << shift));

	return ret;
}

#if LINUX_VERSION_CODE >= VERSION(2,5,0)
void *page_malloc(unsigned long size)
{
	void *address;
	u32 order, align;

	for(order = 0, align = PAGE_SIZE; align < size; order++, align <<= 1);
	address = (void *)__get_free_pages(GFP_ATOMIC, order);
	return address;
}
void page_free(void *address, unsigned long size)
{
	u32 order, align;
	if(address == NULL) {
		return;
	}
	for(order = 0, align = PAGE_SIZE; align < size; order++, align <<= 1);
	free_pages((unsigned long)address, order);
}
int lock_memory(PMDL_OBJECT mdl, unsigned long Buffer, unsigned long Length,
	unsigned long Direction)
{
	u32 first, last;
	unsigned long i;
	int  ret;

	if(mdl->PageList) {
		return -EINVAL;
	}

	/* allocate MDL */
	first = (Buffer & PAGE_MASK) >> PAGE_SHIFT;
	last  = ((Buffer + Length - 1) & PAGE_MASK) >> PAGE_SHIFT;

	mdl->ByteOffset = Buffer & ~PAGE_MASK;
	mdl->ByteCount  = Length;
	mdl->StartVa    = Buffer;
	mdl->NumberOfPages = last - first + 1;
	mdl->PageList = (unsigned long *)kmalloc(
		mdl->NumberOfPages * sizeof(unsigned long),
		GFP_ATOMIC);
	if(mdl->PageList == NULL) {
		return -ENOMEM;
	}

	down_read(&current->mm->mmap_sem);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
	ret = get_user_pages((Buffer & PAGE_MASK),
		mdl->NumberOfPages, (Direction ? FOLL_WRITE : 0) | FOLL_FORCE,
		(struct page **)mdl->PageList, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0)
	ret = get_user_pages((Buffer & PAGE_MASK),
		mdl->NumberOfPages, Direction, 1,
		(struct page **)mdl->PageList, NULL);
#else
	ret = get_user_pages(current, current->mm, (Buffer & PAGE_MASK),
		mdl->NumberOfPages, Direction, 1,
		(struct page **)mdl->PageList, NULL);
#endif
	up_read(&current->mm->mmap_sem);
	if(ret < mdl->NumberOfPages) {
		/* failed to lock pages */
		if(ret > 0) {
			for(i = 0; i < ret; i++) {
				page_cache_release((struct page *)mdl->PageList[i]);
			}
			kfree(mdl->PageList);
			mdl->PageList = NULL;
		}
		return -ENOMEM;
	}

	for(i = 0; i < mdl->NumberOfPages; i++) {
		flush_dcache_page((struct page *)mdl->PageList[i]);
	}
	return 0;
}
int unlock_memory(PMDL_OBJECT mdl)
{
	unsigned long i;

	if(mdl->PageList) {
		for(i = 0; i < mdl->NumberOfPages; i++) {
			if(!PageReserved((struct page *)mdl->PageList[i])) {
				SetPageDirty((struct page *)mdl->PageList[i]);
			}
			page_cache_release((struct page *)mdl->PageList[i]);
		}
		kfree(mdl->PageList);
		mdl->PageList = NULL;
	}
	return 0;
}
int create_scatterlist(PMDL_OBJECT mdl)
{
	int i;
	struct scatterlist *LinuxSg;
	unsigned long length = mdl->ByteCount;

	if(mdl->PageList[0] == 0) {
		return -ENOMEM;
	}
//	if(PageHighMem((struct page *)mdl->PageList[0])) {
//		return -ENOMEM;
//	}

	mdl->LinuxSg =
		page_malloc(sizeof(struct scatterlist) * mdl->NumberOfPages);
	if(mdl->LinuxSg == NULL) {
		return -ENOMEM;
	}
	memset(mdl->LinuxSg, 0, sizeof(struct scatterlist) * mdl->NumberOfPages);

	LinuxSg = mdl->LinuxSg;
	if(mdl->NumberOfPages > 1) {
#if LINUX_VERSION_CODE >= VERSION(2,6,24)
		sg_set_page(&LinuxSg[0], (struct page *)mdl->PageList[0], PAGE_SIZE - LinuxSg[0].offset, mdl->ByteOffset);
#else
		LinuxSg[0].page   = (struct page *)mdl->PageList[0];
		LinuxSg[0].offset = mdl->ByteOffset;
		LinuxSg[0].length = PAGE_SIZE - LinuxSg[0].offset;
#endif
		length -= LinuxSg[0].length;
		for(i = 1; i < mdl->NumberOfPages; i++) {
			if(mdl->PageList[i] == 0) {
				goto nopage;
			}
//			if(PageHighMem((struct page *)mdl->PageList[i])) {
//				goto nopage;
//			}
#if LINUX_VERSION_CODE >= VERSION(2,6,24)
			sg_set_page(&LinuxSg[i], (struct page *)mdl->PageList[i], ((length < PAGE_SIZE) ? length : PAGE_SIZE), 0);
#else
			LinuxSg[i].page   = (struct page *)mdl->PageList[i];
			LinuxSg[i].length = ((length < PAGE_SIZE) ? length : PAGE_SIZE);
#endif
			length -= PAGE_SIZE;
		}
	}
	else {
#if LINUX_VERSION_CODE >= VERSION(2,6,24)
		sg_set_page(&LinuxSg[0], (struct page *)mdl->PageList[0], length, mdl->ByteOffset);
#else
		LinuxSg[0].page   = (struct page *)mdl->PageList[0];
		LinuxSg[0].offset = mdl->ByteOffset;
		LinuxSg[0].length = length;
#endif
	}
	return 0;

nopage:
	page_free(LinuxSg, sizeof(struct scatterlist) * mdl->NumberOfPages);
	return -ENOMEM;
}
int gpg_get_scatter_list(void *pcidev, PMDL_OBJECT mdl, void *Buffer,
	unsigned long Length, unsigned long Direction, PSCATTER_LIST Sg)
{
	int ret;
	unsigned long i;

	Sg->Direction = Direction;
	ret = lock_memory(mdl, (unsigned long)Buffer, Length, Direction);
	if(ret) {
		return ret;
	}
	ret = create_scatterlist(mdl);
	if(ret) {
		return ret;
	}

	pci_map_sg((struct pci_dev *)pcidev, mdl->LinuxSg,
		mdl->NumberOfPages, Sg->Direction);
	Sg->Elements = page_malloc(
		sizeof(SCATTER_ELEMENT) * mdl->NumberOfPages);
	if(Sg->Elements == NULL) {
		return -ENOMEM;
	}
	memset(Sg->Elements, 0, sizeof(SCATTER_ELEMENT) * mdl->NumberOfPages);

	Sg->NumberOfElements = mdl->NumberOfPages;
	for(i = 0; i < mdl->NumberOfPages; i++) {
		Sg->Elements[i].address = mdl->LinuxSg[i].dma_address;
		Sg->Elements[i].length  = mdl->LinuxSg[i].length;
	}
	return ret;
}
int gpg_put_scatter_list(void *pcidev, PMDL_OBJECT mdl, PSCATTER_LIST Sg)
{
	pci_unmap_sg((struct pci_dev *)pcidev, mdl->LinuxSg,
		mdl->NumberOfPages, Sg->Direction);

	/* free Linux scatterlist */
	page_free(mdl->LinuxSg,
		sizeof(struct scatterlist) * Sg->NumberOfElements);
	unlock_memory(mdl);

	/* free SG_ELEMENT */
	page_free(Sg->Elements, sizeof(SCATTER_ELEMENT) * Sg->NumberOfElements);
	Sg->Elements = NULL;
	return 0;
}
#endif

/************************************************/
/* export list                                  */
/************************************************/
#ifdef __RTL__
EXPORT_SYMBOL(gpg_rtl_request_irq);
EXPORT_SYMBOL(gpg_rtl_free_irq);
EXPORT_SYMBOL(gpg_rtl_no_interrupts);
EXPORT_SYMBOL(gpg_rtl_restore_interrupts);
EXPORT_SYMBOL(gpg_rtl_hard_enable_irq);
EXPORT_SYMBOL(gpg_rtl_global_pend_irq);
EXPORT_SYMBOL(gpg_pthread_suspend_np);
EXPORT_SYMBOL(gpg_pthread_wakeup_np);
EXPORT_SYMBOL(gpg_pthread_wait_np);
EXPORT_SYMBOL(gpg_pthread_attr_init);
EXPORT_SYMBOL(gpg_pthread_attr_destroy);
EXPORT_SYMBOL(gpg_usleep);
EXPORT_SYMBOL(gpg_nanosleep);
#endif

EXPORT_SYMBOL(gpg_version_disp_load);
EXPORT_SYMBOL(gpg_version_disp_unload);
EXPORT_SYMBOL(gpg_inb);
EXPORT_SYMBOL(gpg_inw);
EXPORT_SYMBOL(gpg_inl);
EXPORT_SYMBOL(gpg_outb);
EXPORT_SYMBOL(gpg_outw);
EXPORT_SYMBOL(gpg_outl);
EXPORT_SYMBOL(gpg_insb);
EXPORT_SYMBOL(gpg_insw);
EXPORT_SYMBOL(gpg_insl);
EXPORT_SYMBOL(gpg_outsb);
EXPORT_SYMBOL(gpg_outsw);
EXPORT_SYMBOL(gpg_outsl);
EXPORT_SYMBOL(gpg_MINOR);
EXPORT_SYMBOL(gpg_MINOR_f);
EXPORT_SYMBOL(gpg_get_file_private_data);
EXPORT_SYMBOL(gpg_set_file_private_data);
EXPORT_SYMBOL(gpg_get_file_f_version);
EXPORT_SYMBOL(gpg_vmalloc);
EXPORT_SYMBOL(gpg_vfree);
EXPORT_SYMBOL(gpg_kmalloc);
EXPORT_SYMBOL(gpg_kfree);
EXPORT_SYMBOL(gpg_copy_from_user);
EXPORT_SYMBOL(gpg_copy_to_user);
EXPORT_SYMBOL(gpg_memcpy);
EXPORT_SYMBOL(gpg_memset);
EXPORT_SYMBOL(gpg_memmove);
EXPORT_SYMBOL(gpg_readb);
EXPORT_SYMBOL(gpg_readw);
EXPORT_SYMBOL(gpg_readl);
EXPORT_SYMBOL(gpg_writeb);
EXPORT_SYMBOL(gpg_writew);
EXPORT_SYMBOL(gpg_writel);
EXPORT_SYMBOL(gpg_request_region);
EXPORT_SYMBOL(gpg_release_region);
EXPORT_SYMBOL(gpg_check_region);
EXPORT_SYMBOL(gpg_request_mem_region);
EXPORT_SYMBOL(gpg_release_mem_region);
EXPORT_SYMBOL(gpg_check_mem_region);
EXPORT_SYMBOL(gpg_request_irq);
EXPORT_SYMBOL(gpg_free_irq);

EXPORT_SYMBOL(gpg_ioremap);
EXPORT_SYMBOL(gpg_iounmap);
EXPORT_SYMBOL(gpg_get_pci_dev_irq);
EXPORT_SYMBOL(gpg_set_pci_dev_irq);
EXPORT_SYMBOL(gpg_udelay);
EXPORT_SYMBOL(gpg_init_timer);
EXPORT_SYMBOL(gpg_add_timer);
EXPORT_SYMBOL(gpg_del_timer);
EXPORT_SYMBOL(gpg_schedule_timeout);
EXPORT_SYMBOL(gpg_get_HZ);
EXPORT_SYMBOL(gpg_get_jiffies);
EXPORT_SYMBOL(gpg_interruptible_sleep_on_timeout);
#if LINUX_VERSION_CODE >= VERSION(2,4,0)
EXPORT_SYMBOL(gpg_init_waitqueue_head);
#endif
EXPORT_SYMBOL(gpg_interruptible_sleep_on);
EXPORT_SYMBOL(gpg_wake_up_interruptible);
EXPORT_SYMBOL(gpg_save_flags);
EXPORT_SYMBOL(gpg_save_flags_cli);
EXPORT_SYMBOL(gpg_restore_flags);
EXPORT_SYMBOL(gpg_cli);
#ifndef __RTL__
#if LINUX_VERSION_CODE >= VERSION(2,6,0)
EXPORT_SYMBOL(gpg_wait_event_interruptible);
EXPORT_SYMBOL(gpg_flag_clear_and_wake_up_interruptible);
#endif
#endif
EXPORT_SYMBOL(gpg_spin_lock_init);
EXPORT_SYMBOL(gpg_spin_lock);
EXPORT_SYMBOL(gpg_spin_unlock);
EXPORT_SYMBOL(gpg_spin_unlock_irqrestore);
EXPORT_SYMBOL(gpg_spin_lock_irqsave);
EXPORT_SYMBOL(gpg_fasync_helper);
EXPORT_SYMBOL(gpg_kill_fasync);
EXPORT_SYMBOL(gpg_pci_find_device);
EXPORT_SYMBOL(gpg_pci_find_subsys);
EXPORT_SYMBOL(gpg_pci_read_config_byte);
EXPORT_SYMBOL(gpg_pci_read_config_word);
EXPORT_SYMBOL(gpg_pci_read_config_dword);
EXPORT_SYMBOL(gpg_pci_write_config_byte);
EXPORT_SYMBOL(gpg_pci_write_config_word);
EXPORT_SYMBOL(gpg_pci_write_config_dword);
EXPORT_SYMBOL(gpg_pci_set_master);
EXPORT_SYMBOL(gpg_probe_pci_device);
EXPORT_SYMBOL(gpg_get_pci_device_info);
EXPORT_SYMBOL(gpg_printk);
EXPORT_SYMBOL(gpg_read_pcidevice_data);
#if LINUX_VERSION_CODE < VERSION(2,5,0)
EXPORT_SYMBOL(gpg_ide_register);
EXPORT_SYMBOL(gpg_ide_unregister);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
EXPORT_SYMBOL(gpg_get_bus_address);
EXPORT_SYMBOL(gpg_unmap_free_iobuf);
#else 
#endif

#if LINUX_VERSION_CODE >= VERSION(2,5,0)
EXPORT_SYMBOL(gpg_get_scatter_list);
EXPORT_SYMBOL(gpg_put_scatter_list);
#endif

#if LINUX_VERSION_CODE == VERSION(2,6,15)
const char *print_tainted(void) { return NULL; }
#endif


int wrap_init(void)
{
    if(dpg0100_version_disp){
	printk("%s:Interface %s (Ver%s) loaded.\n", 
	       DPG0100_DRIVER_NAME, 
	       DPG0100_DRIVER_DESC,
	       DPG0100_DRIVER_VERSION);
    }

	return 0;
}
void wrap_cleanup(void)
{
    if(dpg0100_version_disp){
	printk("%s:Interface %s (Ver%s) unloaded.\n", 
	       DPG0100_DRIVER_NAME, 
	       DPG0100_DRIVER_DESC,
	       DPG0100_DRIVER_VERSION);
    }
	return;
}

