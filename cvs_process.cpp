#include "CvsLib.h"
#include <winsock2.h>

#define CP_THREAD_EXIT_SUCCESS (1)
#define CP_TERMINATION_TIMEOUT 3000
#define CP_WNDCLASSNAME_SIZE 256
#define CP_READFILE_BUFFER_SIZE 1024
#ifdef CP_USE_CVSNT_20
#define CP_EXTRA_ARGC_COUNT 4
#else
#define CP_EXTRA_ARGC_COUNT 1
#endif
#define CP_SERVEPROTOCOL_SLEEP 20

typedef struct _FindConsoleWindowData FindConsoleWindowData;

enum {
	GP_QUIT,
	GP_GETENV,
	GP_CONSOLE
};

typedef struct
{
	int code;
} GPT_QUIT;

typedef struct
{
	unsigned char empty;
	char* str;
} GPT_GETENV;

typedef struct
{
	unsigned char is_std_err;
	unsigned int len;
	char* str;
} GPT_CONSOLE;

struct _FindConsoleWindowData
{
	DWORD pid;
	HWND hwnd;
	char wnd_class_name[CP_WNDCLASSNAME_SIZE];
};

static CRITICAL_SECTION _cvs_process_destroy_lock;
static CRITICAL_SECTION _cvs_process_stack_lock;
static List*            _cvs_process_cvs_stack                  = NULL;
static List*            _cvs_process_open_cvs_process           = NULL;
static CvsProcess*      _cvs_process_current_cvs_process        = NULL;
static char*            _cvs_process_current_write_buffer       = NULL;
static int              _cvs_process_current_write_buffer_index = 0;
static char             _cvs_process_write_buffer[CP_WRITE_BUFFER_SIZE];
static int              _cvs_process_last_exit_code = 0;


#define STACK_THREAD_LOCK_BEGIN EnterCriticalSection(&_cvs_process_stack_lock)
#define STACK_THREAD_LOCK_END LeaveCriticalSection(&_cvs_process_stack_lock)
#define ENTER_CRITICAL_SECTION_BEGIN EnterCriticalSection(&_cvs_process_destroy_lock)
#define ENTER_CRITICAL_SECTION_END LeaveCriticalSection(&_cvs_process_destroy_lock)

static int    _cvs_process_calc_command_len(int argc, char* const* argv);
static char*  _cvs_process_build_command(int argc, char* const* argv);
static DWORD  _cvs_process_monitor_chld(CvsProcess* process);
static DWORD  _cvs_process_serve_protocol(CvsProcess* process);
static void   _cvs_process_chld_read_process_pipe(CvsProcess* process, const int read_type);
static DWORD  _cvs_process_chld_output_get(CvsProcess* process);
static DWORD  _cvs_process_chld_error_get(CvsProcess* process);
static void   _cvs_process_gp_quit_read(HANDLE fd, WireMessage* msg);
static void   _cvs_process_gp_quit_write(HANDLE fd, WireMessage* msg);
static void   _cvs_process_gp_quit_destroy(WireMessage* msg);
static void   _cvs_process_gp_getenv_read(HANDLE fd, WireMessage* msg);
static void   _cvs_process_gp_getenv_write(HANDLE fd, WireMessage* msg);
static int    _cvs_process_gp_getenv_write_char(HANDLE fd, const char* env);
static void   _cvs_process_gp_getenv_destroy(WireMessage* msg);
static void   _cvs_process_gp_console_read(HANDLE fd, WireMessage* msg);
static void   _cvs_process_gp_console_write(HANDLE fd, WireMessage* msg);
static void   _cvs_process_gp_console_destroy(WireMessage* msg);
static void   _cvs_process_gp_init();
static void   _cvs_process_push(CvsProcess* cvs_process);
static void   _cvs_process_pop();
static void   _cvs_process_wait_for_protocol_threads(CvsProcess* process, int milliseconds);
BOOL CALLBACK _cvs_process_post_close_enum(HWND hwnd, LPARAM lParam);
BOOL CALLBACK _cvs_process_find_console_enum(HWND hwnd, LPARAM lParam);
static HWND   _cvs_process_find_console_window(DWORD pid);
static BOOL   _cvs_process_send_break_key(DWORD pid);
static int    _cvs_process_write(HANDLE fd, char* buf, unsigned long count);
static int    _cvs_process_flush(HANDLE fd);
static void   _cvs_process_close(CvsProcess* cvs_process, BOOL kill_it);
static void   _cvs_process_recv_message(CvsProcess* cvs_process);
static void   _cvs_process_handle_message(WireMessage* msg);
static void   _cvs_process_unicode2char(const WCHAR* a, char* b);
static void   _cvs_process_char2unicode(const char* a, WCHAR* b);
static int    _cvs_process_pipe_write(HANDLE fd, const void* buf, int count);

long cvs_process_glue_consoleout(const char* txt, const long len, const CvsProcess* cvs_process);
long cvs_process_glue_consoleerr(const char* txt, const long len, const CvsProcess* cvs_process);
const char* cvs_process_glue_getenv(const char* name, const CvsProcess* process);
void cvs_process_glue_exit(const int code, const CvsProcess* cvs_process);
void cvs_process_glue_ondestroy(const CvsProcess* cvs_process);

static CvsProcessCallbacks _cvs_process_call_table =
{
	cvs_process_glue_consoleout,
	cvs_process_glue_consoleerr,
	cvs_process_glue_getenv,
	cvs_process_glue_exit,
	cvs_process_glue_ondestroy
};

long cvs_process_glue_consoleout(const char* txt, const long len, const CvsProcess* cvs_process)
{
  CVSLIB_IFNULL2(cvs_process, len);
  if(cvs_process->app_data)
  {
    CvsProcessData* cvs_process_data = (CvsProcessData*)cvs_process->app_data;
    CvsConsole* cvs_console = cvs_process_data_cvs_console_get(cvs_process_data);
    if(cvs_console != NULL)
    {
      return cvs_console_out(cvs_console, txt, len);
    }
  }

  cvs_config_save_outstr(txt, len);

  return len;
}

long cvs_process_glue_consoleerr(const char* txt, const long len, const CvsProcess* cvs_process)
{
  CVSLIB_IFNULL2(cvs_process, len);
  if(cvs_process->app_data)
	{
		CvsProcessData* cvs_process_data = (CvsProcessData*)cvs_process->app_data;
    CvsConsole* cvs_console = cvs_process_data_cvs_console_get(cvs_process_data);
    if(cvs_console != NULL)
    {
      return cvs_console_err(cvs_console, txt, len);
		}
	}

  cvs_config_error_msg_set(txt, len, CVSLIB_ERROR);

	return len;
}

