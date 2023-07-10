/********************************************
 * dpg0100.c dpg0100 device driver
 * 
 * version 1.30-04
 * 
 * Copyright 2002,2013 Interface Corporation
 * 
 *******************************************/
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <asm/io.h>

#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <asm/errno.h>
#include <asm/ioctl.h>
#include "dpg0101.h"

#define VERSION(v,p,s) (((v)<<16)+(p<<8)+s)

// #if LINUX_VERSION_CODE < VERSION(3,0,0)
#include "dpg0100.h"
// #endif

#define BUS_TYPE(subsys) (((subsys) & 0x2000) ? (((subsys) >> 10) & 7) : (((subsys) >> 8) & 7))
#ifndef PCI_ANY_ID
#define PCI_ANY_ID (~0)
#endif

#define DPG0101_DRIVER_NAME "dpg0101"
#define DPG0101_DRIVER_DESC "Common Device Driver"
#define DPG0101_DRIVER_VERSION "1.50.06.00"

MODULE_DESCRIPTION(DPG0101_DRIVER_DESC " Ver" DPG0101_DRIVER_VERSION);
MODULE_AUTHOR("Interface Corporation <http://www.interface.co.jp>");

static int dpg0101_version_disp = 1;

int major_number = 0;
#if LINUX_VERSION_CODE < VERSION(2,6,1)
MODULE_PARM(major_number, "i");
MODULE_PARM(dpg0101_version_disp, "i");
#else
module_param(major_number, int, 0);
module_param(dpg0101_version_disp, int, 0);
#endif

unsigned long NumberOfDevices = 0; 
unsigned long NumberOfCardDevices = 0;

/* Driver Board Control Table */
PDRV_RES_TBL dpg_res[PCI_MAX_DEV] = {NULL, };

typedef struct {
	unsigned short DeviceID;
	unsigned short SubsystemID;
	unsigned long  DevFn;
	unsigned char  BusNumber;
	unsigned char  BoardID;
	unsigned char  AccessType;
	unsigned char  BitShift;
	unsigned long  Address;
	struct pci_dev *PciDevice;
} RESOURCE_TBL, *PRESOURCE_TBL;

static RESOURCE_TBL Resource[PCI_MAX_DEV]; /* Driver Board Control Table */

#if (LINUX_VERSION_CODE < VERSION(2,4,0))
unsigned long pci_resource_start(struct pci_dev *dev, unsigned long bar)
{
	u32 value;
	pci_read_config_dword(dev, PCI_BASE_ADDRESS_0 + bar * 4, &value);
	value &= PCI_BASE_ADDRESS_IO_MASK;
	return value;
}
#endif

void *dpg0101_gpg_pci_find_device(unsigned int vendor, unsigned int device, void *from)
{
#if LINUX_VERSION_CODE < VERSION(3,0,0)
	return gpg_pci_find_device(vendor, device, from);
#else
	return (void *)(pci_get_device(vendor, device, (struct pci_dev *)from));
#endif
}

/* ******************************************************** */
/*  read system call : ifdevmgr_ioctl                       */
/* ******************************************************** */
void check_device(PRESOURCE_TBL res)
{
	unsigned long bustype = BUS_TYPE(res->SubsystemID);
	
	if(bustype == 2) {
		/* CardBus */
		res->AccessType = 2;
		/* CardBus */
		if(res->DeviceID == 4171 && !(res->SubsystemID & 0x2000)) {
			res->AccessType = 0;
			res->Address += 0x1f;
			res->BitShift = 4;
		}
	} else {
		res->AccessType = 0;
	}
}


