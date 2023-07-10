/* ********************************************************************
   diobm.c -source code of Digital Input/Output Driver Programs
   --------------------------------------------------------------------
   Version 1.51-12
   --------------------------------------------------------------------
   Date May 30, 2008
   --------------------------------------------------------------------
   Copyright 2002, 2008 Interface Corporation. All rights reserved.
   --------------------------------------------------------------------
    Ver.       Date           Revision History
   --------------------------------------------------------------------
    1.00-01 June 25, 2002   new release
    1.30-08  Dec 22, 2005   kernel 2.6 supported.
    1.31-09  Jan 24, 2006   fixed of interrupt event.
    1.60-12  May 30, 2008   fixed of create-scatterlist routine.
   ******************************************************************** */

#define VERSION(v,p,s) (((v)<<16)+(p<<8)+s)

#ifdef __RTL__
#if(LINUX_VERSION_CODE < VERSION(2,4,0))
#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif
#endif
#endif

#ifndef __RTL__
 #define DRIVER_NAME "cp2x72c"
 #define REGISTER_NAME "cp2x72c"
 #define IFDIOBM_DEBUG_LEVEL cp2x72c_debuglevel
 #define IFDIOBM_VER_DISP cp2x72c_version_disp
#else
 #define DRIVER_NAME "rcp2x72c"
 #define REGISTER_NAME "cp2x72c"
 #define IFDIOBM_DEBUG_LEVEL rcp2x72c_debuglevel
 #define IFDIOBM_VER_DISP rcp2x72c_version_disp
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/version.h>

#include <linux/mm.h>

#include <asm/page.h>
#include <asm/pgtable.h>

#ifndef VM_RESERVED
	#define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#if(LINUX_VERSION_CODE < VERSION(2,1,0))
	#include <asm/segment.h>
	#include <linux/bios32.h>
#elif(LINUX_VERSION_CODE >= VERSION(2,4,0))
	#include <linux/poll.h>
	#include <asm/uaccess.h>
	#include <linux/highmem.h>
#elif(LINUX_VERSION_CODE >= VERSION(2,2,0))
	#include <linux/vmalloc.h>
	#include <linux/poll.h>
	#include <asm/uaccess.h>
	#include <asm/spinlock.h>
#endif

/* remember about the current version */
#if(LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0))
	#define LINUX_22
#elif(LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	#define LINUX_24
#elif(LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
	#define LINUX_26
#endif

#ifdef LINUX_24
#include <linux/iobuf.h>
#endif
#ifdef LINUX_26
#include <linux/interrupt.h>
#include <linux/pagemap.h>
#include <linux/moduleparam.h>
#endif

#include <linux/mm.h>
#include <linux/timer.h>
#include <linux/param.h>
#include <linux/delay.h>


#if LINUX_VERSION_CODE >= VERSION(2,4,0)
#include <linux/init.h>

int  diobm_init(void);
void diobm_cleanup(void);

module_init(diobm_init);
module_exit(diobm_cleanup);

#else

#define diobm_init init_module
#define diobm_cleanup cleanup_module

#endif

#include "dpg0100.h"

#ifdef __RTL__

#include "fbidiobm.h"

