/*
 * header file for DPG-0100 module
 *
 * Copyright 2002, 2007 Interface Corporation
 */
#if !defined( _DPG0100_H_ )
#define _DPG0100_H_

#define VERSION(v,p,s) (((v)<<16)+(p<<8)+s)

// -----------------------------------------------------------------------------
//	constants
// -----------------------------------------------------------------------------
#define IFDRV_MODULE_AUTHOR "Interface Corporation <http://www.interface.co.jp>"

#define DMA_FROMDEVICE	PCI_DMA_FROMDEVICE
#define DMA_TODEVICE	PCI_DMA_TODEVICE

//MODULE ALIAS for PEX292144: VEN_1147/DEV_0B69
MODULE_ALIAS("pci:v00001147d00000B69sv*sd*bc*sc*i*");
MODULE_LICENSE("Proprietary");

// -----------------------------------------------------------------------------
//	structs
// -----------------------------------------------------------------------------
typedef struct _SCATTER_ELEMENT {
	unsigned long	address;
	unsigned long	length;
} SCATTER_ELEMENT, *PSCATTER_ELEMENT;

typedef struct _SCATTER_LIST {
	unsigned long		NumberOfElements;
	unsigned long		Direction;
	PSCATTER_ELEMENT	Elements;
} SCATTER_LIST, *PSCATTER_LIST;

typedef struct _PCIACCESS_CONFIG {
	unsigned long	AccessType;  /* 0:I/O 1:Memory 2:PciCfg */
	unsigned long	BitShift;    /* access bit */
	unsigned long	Address;     /* access address */
	void			*PciDevice;  /* pci_dev struct(2 only) */
} PCIACCESS_CONFIG, *PPCIACCESS_CONFIG;

typedef struct _MDL_OBJECT {
	unsigned long		StartVa;
	unsigned long		ByteCount;
	unsigned long		ByteOffset;
	unsigned long		NumberOfPages;
	struct scatterlist	*LinuxSg;
	unsigned long		*PageList;
} MDL_OBJECT, *PMDL_OBJECT;


#if !defined PCI_CONFIG_STRUCT
#define PCI_CONFIG_STRUCT
typedef struct _PCI_CONFIG {
	unsigned short	VendorID;		    // 00: Vemdor ID
	unsigned short	DeviceID;		    // 02: Device ID
	unsigned short	Command;		    // 04: Command
	unsigned short	StatusReg;		    // 06: Status
	unsigned char	RevisionID;		    // 08: Revision
	unsigned char	ProgIf;			    // 09: ProgIf
	unsigned char	SubClass;		    // 0A: Sub Class
	unsigned char	BaseClass;		    // 0B: Base
	unsigned char	CacheLineSize;		// 0C: Cache Line
	unsigned char	LatencyTimer;		// 0D: Latency
	unsigned char	HeaderType;		    // 0E: Header;
	unsigned char	BIST;			    // 0F: BIST
	unsigned long	BaseAddresses[6];	// 10: Base Address
	unsigned long	CIS;			    // 28: CIS Pointer
	unsigned short	SubVendorID;		// 2C: Subsystem Vendor ID
	unsigned short	SubSystemID;		// 2E: Subsystem ID
	unsigned long	ROMBaseAddress;		// 30: ROM Base Address
	unsigned char	CapabilitiesPtr;	// 34: Capabilities Pointer
	unsigned char	Reserved1[3];		// 35: Reserved
	unsigned long	Reserved2[1];		// 38: Reserved
	unsigned char	InterruptLine;		// 3C: INT Line
	unsigned char	InterruptPin;		// 3D: INT Pin
	unsigned char	MinimumGrant;		// 3E: MIN_GNT
	unsigned char	MaximumLatency;		// 3F: MAX_LAT
        unsigned char	DeviceSpecific[192];	// 40:
} PCI_CONFIG, *PPCI_CONFIG;
#endif

/*
 * 3.19 API change
 * struct access f->f_dentry->d_inode was replaced by accessor function
 * file_inode(f)
 */
