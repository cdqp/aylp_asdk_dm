/* *************************************************************
   cardutil.c -source code of interface device setup program
   -------------------------------------------------------------
   Version 1.50-09
   -------------------------------------------------------------
   Date 2011/09/22
   -------------------------------------------------------------
   Copyright 2002, 2011 Interface Corporation. All Rights Reserved.
   ************************************************************** */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
#include<fcntl.h>
#include<unistd.h>
#include<ctype.h>
#include<errno.h>

#include "gpgconf.h"
#include "dpgutil.h"

typedef struct {
	unsigned short DeviceID;
	unsigned short SubsystemID;
	unsigned long  DevFn;
	unsigned char  BusNumber;
	unsigned char  BoardID;
	unsigned char  AccessType;
	unsigned char  BitShift;
	unsigned long  Address;
	void          *PciDevice;
} RESOURCE_TBL, *PRESOURCE_TBL;

RESOURCE_TBL Resource[256];
extern char buffer[];
extern char rootpath[];

int    tempfd;
int    change_flag = 0;

#define BUS_TYPE(subsys) (((subsys) & 0x2000) ? (((subsys) >> 10) & 7) : (((subsys) >> 8) & 7))

#define PCI_SLOT(devfn)    (((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)    ((devfn) & 0x07)

extern int GetBoardInfo(int *, int *, int *, char *, FILE *);
extern int GetSectionName(char *, FILE *);


/* =============================================== */
/* QueryBoardSection()                             */
/* =============================================== */
int QueryBoardSection(char *FileName, unsigned short DeviceID,
					  unsigned short SubsystemID, char *BoardName)
{
	FILE *fp;
	int  devid, subsys, rsw;
	char multi[MAX_MULTINAME_LENGTH];
	char SectionName[MAX_DEVNAME_LENGTH];
	int  ret;
	int  FoundName = 0;

	fp = fopen(FileName, "r");
	if(fp == NULL) return -ENODEV;

	while(1) {
		ret = GetSectionName(SectionName, fp);
		if(ret != 0) break;

		if(strcmp(SectionName, "Settings") == 0) continue;
		while(1) {
			ret = GetBoardInfo(&devid, &subsys, &rsw, multi, fp);
			if(ret == -4) continue;
			if(ret < 0)   break;

			if(((unsigned short)devid  == DeviceID) &&
			   ((unsigned short)subsys == SubsystemID)) {
				strcpy(BoardName, SectionName);
				FoundName = 1;
				break;
			}
		}
		if(FoundName == 1) break;
	}
	fclose(fp);

	if(FoundName == 0) {
		return -ENODEV;
	}
	return 0;
}


/* =============================================== */
/* QueryBoardName()                                */
/* =============================================== */
int QueryBoardName(unsigned short DeviceID, unsigned short SubsystemID,
				   char *BoardName)
{
	char DirectoryPath[256];
	char FileName[256];
	DIR *dp;
	struct dirent *entry;
	struct stat    statbuf;
	int  FoundName = 0;

	sprintf(DirectoryPath, "%s/etc/interface", rootpath);
	dp = opendir(DirectoryPath);
	if(dp == NULL) {
		return -ENODEV;
	}
	chdir(DirectoryPath);

	while(1) {
		entry = readdir(dp);
		if(entry == NULL) break;
		stat(entry->d_name, &statbuf);
		if(S_ISDIR(statbuf.st_mode)) continue;
		if(strstr(entry->d_name, ".inf") == NULL) continue;

		sprintf(FileName, "%s/%s", DirectoryPath, entry->d_name);
		if(QueryBoardSection(FileName, DeviceID, SubsystemID, BoardName)) {
			continue;
		}
		FoundName = 1;
		break;
	}
	closedir(dp);

	if(FoundName == 0) {
		return -ENODEV;
	}
	return 0;
}