EXPORT_SYMBOL(DioBmOpen);
EXPORT_SYMBOL(DioBmClose);
EXPORT_SYMBOL(DioBmSetDeviceConfig);
EXPORT_SYMBOL(DioBmSetBufferConfig);
EXPORT_SYMBOL(DioBmSetTriggerConfig);
EXPORT_SYMBOL(DioBmStart);
EXPORT_SYMBOL(DioBmStop);
EXPORT_SYMBOL(DioBmEnd);
EXPORT_SYMBOL(DioBmSoftTrigger);
EXPORT_SYMBOL(DioBmSetEventConfig);
EXPORT_SYMBOL(DioBmSetCounterEventConfig);
EXPORT_SYMBOL(DioBmSetEvent);
EXPORT_SYMBOL(DioBmKillEvent);
EXPORT_SYMBOL(DioBmGetCounter);
EXPORT_SYMBOL(DioBmGetStatus);
EXPORT_SYMBOL(DioBmInputDword);
EXPORT_SYMBOL(DioBmOutputDword);
EXPORT_SYMBOL(DioBmGetLine);
EXPORT_SYMBOL(DioBmSetLine);
EXPORT_SYMBOL(DioBmGetDeviceConfig);
EXPORT_SYMBOL(DioBmGetBufferConfig);
EXPORT_SYMBOL(DioBmGetTriggerConfig);
EXPORT_SYMBOL(DioBmGetEventConfig);
EXPORT_SYMBOL(DioBmGetCounterEventConfig);
EXPORT_SYMBOL(DioBmGetDeviceInfo);
EXPORT_SYMBOL(DioBmCommonGetPciDeviceInfo);
EXPORT_SYMBOL(DioBmSetDeviceConfigEx);
EXPORT_SYMBOL(DioBmGetDeviceConfigEx);
EXPORT_SYMBOL(DioBmStopEx);
EXPORT_SYMBOL(DioBmSoftTriggerEx);
EXPORT_SYMBOL(DioBmSetEdgeEventConfig);
EXPORT_SYMBOL(DioBmGetEdgeEventConfig);
EXPORT_SYMBOL(DioBmSetEventEx);
EXPORT_SYMBOL(DioBmSetFilter);
EXPORT_SYMBOL(DioBmGetFilter);
EXPORT_SYMBOL(DioBmClearSamplingOutput);

EXPORT_SYMBOL(gpg_diobm_spin_unlock_irqrestore);

int rcp2x72c_debuglevel = 0;
int rcp2x72c_version_disp = 1;

#else
int cp2x72c_debuglevel = 0;
int cp2x72c_version_disp = 1;
#endif

#ifndef DIOBM_MAX_DEV
#define DIOBM_MAX_DEV 256
#endif

#ifndef DIOBM_MAJOR
#define DIOBM_MAJOR 0
#endif

/**********************************/

static int  diobm_major = -1; /* Major Number */

extern int  gpg_diobm_ioctl(struct file * file, unsigned int iocmd, unsigned long ioarg);
extern int  gpg_diobm_open(struct inode *inode, struct file *file);
extern int  gpg_diobm_release(struct inode *inode, struct file *file);
extern void gpg_diobm_initialize(void);
extern void gpg_diobm_cleanup(void);

extern unsigned int diobm_interrupt_sub(unsigned char irqno, void* devnum);

/* values */
unsigned long gpg_diobm_page_size = PAGE_SIZE;
unsigned long gpg_diobm_page_shift = PAGE_SHIFT;

#ifdef LINUX_22
struct scatterlist {
	void* address;
	unsigned long length;
};
#endif

unsigned long gpg_diobm_sizeof_struct_scatterlist = sizeof(struct scatterlist);

#ifdef LINUX_24
unsigned long gpg_diobm_sizeof_struct_page = sizeof(struct page);
#endif

unsigned long gpg_diobm_vmalloc_start = 0;
unsigned long gpg_diobm_vmalloc_end = 0;

/* shared */
unsigned long gpg_diobm_virt_to_bus(void* v);
void *gpg_diobm_phys_to_virt(unsigned long v);

char *gpg_diobm_strcpy (char * __dest, char *__src);
int gpg_diobm_strcmp (char *__s1, char *__s2);
unsigned long gpg_diobm_strlen (char *__s);
int gpg_diobm_sprintf (char * __s, char * __format, ...);

/* linux 2.2 */
#ifdef LINUX_22

unsigned long gpg_diobm_user_addr_to_page(void *pBuffer);

void gpg_diobm_atomic_inc(unsigned long page);
void gpg_diobm_atomic_dec(unsigned long page);

#endif

/* linux 2.4 */
#ifdef LINUX_24
int gpg_diobm_alloc_kiovec(int number,void **piobuf);
void gpg_diobm_free_kiovec(int number,void **piobuf);
int gpg_diobm_map_user_kiobuf(int Direction, void *iobuf,void *pBuffer, unsigned long dwBufferSize);
unsigned long gpg_diobm_get_nr_pages(void *iobuf);
unsigned long gpg_diobm_get_offset(void *iobuf);
unsigned long gpg_diobm_get_bus_address(void *iobuf,int index,unsigned long offset);
void gpg_diobm_unmap_free_iobuf(int number,void **piobuf);