#if(LINUX_VERSION_CODE < VERSION(3,9,0))
static inline struct inode *file_inode(const struct file *f)
{
	return (f->f_dentry->d_inode);
}
#endif /* HAVE_FILE_INODE */

/*
 * 4.6 API change
 * Introduces a new get_user_pages() variant: get_user_pages_remote()
 */
#if LINUX_VERSION_CODE >= VERSION(4, 6, 0)
// # define get_user_pages get_user_pages_remote
# ifndef page_cache_release
#  define page_cache_release(page)        put_page(page)
# endif
#endif

// -----------------------------------------------------------------------------
//	function prototypes
// -----------------------------------------------------------------------------
#ifdef __RTL__
int gpg_rtl_free_irq(unsigned int irq);
void gpg_rtl_hard_enable_irq(unsigned int irq);
void gpg_rtl_global_pend_irq(int irq);
int gpg_pthread_wait_np(void);

#ifdef __WRAP__
#include <rtl_core.h>
#include <rtl_sched.h>

int gpg_rtl_request_irq(
	unsigned int irq, 
	unsigned int (*handler)(unsigned int, struct pt_regs *));
void gpg_rtl_no_interrupts(rtl_irqstate_t state);
void gpg_rtl_restore_interrupts(rtl_irqstate_t state);
int gpg_pthread_suspend_np(pthread_t thread);
int gpg_pthread_wakeup_np(pthread_t thread);
int gpg_pthread_attr_setfp_np(pthread_attr_t *attr, int use_fp);
int gpg_pthread_attr_init(pthread_attr_t *thread);
int gpg_pthread_attr_destroy(pthread_attr_t *thread);
int gpg_usleep(useconds_t useconds);
int gpg_nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
#endif /* __WRAP__  */
#endif /* __RTL__ */

void gpg_version_disp_load(char* name, char* desc, char* ver);
void gpg_version_disp_unload(char* name, char* desc, char* ver);

unsigned char gpg_inb(unsigned long port);
unsigned short gpg_inw(unsigned long port);
unsigned int gpg_inl(unsigned long port);
void gpg_outb(unsigned char b, unsigned long port);
void gpg_outw(unsigned short w, unsigned long port);
void gpg_outl(unsigned long l, unsigned long port);
void *gpg_vmalloc(unsigned long size);
void gpg_vfree(void *);
void *gpg_kmalloc(unsigned long size, int flags);
void gpg_kfree(void *);
void *gpg_ioremap(unsigned long offset, unsigned long size);
void gpg_iounmap(void *addr);
void gpg_save_flags(long *flags);
void gpg_save_flags_cli(long *flags);
void gpg_restore_flags (long flags);
void gpg_init_waitqueue_head(void *q);
void gpg_interruptible_sleep_on(void *q);
long gpg_interruptible_sleep_on_timeout(void *q, long timeout);
void gpg_wake_up_interruptible (void *q);
long gpg_get_HZ(void);
unsigned long gpg_get_jiffies(void);
signed long gpg_schedule_timeout(signed long timeout);


int gpg_MINOR(void *);
int gpg_MINOR_f(void *file);
void *gpg_get_file_private_data(void* file);
void gpg_set_file_private_data(void* file, void *private_date);
unsigned long gpg_get_file_f_version(void* file);
#if LINUX_VERSION_CODE >= VERSION(2,6,19)
int gpg_request_irq(unsigned int irq, void (*handler)(int,void*),
				    unsigned long flags, char *device, void *dev_id);
#else
int gpg_request_irq(unsigned int irq, void (*handler)(int,void*,void*),
				    unsigned long flags, char *device, void *dev_id);
#endif

void *gpg_memcpy(void * to, void * from, unsigned int n);
void *gpg_memset(void *s, int c, unsigned int count);

void gpg_udelay(unsigned int n);
void gpg_init_timer(void * timer);
void gpg_add_timer(void * timer);
int  gpg_del_timer(void * timer);
#ifndef __RTL__
#if LINUX_VERSION_CODE >= VERSION(2,6,0)
int gpg_wait_event_interruptible(void *q, unsigned long *flag);
void gpg_flag_clear_and_wake_up_interruptible(void *q, unsigned long *flag);
#endif
#endif

