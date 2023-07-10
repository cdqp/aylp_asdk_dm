/* ************************************************************
   dpg0101.h - dpg0101 device driver
   ------------------------------------------------------------
   version 1.10-02
   ------------------------------------------------------------
   date    2004/01/30
   ------------------------------------------------------------
   Copyright 2002, 2004 Interface Corporation. All rights reserved.
   ************************************************************ */
#define INTERFACE_VENDORID 0x1147
#define PCI_MAX_DEV 255
#define MAX_DEVNAME_LENGTH   256
#define MAX_MULTINAME_LENGTH 32

/* Structure of device infomation */
typedef struct _PCI_DEVICE_INFO {
   unsigned short   VendorID;   /* 00: Vemdor ID */
   unsigned short   DeviceID;   /* 02: Device ID */
   unsigned short   Command;    /* 04: Command   */
   unsigned short   StatusReg;  /* 06: Status    */
   unsigned char    RevisionID; /* 08: Revision  */
   unsigned char    ProgIf;     /* 09: ProgIf    */
   unsigned char    SubClass;   /* 0A: Sub Class */
   unsigned char    BaseClass;  /* 0B: Base      */
   unsigned char    CacheLineSize;       /* 0C: Cache Line */
   unsigned char    LatencyTimer;        /* 0D: Latency */
   unsigned char    HeaderType;          /* 0E: Header */
   unsigned char    BIST;                /* 0F: BIST */
   unsigned long    BaseAddresses[6];    /* 10: Base Address */
   unsigned long    CIS;                 /* 28: CIS Pointer */
   unsigned short   SubVendorID;         /* 2C: Subsystem Vendor ID */
   unsigned short   SubSystemID;         /* 2E: Subsystem ID */
   unsigned long    ROMBaseAddress;      /* 30: ROM Base Address */
   unsigned char    CapabilitiesPtr;     /* 34: Capabilities Pointer */
   unsigned char    Reserved1[3];        /* 35: Reserved */
   unsigned long    Reserved2[1];        /* 38: Reserved */
   unsigned char    InterruptLine;       /* 3C: INT Line */
   unsigned char    InterruptPin;        /* 3D: INT Pin  */
   unsigned char    MinimumGrant;        /* 3E: MIN_GNT  */
   unsigned char    MaximumLatency;      /* 3F: MAX_LAT  */
   unsigned char    DeviceSpecific[192]; /* 40:          */
} PCI_DEVICE_INFO, *PPCI_DEVICE_INFO;

/* structure of board information  */
typedef struct _BOARD_CONF_TBL{
	unsigned int   nType;
	unsigned int   nSubSystemId;
	char           cName[MAX_DEVNAME_LENGTH];
	unsigned int   offset;
	char		   append[MAX_MULTINAME_LENGTH];
} BOARD_CONF_TBL, *PBOARD_CONF_TBL;

/* structure of driver resource  */
typedef struct _DRV_RES_TBL {
	unsigned short  type;       /* board id */
	unsigned char   revid;      /* revision ID */
	unsigned short  subsys;     /* sub system ID */
	unsigned char   rsw;        /* RSW1 */
	char            name[MAX_DEVNAME_LENGTH];     /* board full name */
	char            append[MAX_MULTINAME_LENGTH]; /* name of appendix */
} DRV_RES_TBL, *PDRV_RES_TBL;

/* Structure of I/O Port Read/Write Data */
typedef struct _IO_RW {
	unsigned long uType;
	unsigned long uAdrs;
	unsigned long uData;
} IO_RW, *PIO_RW;

/* Structure of Memory Read/Write Data */
typedef struct _MEM_RW {
	unsigned long uType;
	unsigned long uAdrs;
	unsigned long uData;
} MEM_RW, *PMEMRW;

/* Structure of BoardID access configuration */
typedef struct _IDACCESS_CONFIG {
	unsigned long AccessType;  /* 0:I/O 1:Memory 2:PciCfg */
	unsigned long BitShift;    /* access bit */
	unsigned long Address;     /* access address */
	void          *PciDevice;  /* pci_dev struct(2 only) */
} IDACCESS_CONFIG, *PIDACCESS_CONFIG;

/* =========================================================================
	function prototypes
============================================================================ */
unsigned long IFReadBoardIDForCardBus(PIDACCESS_CONFIG);
unsigned long IFWriteBoardIDForCardBus(PIDACCESS_CONFIG, unsigned long);


/* =========================================================================
	ioctl control code
============================================================================ */
/* Magic Number 'X' */
#define COMMON_IOC_MAGIC 'X'

#define IOCTL_COMMON_GET_PCI_DEVICE_INFO _IOR(COMMON_IOC_MAGIC, 1, PCI_DEVICE_INFO)
#define IOCTL_COMMON_IO_READ		_IOR(COMMON_IOC_MAGIC, 2, IO_RW)
#define IOCTL_COMMON_IO_WRITE		_IOW(COMMON_IOC_MAGIC, 3, IO_RW)
#define IOCTL_COMMON_MEMORY_READ	_IOR(COMMON_IOC_MAGIC, 4, MEM_RW)
#define IOCTL_COMMON_MEMORY_WRITE	_IOW(COMMON_IOC_MAGIC, 5, MEM_RW)
#define IOCTL_COMMON_FIND_DEV		_IOW(COMMON_IOC_MAGIC, 9, BOARD_CONF_TBL)
#define IOCTL_COMMON_GET_DEV		_IOR(COMMON_IOC_MAGIC,10, DRV_RES_TBL)
#define IOCTL_COMMON_WRITE_BOARDID  _IOW(COMMON_IOC_MAGIC,11, unsigned long)