unsigned long gpg_diobm_kmap(void *page);
void gpg_diobm_kunmap(void *page);
void gpg_diobm_get_page(void *page);
void gpg_diobm_put_page(void *page);

void gpg_diobm_list_set_page(void **list,unsigned long index,void *page);
void* gpg_diobm_list_get_page(void **list,unsigned long index);

#endif

/* linux 2.4 & linux 2.6 */
#ifndef LINUX_22
unsigned long
gpg_diobm_pci_map_sg(void *pci,void **Elements, unsigned long NumberOfElements, int Direction);
void
gpg_diobm_pci_unmap_sg(void *pci,void **Elements, unsigned long NumberOfElements, int Direction);
#endif


/* linux 2.6 */
#ifdef LINUX_26
int gpg_diobm_lock_memory(PMDL_OBJECT mdl, unsigned long Buffer, unsigned long Length, unsigned long Direction);
int gpg_diobm_unlock_memory(PMDL_OBJECT mdl);
int gpg_diobm_create_scatterlist(PMDL_OBJECT mdl, unsigned long Buffer, void **list);
#endif

/* linux 2.6.15 */
#if LINUX_VERSION_CODE == VERSION(2,6,15)
const char *print_tainted(void) { return NULL; }
#endif

void gpg_diobm_scatterlist_set_address(void *list,unsigned long address);
void gpg_diobm_scatterlist_set_length(void *list,unsigned long length);
unsigned long gpg_diobm_scatterlist_get_address(void *list);
unsigned long gpg_diobm_scatterlist_get_length(void *list);
unsigned long gpg_diobm_scatterlist_get_dma_address(void *list);
unsigned long gpg_diobm_scatterlist_get_dma_length(void *list);
void* gpg_diobm_scatterlist_pointer_inc(void *list);
void gpg_diobm_pci_enable_device(void *dev);


#ifdef __RTL__

static char rtl_print_buf[1024];

#include <rtl_printf.h>

int gpg_rtl_printf(const char *fmt, ...);

int gpg_rtl_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vsprintf(rtl_print_buf, fmt, args);
	va_end(args);

	return rtl_printf(rtl_print_buf);
}


#ifdef LINUX_22
unsigned long gpg_diobm_vmalloc_addr_to_page(void *list);
#else
void* gpg_diobm_vmalloc_addr_to_page(void *list);
#endif
#endif


typedef struct _DIOBM_TABLE{
	spinlock_t lock;
#if (LINUX_VERSION_CODE >= VERSION(2,4,0))
	wait_queue_head_t diobm_wait;
#else
	struct wait_queue *diobm_wait;
#endif
	int wait_flag;
} DIOBM_TABLE;

static DIOBM_TABLE diobm_table[DIOBM_MAX_DEV];

void gpg_diobm_interruptible_sleep_on(int minor)
{
#ifdef LINUX_26
	diobm_table[minor].wait_flag = 0;
	wait_event_interruptible(diobm_table[minor].diobm_wait, (diobm_table[minor].wait_flag != 0));
#else
	interruptible_sleep_on(&diobm_table[minor].diobm_wait);
#endif
	return;
}

void gpg_diobm_wake_up_interruptible(int minor)
{
	diobm_table[minor].wait_flag = 1;
	wake_up_interruptible(&diobm_table[minor].diobm_wait);
	return;
}

void gpg_diobm_spin_lock_init(int minor)
{
  spin_lock_init(&(diobm_table[minor].lock));
}

void gpg_diobm_spin_lock(int minor)
{
  spin_lock(&(diobm_table[minor].lock));
}

void gpg_diobm_spin_unlock(int minor)
{
  spin_unlock(&(diobm_table[minor].lock));
}

void gpg_diobm_spin_lock_irqsave(int minor, long *flag)
{
	gpg_spin_lock_irqsave(&diobm_table[minor].lock, flag);
	return;
}