/* ******************************************************** */
/*  read system call : ifdevmgr_read                        */
/* ******************************************************** */
ssize_t ifdevmgr_read(struct file *file, char *buffer, size_t length,
					  loff_t *offset)
{
	int index = 0;
	unsigned short subsys;
	IDACCESS_CONFIG Conf;
	struct pci_dev *pdev = NULL;
	
	if(*offset != 0) {
		return 0;
	}
	
	index = 0;
	while(1) {
		pdev = dpg0101_gpg_pci_find_device(INTERFACE_VENDORID, PCI_ANY_ID, pdev);
		if(pdev == NULL) {
			break;
		}
		
		/* Get Sub System ID */
		if (pci_read_config_word(pdev, PCI_SUBSYSTEM_ID, &subsys)) {
			return -ENODEV;
		}
		
		if (subsys & 0x2000) {
			if((subsys & 0x0C00) != 0x0800) {
				continue;
			}
		} else {
			if(!(subsys & 0x200)) {
				continue;
			}
		}
		
		Resource[index].DeviceID = pdev->device;
		Resource[index].SubsystemID = subsys;
		Resource[index].BusNumber = pdev->bus->number;
		Resource[index].DevFn = pdev->devfn;
		Resource[index].Address = pci_resource_start(pdev, 0);
		check_device(&Resource[index]);
		Resource[index].PciDevice = pdev;
		
		/* read BoardID */
		Conf.AccessType = Resource[index].AccessType;
		Conf.Address = Resource[index].Address;
		Conf.BitShift = Resource[index].BitShift;
		Conf.PciDevice = (void *)pdev;
#ifdef DEBUG
		printk("Device:%d type:%d address:%lx\n", pdev->device,
				Conf.AccessType, Conf.Address);
#endif
		Resource[index].BoardID = (unsigned char)IFReadBoardIDForCardBus(&Conf);
		
		index++;
		if(index > PCI_MAX_DEV) break;
	}
	NumberOfCardDevices = index;
	memcpy(buffer, &Resource[0], sizeof(RESOURCE_TBL) * index);

	return sizeof(RESOURCE_TBL) * index;
}


/* ******************************************************** */
/*  Read Hardware Configuration  	                        */
/* ******************************************************** */
void IfReadHardwareConfiguration(struct pci_dev *pdev,
								 PBOARD_CONF_TBL pConfTbl, int devnum)
{
	unsigned char id;
	unsigned int  ioadr[6];
	unsigned int  base_adr, ioadr0 = 0, data;
	int      k;
	void    *memadr;
	unsigned short offset, mask, type, base;
	unsigned short pci_config[6] 
		= {PCI_BASE_ADDRESS_0, PCI_BASE_ADDRESS_1, PCI_BASE_ADDRESS_2,
		   PCI_BASE_ADDRESS_3, PCI_BASE_ADDRESS_4, PCI_BASE_ADDRESS_5};
	
	/* Read REVISION ID */ 
	pci_read_config_byte(pdev, PCI_REVISION_ID, &id);
	dpg_res[devnum]->revid = id;
	
	/* Read PCI Base Address */
	for(k = 0; k < 6; k++) {
		pci_read_config_dword(pdev, pci_config[k], &base_adr);
		ioadr[k] = base_adr;
	}
	dpg_res[devnum]->type = pConfTbl->nType;
	
	memcpy(dpg_res[devnum]->name, pConfTbl->cName, MAX_DEVNAME_LENGTH);
	memcpy(dpg_res[devnum]->append, pConfTbl->append, MAX_MULTINAME_LENGTH);
	
	data = pConfTbl->offset;
	type = (data >> 28) & 0x03;
	
	if(BUS_TYPE(dpg_res[devnum]->subsys) == 2) {
		/* CardBus */
		RESOURCE_TBL Resource;
		IDACCESS_CONFIG Conf;
		
		Resource.DeviceID = pdev->device;
		Resource.SubsystemID = dpg_res[devnum]->subsys;
		Resource.Address = (ioadr[0] & PCI_BASE_ADDRESS_IO_MASK);
		Resource.PciDevice = pdev;
		check_device(&Resource);
		
		Conf.AccessType = Resource.AccessType;
		Conf.Address    = Resource.Address;
		Conf.BitShift   = Resource.BitShift;
		Conf.PciDevice  = (void *)pdev;
		
		dpg_res[devnum]->rsw = (unsigned char)IFReadBoardIDForCardBus(&Conf);
	} else {
		switch(type) {
		case 0x00: /* I/O */
			offset = data & 0xff;
			base = (data >> 8) & 0x0f;
			mask = (data >> 12) & 0x0f;
			mask = (!mask)? 0x0f : 0xff;
			ioadr0 = ioadr[base] & 0xfffffffc;
			dpg_res[devnum]->rsw = (inb(ioadr0 + offset) & mask);
			break;
		case 0x01: /* MEM */
			offset = data & 0xffff;
			base = (data >> 16) & 0x0f;
			mask = (data >> 20) & 0x0f;
			mask = (!mask)? 0x0f : 0xff;
			ioadr0 = ioadr[base] & 0xfffffff0;
			memadr = ioremap(ioadr0, offset+1);
			dpg_res[devnum]->rsw = (readb((void *)(memadr + offset)) & mask);
			iounmap(memadr);
			break;
		case 0x02:
			dpg_res[devnum]->rsw = 0x00;
			break;
		case 0x03: /* I/O or MEM Auto Select */
			offset = data & 0xffff;
			base = (data >> 16) & 0x0f;
			mask = (data >> 20) & 0x0f;
			mask = (!mask)? 0x0f : 0xff;
			
			switch (ioadr[base] & 0x1) {
			case 0x0: /* MEM */
				if (ioadr[base] == 0x00000000) break;
				ioadr0 = ioadr[base] & 0xfffffff0;
				memadr = ioremap(ioadr0, offset+1);
				dpg_res[devnum]->rsw = (readb(memadr + offset) & mask);
				iounmap(memadr);
				break;
			case 0x1: /* I/O */
				if (ioadr[base] == 0xffffffff) break;
				ioadr0 = ioadr[base] & 0xfffffffc;
				dpg_res[devnum]->rsw = (inb(ioadr0 + offset) & mask);
				break;
			}
			break;
		default:
			dpg_res[devnum]->rsw = 0x00;
			break;
		}
	}
	return;
}


