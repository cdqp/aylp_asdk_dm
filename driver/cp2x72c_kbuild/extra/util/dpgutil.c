/* *************************************************************
   dpgutil.c -source code of interface device setup program
   -------------------------------------------------------------
   Version 1.50-09
   -------------------------------------------------------------
   Date 2011/09/22
   -------------------------------------------------------------
   Copyright 2003, 2011 Interface Corporation. All Rights Reserved.
   ************************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <syslog.h>

#include "gpgconf.h"
#include "dpgutil.h"

//char bname[20];
char devnode[MAX_DEVNODENAME_LENGTH];
char rswname[20];

char boardconf[255][30];
GET_BOARD_INF boardinf[255];
DRV_RES_TBL devfind[255];
char model[32];
char szMethod[10], szMulti[10];
char rootpath[256];
char buffer[256];
int  major = -1;
int  exeflag = 0;
int  delflag = 0;
int  mode = 0;
int  quiet = 0;
int  kernel26 = 0;
char *modpath = NULL;

int  EnumerateDevice(FILE *);
int  CheckDuplication(FILE *);
int  ReadDeviceConf(void);
int  DeleteMissingDeviceKey(FILE *);
int  CardSetup(void);
FILE *SetupDevice(void);
FILE *ReadInfSettings();

int  disp(int);
int  save_data(int);
int  lookup_dev(char *);
int  ExecuteConfigurator(FILE *, int);

int  device_list_t(int);
void topmenu_t(void);

void readlines(FILE *fp, char *Buffer, int length);
int SearchNodenamePos(FILE *fp, PCSTR szNodeName,
					long nSearchStartPos, long nSearchEndPos,
					long *pnNodenameStartPos, long *pnNodenameEndPos);



/* ======= Error Routine ========================== */
void Error(int ret)
{
	char szError[80];

	switch(ret) {
	case -1: 
		sprintf(szError,"system error. errno:%d\n",errno);
		perror(szError); 
		break;

	case -2:  printf(" >> The device driver module is not loaded.\n"); break;
	case -3:  printf(" >> The file cannot be opened.\n"); break;
	case -4:  printf(" >> The string of the section cannot be retrieved from the INF file.\n"); break;
	case -10: printf(" >> Command Execution error. \n"); break;
	default:  printf(" >> Another error\n"); break;
	}
	return;
}



/* =============================================== */
void MakeDevNode(int type, int rsw, int subsys, int dnum, 
				 char *app, char *node)
{
	struct stat file_stat;
	char nodename[128];
	char searchname[128];
	int ret;

	if(app[0] == '\0') {
		sprintf(nodename, "%s/dev/pci%d.%04x.%x", rootpath, type,subsys,rsw);
		sprintf(node, "pci%d.%04x.%x", type,subsys,rsw);
	}
	else {
		sprintf(nodename, "%s/dev/pci%d.%04x.%x.%s", rootpath, type,subsys,rsw,
				app);
		sprintf(node, "pci%d.%04x.%x.%s",type,subsys,rsw,app);
	}
	remove(nodename);
	mknod(nodename, (S_IFCHR | 00666), (major << 8) | dnum);

	// check udev file system
	ret = stat(UDEV_FILESYSTEM, &file_stat);
	if(ret >= 0) {
		if(S_ISDIR(file_stat.st_mode)) {
			// check debian
			ret = stat(DEBIAN_VERSION, &file_stat);
			if(ret < 0) {
				ret = mkdir(UDEV_DIRECTORY, 0755);
			}
		}
	}

	// Fedora14 (2011/9/22)
	ret = stat(UDEV_DIRECTORY_LIB, &file_stat);
	if(ret >= 0) {
		if(S_ISDIR(file_stat.st_mode)) {
			sprintf(nodename, UDEV_DIRECTORY_LIB "/%s", node);
			remove(nodename);
			ret = mknod(nodename, (S_IFCHR | 00666), (major << 8) | dnum);
		}
	}

	// redhat
	ret = stat(UDEV_DIRECTORY, &file_stat);
	if(ret >= 0) {
		if(S_ISDIR(file_stat.st_mode)) {
			sprintf(nodename, UDEV_DIRECTORY "/%s", node);
			remove(nodename);
			ret = mknod(nodename, (S_IFCHR | 00666), (major << 8) | dnum);
		}
	}

	// debian (2006/11/01)
	ret = stat(UDEV_LINKSCONF, &file_stat);
	if(ret >= 0) {
		if(S_ISREG(file_stat.st_mode)) {
			FILE * fp;
			sprintf(nodename, "M %s c %3d %3d -m 0666\n", node, major, dnum);
			sprintf(searchname, "%s", node);
			fp = fopen(UDEV_LINKSCONF, "r+");
			if (fp) {
				long nNodenameStartPos = 0;
				long nNodenameEndPos = 0;

				if (!SearchNodenamePos(fp, searchname, 0, 0, &nNodenameStartPos, &nNodenameEndPos)) {
#if 0
					printf("nNodenameStartPos=%ld\n", nNodenameStartPos);
					printf("nNodenameEndPos=%ld\n", nNodenameEndPos);
					printf("size=%ld(%ld)\n", (long)strlen(nodename), nNodenameEndPos - nNodenameStartPos);
#endif
					fseek(fp, nNodenameStartPos, SEEK_SET);
					fwrite(nodename, sizeof(char), strlen(nodename), fp);
				}
				fclose(fp);
			}
		}
	}

	return;
}