void gpg_diobm_spin_unlock_irqrestore(int minor, long flag)
{
	gpg_spin_unlock_irqrestore(&diobm_table[minor].lock, flag);
	return;
}
/*******************************************************************/
/* gpg_2x72c_MINOR_f                                               */
/*******************************************************************/
int gpg_2x72c_MINOR_f(void *file)
{
	return MINOR(file_inode((struct file*)file)->i_rdev);
	// return MINOR(((struct file *)file)->f_dentry->d_inode->i_rdev);
}

#if (LINUX_VERSION_CODE >= VERSION(2,6,36))
static long diobm_ioctl(struct file *file, unsigned int iocmd, unsigned long ioarg)
#else
static int diobm_ioctl(struct inode *inode, struct file *file, unsigned int iocmd, unsigned long ioarg)
#endif
{
	return gpg_diobm_ioctl((void*)file, iocmd, ioarg);
}


static int diobm_open(struct inode *inode, struct file *file)
{
	int ret = 0, minor;

	//minor = MINOR(inode->i_rdev);
	minor = gpg_2x72c_MINOR_f(file);
	if (minor < 0 || minor > DIOBM_MAX_DEV) return -EBADF;

	ret = gpg_diobm_open(inode, file);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
	if(!ret)
	MOD_INC_USE_COUNT;
#endif
	return ret;
}


static int diobm_release(struct inode *inode, struct file *file)
{
	int minor, ret = 0;

	//minor = MINOR(inode->i_rdev);
	minor = gpg_2x72c_MINOR_f(file);
	ret = gpg_diobm_release(inode, file);
	if (ret) return ret;
	if (minor) {}
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
	MOD_DEC_USE_COUNT;
#endif
	return 0;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
struct page *simple_vma_nopage(struct vm_area_struct *vma, unsigned long address, int * type)
{
	struct page *pageptr;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long physaddr = address - vma->vm_start + offset;
	unsigned long pageframe =  physaddr >> PAGE_SHIFT;
	
	if(!pfn_valid(pageframe))
	return NOPAGE_SIGBUS;
	
	pageptr = pfn_to_page(pageframe);
	get_page(pageptr);
	if(type)
		*type = VM_FAULT_MINOR;
	
	return pageptr;
}
static struct vm_operations_struct simple_nopage_vm_ops = {
	.nopage = simple_vma_nopage,
};
#endif

static int diobm_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

	/* Don't try to swap out physical pages.. */
	vma->vm_flags |= VM_RESERVED;
	/*
	* Don't dump addresses that are not real memory to a core file.
	*/
	if (offset >= __pa(high_memory) || (file->f_flags & O_SYNC)) {
		vma->vm_flags |= VM_IO;
	}
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12)
	if (io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end-vma->vm_start, vma->vm_page_prot)){
		return -EAGAIN;
	}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
	if (io_remap_page_range(vma, vma->vm_start, offset, vma->vm_end-vma->vm_start, vma->vm_page_prot)){
		return -EAGAIN;
	}
#else
	vma->vm_ops = &simple_nopage_vm_ops;
#endif
	
	return(0);
}
#endif

unsigned long gpg_diobm_virt_to_bus(void* v)
{
	return (virt_to_bus(v));
}

void *gpg_diobm_phys_to_virt(unsigned long v)
{
	return (phys_to_virt(v));
}

#ifdef LINUX_22
unsigned long gpg_diobm_user_addr_to_page(void *pBuffer)
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;

	unsigned long lpage;
	unsigned long page;

	lpage = VMALLOC_VMADDR(pBuffer);

	pgd = pgd_offset(current->mm,lpage);
	pmd = pmd_offset(pgd,lpage);
	pte = pte_offset(pmd,lpage);
	page = pte_page(*pte);

	return (page);
}

void gpg_diobm_atomic_inc(unsigned long page){
	atomic_inc(&mem_map[MAP_NR(page)].count);
}

void gpg_diobm_atomic_dec(unsigned long page)
{
	atomic_dec(&mem_map[MAP_NR(page)].count);
}

#endif



#ifdef LINUX_24
int gpg_diobm_alloc_kiovec(int number,void **piobuf)
{
	struct kiobuf **kiobuf = (struct kiobuf **)piobuf;
	return (alloc_kiovec(1,kiobuf));
}