const char* cvs_process_glue_getenv(const char* name, const CvsProcess* cvs_process)
{
  CVSLIB_IFNULL2(cvs_process, NULL);

	cvs_config_save_outstr(name, 0);

	if(strncmp(name, "CVS_GETPASS", strlen("CVS_GETPASS")) == 0)
	{
		char *tmp;
		char str[255] = "Digite a senha:";
		if((tmp = strchr((char*)name, '='))!= 0L)
			strcpy(str, ++tmp);

		return cvs_config_password_get();
	}
	else if( strcmp(name, "CVSLIB_YESNO") == 0 )
	{
    cvs_config_save_outstr("CVS pede confirmacao, verifique a saida padrao.", 0);
		return "yes";
	}
	else if(strcmp(name, "HOME") == 0)
	{
    return cvs_config_env_home_get();
	}
	else if(strcmp(name, "CVSUSEUNIXLF") == 0)
	{
    return cvs_config_unixlf_get() ? "1" : NULL;
	}
	else if( strcmp(name, "CVSREAD") == 0 )
	{
		return cvs_config_checkout_readonly_get() ? (char*)"1" : 0L;
	}
	else 
	{
		return getenv(name);
	}
	
	return NULL;
}

void cvs_process_glue_ondestroy(const CvsProcess* cvs_process)
{
  CVSLIB_IFNULL1(cvs_process);
  if(cvs_process->app_data)
	{
		CvsProcessData* cvs_process_data = (CvsProcessData*)cvs_process->app_data;
    CVSLIB_FREE(cvs_process_data);
	}
}

void cvs_process_glue_exit(const int code, const CvsProcess* cvs_process)
{
  char end_msg[255];

  sprintf(end_msg,"\n**** O CVS terminou normalmente com o codigo de saida %d ****\n", code);
  cvs_config_save_outstr(end_msg, 0);
	_cvs_process_last_exit_code = code;
}

void cvs_processes_start()
{
	cvs_config_error_set(0);
  InitializeCriticalSection(&_cvs_process_destroy_lock);
  InitializeCriticalSection(&_cvs_process_stack_lock);
}

void cvs_processes_stop()
{
  DeleteCriticalSection(&_cvs_process_destroy_lock);
  DeleteCriticalSection(&_cvs_process_stack_lock);
}

void cvs_process_init()
{
	_cvs_process_gp_init();
	wire_set_writer(_cvs_process_write);
	wire_set_flusher(_cvs_process_flush);
}

BOOL cvs_process_terminate(CvsProcess* cvs_process)
{
  BOOL res = FALSE;

  CVSLIB_IFNULL2(cvs_process, FALSE);

  if(WaitForSingleObject(cvs_process->pid, 0) == WAIT_TIMEOUT)
  {
    _cvs_process_send_break_key(cvs_process->thread_id);
    if(WaitForSingleObject(cvs_process->pid, CP_TERMINATION_TIMEOUT) == WAIT_OBJECT_0)
    {
      res = TRUE;
    }
    else
    {
      EnumWindows((WNDENUMPROC)_cvs_process_post_close_enum, (LPARAM)cvs_process->thread_id);
      if(WaitForSingleObject(cvs_process->pid, CP_TERMINATION_TIMEOUT) == WAIT_OBJECT_0)
      {
        res = TRUE;
      }
      else
      {
        res = TerminateProcess(cvs_process->pid, 0) != 0;
      }
    }
  }

  return res;
}

CvsProcess* cvs_process_create(const char* name, int argc, char** argv)
{
	CvsProcess* cvs_process;
  int i;

	cvs_process_init();

	cvs_process = (CvsProcess*)malloc(sizeof(CvsProcess));
  CVSLIB_IFNULL2(cvs_process, NULL);

	memset(cvs_process, 0, sizeof(CvsProcess));
	cvs_process->open = FALSE;
	cvs_process->destroy = FALSE;
	cvs_process->pid = 0;
	cvs_process->callbacks = 0L;
	cvs_process->argc = argc + CP_EXTRA_ARGC_COUNT;
	cvs_process->args = (char**)malloc((cvs_process->argc + 1) * sizeof(char*));
	cvs_process->args[0] = _strdup(name);
#ifdef CP_USE_CVSNT_20
	cvs_process->args[1] = _strdup("-cvsgui");
	cvs_process->args[2] = (char*)malloc(16 * sizeof(char));
	cvs_process->args[3] = (char*)malloc(16 * sizeof(char));
#endif

	for(i = 0; i < argc; i++)
		cvs_process->args[CP_EXTRA_ARGC_COUNT + i] = _strdup(argv[i]);
	
	cvs_process->args[cvs_process->argc] = 0L;
	cvs_process->my_read = 0;
	cvs_process->my_write = 0;
	cvs_process->his_read = 0;
	cvs_process->his_write = 0;
	cvs_process->write_buffer_index = 0;
	cvs_process->pstdin = 0;
	cvs_process->pstdout = 0;
	cvs_process->pstderr = 0;
	cvs_process->app_data = NULL;
  cvs_process->thread_id = 0;
	memset(cvs_process->threads, 0, sizeof(cvs_process->threads));
	memset(cvs_process->threads_id, 0, sizeof(cvs_process->threads_id));
	cvs_process->start_threads_event = NULL;
	cvs_process->stop_process_event = NULL;
	cvs_process->command_file_name = NULL;

	return cvs_process;
}