/* =============================================== */
/* disp_card()                                     */
/* =============================================== */
void disp_card(int number)
{
	int  i;
	char prefix[16];
	int  modelnumber;
	char ModelName[MAX_DEVNAME_LENGTH];

	printf("==========================================================="
		   "====\n");
	printf(" Ref.ID | Bus | Dev | Func| Model                          "
		   "| RSW1 \n");
	for(i = 0; i < number; i++) {
		printf("-------------------------------------------------------"
			   "--------\n");

		if(QueryBoardName(Resource[i].DeviceID,
						  Resource[i].SubsystemID, ModelName)) {
			switch(BUS_TYPE(Resource[i].SubsystemID)) {
			case 1: /* Compact PCI */
				strcpy(prefix, "CTP-");
				break;
			case 2: /* CardBus */
				strcpy(prefix, "CBI-");
				break;
			case 0: /* PCI */
			default:
				strcpy(prefix, "PCI-");
				break;
			}

			if(Resource[i].SubsystemID & 0x2000) {
				modelnumber  = Resource[i].DeviceID * 100;
				modelnumber += (Resource[i].SubsystemID & 0x7F);
			}
			else {
				modelnumber  = Resource[i].DeviceID;
			}
			sprintf(ModelName, "%s%d", prefix, modelnumber);
		}
		else {
			if(strlen(ModelName) > 30) {
				ModelName[29] = '.';
				ModelName[30] = 0;
			}
		}
		printf("     %2d | %2d  | %2ld  | %2ld  | %-30s |   %2x \n",
			   i+1, Resource[i].BusNumber, PCI_SLOT(Resource[i].DevFn),
			   PCI_FUNC(Resource[i].DevFn),
			   ModelName, Resource[i].BoardID);
	}
	printf("==========================================================="
		   "====\n");
}


/* =============================================== */
/* input_boardid()                                 */
/* =============================================== */
int input_boardid(int number)
{
	int command;
	int index;
	int ret;
	unsigned long IoBuffer[16];

	printf("Enter Ref.ID: ");
	fgets(buffer, 255, stdin);
	sscanf(buffer, "%d", &command);
	if(command < 1 || command > number) {
		system("clear");
		printf(" Invalid number\n");
		return -1;
	}
	index = command;
	printf("Enter the board id number (0-15). \n");
	printf("If you want to cancel this operation, enter -1.\n");
	printf(": ");
	fgets(buffer, 255, stdin);
	sscanf(buffer, "%d", &command);
	if(command >= 0 && command <= 15) {
		IoBuffer[0] = (unsigned long)(index - 1);
		IoBuffer[1] = (unsigned long)command;
		ret = ioctl(tempfd, IOCTL_COMMON_WRITE_BOARDID, &IoBuffer[0]);
		system("clear");
		if(ret == 0) {
			Resource[index - 1].BoardID = command;
			change_flag = 1;
		}
		if(ret) {
			printf("errno=%d", errno);
		}
		return 0;
	}
	system("clear");
	printf("The specified board id number is invalid.\n");
	return -1;
}


/* =============================================== */
/* edit_boardid()                                  */
/* =============================================== */
int edit_cardid(int number)
{
	int ret;
	int command;

	while(1) {
		printf("************** Command *******************\n");
		printf("  1. Change the board id number.          \n");
		printf("  2. Run the device number setup utility. \n");
		printf(" 99. Exit the program.                    \n");
		printf("******************************************\n");
		printf("Enter the command number: ");
		fgets(buffer, 255, stdin);
		sscanf(buffer, "%d", &command);
		switch(command) {
		case 1:
			ret = input_boardid(number);
			if(ret < 0) return ret;
			break;
		case 2:
			return 2;
		case 99:
			return 1;
		}
		break;
	}
	return 0;
}


/* =============================================== */
/* CardSetup()                                     */
/* =============================================== */
int CardSetup(void)
{
	int dpg0101_major;
	int ret;
	char cmd[16];
	int length;

	change_flag = 0;
	/* === load dpg0101 module === */
	dpg0101_major = lookup_dev("dpg0101");
	if(dpg0101_major < 0) {
		if(dpg0101_major == -ENODEV) {
			try_insmod("dpg0101", "-f");
			dpg0101_major = lookup_dev("dpg0101");
		}
	}
	if(dpg0101_major < 0) {
		Error(dpg0101_major);
		return dpg0101_major;
	}

	sprintf(cmd, "/dev/dpg0101");
	remove(cmd);
	mknod(cmd, (S_IFCHR | 00666), (dpg0101_major << 8) | 0);

	tempfd = open("/dev/dpg0101", O_RDWR);
	if(tempfd < 0) {
		return -1;
	}

	length = read(tempfd, &Resource[0], 1);
	length /= sizeof(RESOURCE_TBL);
	if(length == 0) {
		printf("CardBus device not found.\n");
		return 0;
	}

	system("clear");

	printf("\n");
	printf("**************************************************\n");
	printf(" CardBus Setup Utility  \n");
	printf("--------------------------------------------------\n");
	printf(" Version: 1.50-09  \n");
	printf("--------------------------------------------------\n");
	printf(" Copyright 2002, 2011      Interface Corporation. \n");
	printf("                             All rights reserved. \n");
	printf("**************************************************\n");
	printf("\n");

	while(1) {
		disp_card(length);
		ret = edit_cardid(length);
		if(ret != 0) break;
	}
	close(tempfd);
	remove(cmd);

	if(change_flag) {
		printf("To identify each card, we recommend to put a sticker with a ID number on the card.");
	}
	return ret;
}
