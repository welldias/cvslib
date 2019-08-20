#ifndef _CVSLIB_CVS_PROCESS_H_
#define _CVSLIB_CVS_PROCESS_H_

#define CP_WRITE_BUFFER_SIZE 512
#define CP_THREADS_COUNT 4	

enum _ProcessErrorInfo
{
	ERROR_INFO_SUCCESS = 0,            /* sucesso */
	ERROR_INFO_BAD_ARGUMENTS,          /* argumento invalido */
	ERROR_INFO_NO_MEMORY,              /* memoria insuficiente */
	ERROR_INFO_COMMAND_TMP_FILE_ERROR, /* problemas ao criar o arquivo de comando */
	ERROR_INFO_COMMAND_FILE_ERROR,     /* problemas ao abrir o arquivo de comando */
	ERROR_INFO_SYSTEM_ERROR            /* erro do sistema */
};

struct _CvsProcessCallbacks
{
	long (*consoleout)(const char* txt, const long len, const CvsProcess* process);
	long (*consoleerr)(const char* txt, const long len, const CvsProcess* process);
	const char* (*getenv)(const char* name, const CvsProcess* process);
	void (*exit)(const int code, const CvsProcess* process);
	void (*ondestroy)(const CvsProcess* process);
};

struct _CvsProcess
{
	unsigned int open;
	unsigned int destroy;

	HANDLE pid;
	char** args;
	int argc;

	HANDLE my_read;
	HANDLE my_write;

	HANDLE his_read;
	HANDLE his_write;

	HANDLE pstdin;
	HANDLE pstdout;
	HANDLE pstderr;

	char write_buffer[CP_WRITE_BUFFER_SIZE];
	int  write_buffer_index;

	void* app_data;

	DWORD thread_id;
	HANDLE threads[CP_THREADS_COUNT];
	DWORD  threads_id[CP_THREADS_COUNT];
	HANDLE start_threads_event;
	HANDLE stop_process_event;

	const char* command_file_name;

	CvsProcessCallbacks* callbacks;
};

struct _CvsProcessStartupInfo
{
	int has_tty;
	char current_directory[MAX_PATH];
	WORD show_window_state;
	int command_line_limit;
	ProcessErrorInfo error_info;
	DWORD last_error_value;
};

void                   cvs_processes_start();
void                   cvs_processes_stop();
CvsProcess*            cvs_process_create();
void                   cvs_process_destroy(CvsProcess* cvs_process);
void                   cvs_process_init();
BOOL                   cvs_process_terminate(CvsProcess* cvs_process);
CvsProcessStartupInfo* cvs_process_startup_info_create();
void                   cvs_process_startup_info_init(CvsProcessStartupInfo* process_startup_info);
int                    cvs_process_launch(BOOL test, const char* path, int argc, char* argv[], CvsConsole* cvs_console, const char* cvs_file);
int                    cvs_process_launch2(const char* path, int argc, char* argv[]);
int                    cvs_process_is_active(CvsProcess* cvs_process);
int                    cvs_process_give_time();
void                   cvs_process_kill(CvsProcess* cvs_process);
void                   cvs_process_stop(CvsProcess* cvs_process);

#endif /* _CVSLIB_CVS_PROCESS_H_ */
