/********************************************
   dpgutil.h -source code of interface device setup program
   -------------------------------------------------------------
   Version 1.30-07
   -------------------------------------------------------------
   Date 2006/11/01
   -------------------------------------------------------------
   Copyright 2003, 2006 Interface Corporation. All Rights Reserved.
 *******************************************/
/* structure of global operation */
#define MAX_DEVNAME_LENGTH     256
#define MAX_MULTINAME_LENGTH   32
#define MAX_DEVNODENAME_LENGTH 256

/* symbol of udev file system */
#define UDEV_FILESYSTEM  "/etc/udev"
#define UDEV_DIRECTORY   "/etc/udev/devices"
#define UDEV_LINKSCONF   "/etc/udev/links.conf"
#define UDEV_DIRECTORY_LIB   "/lib/udev/devices"

/* symbol of debian system version */
#define DEBIAN_VERSION   "/etc/debian_version"

/* structure of board information  */
/* fixed 1.20:change member's array size 'multi', 'bname' */
typedef struct _GET_BOARD_INF{
	int dnum;
	int type;
	int rsw;
	int subid;
	char devnode[MAX_DEVNODENAME_LENGTH];
	char multi[MAX_MULTINAME_LENGTH];
	char bname[MAX_DEVNAME_LENGTH];
	char boardconfig[32];
} GET_BOARD_INF, *PGET_BOARD_INF;

/* structure of board configuration  */
/* fixed 1.20:change member's array size 'cName', 'append' */
typedef struct _BOARD_CONF_TBL{
	unsigned int   nType;
	unsigned int   nSubSystemId;
	char           cName[MAX_DEVNAME_LENGTH];
	unsigned int   offset;
	char           append[MAX_MULTINAME_LENGTH];
} BOARD_CONF_TBL, *PBOARD_CONF_TBL;

/* structure of driver resource  */
/* fixed 1.20:change member's array size 'name', 'append' */
typedef struct _DRV_RES_TBL {
	unsigned short type;        /* board id */
	unsigned char  revid;       /* revision ID */
	unsigned short subsys;      /* sub system ID */
	unsigned char  rsw;         /* RSW1 */
	char           name[MAX_DEVNAME_LENGTH];     /* board full name */
	char           append[MAX_MULTINAME_LENGTH]; /* name of appendix */
} DRV_RES_TBL, *PDRV_RES_TBL;

/* structure of get device name or numvber by RSW1 */
/* fixed 1.20:change member name 'devicenumber' */
typedef struct _GET_DEVRSW {
	unsigned int devid;
	unsigned int sysid;
	unsigned int rsw;
	unsigned char multi[10];
	unsigned char devicenumber[20];
} GET_DEVRSW, *PGET_DEVRSW;

/* 2003/05/13 fix */
/*#define SEPARATORS " /,\n[]="*/
#define SEPARATORS      " ,\n[]="
#define SECT_SEPARATORS "\n[]"
#define CARDEXE         "ifcardsetup"

/* Magic Number 'X' */
#define COMMON_IOC_MAGIC 'X'
#define ULSIZE  (sizeof(unsigned long))
#define IOCTL_COMMON_FIND_DEV	_IOW(COMMON_IOC_MAGIC, 9, BOARD_CONF_TBL)
#define IOCTL_COMMON_GET_DEV	_IOR(COMMON_IOC_MAGIC,10, DRV_RES_TBL)
#define IOCTL_COMMON_GET_DEVRSW	_IOR(COMMON_IOC_MAGIC,11, GET_DEVRSW)
#define IOCTL_COMMON_WRITE_BOARDID _IOW(COMMON_IOC_MAGIC,11, ULSIZE * 2)

#define cls(l); { short i; for(i = l; i < 25; i++) lineclr(i); }
#define Locate(x, y) printf("\x1b[%d;%dH", (y)+1, (x)+1)
#define lineclr(l) printf("\x1b[s\x1b[%d;1H\x1b[0K\x1b[u", l+1);

void Error(int);
void remove_module(char *);
int  try_insmod(char *, char *);
int  try_modprobe(char *, char *);
int  lookup_dev(char *);
void DeleteDevNode(int, int, int, int, char *, char *);