/* ******************************************************** */
/*  Find PCI Dev Call Function : 		                    */
/* ******************************************************** */
int DpgPciFindDev(PBOARD_CONF_TBL pConfTbl)
{
	int      devnum;
	unsigned short subsys;
	struct   pci_dev *pdev = NULL;
	
	devnum = NumberOfDevices;
	pdev = NULL;
	while(1) {
		pdev = dpg0101_gpg_pci_find_device(INTERFACE_VENDORID, pConfTbl->nType, pdev);
		if(pdev == NULL) break;
		
		/* Get Sub System ID */
		pci_read_config_word(pdev, PCI_SUBSYSTEM_ID, &subsys);
		if(pConfTbl->nSubSystemId != subsys) {
			continue;
		}
		
		dpg_res[devnum] = kmalloc(sizeof(DRV_RES_TBL), GFP_KERNEL);
		if(dpg_res[devnum] == NULL) {
			break;
		}
		memset(dpg_res[devnum], 0, sizeof(DRV_RES_TBL));
		dpg_res[devnum]->subsys = subsys;
		
		IfReadHardwareConfiguration(pdev, pConfTbl, devnum);
		
		devnum++;
		if(devnum > PCI_MAX_DEV) break;
	} //while
	NumberOfDevices = devnum;
	
	return 0;
}

