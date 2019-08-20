#ifndef _CVSLIB_H_
#define _CVSLIB_H_

#ifndef WINVER
#define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif						

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

#define _CRT_SECURE_NO_DEPRECATE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>

#ifdef CVSLIB_EXPORTS
#define CVSAPI extern "C" __declspec(dllexport)
#else
#define CVSAPI __declspec(dllimport)
#endif

#ifdef __BORLANDC__
#define CVSLIB_WINAPI WINAPI
#else
#define CVSLIB_WINAPI
#endif

#define CVSLIB_IFNULL1(a) if(a == NULL) return
#define CVSLIB_IFNULL2(a, b) if(a == NULL) return b
#define CVSLIB_CLOSE_HANDLE(a) do{if(a){CloseHandle(a); a = 0L;}}while(0)
#define CVSLIB_FREE(a)	do{if(a){free(a); a = 0L;}}while(0)
#define CVSLIB_MATCH(a, b) (!strncmp((a), (b), strlen(b)))
#define CVSLIB_ZEROMEM(obj) memset(&obj, 0, sizeof(obj))
#define CVSLIB_MEMSET(obj, c) memset(&obj, c, sizeof(obj))
#define CVSLIB_NUM_ELEMS(a) (sizeof(a)/sizeof(a[0]))

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

#ifdef WIN32
#define PATH_DELIMITER '\\'
#else
#define PATH_DELIMITER '/'
#endif

#define CVSLIB_ERROR -1
#define CVSLIB_ERROR_INVALID_PARAMETER -2
#define CVSLIB_ERROR_MEMORY -3
#define CVSLIB_ERROR_IO -4
#define CVSLIB_ERROR_NOT_FOUND -5

typedef int FSSpec;
typedef FSSpec UFSSpec; 

typedef enum _EditCmdType EditCmdType;
typedef enum _TagCmdType TagCmdType;
typedef enum _AddCmdType AddCmdType;
typedef enum _AnnotateCmdType AnnotateCmdType;
typedef enum _SelCmdType SelCmdType;
typedef enum _FilesSelectionType FilesSelectionType;
typedef enum _HandleCmdType HandleCmdType;
typedef enum _ProcessErrorInfo ProcessErrorInfo;
typedef enum _LogNodeType LogNodeType;

typedef struct _List List;
typedef struct _CvsArgs CvsArgs;
typedef struct _CvsFileEntry CvsFileEntry;
typedef struct _CvsMultiFilesEntry CvsMultiFilesEntry;
typedef struct _CvsMultiFiles CvsMultiFiles;
typedef struct _CvsSelectionHandler CvsSelectionHandler;
typedef struct _CvsProcessCallbacks CvsProcessCallbacks;
typedef struct _CvsProcess CvsProcess;
typedef struct _CvsProcessStartupInfo CvsProcessStartupInfo;
typedef struct _WireMessage WireMessage;
typedef struct _CvsConfigCheckout CvsConfigCheckout;
typedef struct _CvsConfigUpdate CvsConfigUpdate;
typedef struct _CvsConfigCommit CvsConfigCommit;
typedef struct _CvsConsole CvsConsole;
typedef struct _CvsProcessData CvsProcessData;
typedef struct _CvsLockedFile CvsLockedFile;
typedef struct _CvsListLockedFiles CvsListLockedFiles;
typedef struct _CvsLogFile CvsLogFile;
typedef struct _CvsListSymbolicName CvsListSymbolicName;
typedef struct _CvsListLogFile CvsListLogFile;

#include <list.h>
#include <cvs_args.h>
#include <cvs_prompt_files.h>
#include <cvs_commands.h>
#include <cvs_process.h>
#include <cvs_wire.h>
#include <cvs_config.h>
#include <cvs_process_data.h>
#include <cvs_do.h>
#include <cvs_functions.h>
#include <cvs_list_locked_files.h>
#include <cvs_list_log_file.h>

#endif /*_CVSLIB_H_*/