void gpg_diobm_free_kiovec(int number,void **piobuf)
{
	struct kiobuf **kiobuf = (struct kiobuf **)piobuf;
	return (free_kiovec(number,kiobuf));
}

int gpg_diobm_map_user_kiobuf(int Direction, void *iobuf,void *pBuffer, unsigned long dwBufferSize)
{
	struct kiobuf *kiobuf = (struct kiobuf*)iobuf;

	if(!Direction){
		Direction = READ;
	}else{
		Direction = WRITE;
	}

	return (map_user_kiobuf(Direction,kiobuf,(unsigned long)pBuffer,dwBufferSize));
}

unsigned long gpg_diobm_get_nr_pages(void *iobuf)
{
	struct kiobuf *kiobuf = (struct kiobuf *)iobuf;
	return ((unsigned long)kiobuf->nr_pages);
}

unsigned long gpg_diobm_get_offset(void *iobuf)
{
	struct kiobuf *kiobuf = (struct kiobuf *)iobuf;
	return ((unsigned long)kiobuf->offset);
}

unsigned long gpg_diobm_get_bus_address(void *iobuf,int index,unsigned long offset)
{
	return gpg_get_bus_address(iobuf, index, offset);
}

void gpg_diobm_unmap_free_iobuf(int number,void **piobuf)
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

unsigned long gpg_diobm_kmap(void *page)
{
	return ((unsigned long)kmap((struct page *)page));
}

void gpg_diobm_kunmap(void *page)
{
	kunmap((struct page *)page);
}

void gpg_diobm_get_page(void *page)
{
	get_page((struct page *)page);
}

void gpg_diobm_put_page(void *page)
{
	put_page((struct page *)page);
}

void gpg_diobm_list_set_page(void **list,unsigned long index,void *page)
{
	struct page **ppage = (struct page**)list;
	ppage[index] = page;
}

void* gpg_diobm_list_get_page(void **list,unsigned long index)
{
	struct page **ppage = (struct page**)list;
	return ((void *)ppage[index]);
}

#endif

#ifndef LINUX_22

unsigned long
gpg_diobm_pci_map_sg(void *pci,void **Elements,unsigned long NumberOfElements,int Direction)
{
	if(!Direction){
		Direction = PCI_DMA_FROMDEVICE;
	}else{
		Direction = PCI_DMA_TODEVICE;
	}

	return (pci_map_sg((struct pci_dev*)pci,(struct scatterlist*)Elements,(int)NumberOfElements,Direction));
}

void
gpg_diobm_pci_unmap_sg(void *pci,void **Elements,unsigned long NumberOfElements,int Direction)
{
	if(!Direction){
		Direction = PCI_DMA_FROMDEVICE;
	}else{
		Direction = PCI_DMA_TODEVICE;
	}

	pci_unmap_sg((struct pci_dev*)pci,(struct scatterlist*)Elements,(int)NumberOfElements,Direction);
}

#endif


#ifdef LINUX_26

void *gpg_diobm_page_malloc(unsigned long size)
{
	void *address;
	unsigned long order, align;

	for(order = 0, align = PAGE_SIZE; align < size; order++, align <<= 1);
	address = (void *)__get_free_pages(GFP_ATOMIC, order);
	return address;
}

void gpg_diobm_page_free(void *address, unsigned long size)
{
	unsigned long order, align;
	if(address == NULL) {
		return;
	}
	for(order = 0, align = PAGE_SIZE; align < size; order++, align <<= 1);
	free_pages((unsigned long)address, order);
}

