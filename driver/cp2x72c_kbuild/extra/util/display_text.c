/* *************************************************************
   display_text.c -source code of interface device setup program
   -------------------------------------------------------------
   Version 1.50-09
   -------------------------------------------------------------
   Date 2011/09/22
   -------------------------------------------------------------
   Copyright 2003, 2011 Interface Corporation. All Rights Reserved.
   ************************************************************** */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
#include<fcntl.h>
#include<unistd.h>
#include<ctype.h>
#include<errno.h>

#include "gpgconf.h"
#include "dpgutil.h"

extern char rswname[];
extern char szMulti[];
extern GET_BOARD_INF boardinf[];
extern int quiet;

void topmenu_t(void)
{
	if(quiet) return;

	system("clear");
	printf("\n\n");
	printf("**************************************************\n");
	printf(" Setup Utility  \n");
	printf("--------------------------------------------------\n");
	printf(" Version: 1.50-09  \n");
	printf("--------------------------------------------------\n");
	printf(" Copyright 2003, 2011      Interface Corporation. \n");
	printf("                             All rights reserved. \n");
	printf("**************************************************\n");
	printf("\n");
}

int device_list_t(int number)
{
	int j;
	char var1[256];
	char var2[256];
	char DevNameBuffer[MAX_DEVNAME_LENGTH];

	if(quiet) return number;

	strcpy(var1, "====================================================="
		   "========");
	strcpy(var2, "-----------------------------------------------------"
		   "--------");
	if(strcmp(szMulti, "NONE") == 0) {
		strcat(var1, "\n");
		strcat(var2, "\n");
	}
	else {
		strcat(var1, "=======\n");
		strcat(var2, "-------\n");
	}

	printf("%s", var1);
	if(strcmp(rswname, "NONE")) {
		if(!strcmp(szMulti, "NONE")) {
			printf(" Ref.ID | Model                          "
				   "| RSW1 | Device Name \n");
		}
		else {
			printf(" Ref.ID | Model                          "
				   "| RSW1 | %5s | Device Name \n",
				   szMulti);
		}
		for(j = 0; j < number; j++) {
			if(boardinf[j].type == 0) {
				continue;
			}
			strcpy(DevNameBuffer, boardinf[j].bname);
			if(strlen(DevNameBuffer) > 30) {
				/* shorten the device name */
				DevNameBuffer[29] = '.';
				DevNameBuffer[30] = 0;
			}

			if(!strcmp(szMulti, "NONE")) {
				printf("%s     %2d | %-30s |   %2x | %s%d\n",
					   var2, j+1, DevNameBuffer, boardinf[j].rsw,
					   rswname, boardinf[j].dnum);
			}
			else {
				printf("%s     %2d | %-30s |   %2x | %5s | %s%d\n",
					   var2, j+1, DevNameBuffer, boardinf[j].rsw,
					   boardinf[j].multi, rswname, boardinf[j].dnum);
			}
		}
	}
	else {
		if(!strcmp(szMulti, "NONE")) {
			printf(" Ref.ID | Model                          "
				   "| RSW1 | Device No.  \n");
		}
		else {
			printf(" Ref.ID | Model                          "
				   "| RSW1 | %5s | Device No.  \n",
				   szMulti);
		}
		for(j = 0; j < number; j++){
			if(boardinf[j].type == 0) {
				continue;
			}
			strcpy(DevNameBuffer, boardinf[j].bname);
			if(strlen(DevNameBuffer) > 30) {
				/* shorten the device name */
				DevNameBuffer[29] = '.';
				DevNameBuffer[30] = 0;
			}

			if(!strcmp(szMulti, "NONE")) {
				printf("%s     %2d | %-30s |   %2x |   %3d\n",
					   var2, j+1, DevNameBuffer, boardinf[j].rsw, 
					   boardinf[j].dnum);
			}
			else {
				printf("%s     %2d | %-30s |   %2x | %5s |   %3d\n",
					   var2, j+1, DevNameBuffer, boardinf[j].rsw,
					   boardinf[j].multi, boardinf[j].dnum);
			}
		}
	}
	printf("%s\n", var1);
	j++;
	return j;
}