//$$$ Mode addition (2007/05/31)
void DeleteDevNode(int type, int rsw, int subsys, int dnum, 
					char *app, char *node)
{
	struct stat file_stat;
	char nodename[128];
	char searchname[128];
	int ret;
//	int i;

	if(app[0] == '\0') {
		sprintf(nodename, "%s/dev/pci%d.%04x.%x", rootpath, type,subsys,rsw);
		sprintf(node, "pci%d.%04x.%x", type,subsys,rsw);
	}
	else {
		sprintf(nodename, "%s/dev/pci%d.%04x.%x.%s", rootpath, type,subsys,rsw,
				app);
		sprintf(node, "pci%d.%04x.%x.%s",type,subsys,rsw,app);
	}
	remove(nodename);

	// Fedora14 (2011/9/22)
	ret = stat(UDEV_DIRECTORY_LIB, &file_stat);
	if(ret >= 0) {
		if(S_ISDIR(file_stat.st_mode)) {
			sprintf(nodename, UDEV_DIRECTORY_LIB "/%s", node);
			remove(nodename);
		}
	}

	// redhat
	ret = stat(UDEV_DIRECTORY, &file_stat);
	if(ret >= 0) {
		if(S_ISDIR(file_stat.st_mode)) {
			sprintf(nodename, UDEV_DIRECTORY "/%s", node);
			remove(nodename);
		}
	}

	// debian
	ret = stat(UDEV_LINKSCONF, &file_stat);
	if(ret >= 0) {
		if(S_ISREG(file_stat.st_mode)) {
			FILE * fp;
			sprintf(nodename, "# %s c %3d %3d -m 0666\n", node, major, dnum);
			sprintf(searchname, "%s", node);
			fp = fopen(UDEV_LINKSCONF, "r+");
			if (fp) {
				long nNodenameStartPos = 0;
				long nNodenameEndPos = 0;

				if (!SearchNodenamePos(fp, searchname, 0, 0, &nNodenameStartPos, &nNodenameEndPos)) {
					fseek(fp, nNodenameStartPos, SEEK_SET);
					fwrite(nodename, sizeof(char), strlen(nodename), fp);
#if 0
					for (i = 0; i < (strlen(nodename) - 1); i++) {
						fwrite(" ", sizeof(char), 1, fp);
					}
					fwrite("\n", sizeof(char), 1, fp);
#endif
				}
				fclose(fp);
			}
		}
	}

	return;
}
//$$$

