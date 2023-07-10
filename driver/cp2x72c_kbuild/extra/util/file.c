/* *************************************************************
   file.c -source code of interface device setup program
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <syslog.h>

#include "gpgconf.h"
#include "dpgutil.h"


extern char szMethod[], szMulti[];
extern char rootpath[];
extern DRV_RES_TBL devfind[];
extern GET_BOARD_INF boardinf[];
extern char boardconf[255][30];
extern char model[];
extern char *modpath;
extern int kernel26;

void Error(int);

/* =============================================== */
int GetSection(char *section, char *factor, char *data, FILE *fp)
{
	int  flag = 0;
	char szBuffer[MAX_DEVNAME_LENGTH];
	char *sToken;
	//@@@@@ 2004/01/05
	char *sRet;
	//@@@@@

	fseek(fp, 0, 0);
	while(!feof(fp)) {
		fgets(szBuffer, sizeof(szBuffer), fp);
		sToken = strtok(szBuffer, SEPARATORS);
		if(sToken) {
			if(!strcmp(sToken, section)) {
				fgets(szBuffer, sizeof(szBuffer), fp);
				while(1) {
					sToken = strtok(szBuffer, SEPARATORS);
					if(sToken == NULL && flag == 0) return -1;
					if(sToken == NULL && flag == 1) return 0;
					if(!strcmp(sToken, factor)) {
						sToken = strtok(NULL, SEPARATORS);
						if(sToken) {
							flag = 1;
							sprintf(data, "%s", sToken);
						}
					}
					//@@@@@ 2004/01/05
					//fgets(szBuffer,sizeof(szBuffer),fp);
					sRet = fgets(szBuffer, sizeof(szBuffer), fp);
					if(sRet == NULL && flag == 0) return -1;
					if(sRet == NULL && flag == 1) return 0;
					//@@@@@
				}
			}
		}
	}
	return -4;
}


FILE *ReadInfSettings(void)
{
	FILE *fp;

	char filename[128];
	int  ret;

	sprintf(filename, "%s/etc/interface/gpg%s.inf", rootpath, model);

	fp = fopen(filename, "r");
	if(fp == NULL) {
		fprintf(stderr, "%s could not be opened.\n", filename);
		return NULL;
	}

	/* === Get method === */
	ret = GetSection("Settings", "DeviceNumber", szMethod, fp);
	if(ret) {
		printf("The device number could not be read.\n");
		fclose(fp);
		return NULL;
	}

	/* === Get multifunction === */
	ret = GetSection("Settings", "MultiFunction", szMulti, fp);
	if(ret) {
		sprintf(szMulti, "NONE");
	}

	return fp;
}


/* =============================================== */
int GetSectionName(char *szName, FILE *fp)
{
	char szBuffer[MAX_DEVNAME_LENGTH];
	char *pLine, *pDelim;
	size_t NameSize;

	while(!feof(fp)) {
		fgets(szBuffer, sizeof(szBuffer), fp);
		pLine = szBuffer;
		/* strip space and tab code */
		while(*pLine == ' ' || *pLine == '\t') pLine++;
		/* skip comment line */
		if(!*pLine || *pLine == '#') continue;

		if(*pLine != '[') continue;
		pLine++;
		pDelim = strchr(pLine, ']');
		if(pDelim == NULL || pDelim == pLine) continue;
		NameSize = (size_t)(pDelim - pLine);
		pDelim--;
		while(*pDelim == ' ' || *pDelim == '\t') {
			pDelim--;
			NameSize--;
		}
		strncpy(szName, pLine, NameSize);
		szName[NameSize] = 0;
		return 0;
	}
	return -4;
}


/* =============================================== */
/* GetBoardInfo :                                  */
/*   read board information from INF file          */
/* =============================================== */
int GetBoardInfo(int *type, int *subsys, int *rsw, char *multi, FILE *fp)
{
	char szBuffer[MAX_DEVNAME_LENGTH];
	int  nWord = 0;
	char *sToken;

	*type = *subsys = *rsw = 0;
	multi[0] = '\0';

	fgets(szBuffer, sizeof(szBuffer), fp);
	sToken = strtok(szBuffer, SEPARATORS);
	if(sToken == NULL) return -1;
	if(strcmp(sToken, "DevConfig") != 0) {
		return -4;
	}

	sToken = strtok(NULL, SEPARATORS);
	while(sToken) {
		switch(nWord) {
		case 0:
			*type = atoi(sToken);
			break;
		case 1:
			sscanf(sToken, "%4x", subsys);
			break;
		case 2:
			sscanf(sToken, "%8x", rsw);
			break;
		case 3:
			sprintf(multi, "%s", sToken);
			break;
		}
		nWord++;
		sToken = strtok(NULL, SEPARATORS);
	}
	return 0;
}