/* ******************************************************** */
/*  ioctl system call : ifdevmgr_ioctl 	                    */
/* ******************************************************** */
#if (LINUX_VERSION_CODE >= VERSION(2,6,36))
long ifdevmgr_ioctl(struct file *file, unsigned int iocmd, unsigned long ioarg)
#else
int ifdevmgr_ioctl(struct inode *inode, struct file *file, unsigned int iocmd, unsigned long ioarg)
#endif
{
	int				ret = 0;
	int				index;
	unsigned long	*memadr; 
	BOARD_CONF_TBL	DpgConfTbl;
	IDACCESS_CONFIG	Conf;
	unsigned long	Buffer[16]; // 4.8: should use dynamic allocation
	
	switch(iocmd) {
	case IOCTL_COMMON_IO_READ: 
		{
			IO_RW *req = (IO_RW *)&Buffer[0];
			ret = copy_from_user(req, (void *)ioarg, sizeof(IO_RW));
			if (ret < 0) {
				return -EFAULT;
			}
			
			switch(req->uType) {
			case 0: req->uData = inb(req->uAdrs); break;
			case 1: req->uData = inw(req->uAdrs); break;
			case 2: req->uData = inl(req->uAdrs); break;
			}
			ret = copy_to_user((void *)ioarg, req, sizeof(IO_RW));
			break;
		}
	case IOCTL_COMMON_IO_WRITE: 
		{
			IO_RW *req = (IO_RW *)&Buffer[0];
			ret = copy_from_user(req, (void *)ioarg, sizeof(IO_RW));
			if (ret < 0) {
				return -EFAULT;
			}
			
			switch(req->uType) {
			case 0: outb((req->uData & 0xff), req->uAdrs); break;
			case 1: outw((req->uData & 0xffff), req->uAdrs); break;
			case 2: outl(req->uData, req->uAdrs); break;
			}
			break;
		}
	case IOCTL_COMMON_MEMORY_READ: 
		{
			MEM_RW *req = (MEM_RW *)&Buffer[0];
			ret = copy_from_user(req, (void *)ioarg, sizeof(MEM_RW));
			if(ret < 0) {
				return -EFAULT;
			}
			
			if(req->uAdrs <= 0xfffff) return(-EINVAL); 
			switch(req->uType){
			case 0:
				memadr = ioremap(req->uAdrs, 4);
				memcpy(&(req->uData), memadr, 1);
				break;
			case 1:
				memadr = ioremap(req->uAdrs, 4);
				memcpy(&(req->uData), memadr, 2);
				break;
			case 2:
				memadr = ioremap(req->uAdrs,4);
				memcpy(&(req->uData), memadr, 4);
				break;
			}
			ret = copy_to_user((void *)ioarg, req, sizeof(MEM_RW));
			break;
		}
	case IOCTL_COMMON_MEMORY_WRITE: 
		{
			MEM_RW *req = (MEM_RW *)&Buffer[0];
			ret = copy_from_user(req, (void *)ioarg, sizeof(MEM_RW));
			if(ret < 0) {
				return -EFAULT;
			}
			
			if(req->uAdrs <= 0xfffff) return(-EINVAL); 
			switch(req->uType){
			case 0:
				memadr = ioremap(req->uAdrs, 4);
				memcpy(memadr, &(req->uData), 1);
				break;
			case 1:
				memadr = ioremap(req->uAdrs, 4);
				memcpy(memadr, &(req->uData), 2);
				break;
			case 2:
				memadr = ioremap(req->uAdrs, 4);
				memcpy(memadr, &(req->uData), 4);
				break;
			}
			break;
		}
	case IOCTL_COMMON_FIND_DEV:
		ret = copy_from_user(&DpgConfTbl, (void *)ioarg, sizeof(BOARD_CONF_TBL));
		if(ret < 0) {
			return -EFAULT;
		}
		
		ret = DpgPciFindDev(&DpgConfTbl);
		if(ret) return ret;
		break;
	case IOCTL_COMMON_GET_DEV: 
		{
			PDRV_RES_TBL Dest = (PDRV_RES_TBL)ioarg;
			for(index = 0; index < NumberOfDevices; index++) {
				ret = copy_to_user((void *)Dest, dpg_res[index], sizeof(DRV_RES_TBL));
				if(ret < 0) {
					return -EFAULT;
				}
				Dest++;
			}
			break;
		}
	case IOCTL_COMMON_WRITE_BOARDID:
		ret = copy_from_user(Buffer, (void *)ioarg, sizeof(unsigned long)*2);
		if (ret < 0) {
			return -EFAULT;
		}
		if (Buffer[0] > NumberOfCardDevices) {
			return -EINVAL;
		}
		
		/* write BoardID */
		index = Buffer[0];
		Conf.AccessType = Resource[index].AccessType;
		Conf.Address = Resource[index].Address;
		Conf.BitShift = Resource[index].BitShift;
		Conf.PciDevice = Resource[index].PciDevice;
		IFWriteBoardIDForCardBus(&Conf, Buffer[1]);
		break;
	default: 
		ret = -EINVAL;
		break;
	}
	return ret;
}