void readlines(FILE *fp, char *Buffer, int length)
{
	fgets(Buffer, length, fp);
	if(Buffer[0] > 0) {
		Buffer[strlen(Buffer) - 1] = 0;
	}
}

int SearchNodenamePos(
	FILE	*fp,						//
	PCSTR	szNodeName,					//
	long	nSearchStartPos,			//
	long	nSearchEndPos,				//
	long	*pnNodenameStartPos,		//
	long	*pnNodenameEndPos			//
	)
{
	char szBuffer[256];
	long nCurrentPos = 0;

	//initialize
	*pnNodenameStartPos = 0;
	*pnNodenameEndPos = 0;

	fseek(fp, nSearchStartPos, SEEK_SET);

	while(1) {
		nCurrentPos = ftell(fp);

		if((nCurrentPos <= nSearchEndPos) && (nSearchEndPos > 0)) {
			break;
		}

		if(feof(fp) != 0) break;
		readlines(fp, szBuffer, sizeof(szBuffer));

		if(strstr(szBuffer, szNodeName)) {
			break;
		}
	}
	if((nCurrentPos > nSearchEndPos) && (nSearchEndPos > 0)) {
		return -1;
	}
	*pnNodenameStartPos = nCurrentPos;
	*pnNodenameEndPos = ftell(fp);

	fseek(fp, nSearchStartPos, SEEK_SET);
	return 0;
}

int CheckModpath(void)
{
	struct utsname uts;

	putenv("PATH=/bin:/sbin:/usr/bin:/usr/sbin");
	if(uname(&uts) != 0) {
		printf("uname execute error\n");
		return -1;
	}
	modpath = (char *)malloc(strlen(uts.release) + 14);
	sprintf(modpath, "/lib/modules/%s", uts.release);
	if (strstr(uts.release, "2.2")) {
		kernel26 = 0;
	} else if (strstr(uts.release, "2.4")) {
		kernel26 = 0;
	} else {
		kernel26 = 1;
	}
	if(access(modpath, X_OK) != 0) {
		printf("cannot access %s: %m\n", modpath);
		return -1;
	}
	return 0;
}

/* =============================================== */
void SortDevice(void)
{
	int i,j;
	GET_BOARD_INF dummy;

	for(i = 0; i < 254; i++){
		if(boardinf[i].dnum == 0) break;

		if(boardinf[i].type == 0) {
			continue;
		}
		for(j = i + 1; j < 255; j++) {
			if(boardinf[j].dnum == 0)break;
			if(boardinf[j].type != 0 && 
			   boardinf[i].dnum > boardinf[j].dnum){
				memcpy(&dummy, &boardinf[i], sizeof(GET_BOARD_INF));
				memcpy(&boardinf[i], &boardinf[j], sizeof(GET_BOARD_INF));
				memcpy(&boardinf[j], &dummy, sizeof(GET_BOARD_INF));
			}//if type
		} //for j
	} //for
}

		
/* =============================================== */
int InitDeviceTable(FILE *fp)
{
	int ret;

	EnumerateDevice(fp);
	if(devfind[0].type == 0) {
		printf(" No Interface PCI/CompactPCI board is found.\n");
		return -1;
	}
	ret = CheckDuplication(fp);
	if(ret) return ret;

	return 0;
}