// FROM_DEVICE:2 TO_DEVICE:1
int gpg_diobm_lock_memory(PMDL_OBJECT mdl, unsigned long Buffer, unsigned long Length, unsigned long Direction)
{
	unsigned long first, last;
	unsigned long i;
	int  ret;

	if(!Direction){
		Direction = PCI_DMA_FROMDEVICE;
	}else{
		Direction = PCI_DMA_TODEVICE;
	}

	if(mdl->PageList) {
		// free old page list
		gpg_diobm_unlock_memory(mdl);
	}

	// allocate MDL
	first = (Buffer & PAGE_MASK) >> PAGE_SHIFT;
	last  = ((Buffer + Length - 1) & PAGE_MASK) >> PAGE_SHIFT;

	mdl->ByteOffset = Buffer & ~PAGE_MASK;
	mdl->ByteCount  = Length;
	mdl->StartVa    = Buffer;
	mdl->NumberOfPages = last - first + 1;
	mdl->PageList = (unsigned long *)kmalloc(mdl->NumberOfPages * sizeof(unsigned long), GFP_ATOMIC);
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
		// failed to lock pages
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

int gpg_diobm_unlock_memory(PMDL_OBJECT mdl)
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

#if (LINUX_VERSION_CODE >= VERSION(2,6,24))
int gpg_diobm_create_scatterlist(PMDL_OBJECT mdl, unsigned long Buffer, void **list)
{
	int                i;
	unsigned long      length = mdl->ByteCount;
	struct scatterlist *LinuxSg;

	if (mdl->PageList[0] == 0) return -ENOMEM;

	LinuxSg = gpg_diobm_page_malloc(sizeof(struct scatterlist) * mdl->NumberOfPages);
	if (LinuxSg == NULL) return -ENOMEM;
	memset(LinuxSg, 0, sizeof(struct scatterlist) * mdl->NumberOfPages);

	sg_set_page(LinuxSg, (struct page *)mdl->PageList[0], 0, Buffer & ~PAGE_MASK);
	LinuxSg[0].offset = mdl->ByteOffset;
	if (mdl->NumberOfPages > 1) {
		LinuxSg[0].length = PAGE_SIZE - LinuxSg[0].offset;
		length -= LinuxSg[0].length;
		for (i = 1; i < mdl->NumberOfPages; i++) {
			if (mdl->PageList[i] == 0) goto nopage;
			sg_set_page(&LinuxSg[i], (struct page *)mdl->PageList[i], length < PAGE_SIZE ? length : PAGE_SIZE, 0);
		}
	} else {
		LinuxSg[0].length = length;
	}

	*list = (void *)LinuxSg;

	return 0;
nopage:
	gpg_diobm_page_free(LinuxSg, sizeof(struct scatterlist) * mdl->NumberOfPages);
	*list = NULL;
	return -ENOMEM;
}

#else
int gpg_diobm_create_scatterlist(PMDL_OBJECT mdl, unsigned long Buffer, void **list)
{
	int i;
	struct scatterlist *LinuxSg;
	unsigned long length = mdl->ByteCount;
	
	if(mdl->PageList[0] == 0) {
		return -ENOMEM;
	}
	
	LinuxSg = gpg_diobm_page_malloc(
		sizeof(struct scatterlist) * mdl->NumberOfPages);
	if(LinuxSg == NULL) {
		return -ENOMEM;
	}
	memset(LinuxSg, 0, sizeof(struct scatterlist) * mdl->NumberOfPages);
	
	LinuxSg[0].page   = (struct page *)mdl->PageList[0];
	LinuxSg[0].offset = mdl->ByteOffset;
	if(mdl->NumberOfPages > 1) {
		LinuxSg[0].length = PAGE_SIZE - LinuxSg[0].offset;
		length -= LinuxSg[0].length;
		for(i = 1; i < mdl->NumberOfPages; i++) {
			if(mdl->PageList[i] == 0) {
				goto nopage;
			}
			
			LinuxSg[i].page   = (struct page *)mdl->PageList[i];
			LinuxSg[i].length = ((length < PAGE_SIZE) ? length : PAGE_SIZE);
			length -= PAGE_SIZE;
		}
	}
	else {
		LinuxSg[0].length = length;
	}
	*list = (void *)LinuxSg;
	return 0;

nopage:
	gpg_diobm_page_free(LinuxSg, sizeof(struct scatterlist) * mdl->NumberOfPages);
	*list = NULL;
	return -ENOMEM;
}
#endif

#endif


#ifdef __RTL__
#ifdef LINUX_22
unsigned long gpg_diobm_vmalloc_addr_to_page(void *list)
#else
void* gpg_diobm_vmalloc_addr_to_page(void *list)
#endif
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;
	unsigned long lpage;

#ifdef LINUX_22
	unsigned long page;
#else
	struct page *page;
#endif

	lpage = VMALLOC_VMADDR(gpg_diobm_scatterlist_get_address(list));

#if LINUX_VERSION_CODE >= VERSION(2,4,0)
	spin_lock(&init_mm.page_table_lock);
#endif

	pgd = pgd_offset(&init_mm,lpage);
	pmd = pmd_offset(pgd,lpage);
	pte = pte_offset(pmd,lpage);
	page = pte_page(*pte);

#if LINUX_VERSION_CODE >= VERSION(2,4,0)
	spin_unlock(&init_mm.page_table_lock);
#endif

#ifdef LINUX_22
	return (page);
#else
	return ((void*)page);
#endif

}
#endif