void cvs_process_destroy(CvsProcess* cvs_process)
{
  int i;

  CVSLIB_IFNULL1(cvs_process);

  _cvs_process_close(cvs_process, TRUE);

  if(cvs_process == _cvs_process_current_cvs_process)
    _cvs_process_pop();

  if(!cvs_process->destroy)
  {
    cvs_process->destroy = TRUE;
    cvs_process->callbacks->ondestroy(cvs_process);

    if(cvs_process->args != NULL)
    {
      if(cvs_process->command_file_name)
      {
        if(_unlink(cvs_process->command_file_name) == 0)
          cvs_process->command_file_name = NULL;
      }
      for(i = 0; i < cvs_process->argc; i++)
      {
        CVSLIB_FREE(cvs_process->args[i]);
      }
      CVSLIB_FREE(cvs_process->args);
    }

    for(i = 0; i < CP_THREADS_COUNT; i++)
    {
      CVSLIB_CLOSE_HANDLE(cvs_process->threads[i]);
    }

    CVSLIB_CLOSE_HANDLE(cvs_process->start_threads_event);
    CVSLIB_CLOSE_HANDLE(cvs_process->stop_process_event);
    CVSLIB_CLOSE_HANDLE(cvs_process->pid);
    CVSLIB_CLOSE_HANDLE(cvs_process->my_read);
    CVSLIB_CLOSE_HANDLE(cvs_process->my_write);
    CVSLIB_CLOSE_HANDLE(cvs_process->his_read);
    CVSLIB_CLOSE_HANDLE(cvs_process->his_write);
    CVSLIB_CLOSE_HANDLE(cvs_process->pstdin);
    CVSLIB_CLOSE_HANDLE(cvs_process->pstdout);
    CVSLIB_CLOSE_HANDLE(cvs_process->pstderr);

    CVSLIB_FREE(cvs_process);
  }
}


static void _cvs_process_close(CvsProcess* cvs_process, BOOL kill_it)
{
  CvsProcess* process;

	ENTER_CRITICAL_SECTION_BEGIN;
	if(cvs_process && cvs_process->open)
	{
		if(kill_it && cvs_process->pid)
		{
			cvs_process_terminate(cvs_process);
		}

		_cvs_process_wait_for_protocol_threads(cvs_process, INFINITE);
		cvs_process->open = FALSE;
		wire_clear_error();
    
    STACK_THREAD_LOCK_BEGIN;
    process = (CvsProcess*)list_find(_cvs_process_open_cvs_process, cvs_process);
		if(process)
      _cvs_process_open_cvs_process = list_remove(_cvs_process_open_cvs_process, process);
		STACK_THREAD_LOCK_END;
	}

  ENTER_CRITICAL_SECTION_END;
}

CvsProcessStartupInfo* cvs_process_startup_info_create()
{
  CvsProcessStartupInfo* process_startup_info;

	process_startup_info = (CvsProcessStartupInfo*)malloc(sizeof(CvsProcessStartupInfo));
  CVSLIB_IFNULL2(process_startup_info, NULL);
  cvs_process_startup_info_init(process_startup_info);
  return process_startup_info;
}

void cvs_process_startup_info_init(CvsProcessStartupInfo* process_startup_info)
{
  CVSLIB_IFNULL1(process_startup_info);
  process_startup_info->has_tty = FALSE;
  memset(process_startup_info->current_directory, 0, sizeof(process_startup_info->current_directory));
  process_startup_info->error_info = ERROR_INFO_SUCCESS;
  process_startup_info->show_window_state = SW_HIDE /*SW_SHOWMINNOACTIVE*/;
  process_startup_info->command_line_limit = 0;
  process_startup_info->last_error_value = 0;
}

int cvs_process_is_active(CvsProcess* cvs_process)
{
  CvsProcess* process;
  int res = 0;

  STACK_THREAD_LOCK_BEGIN;
  process = (CvsProcess*)list_find(_cvs_process_open_cvs_process, cvs_process);
  res = process ? 1 : 0;
  STACK_THREAD_LOCK_END;

	return res;
}

void cvs_process_kill(CvsProcess* cvs_process)
{
	if(cvs_process_is_active(cvs_process))
	{
		cvs_process_destroy(cvs_process);
	}
}

void cvs_process_stop(CvsProcess* cvs_process)
{
	CVSLIB_IFNULL1(cvs_process);
	SetEvent(cvs_process->stop_process_event);
}

