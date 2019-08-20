#include "CvsLib.h"

static void _cvs_console_mode_set(CvsConsole* cvs_console, BOOL binary);
static BOOL _cvs_console_is_mode_set(CvsConsole* cvs_console);

CvsConsole* cvs_console_create(FILE* file_out, BOOL auto_close)
{
	CvsConsole* cvs_console;
	
	cvs_console = (CvsConsole*)malloc(sizeof(CvsConsole));
	CVSLIB_IFNULL2(cvs_console, NULL);
	memset(cvs_console, 0, sizeof(CvsConsole));
  cvs_console->auto_close = auto_close;
  cvs_console->mode_set = FALSE;
  cvs_console->file_out = file_out;
  return cvs_console;
}

void cvs_console_destroy(CvsConsole* cvs_console)
{
  CVSLIB_IFNULL1(cvs_console);

  if(cvs_console->auto_close && cvs_console->file_out)
  {
		fclose(cvs_console->file_out);
		_unlink(cvs_console->file_name);
	}

  CVSLIB_FREE(cvs_console);
}

BOOL cvs_console_open(CvsConsole* cvs_console, const char* file_name)
{
  CVSLIB_IFNULL2(cvs_console, FALSE);
  CVSLIB_IFNULL2(file_name, FALSE);
  cvs_console->file_out = fopen(file_name, "w+");
  strcpy(cvs_console->file_name, file_name);
	return cvs_console->file_out != NULL;
}

BOOL cvs_console_open(CvsConsole* cvs_console)
{
	char temp_name[_CVSLIB_FILE_NAME_SIZE];
	
  CVSLIB_IFNULL2(cvs_console, FALSE);
	if(cvs_functions_create_temp_file(temp_name) == 0)
		return cvs_console_open(cvs_console, temp_name);
	else
		return FALSE;
}

long cvs_console_out(CvsConsole* cvs_console, const char* txt, long len)
{
  CVSLIB_IFNULL2(cvs_console, -1);
  CVSLIB_IFNULL2(txt, -1);
  if(!_cvs_console_is_mode_set(cvs_console))
		_cvs_console_mode_set(cvs_console, len == 0);
  fwrite(txt, sizeof(char), len, cvs_console->file_out);

	return len;
}

long cvs_console_err(CvsConsole* cvs_console, const char* txt, long len)
{
  CVSLIB_IFNULL2(cvs_console, -1);
  CVSLIB_IFNULL2(txt, -1);
  cvs_config_error_msg_set(txt, len, CVSLIB_ERROR);
	return len;
}

CvsProcessData* cvs_process_data_create()
{
	CvsProcessData* cvs_process;
	
	cvs_process = (CvsProcessData*)malloc(sizeof(CvsProcessData));
	CVSLIB_IFNULL2(cvs_process, NULL);
	memset(cvs_process, 0, sizeof(CvsProcessData));
	return cvs_process;
}

void cvs_process_data_cvs_console_set(CvsProcessData* cvs_process, CvsConsole* cvs_console)
{
  CVSLIB_IFNULL1(cvs_process);
  cvs_process->cvs_console = cvs_console;
}

CvsConsole* cvs_process_data_cvs_console_get(CvsProcessData* cvs_process)
{
  CVSLIB_IFNULL2(cvs_process, NULL);
  return cvs_process->cvs_console;
}


static void _cvs_console_mode_set(CvsConsole* cvs_console, BOOL binary)
{
  int mode;
  CVSLIB_IFNULL1(cvs_console);

  if(binary)
  {
    mode = _O_BINARY;
  }
  else if(cvs_config_unixlf_get())
  {
    mode = _O_BINARY;
  }
  else
  {
    mode = _O_TEXT;
  }
  _setmode(_fileno(cvs_console->file_out), mode);
  cvs_console->mode_set = TRUE;
}

static BOOL _cvs_console_is_mode_set(CvsConsole* cvs_console)
{
  CVSLIB_IFNULL2(cvs_console, FALSE);
  
  return cvs_console->mode_set;
}
