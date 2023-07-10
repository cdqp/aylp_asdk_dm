/* *************************************************************
   conf.c -source code of interface device setup program
   -------------------------------------------------------------
   Version 1.20-06
   -------------------------------------------------------------
   Date 2004/05/14
   -------------------------------------------------------------
   Copyright 2003, 2004 Interface Corporation. All Rights Reserved.
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

extern char szMethod[], szMulti[];
extern char rootpath[];
extern DRV_RES_TBL devfind[];
extern GET_BOARD_INF boardinf[];
extern char boardconf[255][30];
extern char model[];

int ReadDeviceConf()
{
	int  i, j;
	int  ret;
	char nodename[MAX_DEVNODENAME_LENGTH];
	char Categoly[16];
	char *multi;
	DWORD dwLength1, dwLength2, dwLength3;
	DWORD dwDispOption,dwBoardConfig;
	HKEY hKey;

	i = 0;
	sprintf(Categoly, "gpg%s", model);
	for(j = 0; j < 255; j++) {
		if(devfind[j].type == 0) break;

		if(devfind[j].append[0] == '\0') {
			multi = NULL;
		}
		else {
			multi = devfind[j].append;
		}

		ret = GpgConfCreateBaseKey(
			Categoly,			    // gpgxxxx
			(DWORD)devfind[j].type,	// device ID
			(DWORD)devfind[j].subsys,   // subsys ID
			(DWORD)devfind[j].rsw,	// rsw
			multi,	                // multifunction
			GPG_CONF_KEY_READ_WRITE,    // option
			&hKey,			        // return key address
			&dwDispOption	        // dispoption
			);
		if(ret) {
			Error(-3);
			return -1;
		}
		if(!hKey) {
			continue;
		}

		dwLength1 = MAX_DEVNODENAME_LENGTH;
		dwLength2 = MAX_DEVNAME_LENGTH;

		ret = GpgConfQueryBasicValue (
			hKey,                      // key
			(PDWORD)&boardinf[i].dnum, // device number
			boardinf[i].devnode,       // device node
			(PDWORD)&dwLength1,        // length of device node
			boardinf[i].bname,         // name of board
			(PDWORD)&dwLength2         // length of board name
			);

		boardinf[i].type = devfind[j].type;
		boardinf[i].rsw = devfind[j].rsw;
		boardinf[i].subid = devfind[j].subsys;
		memcpy(boardinf[i].multi, devfind[j].append,
			   strlen(devfind[j].append));
		if(ret == GPG_CONF_ERROR_PARAMETER) {
			memcpy(boardinf[i].bname, devfind[j].name,
				   strlen(devfind[j].name));
			memcpy(boardinf[i].boardconfig, boardconf[j],
				   strlen(boardconf[j]));
			boardinf[i].dnum = -1;
		}
		else {
			struct stat file_stat;

			sprintf(nodename, "%s/%s", rootpath, boardinf[i].devnode);
			remove(nodename);

			// redhat
			ret = stat(UDEV_DIRECTORY, &file_stat);
			if(ret >= 0) {
				if(S_ISDIR(file_stat.st_mode)) {
					sprintf(nodename, UDEV_DIRECTORY "/%s", boardinf[i].devnode);
					remove(nodename);
				}
			}

			if(boardconf[j][0] != '\0') {
				dwLength3 = sizeof(DWORD);
				ret = GpgConfQueryValue(
					hKey,
					"BoardConfig",
					&dwDispOption,
					(PBYTE)&dwBoardConfig,
					&dwLength3
					);
				if(ret) {
					Error(-3);
					GpgConfCloseKey(hKey);
					return -1;
				} 
				sprintf(boardinf[i].boardconfig, "%x", dwBoardConfig);
			}
		}
		GpgConfCloseKey(hKey);
		i++;
	} /* *** for j<255 *** */
	return i;
}

/* =============================================== */
int save_data(int nNo)
{
	int j, nRet;
	char szfname[10], *multi;
	DWORD dwDispOption;
	DWORD dwLength1, dwLength2, dwLength3;
	DWORD dwBoardConfig;
	HKEY hKey;

	sprintf(szfname, "gpg%s", model);
	for(j = 0; j < nNo; j++) {
		if(boardinf[j].type != 0){

			if(boardinf[j].multi[0] == '\0') multi = NULL;
			else			         multi = boardinf[j].multi;
			nRet = GpgConfCreateBaseKey (
				szfname,                  // gpgxxxx
				(DWORD)boardinf[j].type,  // device ID
				(DWORD)boardinf[j].subid, // subsys ID
				(DWORD)boardinf[j].rsw,   // rsw
				multi,                    // multifunction
				GPG_CONF_KEY_WRITE,       // option
				&hKey,                    // return key address
				&dwDispOption             // dispoption
				);
			if(hKey == NULL) {
				Error(-3);
				return -1;
			}

			dwLength1 = strlen(boardinf[j].devnode)+1;
			dwLength2 = strlen(boardinf[j].bname)+1;
			nRet = GpgConfSetBasicValue (
				hKey,                      // return key address
				(DWORD)boardinf[j].dnum,   // device number
				boardinf[j].devnode,       // device node
				dwLength1,                 // length of device node
				boardinf[j].bname,         // name of board
				dwLength2                  // length of board name
				);
			if(nRet){
				Error(nRet);
				GpgConfCloseKey(hKey);
				return -1;
			}
			if(boardinf[j].boardconfig[0] != '\0'){
				sscanf(boardinf[j].boardconfig,"%x",&dwBoardConfig);
				dwLength3 = sizeof(DWORD);
				nRet = GpgConfSetValue(
					hKey,
					"BoardConfig",
					GPG_CONF_TYPE_DWORD,
					(PBYTE)&dwBoardConfig,
					dwLength3
					);
				if(nRet) {
					Error(nRet);
					GpgConfCloseKey(hKey);
					return -1;
				}
			}
			GpgConfCloseKey(hKey);
		}
	}
	return 0;
}