static CvsProcess* _cvs_process_run(const char* name, int argc, char** argv,
                                    CvsProcessCallbacks* callbacks, 
                                    CvsProcessStartupInfo* startup_info,
                                    void* app_data)
{
  CvsProcess* cvs_process;
	SECURITY_ATTRIBUTES lsa = { 0 };
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	HANDLE dup_in = 0L, dup_out = 0L;
	HANDLE std_child_in = 0L, std_child_out = 0L, std_child_err = NULL;
	HANDLE std_old_in = 0l, std_old_out = 0l, std_old_err = 0l;
	HANDLE std_dup_in = 0l, std_dup_out = 0l, std_dup_err = 0l;
	BOOL res_create = FALSE;
	char* command = NULL;
	int i = 0;
	int cnt = 0;
	LPTHREAD_START_ROUTINE threads_func[CP_THREADS_COUNT] = { 0 };

	if(!callbacks || !startup_info)
	{
		if(startup_info)
			startup_info->error_info = ERROR_INFO_BAD_ARGUMENTS;
		return NULL;
	}

	cvs_process = cvs_process_create(name, argc, argv);
	if(!cvs_process)
	{
		startup_info->error_info = ERROR_INFO_NO_MEMORY;
		return NULL;
	}

	cvs_process->callbacks = callbacks;
	cvs_process->app_data = app_data;

	lsa.nLength = sizeof(SECURITY_ATTRIBUTES);
	lsa.lpSecurityDescriptor = NULL;
	lsa.bInheritHandle = TRUE;
	
	if(!CreatePipe(&cvs_process->his_read, &dup_in, &lsa, 0))
		goto error;

	if(!CreatePipe(&dup_out, &cvs_process->his_write, &lsa, 0))
		goto error;
	
	if(!DuplicateHandle(GetCurrentProcess(),
    dup_in,
    GetCurrentProcess(), 
    &cvs_process->my_write,
    0, 
    FALSE, 
    DUPLICATE_SAME_ACCESS))
	{
		goto error;
	}

  CVSLIB_CLOSE_HANDLE(dup_in);
	
	if(!DuplicateHandle(GetCurrentProcess(),
    dup_out,
		GetCurrentProcess(),
    &cvs_process->my_read, 
    0, 
    FALSE, 
    DUPLICATE_SAME_ACCESS))
	{
		goto error;
	}
	
	CVSLIB_CLOSE_HANDLE(dup_out);
	
  if(!startup_info->has_tty)
	{
    if(!CreatePipe(&std_child_in, &std_dup_in, &lsa, 0))
			goto error;
		
    if(!CreatePipe(&std_dup_out, &std_child_out, &lsa, 0))
			goto error;
		
    if(!CreatePipe(&std_dup_err, &std_child_err, &lsa, 0))
			goto error;
		
		if(!DuplicateHandle(GetCurrentProcess(),
      std_dup_in,
			GetCurrentProcess(), 
      &cvs_process->pstdin,
      0, 
      FALSE, 
      DUPLICATE_SAME_ACCESS))
		{
			goto error;
		}
		
		CVSLIB_CLOSE_HANDLE(std_dup_in);
		
		if(!DuplicateHandle(GetCurrentProcess(), 
      std_dup_out,
			GetCurrentProcess(), 
      &cvs_process->pstdout, 
      0, 
      FALSE, 
      DUPLICATE_SAME_ACCESS))
		{
			goto error;
		}
		
    CVSLIB_CLOSE_HANDLE(std_dup_out);
		
		if(!DuplicateHandle(GetCurrentProcess(),
      std_dup_err,
			GetCurrentProcess(),
      &cvs_process->pstderr,
      0,
      FALSE,
      DUPLICATE_SAME_ACCESS))
		{
			goto error;
		}
	
		CVSLIB_CLOSE_HANDLE(std_dup_err);
	}

#ifdef CP_USE_CVSNT_20
	sprintf(cvs_process->args[2], "%d", cvs_process->his_read);
	sprintf(cvs_process->args[3], "%d", cvs_process->his_write);
#endif
	
  if(startup_info->command_line_limit > 0 && 
    _cvs_process_calc_command_len(cvs_process->argc, cvs_process->args) > startup_info->command_line_limit)
	{
    char** old_args;
    char command_file_name[MAX_PATH] = {0};
		int cvs_command_pos = 6;
    int command_split_pos;
    FILE* cmd_file;
    
		if(GetTempPath(MAX_PATH-1, command_file_name) && 
			GetTempFileName(command_file_name, "cpc", 0, command_file_name))
		{
			OSVERSIONINFO vi = { 0 };
			vi.dwOSVersionInfoSize = sizeof(vi);
      GetVersionEx(&vi);
			
			if(vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
			{
				MoveFileEx(command_file_name, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
			}

			cvs_process->argc += 2;
			old_args = cvs_process->args;
			cvs_process->args = (char**)malloc((cvs_process->argc + 1) * sizeof(char*));
			cvs_process->args[0] = old_args[0];
			cvs_process->args[1] = old_args[1];
			cvs_process->args[2] = old_args[2];
			cvs_process->args[3] = old_args[3];
			cvs_process->args[4] = _strdup("-F");
			cvs_process->args[5] = _strdup(command_file_name);
			cvs_process->command_file_name = cvs_process->args[5];

      for(i = 4; old_args[i]; i++)
        cvs_process->args[i+2] = old_args[i];
			
			cvs_process->args[cvs_process->argc] = 0L;
			CVSLIB_FREE(old_args);

      for(i = cvs_command_pos; i < cvs_process->argc; i++)
      {
        if(*cvs_process->args[i] != '-')
        {
          cvs_command_pos = i;
          break;
        }
      }

      command_split_pos = cvs_process->argc;
      do{
        command_split_pos = cvs_command_pos + (command_split_pos - cvs_command_pos) / 2;
        if(command_split_pos <= cvs_command_pos)
        {
          command_split_pos = cvs_command_pos;
          break;
        }
      }while(_cvs_process_calc_command_len(command_split_pos, cvs_process->args) > startup_info->command_line_limit);
			
      command = _cvs_process_build_command(cvs_process->argc - command_split_pos, cvs_process->args + command_split_pos);
			if(command == NULL)
			{
				startup_info->error_info = ERROR_INFO_NO_MEMORY;
				goto error;
			}
			
			if(cmd_file = fopen(command_file_name, "wt"))
			{
				fputs(command, cmd_file);
				fclose(cmd_file);
			}
			else
			{
        startup_info->error_info = ERROR_INFO_COMMAND_FILE_ERROR;
				goto error;
			}
			
			CVSLIB_FREE(command);
			
			command = _cvs_process_build_command(command_split_pos, cvs_process->args);
			if(command == NULL)
			{
				startup_info->error_info = ERROR_INFO_NO_MEMORY;
				goto error;
			}
		}
		else
		{
      startup_info->error_info = ERROR_INFO_COMMAND_TMP_FILE_ERROR;
			goto error;
		}
	}
	
	if(!command)
	{
		command = _cvs_process_build_command(cvs_process->argc, cvs_process->args);
		if(command == NULL)
		{
			startup_info->error_info = ERROR_INFO_NO_MEMORY;
			goto error;
		}
		cvs_config_save_outstr(command, 0);
	}

	si.cb         = sizeof(STARTUPINFO);
  si.dwFlags    = startup_info->has_tty ? 0 : STARTF_USESTDHANDLES;
  si.hStdInput  = std_child_in;
  si.hStdOutput = std_child_out;
  si.hStdError  = std_child_err;
	
	if(!startup_info->has_tty)
	{
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = startup_info->show_window_state;
	
    std_old_in  = GetStdHandle(STD_INPUT_HANDLE);
		std_old_out = GetStdHandle(STD_OUTPUT_HANDLE);
		std_old_err = GetStdHandle(STD_ERROR_HANDLE);

		SetStdHandle(STD_INPUT_HANDLE,  std_child_in);
		SetStdHandle(STD_OUTPUT_HANDLE, std_child_out);
		SetStdHandle(STD_ERROR_HANDLE,  std_child_err);
	}
	
	res_create = CreateProcess(
		NULL,
		command,
		NULL,
		NULL,
		TRUE,
		NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED,
		NULL,
		startup_info->current_directory && strlen(startup_info->current_directory) ? startup_info->current_directory : NULL,
		&si,
		&pi);
	
	if(!res_create)
	{
		startup_info->last_error_value = GetLastError();
    startup_info->error_info = ERROR_INFO_SYSTEM_ERROR;
	}

	if(!startup_info->has_tty)
	{
		SetStdHandle(STD_INPUT_HANDLE,  std_old_in);
		SetStdHandle(STD_OUTPUT_HANDLE, std_old_out);
		SetStdHandle(STD_ERROR_HANDLE,  std_old_err);
	}
	
	if(!res_create)
		goto error;
	
  cvs_process->thread_id = pi.dwProcessId;
  cvs_process->pid = pi.hProcess;

  cvs_process->stop_process_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(!cvs_process->stop_process_event)
		goto error;
	
	cvs_process->start_threads_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(!cvs_process->start_threads_event)
		goto error;
	
	cnt = 0;
	threads_func[cnt++] = (LPTHREAD_START_ROUTINE)_cvs_process_serve_protocol;
	threads_func[cnt++] = (LPTHREAD_START_ROUTINE)_cvs_process_monitor_chld;
	
	if(!startup_info->has_tty)
	{
    threads_func[cnt++] = (LPTHREAD_START_ROUTINE)_cvs_process_chld_output_get;
		threads_func[cnt++] = (LPTHREAD_START_ROUTINE)_cvs_process_chld_error_get;
	}
	
	for(i = 0; i < cnt; i++)
	{
		if((cvs_process->threads[i] = CreateThread(
			(LPSECURITY_ATTRIBUTES)NULL,
			(DWORD)0,
			(LPTHREAD_START_ROUTINE)threads_func[i],
			(LPVOID)cvs_process,
			(DWORD)0,
      (LPDWORD)&cvs_process->threads_id[i])) == NULL)
		{
			goto error;
		}
	}
	
	CVSLIB_CLOSE_HANDLE(cvs_process->his_read);
	CVSLIB_CLOSE_HANDLE(cvs_process->his_write);

	if(!startup_info->has_tty)
	{
    CVSLIB_CLOSE_HANDLE(std_child_in);
		CVSLIB_CLOSE_HANDLE(std_child_out);
		CVSLIB_CLOSE_HANDLE(std_child_err);
	}
	
	ResumeThread(pi.hThread);

	CVSLIB_CLOSE_HANDLE(pi.hThread);
	CVSLIB_FREE(command);
	
	goto goodboy;
	
error:
	if(startup_info->error_info == ERROR_INFO_SUCCESS)
	{
		startup_info->last_error_value = GetLastError();
		startup_info->error_info = ERROR_INFO_SYSTEM_ERROR;
	}

	SetEvent(cvs_process->stop_process_event);

	if(cvs_process->pid)
	{
		TerminateProcess(cvs_process->pid, 0);
	}

	CVSLIB_CLOSE_HANDLE(dup_in);
	CVSLIB_CLOSE_HANDLE(dup_out);
	CVSLIB_CLOSE_HANDLE(std_child_in);
	CVSLIB_CLOSE_HANDLE(std_child_out);
	CVSLIB_CLOSE_HANDLE(std_child_err);
	CVSLIB_CLOSE_HANDLE(std_dup_in);
	CVSLIB_CLOSE_HANDLE(std_dup_out);
	CVSLIB_CLOSE_HANDLE(std_dup_err);

	cvs_process_destroy(cvs_process);
	
	CVSLIB_FREE(command);
	
	return 0L;

goodboy:

  STACK_THREAD_LOCK_BEGIN;
  _cvs_process_open_cvs_process = list_append(_cvs_process_open_cvs_process, cvs_process);
  STACK_THREAD_LOCK_END;

	cvs_process->open = TRUE;

	SetEvent(cvs_process->start_threads_event);

	return cvs_process;
}

int cvs_process_launch(BOOL test, const char* path, int argc, char* argv[], CvsConsole* cvs_console, const char* cvs_file)
{
  char cvs_cmd[MAX_PATH];

  CvsProcessStartupInfo startup_info;
	CvsProcessData* cvs_process_data = NULL;
  CvsProcess* proc = NULL;

#ifndef CP_USE_CVSNT_20
	SetEnvironmentVariable("CVSREAD", 
		(cvs_config_checkout_readonly_get() ? "1" : "0"));
#endif

  cvs_process_startup_info_init(&startup_info);

  startup_info.has_tty = (!test ? cvs_config_console() : TRUE); 
  if(path)
	  strcpy(startup_info.current_directory, path);

  startup_info.show_window_state = cvs_config_show_cvs();

	if(cvs_config_enable_cmd_line_limit())
	{
		startup_info.command_line_limit = cvs_config_cmd_line_limit();
	}

	if(cvs_console)
	{
    cvs_process_data = cvs_process_data_create();
		if(cvs_process_data)
      cvs_process_data_cvs_console_set(cvs_process_data, cvs_console);
	}

  if(cvs_process_is_active(cvs_config_active_cvs_process_get()))
	{
    cvs_config_error_msg_set("ERRO: Outro comando CVS em execução!", 0, CVSLIB_ERROR);
		return -1;
	}
	
	if(!test)
	{
    if(cvs_config_cvs_path_get(cvs_cmd) == NULL)
    {
      cvs_config_error_msg_set("ERRO: CVS.EXE não instalado!", 0, CVSLIB_ERROR);
      return -1;
    }
    _cvs_process_last_exit_code = 0;
	}
	else
	{
		strcpy(cvs_cmd, cvs_file);
    _cvs_process_last_exit_code = -97; /* nao suportado */
	}
	
  proc = _cvs_process_run(cvs_cmd, argc - 1, (char**)argv + 1, &_cvs_process_call_table, &startup_info, (void*)cvs_process_data);
	if(proc == 0L)
	{
		char* error_details = NULL;

		if(startup_info.error_info == ERROR_INFO_BAD_ARGUMENTS)
		{
			error_details = _strdup("ERROR: Argumentos Invalidos");
		}
		else if(startup_info.error_info == ERROR_INFO_NO_MEMORY)
		{
			error_details = _strdup("ERROR: Memoria insuficiente para criar um processo para o CVS");
		}
		else if(startup_info.error_info == ERROR_INFO_COMMAND_TMP_FILE_ERROR)
		{
			error_details = _strdup("ERROR: Impossivel criar arquivo temporario");
		}
		else if(startup_info.error_info == ERROR_INFO_COMMAND_FILE_ERROR)
		{
      error_details = _strdup("ERROR: Impossivel abrir arquivo temporario");
		}
		else if(startup_info.error_info == ERROR_INFO_SYSTEM_ERROR)
		{
      char* ptr;
			LPVOID lpv_buffer = NULL;
			
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, 
        startup_info.last_error_value,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&lpv_buffer, 
				0, 
				NULL);
	
			if(lpv_buffer)
			{
        error_details = _strdup((char*)lpv_buffer);
        LocalFree(lpv_buffer);
        ptr = strchr(error_details, '\r');
        if(ptr)
          ptr = strchr(error_details, '\n');
        if(!ptr)
          *ptr = 0;
			}
		}
		else
		{
      error_details = _strdup("ERROR: Erro desconhecido");
		}

    cvs_config_error_msg_set(error_details, 0, CVSLIB_ERROR);
    //cvs_config_save_errstr(cvs_cmd, 0);
    CVSLIB_FREE(error_details);
	}
	else 
	{
    cvs_config_cvs_process_running_set(TRUE);
    cvs_config_active_cvs_process_set(proc);
    cvs_config_peek_pump_idle(TRUE);
    
    if(cvs_config_running_background_get())
    {
			_cvs_process_last_exit_code = 0;
    }
    else
		{
			time_t start = time(NULL);
		
			while(TRUE)
			{
        cvs_config_peek_pump_idle(FALSE);
		
				if(!cvs_process_is_active(proc))
					break;
				
				if (test && ((time(NULL) - start) > 10))
				{	
					cvs_config_error_msg_set("ERROR: Timeout no teste do cvs", 0, CVSLIB_ERROR);
					cvs_process_stop(proc);
				}
			}
		}
	}

	return _cvs_process_last_exit_code;
}

static int _cvs_process_calc_command_len(int argc, char* const* argv)
{
  int i;
  int len = 0;
  const char* a;

  for(i = 0; i < argc; i++)
  {
    if(argv[i])
    {
      len += 2;

      for(a = argv[i]; *a; a++)
      {
        if(0 < i)
        {
          switch(*a)
          {
          case '"':
            len++;
            break;
          case '\\':
            if((a + 1) && (*(a + 1) == '\0'))
            {
              len++;
            }
            break;
          default:
            break;
          }
        }

        len++;
      }

      len++;
    }
  }

  return len;
}

static char* _cvs_process_build_command(int argc, char* const* argv)
{
	int len = _cvs_process_calc_command_len(argc, argv);
  int i;
  const char* a;
  char* command;
	char* p;

	command = (char*)malloc(len + 10);
	if(!command)
	{
		errno = ERROR_INFO_NO_MEMORY;
		return command;
	}
	
  p = command;
	*p = '\0';
	
	for(i = 0; i < argc; i++)
	{
		if(argv[i])
		{
			*p++ = '"';

			for(a = argv[i]; *a; a++)
			{
				if(0 < i)
				{
					switch(*a)
					{
					case '"':
						*p++ = '\\';
						break;
					case '\\':
						if((a + 1) && (*(a + 1) == '\0'))
						{
							*p++ = '\\';
						}
						break;
					default:
						break;
					}
				}

				*p++ = *a;
			}
			
			*p++ = '"';
			*p++ = ' ';
		}
	}
	
	if(p > command)
		p[-1] = '\0';
	
	return command;
}

static DWORD _cvs_process_monitor_chld(CvsProcess* process)
{
  const HANDLE enter_handles[2] = { process->start_threads_event, process->stop_process_event };

  if(WaitForMultipleObjects(2, enter_handles, FALSE, INFINITE) == WAIT_OBJECT_0)
  {
    const HANDLE wait_handles[2] = {process->pid, process->stop_process_event};

    if(WaitForMultipleObjects(2, wait_handles, FALSE, INFINITE) == WAIT_OBJECT_0)
    {
      /* espera ate que as threads terminem */
      _cvs_process_wait_for_protocol_threads(process, INFINITE);
      cvs_process_destroy(process);
    }
    else
    {
      cvs_process_kill(process);
    }
  }

  return CP_THREAD_EXIT_SUCCESS;
}

static DWORD _cvs_process_serve_protocol(CvsProcess* process)
{
  const HANDLE enter_handles[2] = { process->start_threads_event, process->stop_process_event };

  if(WaitForMultipleObjects(2, enter_handles, FALSE, INFINITE) == WAIT_OBJECT_0)
  {
    while(TRUE)
    {
      DWORD total_bytes_avail = 0;
      if(!PeekNamedPipe(process->my_read, 0L, 0 , 0L, &total_bytes_avail, 0L))
      {
        break;
      }

      if(total_bytes_avail == 0)
      {
        if(WaitForSingleObject(process->pid, CP_SERVEPROTOCOL_SLEEP) == WAIT_OBJECT_0)
        {
          if(!PeekNamedPipe(process->my_read, 0L, 0 , 0L, &total_bytes_avail, 0L) || total_bytes_avail == 0)
          {
            break;
          }
        }

        continue;
      }

      _cvs_process_recv_message(process);
    }
  }

  return CP_THREAD_EXIT_SUCCESS;
}

static void _cvs_process_chld_read_process_pipe(CvsProcess* process, const int read_type)
{
	char buff[CP_READFILE_BUFFER_SIZE] = { 0 };
	const HANDLE read_pipe = read_type ? process->pstderr : process->pstdout;

	while(TRUE)
	{
		DWORD bytes_read = 0;
		DWORD total_bytes_avail = 0;

		if(!PeekNamedPipe(read_pipe, 0L, 0 , 0L, &total_bytes_avail, 0L))
		{
			break;
		}
		
		if(total_bytes_avail == 0)
		{
			if(WaitForSingleObject(process->pid, CP_SERVEPROTOCOL_SLEEP) == WAIT_OBJECT_0)
			{
				if(!PeekNamedPipe(read_pipe, 0L, 0 , 0L, &total_bytes_avail, 0L) || total_bytes_avail == 0)
				{
					break;
				}
			}
			continue;
		}

		if(ReadFile(read_pipe, buff, CP_READFILE_BUFFER_SIZE - 1, &bytes_read, NULL))
		{
			buff[bytes_read] = '\0';
			
			if(read_type)
			{
				process->callbacks->consoleerr(buff, bytes_read, process);
			}
			else
			{
				process->callbacks->consoleout(buff, bytes_read, process);
			}
		}
		else
		{
			break;
		}
	}
}

static DWORD _cvs_process_chld_output_get(CvsProcess* process)
{
	const HANDLE enter_handles[2] = { process->start_threads_event, process->stop_process_event };
	
	if(WaitForMultipleObjects(2, enter_handles, FALSE, INFINITE) == WAIT_OBJECT_0)
	{
		_cvs_process_chld_read_process_pipe(process, 0);
	}

	return CP_THREAD_EXIT_SUCCESS;
}

static DWORD _cvs_process_chld_error_get(CvsProcess* process)
{
  const HANDLE enter_handles[2] = { process->start_threads_event, process->stop_process_event };
	
	if(WaitForMultipleObjects(2, enter_handles, FALSE, INFINITE) == WAIT_OBJECT_0)
	{
		_cvs_process_chld_read_process_pipe(process, 1);
	}

	return CP_THREAD_EXIT_SUCCESS;
}

static void _cvs_process_gp_quit_read(HANDLE fd, WireMessage* msg)
{
	GPT_QUIT* t = (GPT_QUIT*)malloc(sizeof(GPT_QUIT));
	if(t == 0L)
		return;

	if(!wire_read_int32(fd, (unsigned int*)&t->code, 1))
		return;

	msg->data = t;
}

static void _cvs_process_gp_quit_write(HANDLE fd, WireMessage* msg)
{
	GPT_QUIT* t = (GPT_QUIT*)msg->data;
	if(!wire_write_int32(fd, (unsigned int*)&t->code, 1))
		return;
}

static void _cvs_process_gp_quit_destroy(WireMessage* msg)
{
	CVSLIB_FREE(msg->data);
}

static void _cvs_process_gp_getenv_read(HANDLE fd, WireMessage* msg)
{
	GPT_GETENV* t = (GPT_GETENV*)malloc(sizeof(GPT_GETENV));
	if(t == 0L)
		return;

	if(!wire_read_int8(fd, &t->empty, 1))
		return;

	if(!wire_read_string(fd, &t->str, 1))
		return;

	msg->data = t;
}

static void _cvs_process_gp_getenv_write(HANDLE fd, WireMessage* msg)
{
	GPT_GETENV* t = (GPT_GETENV*)msg->data;
	if(!wire_write_int8(fd, &t->empty, 1))
		return;

	if(!wire_write_string(fd, &t->str, 1, -1))
		return;
}

static int _cvs_process_gp_getenv_write_char(HANDLE fd, const char* env)
{
	WireMessage msg;
	GPT_GETENV* t = (GPT_GETENV*)malloc(sizeof(GPT_GETENV));

	msg.type = GP_GETENV;
	msg.data = t;

	t->empty = env == 0L;
	t->str = _strdup(env == 0L ? "" : env);

	if(!wire_write_msg(fd, &msg))
		return FALSE;

	wire_destroy(&msg);
	if(!wire_flush(fd))
		return FALSE;

	return TRUE;
}

static void _cvs_process_gp_getenv_destroy(WireMessage* msg)
{
	GPT_GETENV* t = (GPT_GETENV*)msg->data;
	CVSLIB_FREE(t->str);
	CVSLIB_FREE(t);
}

static void _cvs_process_gp_console_read(HANDLE fd, WireMessage* msg)
{
	GPT_CONSOLE* t = (GPT_CONSOLE*)malloc(sizeof(GPT_CONSOLE));
	if(t == 0L)
		return;

	if(!wire_read_int8(fd, &t->is_std_err, 1))
		return;

	if(!wire_read_int32(fd, &t->len, 1))
		return;

	if(!wire_read_string(fd, &t->str, 1))
		return;

	msg->data = t;
}

static void _cvs_process_gp_console_write(HANDLE fd, WireMessage* msg)
{
	GPT_CONSOLE* t = (GPT_CONSOLE*)msg->data;
	
	if(!wire_write_int8(fd, &t->is_std_err, 1))
		return;
	
	if(!wire_write_int32(fd, &t->len, 1))
		return;
	
	if(!wire_write_string(fd, &t->str, 1, t->len))
		return;
}

static void _cvs_process_gp_console_destroy(WireMessage* msg)
{
	GPT_CONSOLE* t = (GPT_CONSOLE*)msg->data;
	CVSLIB_FREE(t->str);
	CVSLIB_FREE(t);
}

static void _cvs_process_gp_init()
{
	wire_register(GP_QUIT,
		_cvs_process_gp_quit_read,
		_cvs_process_gp_quit_write,
		_cvs_process_gp_quit_destroy);

	wire_register(GP_GETENV,
		_cvs_process_gp_getenv_read,
		_cvs_process_gp_getenv_write,
		_cvs_process_gp_getenv_destroy);

	wire_register(GP_CONSOLE,
		_cvs_process_gp_console_read,
		_cvs_process_gp_console_write,
		_cvs_process_gp_console_destroy);
}

static void _cvs_process_push(CvsProcess* cvs_process)
{

  STACK_THREAD_LOCK_BEGIN;
  if(cvs_process)
  {
    _cvs_process_current_cvs_process = cvs_process;
    _cvs_process_current_write_buffer_index = _cvs_process_current_cvs_process->write_buffer_index;
    _cvs_process_current_write_buffer = _cvs_process_current_cvs_process->write_buffer;
    _cvs_process_cvs_stack = list_append(_cvs_process_cvs_stack, cvs_process);
  }
  else
  {
    _cvs_process_current_write_buffer_index = 0;
    _cvs_process_current_write_buffer = NULL;
  }
  STACK_THREAD_LOCK_END;

}

static void _cvs_process_pop()
{
	STACK_THREAD_LOCK_BEGIN;
  if(_cvs_process_current_cvs_process)
	{
    _cvs_process_current_cvs_process->write_buffer_index = _cvs_process_current_write_buffer_index;
    _cvs_process_cvs_stack = list_remove_list(_cvs_process_cvs_stack, list_last(_cvs_process_cvs_stack));
	}

  if(list_count(_cvs_process_cvs_stack) > 0)
	{
    List *l = list_last(_cvs_process_cvs_stack);
    _cvs_process_current_cvs_process = (CvsProcess*)l->data;
		_cvs_process_current_write_buffer_index = _cvs_process_current_cvs_process->write_buffer_index;
		_cvs_process_current_write_buffer = _cvs_process_current_cvs_process->write_buffer;
	}
	else
	{
		_cvs_process_current_cvs_process = NULL;
		_cvs_process_current_write_buffer_index = 0;
		_cvs_process_current_write_buffer = NULL;
	}
  STACK_THREAD_LOCK_END;
}

static void _cvs_process_wait_for_protocol_threads(CvsProcess* process, int milliseconds)
{
  int i;
	int cnt = 0;
	HANDLE wait_handles[CP_THREADS_COUNT] = { 0 };
	const DWORD cur_thread = GetCurrentThreadId();
	
	for(i = 0; i < CP_THREADS_COUNT; i++)
	{
    if(process->threads[i] && (cur_thread != process->threads_id[i]))
			wait_handles[cnt++] = process->threads[i];
	}
	
	WaitForMultipleObjects(cnt, wait_handles, TRUE, milliseconds);
}

BOOL CALLBACK _cvs_process_post_close_enum(HWND hwnd, LPARAM lParam)
{
	DWORD pid = 0;

	GetWindowThreadProcessId(hwnd, &pid);
	if(pid == (DWORD)lParam)
	{
		PostMessage(hwnd, WM_CLOSE, 0, 0);
	}
	
	return TRUE;
}

BOOL CALLBACK _cvs_process_find_console_enum(HWND hwnd, LPARAM lParam)
{
	DWORD pid;
	BOOL res = TRUE;

  FindConsoleWindowData* data = (FindConsoleWindowData*)lParam;
	GetWindowThreadProcessId(hwnd, &pid);
	if(pid == data->pid)
	{
		char wnd_class_name[CP_WNDCLASSNAME_SIZE] = { 0 };
		if(GetClassName(hwnd, wnd_class_name, CP_WNDCLASSNAME_SIZE-1))
		{
			if(_stricmp(wnd_class_name, data->wnd_class_name) == 0)
			{
				data->hwnd = hwnd;
				res = FALSE;
			}
		}
	}

	return res;
}

static HWND _cvs_process_find_console_window(DWORD pid)
{
	OSVERSIONINFO vi = { 0 };
	FindConsoleWindowData data = { 0 };

	vi.dwOSVersionInfoSize = sizeof(vi);
	GetVersionEx(&vi);
	
	if(vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
		strcpy(data.wnd_class_name, "ConsoleWindowClass");
	else
		strcpy(data.wnd_class_name, "tty");
	
	data.pid = pid;
	data.hwnd = 0;
  EnumWindows(_cvs_process_find_console_enum, (LPARAM)&data);
	
	return data.hwnd;
}

static BOOL _cvs_process_send_break_key(DWORD pid)
{
	BOOL res = FALSE;
	HWND hwnd;
  
  hwnd = _cvs_process_find_console_window(pid);
	if(hwnd)
	{
    HWND foreground_wnd; 
		BYTE control_scan_code;
    BYTE vk_break_code;
    BYTE break_scan_code;
    
    control_scan_code= (BYTE)MapVirtualKey(VK_CONTROL, 0);

		vk_break_code = VK_CANCEL;
		break_scan_code = (BYTE)MapVirtualKey(vk_break_code, 0);

		vk_break_code = 'C';
		break_scan_code = (BYTE)MapVirtualKey(vk_break_code, 0);
		
		foreground_wnd = GetForegroundWindow();
		if(foreground_wnd && SetForegroundWindow(hwnd))
		{
			keybd_event(VK_CONTROL, control_scan_code, 0, 0);
			keybd_event(vk_break_code, break_scan_code, 0, 0);
			keybd_event(vk_break_code, break_scan_code, KEYEVENTF_KEYUP, 0);
			keybd_event(VK_CONTROL, control_scan_code, KEYEVENTF_KEYUP, 0);
			SetForegroundWindow(foreground_wnd);
			res = TRUE;
		}
	}

	return res;
}

static int _cvs_process_write(HANDLE fd, char* buf, unsigned long count)
{
	unsigned long bytes;

	if(_cvs_process_current_write_buffer == NULL)
		_cvs_process_current_write_buffer = _cvs_process_write_buffer;

	while(count > 0)
	{
		if((_cvs_process_current_write_buffer_index + count) >= CP_WRITE_BUFFER_SIZE)
		{
			bytes = CP_WRITE_BUFFER_SIZE - _cvs_process_current_write_buffer_index;
			memcpy(&_cvs_process_current_write_buffer[_cvs_process_current_write_buffer_index], buf, bytes);
			_cvs_process_current_write_buffer_index += bytes;
			
			if(!wire_flush(fd))
				return FALSE;
		}
		else
		{
			bytes = count;
			memcpy(&_cvs_process_current_write_buffer[_cvs_process_current_write_buffer_index], buf, bytes);
			_cvs_process_current_write_buffer_index += bytes;
		}

		buf += bytes;
		count -= bytes;
	}

	return TRUE;
}

static int _cvs_process_flush(HANDLE fd)
{
  int count = 0;
  int bytes;

	if(_cvs_process_current_write_buffer_index > 0)
	{
		while(count != _cvs_process_current_write_buffer_index)
		{
			do
			{
				bytes = _cvs_process_pipe_write(fd, &_cvs_process_current_write_buffer[count], (_cvs_process_current_write_buffer_index - count));
			}while((bytes == -1) && (errno == EAGAIN));

			if(bytes == -1)
				return FALSE;

			count += bytes;
		}
		_cvs_process_current_write_buffer_index = 0;
	}

	return TRUE;
}

static void _cvs_process_recv_message(CvsProcess* cvs_process)
{
	WireMessage msg = { 0 };

	_cvs_process_push(cvs_process);

	if(!wire_read_msg(cvs_process->my_read, &msg))
	{
		cvs_process_stop(cvs_process);
	}
	else
	{
		_cvs_process_handle_message(&msg);
		wire_destroy(&msg);
	}

	if(cvs_process_is_active(_cvs_process_current_cvs_process))
	{
		_cvs_process_pop();
	}
}

static void _cvs_process_handle_message(WireMessage* msg)
{
	switch(msg->type)
	{
	case GP_QUIT:
		{
			GPT_QUIT* t = (GPT_QUIT*)msg->data;
      _cvs_process_current_cvs_process->callbacks->exit(t->code, _cvs_process_current_cvs_process);
			break;
		}
	case GP_GETENV:
		{
			GPT_GETENV* t = (GPT_GETENV*)msg->data;
			_cvs_process_push(_cvs_process_current_cvs_process);
      _cvs_process_gp_getenv_write_char(_cvs_process_current_cvs_process->my_write, _cvs_process_current_cvs_process->callbacks->getenv(t->str, _cvs_process_current_cvs_process));
			_cvs_process_pop();
			break;
		}
	case GP_CONSOLE:
		{
			GPT_CONSOLE* t = (GPT_CONSOLE*)msg->data;
			if(t->is_std_err)
        _cvs_process_current_cvs_process->callbacks->consoleerr(t->str, t->len, _cvs_process_current_cvs_process);
			else
        _cvs_process_current_cvs_process->callbacks->consoleout(t->str, t->len, _cvs_process_current_cvs_process);
			break;
		}
	}
}

static void _cvs_process_char2unicode(const char* a, WCHAR* b)
{
  int i;
  int tam = (int)strlen(a) + 1;

  for(i=0; i<tam; i++)
    b[i] = (WCHAR)a[i];
}

static void _cvs_process_unicode2char(const WCHAR* a, char* b)
{
  int i;
  int tam = (int)wcslen(a) + 1;

  for(i=0; i<tam; i++)
    b[i] = (char)a[i];
}

static int _cvs_process_pipe_write(HANDLE fd, const void* buf, int count)
{
	DWORD dwrite;
	if( !WriteFile(fd, buf, count, &dwrite, NULL) )
	{
		errno = EINVAL;
		return -1;
	}

	return dwrite;
}