/* =============================================== */
int QueryBoardIDToDriver(int num)
{
	int i, j, ret, fd, c;
	int driver_major;
	int sum = 0;
	char devmake[64];
	char drivername[64];
	GET_DEVRSW GetDevRsw;

	sprintf(drivername, "cp%s", model);
	driver_major = lookup_dev(drivername);
	if(driver_major < 0) {
		ret = try_insmod("dpg0100", "-f");
		ret = try_insmod(drivername, "-f");
		if(ret != 0) {
			try_modprobe(drivername, "");
		}
		driver_major = lookup_dev(drivername);
		if(driver_major < 0) {
			Error(driver_major);
			return -1;
		}
	}

	sprintf(devmake, "/dev/dpg0101");
	unlink(devmake);
	mknod(devmake, (S_IFCHR | 00666), (driver_major << 8) | 0);

	fd = open("/dev/dpg0101", O_RDWR);
	for(j = 0; j < num; j++) {
		sum = 0;
		GetDevRsw.devid = boardinf[j].type;
		GetDevRsw.sysid = boardinf[j].subid;
		GetDevRsw.rsw   = boardinf[j].rsw;
		sprintf((char*)GetDevRsw.multi, "%s", boardinf[j].multi);

		ret = ioctl(fd, IOCTL_COMMON_GET_DEVRSW, 
					 (unsigned long)&GetDevRsw);
		if(ret) {
			close(fd);
			Error(ret);
			return -1;
		}
		i = 0;
		while((c = (int)GetDevRsw.devicenumber[i])) {
			if(isdigit(c)) {
				do {
					sum = sum*10+c - '0';
					i++;
					c = (int)GetDevRsw.devicenumber[i];
				} while(isdigit(c));
				break;
			}
			rswname[i] = GetDevRsw.devicenumber[i];
			i++;
		}

		boardinf[j].dnum = sum;
		/* *** Create a device node. *** */	
		MakeDevNode(boardinf[j].type, boardinf[j].rsw,
					boardinf[j].subid, boardinf[j].dnum, 
					boardinf[j].multi, boardinf[j].devnode); 
	}
	close(fd);
	remove(devmake);

	return 0;
}

int AllocateBoardID(int total)
{
	int j, k, dnum;
	int flag;
	struct stat file_stat;
	int ret;

	for(j = 0; j < total; j++) {
		if(boardinf[j].dnum != -1) {
			ret = stat(boardinf[j].devnode, &file_stat);
			if(ret == 0) {
				if(((file_stat.st_rdev >> 8) & 0xff) == major) {
					continue;
				}
			}
			MakeDevNode(boardinf[j].type,  boardinf[j].rsw,
						boardinf[j].subid, boardinf[j].dnum,
						boardinf[j].multi, boardinf[j].devnode);
			continue;
		}
		/* alloc number */
		for(dnum = 0; dnum < 255; dnum++) {
			flag = 0;
			for(k = 0; k < 255; k++) {
				if(boardinf[k].dnum == (dnum+1)) break;
				else if(boardinf[k].dnum == 0) {
					boardinf[j].dnum = dnum+1;
					flag = 1;
					break;
				}
			} 
			if(flag) break;
		}
		if(dnum == 255) {
			return -1;
		}
		MakeDevNode(boardinf[j].type, boardinf[j].rsw,
					boardinf[j].subid, boardinf[j].dnum, 
					boardinf[j].multi, boardinf[j].devnode);
	}
	return 0;
}

/* =============================================== */
//$$$ Mode addition (2007/05/31)
int DeleteBoardID(int total)
{
	int j, k, dnum;
	int flag;
	struct stat file_stat;
	int ret;
	char categoly[64];

	sprintf(categoly, "gpg%s", model);

	for(j = 0; j < total; j++) {
		if(boardinf[j].dnum != -1) {
			ret = stat(boardinf[j].devnode, &file_stat);
			if(ret == 0) {
				if(((file_stat.st_rdev >> 8) & 0xff) == major) {
					continue;
				}
			}
			DeleteDevNode(boardinf[j].type,  boardinf[j].rsw,
						  boardinf[j].subid, boardinf[j].dnum,
						  boardinf[j].multi, boardinf[j].devnode);

			/* An applicable key is deleted from CONF file */
			ret = GpgConfDeleteBaseKey(
									categoly,
									boardinf[j].type, boardinf[j].subid,
									boardinf[j].rsw, boardinf[j].multi);
			continue;
		}
		/* alloc number */
		for(dnum = 0; dnum < 255; dnum++) {
			flag = 0;
			for(k = 0; k < 255; k++) {
				if(boardinf[k].dnum == (dnum+1)) break;
				else if(boardinf[k].dnum == 0) {
					boardinf[j].dnum = dnum+1;
					flag = 1;
					break;
				}
			} 
			if(flag) break;
		}
		if(dnum == 255) {
			return -1;
		}
		DeleteDevNode(boardinf[j].type, boardinf[j].rsw,
					  boardinf[j].subid, boardinf[j].dnum, 
					  boardinf[j].multi, boardinf[j].devnode);

		/* An applicable key is deleted from CONF file */
		ret = GpgConfDeleteBaseKey(
								categoly,
								boardinf[j].type, boardinf[j].subid,
								boardinf[j].rsw, boardinf[j].multi);
	}
	return 0;
}
//$$$