/* =============================================== */
/* EnumerateDevice :                               */
/* =============================================== */
int EnumerateDevice(FILE *fp)
{
	char cmd[64];
	char SectionName[MAX_DEVNAME_LENGTH];
	char multi[MAX_MULTINAME_LENGTH];
	int  devid, subsys, rsw, dpg0101_major;
	int  fd;
	int  ret;
	BOARD_CONF_TBL DevConf;

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

	fd = open("/dev/dpg0101", O_RDWR);
	if(fd < 0) {
		return -1;
	}
	while(1) {
		ret = GetSectionName(SectionName, fp);

		if(ret != 0) break;
		if(strcmp(SectionName, "Settings") == 0) continue;

		memset(multi, '\0', sizeof(multi));
		sprintf(DevConf.cName, "%s" , SectionName);

		while(1) {
			ret = GetBoardInfo(&devid, &subsys, &rsw, multi, fp);
			if(ret != 0) {
				break;
			}
			DevConf.nType = devid;
			DevConf.nSubSystemId = subsys;
			DevConf.offset = rsw; 
			sprintf(DevConf.append, "%s", multi);

			ret = ioctl(fd, IOCTL_COMMON_FIND_DEV, 
						(unsigned long)&DevConf);
			if(ret < 0) {
				close(fd);
				remove_module("dpg0101");
				Error(ret);
				return -1;
			}
		}
	}

	ret = ioctl(fd, IOCTL_COMMON_GET_DEV, (unsigned long)devfind);
	close(fd);
	remove_module("dpg0101");
	remove(cmd);
	if(ret) {
		Error(ret);
		return -1;
	}
	return 0;
}


/* =============================================== */
/* CheckDuplication :                              */
/* =============================================== */
int CheckDuplication(FILE *fp)
{
	int i, j;
	for(i = 0; i < 254; i++) {
		if(devfind[i].type == 0) break;
		GetSection(devfind[i].name, "BoardConfig", boardconf[i], fp);
		for(j = i + 1; j < 255; j++) {
			if(devfind[j].type == 0) break;
			if(devfind[i].type != devfind[j].type) {
				continue;
			}
			if((devfind[i].rsw == devfind[j].rsw) &&
			   (devfind[i].subsys == devfind[j].subsys)) {
				if(!strcmp(szMulti, "NONE")) {
					printf("Board model %s: The RSW1 setting is conflicted with others.\n", 
						   devfind[j].name);
					return -1;
				}
				else if(!strcmp(devfind[i].append, devfind[j].append)){
					printf("Board Type %s  RSW1 overlaps.\n", 
						   devfind[j].name);
					return -1;
				}
			}
		}//for j
	}//for i
	return 0;
}

//2003/5/17 add
/* =============================================== */
/* DeleteMissingDeviceKey                          */
/* =============================================== */
void DeleteMissingDeviceKey(FILE *fp)
{
	int  ret, devid, subsys, rsw;
	int  i, j;
	int  flag, flag2;
	char *func;
	char SectionName[MAX_DEVNAME_LENGTH], multi[MAX_MULTINAME_LENGTH];
	char Categoly[64];
	char node[MAX_DEVNODENAME_LENGTH];

	sprintf(Categoly, "gpg%s", model);
	fseek(fp, 0, 0);

	while(1) {
		ret = GetSectionName(SectionName, fp);
		if(ret != 0) break;
		if(strcmp(SectionName, "Settings") == 0) {
			continue;
		}
		while(1) {
			/* device information is acquired from INF file */
			ret = GetBoardInfo(&devid, &subsys, &rsw, multi, fp);
			if(ret != 0) break;

			if(multi[0] == '\0') {
				func = NULL;
			}
			else {
				func = multi;
			}

			/* It is confirmed whether a device exists or not */
			for(i = 0, flag = 0; i < 255; i++) {
				if(devfind[i].type == 0) break;
				if((devid  == devfind[i].type) &&
				   (subsys == devfind[i].subsys) &&
				   (!strcmp(multi, devfind[i].append))) {
					flag = 1;
					break;
				}
			}
			for(j = 0; j < 255; j++) {
				flag2 = 0;
				/* A device does not exist */
				if(flag) {
					for(i = 0; i < 255; i++) {
						if(devfind[i].type == 0) break;
						if((devid  == devfind[i].type) &&
						   (subsys == devfind[i].subsys) &&
						   (!strcmp(multi, devfind[i].append)) &&
						   (j == devfind[i].rsw)) {
							flag2 = 1;
							break;
						}
					}
				}
				if(flag2) continue;
				/* An applicable key is deleted from CONF file */
				ret = GpgConfDeleteBaseKey(
					Categoly,
					devid,
					subsys,
					j,
					func);
				if(ret == 0) {
					if(multi[0] == '\0') {
						sprintf(node, "/dev/pci%d.%04x.%x", devid, subsys, j);
					}
					else {
						sprintf(node, "/dev/pci%d.%04x.%x.%s",
								devid, subsys, j, multi);
					}
					//$$$ Mode addition (2007/05/31)
//					remove(node);
					DeleteDevNode(devid,  j, subsys, -1, multi, node);
					//$$$
				}
			}
		}
	}
	return;
}