/************************************************/
/* pci_inportb                                  */
/************************************************/
static unsigned long pci_inportb(PIDACCESS_CONFIG Conf)
{
	unsigned long data = 0;
	
	switch(Conf->AccessType){
	case 0:	/* IO */
		data = (unsigned long)inb(Conf->Address);
		break;
	case 1: /* Memory */
		data = (unsigned long)readb((void *)Conf->Address);
		break;
	case 2: /* configuration */
	default:
		pci_read_config_byte((struct pci_dev *)Conf->PciDevice, 0xf8,
							 (u8 *)&data);
		break;
	}
	
	return data;
}

/************************************************/
/* pci_outportb                                 */
/************************************************/
static void pci_outportb(PIDACCESS_CONFIG Conf, unsigned long data)
{
	switch(Conf->AccessType) {
	case 0: /* IO */
		 outb(data, Conf->Address);
		 break;
	case 1: /* Memory */
		writeb(data, (void *)Conf->Address);
		break;
	case 2: /* configuration */
	default:
		pci_write_config_byte((struct pci_dev *)Conf->PciDevice, 0xf8,
							 (u8)data);
		break;
	}
}

/************************************************/
/* IFWritePCIDevice                             */
/************************************************/
unsigned long IFWritePCIDevice(PIDACCESS_CONFIG Conf, unsigned long Offset,
							   unsigned long Data)
{
	int          i;
	unsigned long counter, status;
	unsigned long shift = (unsigned long)Conf->BitShift;
	
	/* EWEN */                       /*  CS  SK  DI  */
	pci_outportb(Conf, 0 << shift);  /*  0   0   0   */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  STRAT BIT   */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0                   */
	pci_outportb(Conf, 5 << shift);  /*  1   0   1   W1  */
	pci_outportb(Conf, 7 << shift);  /*  1   1   1       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  OP (0) RD/WR ENABLE*/
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  OP (0) */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 5 << shift);  /*  1   0   1   W1  OP (1) */
	pci_outportb(Conf, 7 << shift);  /*  1   1   1       */
	pci_outportb(Conf, 5 << shift);  /*  1   0   1   W1  OP (1) */
	pci_outportb(Conf, 7 << shift);  /*  1   1   1       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 0 << shift);
	
	/* DTWR */                       /*  CS  SK  DI  */
	pci_outportb(Conf, 0 << shift);  /*  0   0   0   */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  STRAT BIT   */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0                   */
	pci_outportb(Conf, 5 << shift);  /*  1   0   1   W1  */
	pci_outportb(Conf, 7 << shift);  /*  1   1   1       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  OP (0) WRITE */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 5 << shift);  /*  1   0   1   W1  OP (1) */
	pci_outportb(Conf, 7 << shift);  /*  1   1   1       */
	
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
	
	for(i = 7; i >= 0; i--) {               /* DATA  WRITE  (D15-D8)    */
		if(((Data >> (i + 8)) & 1) == 0) {
			pci_outportb(Conf, 4 << shift); /*  1   0   0   W0  */
			pci_outportb(Conf, 6 << shift); /*  1   1   0       */
		}
		else {
			pci_outportb(Conf, 5 << shift); /*  1   0   1   W1  */
			pci_outportb(Conf, 7 << shift); /*  1   1   1       */
		}
	}
     
	for(i = 7; i >= 0; i--) {               /* DATA  WRITE  (D7-D0)     */
		if(((Data >> i) & 1) == 0) {
			pci_outportb(Conf, 4 << shift); /*  1   0   0   W0  */
			pci_outportb(Conf, 6 << shift); /*  1   1   0       */
		}
		else {
			pci_outportb(Conf, 5 << shift); /*  1   0   1   W1  */
			pci_outportb(Conf, 7 << shift); /*  1   1   1       */
		}
	}
	
	pci_outportb(Conf, 0 << shift);         /*  0   0   0   STR */
	pci_inportb(Conf);              // DELAY
	pci_inportb(Conf);              // DELAY
	pci_inportb(Conf);              // DELAY
	pci_outportb(Conf, 4 << shift);         /*  1   0   0       */
	
	counter = 3000;
	while(counter--) {
		status = (pci_inportb(Conf) >> shift);
		if(status & 0x01) break;
		udelay(1);
	}
	pci_outportb(Conf, 0 << shift);
	
	/* EWEN */                       /*  CS  SK  DI  */
	pci_outportb(Conf, 0 << shift);  /*  0   0   0   */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  STRAT BIT   */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0                   */
	pci_outportb(Conf, 5 << shift);  /*  1   0   1   W1  */
	pci_outportb(Conf, 7 << shift);  /*  1   1   1       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  OP (0) RD/WR ENABLE*/
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  OP (0) */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W1  OP (0) */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W1  OP (0) */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 4 << shift);  /*  1   0   0   W0  */
	pci_outportb(Conf, 6 << shift);  /*  1   1   0       */
	pci_outportb(Conf, 0 << shift);
	
	return 0;
}