/* =============================================== */
int operate_dev(int num, int nNo, int *number)
{
	int inum, dnum, nRet;
	char devmake[40], *multi;
//	struct stat file_stat;
//	int ret;

	printf("Enter Ref.ID: ");
	fgets(buffer, 255, stdin);
	sscanf(buffer, "%d", &inum);
	if(inum < 1 || inum >= nNo || boardinf[inum-1].type == 0) {
		system("clear");
		printf(" Invalid number \n");
		return -1;
	}

	if(num == 1) {
		printf("Enter the device number (1-255). \n");
		printf("If you want to cancel this operation, enter 0.\n");
		printf(": ");
		fgets(buffer, 255, stdin);
		sscanf(buffer, "%d", &dnum);
		if(dnum < 1 || dnum > 255 ){
			system("clear");
			printf("The specified device number is invalid.\n");
			return -1;
		}

		//$$$ Mode addition (2007/05/31)
//		sprintf(devmake, "%s/dev/%s", rootpath, boardinf[inum-1].devnode);
//		remove(devmake);

		DeleteDevNode(boardinf[inum-1].type,  boardinf[inum-1].rsw,
					  boardinf[inum-1].subid, boardinf[inum-1].dnum,
					  boardinf[inum-1].multi, boardinf[inum-1].devnode);
		//$$$

		boardinf[inum-1].dnum = dnum;
		MakeDevNode(boardinf[inum-1].type,  boardinf[inum-1].rsw,
					boardinf[inum-1].subid, boardinf[inum-1].dnum,
					boardinf[inum-1].multi, boardinf[inum-1].devnode);
	}
	else if(num == 2){ /* *** if num == 2 *** */
		sprintf(devmake, "gpg%s", model);
		if(boardinf[inum-1].multi[0] == '\0') multi = NULL;
		else			          multi = boardinf[inum-1].multi;
		nRet = GpgConfDeleteBaseKey(
			devmake,
			boardinf[inum-1].type,
			boardinf[inum-1].subid,
			boardinf[inum-1].rsw,
			multi
			);
		if(nRet){
			Error(-4);
			return -1;
		}
		memset(devmake,'\0',sizeof(devmake));

		//$$$ Mode addition (2007/05/31)
//		sprintf(devmake, "%s/dev/%s", rootpath, boardinf[inum-1].devnode);
//		remove(devmake);

		DeleteDevNode(boardinf[inum-1].type,  boardinf[inum-1].rsw,
					  boardinf[inum-1].subid, boardinf[inum-1].dnum,
					  boardinf[inum-1].multi, boardinf[inum-1].devnode);
/*
		// redhat
		ret = stat(UDEV_DIRECTORY, &file_stat);
		if(ret >= 0) {
			if(S_ISDIR(file_stat.st_mode)) {
				sprintf(devmake, UDEV_DIRECTORY "/%s", boardinf[inum-1].devnode);
				remove(devmake);
			}
		}
*/
		//$$$

		boardinf[inum-1].dnum = 0;
		boardinf[inum-1].type = 0;
		boardinf[inum-1].rsw = -1;
	}
	else if(num == 4) {
		save_data(nNo);
		*number = inum-1;
	}

	system("clear");

	return 0;
}