/* =============================================== */
/* ExecuteConfigurator                             */
/* =============================================== */
int ExecuteConfigurator(FILE *fp, int num)
{
	int ret;
	char filename[128];
	char cmd[128];

	ret = GetSection(boardinf[num].bname, "Configulator",
							  filename, fp);
	if(ret == 0) {
		sprintf(cmd,"/usr/bin/%s %s %d", filename, model,
				boardinf[num].dnum);
		if(system(cmd)) {
			Error(-10);
			return -1;
		}
	}
	return 0;
}


static int execute(char *msg, char *cmd)
{
	int ret, __attribute__((unused))  first = 1;
	FILE *f;
	char line[256];

	strcat(cmd, " 2>&1");
	f = popen(cmd, "r");
	while(fgets(line, 255, f)) {
		line[strlen(line)-1] = '\0';
		first = 0;
	}
	ret = pclose(f);
	if(WIFEXITED(ret)) {
		if(WEXITSTATUS(ret)) {
			syslog(LOG_ERR, "%s exited with status %d\n",
				   msg, WEXITSTATUS(ret));
		}
		return WEXITSTATUS(ret);
    }
	else {
		syslog(LOG_ERR, "%s exited on signal %d\n", msg, WTERMSIG(ret));
	}
    return -1;
}

int lookup_dev(char *name)
{
	FILE *f;
	int n;
	char s[32], t[32];

	f = fopen("/proc/devices", "r");
	if(f == NULL) {
		return -errno;
	}

    while(fgets(s, 32, f) != NULL) {
		if(sscanf(s, "%d %s", &n, t) == 2) {
			if(strcmp(name, t) == 0) {
				break;
			}
		}
    }
    fclose(f);
    if(strcmp(name, t) == 0) {
		return n;
	}
    else {
		return -ENODEV;
	}
}

int try_insmod(char *mod, char *opts)
{
	char *cmd = malloc(strlen(mod) + strlen(modpath) +
					   (opts ? strlen(opts) : 0) + 30);
	int ret;

	strcpy(cmd, "insmod ");
	if(strchr(mod, '/') != NULL) {
		if(kernel26) {
			sprintf(cmd+7, "%s/%s.ko", modpath, mod);
		}
		else {
			sprintf(cmd+7, "%s/%s.o", modpath, mod);
		}
	}
	else {
		if(kernel26) {
			sprintf(cmd+7, "%s/misc/%s.ko", modpath, mod);
		}
		else {
			sprintf(cmd+7, "%s/misc/%s.o", modpath, mod);
		}
	}
    if(access(cmd+7, R_OK) != 0) {
		syslog(LOG_ERR, "module %s not available\n", cmd+7);
		free(cmd);
		return -1;
    }
    if(opts) {
		strcat(cmd, " ");
		if(kernel26 == 0) {
			strcat(cmd, opts);
		}
    }
    ret = execute("insmod", cmd);
    free(cmd);
    return ret;
}

int try_modprobe(char *mod, char *opts)
{
	char *cmd = malloc(strlen(mod) + (opts ? strlen(opts) : 0) + 20);
	char *s = strrchr(mod, '/');
	int  ret;

	sprintf(cmd, "modprobe %s", (s) ? s+1 : mod);
	if(opts) {
		strcat(cmd, " ");
		strcat(cmd, opts);
	}
	ret = execute("modprobe", cmd);
	free(cmd);
	return ret;
}

void remove_module(char *mod)
{
    char *s, cmd[128];

	s = strrchr(mod, '/');
	s = (s) ? s+1 : mod;
	sprintf(cmd, "rmmod %s", s);
	execute("rmmod", cmd);
}