/************************************************/
/* IFReadPCIDevice                              */
/************************************************/
unsigned long IFReadPCIDevice(PIDACCESS_CONFIG Conf, unsigned long Offset)
{
	int          i;
	unsigned int ret;
	unsigned long Data;
	unsigned long shift = (unsigned long)Conf->BitShift;
	
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

/************************************************/
/* IFWriteBoardIDForCardBus                     */
/************************************************/
unsigned long IFWriteBoardIDForCardBus(PIDACCESS_CONFIG Conf,
									   unsigned long BoardID)
{
	if(Conf->AccessType == 2) {
		/* configration access */
		Conf->BitShift = 0;
	}
	IFWritePCIDevice(Conf, 0x10, BoardID);
	
	return 0;
}

/************************************************/
/* IFReadBoardIDForCardBus                      */
/************************************************/
unsigned long IFReadBoardIDForCardBus(PIDACCESS_CONFIG Conf)
{
	unsigned long BoardID;
	
	if(Conf->AccessType == 2) {
		/* configration access */
		Conf->BitShift = 0;
	}
	BoardID  = IFReadPCIDevice(Conf, 0x10);
	
	return BoardID;
}


/* ******************************************** */
/*  Operation Table : 	                        */
/* ******************************************** */
static struct file_operations dpg_fops = {
#if (LINUX_VERSION_CODE >= VERSION(2,4,0))
	THIS_MODULE,
#endif /* MODULE */
	.read = ifdevmgr_read,	/* read    */
#if (LINUX_VERSION_CODE >= VERSION(2,6,36))
	.unlocked_ioctl = ifdevmgr_ioctl, /* ioctl   */
#else
	.ioctl = ifdevmgr_ioctl, /* ioctl   */
#endif
};

#if LINUX_VERSION_CODE < VERSION(2,5,0)
EXPORT_NO_SYMBOLS;
#endif

/* ******************************************** */
/*  init_module : 		                    	*/
/* ******************************************** */
int init_module(void)
{
	int ret;
	
	/* Initialize */
	ret = register_chrdev(major_number, "dpg0101", &dpg_fops);
	if(ret < 0) {
		return ret;
	}
	if(major_number == 0) {
		major_number = ret;
	}
	
	if(dpg0101_version_disp){
	    printk("%s:Interface %s (Ver%s) loaded.\n", 
		   DPG0101_DRIVER_NAME, 
		   DPG0101_DRIVER_DESC,
		   DPG0101_DRIVER_VERSION);
	}
	
	return 0;
}


/* ******************************************** */
/*  cleanup_module :              	       	    */
/* ******************************************** */
void cleanup_module(void)
{
	int index;
	
	unregister_chrdev(major_number, "dpg0101");
	
	for(index = 0; index < PCI_MAX_DEV; index++) {
		if(dpg_res[index] != NULL) {
			kfree(dpg_res[index]);
			dpg_res[index] = NULL;
		}
	}
	if(dpg0101_version_disp){
	    printk("%s:Interface %s (Ver%s) unloaded.\n", 
		   DPG0101_DRIVER_NAME, 
		   DPG0101_DRIVER_DESC,
		   DPG0101_DRIVER_VERSION);
	}
}