/* =============================================== */
int end_next_prog(int nNo)
{
	int j, nRet;
	int flag, k;

	flag = 0;
	for(j = nNo; j >= 0; j--){
		for(k = 0; k < j; k++){
			if((boardinf[j].type != 0) &&
			   (boardinf[j].dnum == boardinf[k].dnum)) {
				flag = 1;
				break;
			}
		}
		if(flag) break;
	}
	if(flag) {
		system("clear");
		nRet = disp(nNo);
		printf(" The device number %d is already used.\n", boardinf[j].dnum);
		printf(" Change the device number again.\n");
		return -1;
	}
	else {
		nRet = save_data(nNo);
		return(nRet);
	}//else flag
}


/* =============================================== */
int Change_Dev(int nNo, int *inum)
{
	int num, nRet, __attribute__((unused)) dummy;

	while(1) {
		printf("************** Command ***************\n");
		printf("  1. Change the device number.        \n");
		printf("  2. Delete the device number.        \n");
		printf("  3. Load new device setting file.    \n");
		printf("  4. Run the initialization program.  \n");
		printf("  5. Run the CardBus ID setup utility.\n");
		printf(" 99. Exit the program.                \n");
		printf("**************************************\n");
		printf("Enter the command number: ");
		fgets(buffer, 255, stdin);
		sscanf(buffer, "%d", &num);
		switch(num) {
		case 1:
			if(!strcmp(szMethod, "RSW1")) {
				system("clear");
				printf("The device number cannot be changed.\n");
				/* *** Display *** */
				dummy = disp(nNo);
				break;
			}
		case 2:	
			nRet = operate_dev(num, nNo, inum);
			/* *** Display *** */
			dummy = disp(nNo);
			break;
		case 4:
			nRet = operate_dev(num, nNo, inum);
			if(nRet == 0) return 2;
			dummy = disp(nNo);
			break;
		case 3: 
		case 5:
		case 99:
			nRet = end_next_prog(nNo);
			if(nRet) break;
			if(num == 3) {
				strcpy(model, "");
				/*@@@@@ 2006/2/10 by nakaya*/
				major = -1;
				/*@@@@@*/
				return 1;
			}
			if(num == 5) return 3;
			return 0;
		}//switch
	}//while
        
	return 0;
}


/* =============================================== */
int disp(int number)
{
	return device_list_t(number);
}

/* =============================================== */
int DisplayDevice(FILE *fp)
{
	int ret;
	int total;
	int num;

	total = ReadDeviceConf();
	if(total < 0) {
		return total;
	}

	//$$$ Mode addition (2007/05/31)
	if (delflag == 1) {
		/* *** Delete device node *** */
		DeleteBoardID(total);
		return 0;
	}
	//$$$

	if(!strcmp(szMethod, "soft")) { // by soft
		AllocateBoardID(total);
	}
	else { // by RSW1
		ret = QueryBoardIDToDriver(total);
		if(ret) return -1;
	}

	/* *** Sort Device Number *** */
	SortDevice();

	/* *** Display *** */
	total = disp(total);

	if(exeflag == 0) {
		while(1) {
			/* === Change devfind === */
			ret = Change_Dev(total, &num);
			if(ret == 1) {
				return 1;
			}
			if(ret == 2) {
				if(ExecuteConfigurator(fp, num)) {
					return -1;
				}
				system("clear");
				total = disp(total);
				continue;
			}
			if(ret == 3) {
				return 2;
			}
			break;
		}
	}
	else {
		ret = end_next_prog(total);
	}

	return 0;
}

