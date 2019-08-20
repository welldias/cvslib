#ifndef _CVSLIB_CVS_PROCESS_DATA_H_
#define _CVSLIB_CVS_PROCESS_DATA_H_

#define _CVSLIB_FILE_NAME_SIZE 512

struct _CvsConsole
{
	BOOL  auto_close;
	BOOL  mode_set;
	char  file_name[_CVSLIB_FILE_NAME_SIZE];
	FILE* file_out;
};

struct _CvsProcessData
{
	CvsConsole* cvs_console;
};

CvsConsole*  cvs_console_create(FILE* file_out, BOOL auto_close);
void         cvs_console_destroy(CvsConsole* cvs_console);
BOOL         cvs_console_open(CvsConsole* cvs_console, const char* file_name);
BOOL         cvs_console_open(CvsConsole* cvs_console);
long         cvs_console_out(CvsConsole* cvs_console, const char* txt, long len); /* controle do stdout do sistema */
long         cvs_console_err(CvsConsole* cvs_console, const char* txt, long len); /* controle do stderr do sistema */

CvsProcessData* cvs_process_data_create();
void            cvs_process_data_cvs_console_set(CvsProcessData* cvs_process, CvsConsole* cvs_console);
CvsConsole*     cvs_process_data_cvs_console_get(CvsProcessData* cvs_process);

#endif /* _CVSLIB_CVS_PROCESS_DATA_H_ */