void gpg_diobm_scatterlist_set_address(void *list,unsigned long address)
{
	struct scatterlist *sl = (struct scatterlist*)list;

#ifdef LINUX_26
	sl->dma_address = (dma_addr_t)address;
#else
	sl->address = (void*)address;
#endif
}

void gpg_diobm_scatterlist_set_length(void *list,unsigned long length)
{
	struct scatterlist *sl = (struct scatterlist*)list;

	sl->length = length;
}

unsigned long gpg_diobm_scatterlist_get_address(void *list)
{
	struct scatterlist *sl = (struct scatterlist*)list;

#ifdef LINUX_26
	return ((unsigned long)sl->dma_address);
#else
	return ((unsigned long)sl->address);
#endif
}

unsigned long  gpg_diobm_scatterlist_get_length(void *list)
{
	struct scatterlist *sl = (struct scatterlist*)list;

	return (sl->length);
}

void* gpg_diobm_scatterlist_pointer_inc(void *list)
{
	struct scatterlist *sl = (struct scatterlist*)list;

	sl++;
	return ((void*)sl);
}

unsigned long gpg_diobm_scatterlist_get_dma_address(void *list)
{
	struct scatterlist *sl = (struct scatterlist*)list;

#ifdef LINUX_22
	return ((unsigned long)sl->address);
#elif defined LINUX_24
	return ((unsigned long)sg_dma_address(sl));
#else
 #ifdef SH4
	return ((unsigned long)(sl->dma_address & 0x1fffffff));
 #else
	return ((unsigned long)sg_dma_address(sl));
 #endif
#endif

}

unsigned long gpg_diobm_scatterlist_get_dma_length(void *list)
{
	struct scatterlist *sl = (struct scatterlist*)list;
#ifdef LINUX_22
	return (sl->length);
#else
	return (sg_dma_len(sl));
#endif
}

void gpg_diobm_pci_enable_device(void *dev)
{
#if(LINUX_VERSION_CODE >= VERSION(2,3,0))
	int ret;
	ret = pci_enable_device((struct pci_dev *)dev);
	if (ret){}
#endif
}

char *gpg_diobm_strcpy (char *__dest, char *__src)
{
	return (strcpy(__dest,__src));
}

int gpg_diobm_strcmp ( char *__s1,char *__s2)
{
	return (strcmp(__s1,__s2));
}

unsigned long gpg_diobm_strlen (char *__s)
{
	return (strlen(__s));
}

int gpg_diobm_sprintf (char *__s,char * __format, ...)
{
	va_list args;
	int ret;
	
	va_start(args, __format);
	ret = vsprintf(__s, __format, args);
	va_end(args);
	
	return ret;
}

void *gpg_diobm_alloc_consistent(size_t length, unsigned long *address, void *pci)
{
	void *virt_address;

#if (LINUX_VERSION_CODE > VERSION(2,4,0))
	virt_address = pci_alloc_consistent((struct pci_dev*)pci, length, (dma_addr_t *)address);
#else
	virt_address = (void *)__get_free_pages(GFP_KERNEL || __GFP_DMA, 8);
	if (virt_address) {
		*address=  virt_to_phys(virt_address);
	}
#endif
	return virt_address;
}

void gpg_diobm_free_consistent(size_t size, void* virt_addr, unsigned long address, void* pci)
{

#if(LINUX_VERSION_CODE > VERSION(2,4,0))
	pci_free_consistent((struct pci_dev*)pci, size, virt_addr, address);
#else
	free_pages((unsigned long)virt_addr, 8);
#endif
}



