
#if !defined( _GPGCONF_H_ )
#define _GPGCONF_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char BYTE,*PBYTE;
typedef unsigned short WORD,*PWORD;
typedef unsigned int DWORD,*PDWORD;
typedef void* HANDLE;
typedef HANDLE HKEY,*PHKEY;
typedef char* PCSTR;
typedef void* PVOID;
typedef int BOOL,BOOLEAN;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define GPG_CONF_HKEY_CATEGORY_ROOT			(HKEY)0x00000001

#define GPG_CONF_CREATED_NEW_KEY			0x00000001
#define GPG_CONF_OPENED_EXISTING_KEY		0x00000002

#define GPG_CONF_KEY_READ						0x00000001
#define GPG_CONF_KEY_WRITE						0x00000002
#define GPG_CONF_KEY_READ_WRITE					(GPG_CONF_KEY_READ | GPG_CONF_KEY_WRITE)

#define GPG_CONF_KEY_ROOT					"Root"

#define GPG_CONF_VALUE_RSW1					"RSW1"
#define GPG_CONF_VALUE_DEV_NODE_NAME		"DevNodeName"
#define GPG_CONF_VALUE_DEV_NUM				"DevNum"
#define GPG_CONF_VALUE_MODEL_NAME			"ModelName"
#define GPG_CONF_VALUE_FUNCTION			"Function"

#define GPG_CONF_DEVICE_KEY_PREFIX			"device"


#define GPG_CONF_DEV_NODE_NAME_MAX_LENGTH	64
#define GPG_CONF_MODEL_NAME_MAX_LENGTH		128

#define GPG_CONF_ENTRY_NAME_MAX_LENGTH		256		//max length subkey name ,value name 

#define GPG_CONF_TYPE_SZ					0x00000001
#define GPG_CONF_TYPE_DWORD					0x00000002
#define GPG_CONF_TYPE_BINARY						0x00000003

//-----------------------------------------------------------------

#define	GPG_CONF_ERROR_SUCCESS					0			// Completed successfully
#define GPG_CONF_ERROR_INVALID_HANDLE			0xC0000003	// The device handle is invalid.
#define GPG_CONF_ERROR_INSUFFICIENT_BUFFER		0xC0000007	// Data area passed to the system call is too small.
#define GPG_CONF_ERROR_IO_PENDING				0xC0000008	// An asynchronous I/O operation is in progress.
#define GPG_CONF_ERROR_NOT_SUPPORTED			0xC0000009	// The feature is not supported.
#define	GPG_CONF_ERROR_MEMORY_NOTALLOCATED		0xC0001000	// Allocating work area failed.	
#define	GPG_CONF_ERROR_PARAMETER				0xC0001001	// Parameters passed to the function are invalid.
#define	GPG_CONF_ERROR_INVALID_CALL				0xC0001002	// Invalid function call was occurred.
#define	GPG_CONF_ERROR_NULL_POINTER				0xC0001003	// NULL pointer is passed between the driver and the DLL.
#define	GPG_CONF_ERROR_OPEN_FILE				0xC0001004	// Open file faild.
#define	GPG_CONF_ERROR_NO_MORE_ENTRY			0xC0001005	// No more entry.
#define	GPG_CONF_ERROR_ANOTHER					0xC0001010	// Another error occuerd.

//-----------------------------------------------------------------



int
GpgConfOpenKey(
	HKEY	hKey,				//
	PCSTR	pSubKey,			//
	DWORD	dwOptions,			//
	PHKEY	phkResult			//
	);

int
GpgConfCloseKey(
	HKEY	hKey				//
	);

int
GpgConfEnumKey (
	HKEY	hKey,				//
	DWORD	dwIndex,			//
	PCSTR	pName,				//
	PDWORD	pcName				//
	);

int
GpgConfQueryInfoKey (
	HKEY	hKey,				//
	PDWORD	pcSubKeys,			//
	PDWORD	pcMaxSubKeyLen,		//
	PDWORD	pcValues,			//
	PDWORD	pcMaxValueNameLen,	//
	PDWORD	pcMaxValueLen		//
	);

int
GpgConfCreateKey (
	HKEY	hKey,				//
	PCSTR	pSubKey,			//
	DWORD	dwOptions,			//
	PHKEY	phkResult,			//
	PDWORD	pdwDisposition		//
	);

int
GpgConfDeleteKey (
	HKEY	hKey,				//
	PCSTR	pSubKey				//
	);

int
GpgConfEnumValue (
	HKEY	hKey,				//
	DWORD	dwIndex,			//
	PCSTR	pValueName,			//
	PDWORD	pcValueName,		//
	PDWORD	pType,				//
	PBYTE	pData,				//
	PDWORD	pcbData				//
	);

int
GpgConfQueryValue (
	HKEY	hKey,				//
	PCSTR	pValueName,			//
	PDWORD	pType,				//
	PBYTE	pData,				//
	PDWORD	pcbData				//
	);


int
GpgConfSetValue (
	HKEY		hKey,			//
	PCSTR		pValueName,		//
	DWORD		dwType,			//
	const BYTE	*pData,			//
	DWORD		cbData			//
	);

int
GpgConfDeleteValue (
	HKEY	hKey,				//
	PCSTR	pValueName			//
	);

//-------------------------------------------------------------------------

int
GpgConfOpenBaseKey(
	PCSTR	szCategory,			//
	DWORD	dwDeviceID,			//
	DWORD	dwSubSystemID,		//
	DWORD	dwRSW1,				//
	PCSTR	szFunction,			//
	DWORD	dwOptions,			//
	PHKEY	phkResult			//
	);

int
GpgConfQueryBasicValue (
	HKEY	hKey,					//
	PDWORD	pdwDevNum,				//
	PCSTR	szDevNodeName,			//
	PDWORD	pcbDevNodeNameLength,	//
	PCSTR	szModelName,			//
	PDWORD	pcbModelNameLength		//
	);

int
GpgConfCreateBaseKey (
	PCSTR	szCategory,			//
	DWORD	dwDeviceID,			//
	DWORD	dwSubSystemID,		//
	DWORD	dwRSW1,				//
	PCSTR	szFunction,			//
	DWORD	dwOptions,			//
	PHKEY	phkResult,			//
	PDWORD	pdwDisposition		//
	);

int
GpgConfSetBasicValue (
	HKEY	hKey,					//
	DWORD	dwDevNum,				//
	PCSTR	szDevNodeName,			//
	DWORD	cbDevNodeNameLength,	//
	PCSTR	szModelName,			//
	DWORD	cbModelNameLength		//
	);

int
GpgConfDeleteBaseKey(
	PCSTR	szCategory,			//
	DWORD	dwDeviceID,			//
	DWORD	dwSubSystemID,		//
	DWORD	dwRSW1,				//
	PCSTR	szFunction			//
	);


#ifdef __cplusplus
}
#endif

#endif