void gpg_spin_lock_init(void *lock);

void gpg_spin_lock(void *lock);
void gpg_spin_unlock(void *lock);
void gpg_spin_lock_irqsave(void *lock, long *flags);
void gpg_spin_unlock_irqrestore(void *lock, long flags);

int gpg_fasync_helper(int fd, void *filp, int on, void ** fapp);
void gpg_kill_fasync(void *async, int sig, ...);

void *gpg_pci_find_device(unsigned int vendor, unsigned int device,
						  void *from);
void *gpg_pci_find_subsys(unsigned int vendor, unsigned int device,
						  unsigned int ss_vendor, unsigned int ss_device,
						  void *from);
int gpg_pci_read_config_byte(void *dev, int where, unsigned char *val);
int gpg_pci_read_config_word(void *dev, int where, unsigned short *val);
int gpg_pci_read_config_dword(void *dev, int where, unsigned int *val);
int gpg_pci_write_config_byte(void *dev, int where, unsigned char val);
int gpg_pci_write_config_word(void *dev, int where, unsigned short val);
int gpg_pci_write_config_dword(void *dev, int where, unsigned int val);

void *gpg_probe_pci_device(unsigned int vendor, unsigned int device,
						   unsigned int ss_vendor, unsigned int ss_device,
						   int rsw, int boardid);
int  gpg_get_pci_dev_irq(void *dev);
void gpg_set_pci_dev_irq(void *dev, unsigned int data);


unsigned long gpg_copy_from_user(void *to, void *from, unsigned long n);
unsigned long gpg_copy_to_user(void *to, void *from, unsigned long n);
int gpg_check_region(unsigned int from, unsigned int extent);
void gpg_request_region(unsigned int from, unsigned int extent, 
						const char *name);
void gpg_release_region(unsigned int from, unsigned int extent);

void gpg_free_irq(unsigned int irq, void *dev_id);
int gpg_printk(const char *fmt, ...);

int gpg_ide_register(int io_port, int ctl_port, int irq);
void gpg_ide_unregister(unsigned int index);

void gpg_memmove(void *dest,const void *src, unsigned int n);

void gpg_insb(unsigned long port, void *dst, unsigned long count);
void gpg_insw(unsigned long port, void *dst, unsigned long count);
void gpg_insl(unsigned long port, void *dst, unsigned long count);
void gpg_outsb(unsigned long port, const void *src, unsigned long count);
void gpg_outsw(unsigned long port, const void *src, unsigned long count);
void gpg_outsl(unsigned long port, const void *src, unsigned long count);
unsigned long gpg_read_pcidevice_data(PPCIACCESS_CONFIG, unsigned long);

unsigned char gpg_readb(unsigned long port);
unsigned short gpg_readw(unsigned long port);
unsigned int gpg_readl(unsigned long port);
void gpg_writeb(unsigned char b, unsigned long port);
void gpg_writew(unsigned short w, unsigned long port);
void gpg_writel(unsigned long l, unsigned long port);

int gpg_check_mem_region(unsigned int from, unsigned int extent);
void gpg_request_mem_region(unsigned int from, unsigned int extent,
							const char *name);
void gpg_release_mem_region(unsigned int from, unsigned int extent);
void gpg_pci_set_master(void *dev);

/* kernel 2.4 only */
unsigned long gpg_get_bus_address(void *iobuf,int index, unsigned long offset);
void gpg_unmap_free_iobuf(int number,void **piobuf);

/* kernel 2.6 only */
int gpg_get_scatter_list(void *pcidev, PMDL_OBJECT mdl, void *Buffer,
	unsigned long Length, unsigned long Direction, PSCATTER_LIST Sg);
int gpg_put_scatter_list(void *pcidev, PMDL_OBJECT mdl, PSCATTER_LIST Sg);

#endif