/* =============================================== */
FILE *SetupDevice(void)
{
	FILE *fp;
	char drivername[16];
	int  ret;

	/* initialize table */
	sprintf(rswname, "NONE");	
	memset(&devfind[0], 0, sizeof(DRV_RES_TBL)*255);
	memset(&boardinf[0], 0, sizeof(GET_BOARD_INF)*255);
	memset(&boardconf[0][0], 0, sizeof(boardconf[0])*255);

	if((exeflag == 0) && (delflag == 0)) {
		topmenu_t();
		if(model[0] == '\0') {
			printf("Enter the model number of the product: GPG/GPH-");
			fgets(buffer, 255, stdin);
			sscanf(buffer, "%s", model);
		}
	}

	fp = ReadInfSettings();
	if(fp == NULL) {
		return NULL;
	}

	/* === Initialize device table === */
	ret = InitDeviceTable(fp);
	if(ret) {
		fclose(fp);
		return NULL;
	}

	/* === get driver major number === */
	if(major == -1) {
		sprintf(drivername, "cp%s", model);
		major = lookup_dev(drivername);
		if(major < 0) {
			ret = try_insmod("dpg0100", "-f");
			ret = try_insmod(drivername, "-f");
			if(ret != 0) {
				try_modprobe(drivername, "");
			}
			major = lookup_dev(drivername);
			if(major < 0) {
				Error(major);
				fclose(fp);
				return NULL;
			}
		}
	}

	/* === delete the missing device's key(CONF file) === */
	DeleteMissingDeviceKey(fp);

	return fp;
}

int main(int argc, char *argv[])
{
	int  ret;
	int  optch;
	int  errflag;
	int  i;
	int  length;
	int  exitcode = 0;
	char *ptr;
	FILE *fp;
	mode_t old_mask;

	strcpy(rootpath, "");
	strcpy(model, "");
	errflag = 0;

	while((optch = getopt(argc, argv, "qhcs:r:m:d:")) != -1) {
		switch(optch) {
		case 'q':
			quiet = 1;
			break;
		case 'h':
			errflag = 1;
			break;
		case 'c':
			mode = 1;
			break;
		case 's':
			exeflag = 1;
			strcpy(model, optarg);
			break;
		case 'r':
			strcpy(rootpath, optarg);
			break;
		case 'm':
			sscanf(optarg, "%d", &major);
			break;
		//$$$ Mode addition (2007/05/31)
		case 'd':
			delflag = 1;
			strcpy(model, optarg);
			break;
		//$$$
		}
	}
	if(errflag) {
		printf("usage: %s [-h] [-q] [-s modelnumber] [-r rootpath] "
			   "[-m majornumber] [-d modelnumber] \n",
			   argv[0]);
		return(0);
	}
	if(quiet == 1) {
		if((exeflag == 0) && (delflag == 0)) {
			quiet = 0;
		}
	}

	if((major < -1) || (major >= 0 && major < 3) || major > 255) {
		printf("specified major number is invalid.\n");
		return EXIT_FAILURE;
	}

	length = strlen(argv[0]);
	ptr = argv[0];
	for(i = 0; i < length - 1; i++) {
		if(strcmp(ptr, CARDEXE) == 0) {
			mode = 1;
			break;
		}
		ptr++;
	}

	CheckModpath();
	openlog("dpg0101", 0, LOG_INFO);

	/* === set create file permission === */
	old_mask = umask(0000);

	while(1) {
		if(mode) {
			ret = CardSetup();
			if(ret < 0) break;
			if(ret == 0 || ret == 1) break;
			if(ret == 2) {
				/* switch to device number setup utility */
				mode = 0;
			}
			if(mode) continue;
		}

		fp = SetupDevice();
		if(fp == NULL) {
			exitcode = EXIT_FAILURE;
			break;
		}

		/* === Display Device List === */
		ret = DisplayDevice(fp);
		if(ret < 0) {
			fclose(fp);
			exitcode = EXIT_FAILURE;
			break;
		}
		fclose(fp);
		if(ret == 1) {
			continue;
		}
		if(ret == 2) {
			mode = 1;
			continue;
		}
		break;
	}

	/* === reset create file permission === */
	umask(old_mask);
	free(modpath);
	closelog();
	return exitcode;
}