/************************************************/
/* interrupt service routine                    */
/************************************************/
#if (LINUX_VERSION_CODE >= VERSION(2,6,0))
#if (LINUX_VERSION_CODE >= VERSION(2,6,19))
irqreturn_t diobm_interrupt(int irq, void *dev_id)
#else
irqreturn_t diobm_interrupt(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
	if (diobm_interrupt_sub(irq, dev_id)) {
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}
#else
void diobm_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	diobm_interrupt_sub(irq, dev_id);
}
#endif

int gpg_diobm_request_irq (unsigned int irq, char *device, void *param)
{
	int ret = 0;
#if (LINUX_VERSION_CODE >= VERSION(2,6,24))
	ret=request_irq(irq, diobm_interrupt, IRQF_SHARED, device, param);
#else
	request_irq(irq, diobm_interrupt, SA_INTERRUPT | SA_SHIRQ, device, param);
#endif
	return ret;
}

#ifdef LINUX_26
module_param(diobm_major, int, 0);
module_param(IFDIOBM_DEBUG_LEVEL, int, 0);
module_param(IFDIOBM_VER_DISP, int, 0);
#else
MODULE_PARM(diobm_major, "i");
MODULE_PARM(IFDIOBM_DEBUG_LEVEL,"i");
MODULE_PARM(IFDIOBM_VER_DISP, "i");
#endif

#define IFDIOBM_DRIVER_DESC "Bus Master Digital Input/Output Device Driver"
#define IFDIOBM_DRIVER_VERSION "2.20.18.00"

MODULE_DESCRIPTION(IFDIOBM_DRIVER_DESC " Ver" IFDIOBM_DRIVER_VERSION);
MODULE_AUTHOR(IFDRV_MODULE_AUTHOR);

/* ******************************************** */
/*  Operation Table :                           */
/* ******************************************** */
static struct file_operations diobm_fops = {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
	.owner = THIS_MODULE,
#endif
#if (LINUX_VERSION_CODE >= VERSION(2,6,36))
	.unlocked_ioctl = diobm_ioctl,
#else
	.ioctl   = diobm_ioctl,
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
	.mmap    = diobm_mmap,
#endif
	.open    = diobm_open,
	.release = diobm_release,
};

/* ******************************************** */
/*  init_module : Insert Module                 */
/* ******************************************** */
#ifdef MODULE
int diobm_init(void)
{
	int i, ret;

#ifndef __RTL__
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
	EXPORT_NO_SYMBOLS;
#endif
#endif

	if(diobm_major == -1) diobm_major = DIOBM_MAJOR;

	gpg_diobm_initialize();

	for (i=0;i<DIOBM_MAX_DEV;i++){
	//$$$ fixed Ver02.00-15
	gpg_diobm_spin_lock_init(i);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
		init_waitqueue_head(&diobm_table[i].diobm_wait);
#else
		diobm_table[i].diobm_wait = NULL;
#endif
	}
	
	gpg_diobm_vmalloc_start = VMALLOC_START;
	gpg_diobm_vmalloc_end = VMALLOC_END;
	
	/* Initialize */
	ret = register_chrdev(diobm_major, REGISTER_NAME, &diobm_fops);
	if (ret < 0) return ret;
	if (diobm_major == 0) diobm_major = ret;
	
	if (IFDIOBM_VER_DISP) {
		gpg_version_disp_load(DRIVER_NAME, IFDIOBM_DRIVER_DESC, IFDIOBM_DRIVER_VERSION);
	}
	
	return 0;
}


/* ******************************************** */
/*  cleanup_module :                            */
/* ******************************************** */
void diobm_cleanup(void)
{
	gpg_diobm_cleanup();
	unregister_chrdev(diobm_major, REGISTER_NAME);
	
	if (IFDIOBM_VER_DISP) {
		gpg_version_disp_unload(DRIVER_NAME, IFDIOBM_DRIVER_DESC, IFDIOBM_DRIVER_VERSION);
	}
	return;
}
#endif /* MODULE */

