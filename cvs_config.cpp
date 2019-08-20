#include "CvsLib.h"

static BOOL        _cvs_config_has_logged_in = FALSE;
static BOOL        _cvs_config_prune = TRUE;
static char        _cvs_config_module_name[MAX_PATH+1] = "\0";
static char        _cvs_config_cvs_root[MAX_PATH+1] = "\0";
static char        _cvs_config_cvsnt_path[MAX_PATH] = "\0";
static char        _cvs_config_dir_log_name[MAX_PATH] = "\0";
static char        _cvs_config_dir_log_error_name[MAX_PATH] = "\0";
static BOOL        _cvs_config_console = FALSE;
static int         _cvs_config_show_cvs = SW_HIDE; /* SW_SHOWNORMAL  SW_SHOWNOACTIVATE SW_SHOWMINNOACTIVE; */
static BOOL        _cvs_config_enable_cmd_line_limit = TRUE;
static int         _cvs_config_cmd_line_limit = 32000;
static CvsProcess* _cvs_config_active_cvs_process = NULL;
static BOOL        _cvs_config_cvs_process_running = FALSE;
static char        _cvs_config_password[MAX_PATH+1] = "\0";
static char        _cvs_config_home_env[MAX_PATH+1] = "\0";
static BOOL        _cvs_config_unixlf = FALSE;
static char        _cvs_config_str_error[CVSLIB_STR_ERROR+1] = "\0";
static int         _cvs_config_error = 0;
static BOOL        _cvs_config_checkout_readonly = TRUE;
static BOOL        _cvs_config_running_background = FALSE;
static CvsConfigUpdate _cvs_config_update;

BOOL cvs_config_prune_get()
{
  return _cvs_config_prune;
}

void cvs_config_cvsroot_set(char* cvsroot)
{
  strcpy(_cvs_config_cvs_root, cvsroot);
}

char* cvs_config_cvsroot_get()
{
  return _cvs_config_cvs_root;
}

void cvs_config_module_name_set(char* module_name)
{
  strcpy(_cvs_config_module_name, module_name);
}

BOOL cvs_config_force_root(BOOL* force_cvsroot, char* cvsroot)
{
  *force_cvsroot = TRUE;
  strcpy(cvsroot, _cvs_config_cvs_root);
  return TRUE;
}

BOOL cvs_config_console()
{
  return _cvs_config_console;
}

int cvs_config_show_cvs()
{
  return _cvs_config_show_cvs;
}

void cvs_config_password_set(char* password)
{
  strcpy(_cvs_config_password, password);
}

char* cvs_config_password_get()
{
  return _cvs_config_password;
}

BOOL cvs_config_enable_cmd_line_limit()
{
  return _cvs_config_enable_cmd_line_limit;
}

int cvs_config_cmd_line_limit()
{
  return _cvs_config_cmd_line_limit;
}

BOOL cvs_config_has_loogedin()
{
  return _cvs_config_has_logged_in;
}

void cvs_config_active_cvs_process_set(CvsProcess* cvs_process)
{
  _cvs_config_active_cvs_process = cvs_process;
}

void cvs_config_cvs_process_running_set(BOOL running)
{
  _cvs_config_cvs_process_running = running;
}

BOOL cvs_config_cvs_process_running_get()
{
  return _cvs_config_cvs_process_running;
}

CvsProcess* cvs_config_active_cvs_process_get()
{
  return _cvs_config_active_cvs_process;
}

void cvs_config_loogedin_set(BOOL state)
{
  _cvs_config_has_logged_in = state; 
}

BOOL cvs_config_running_background_get()
{
	return _cvs_config_running_background;
}

void cvs_config_running_background_set(BOOL background)
{
	_cvs_config_running_background = background;
}

BOOL cvs_config_checkout_get(CvsMultiFiles* mf, CvsConfigCheckout* config_checkout)
{
  CVSLIB_IFNULL2(mf, FALSE);
  CVSLIB_IFNULL2(config_checkout, FALSE);

  memset(config_checkout->date, 0, sizeof(config_checkout->date));
  memset(config_checkout->rev, 0, sizeof(config_checkout->rev));
  memset(config_checkout->rev1, 0, sizeof(config_checkout->rev1));
  memset(config_checkout->rev2, 0, sizeof(config_checkout->rev2));
  config_checkout->branch_point_merge = FALSE;
	config_checkout->three_way_conflicts = FALSE;
  memset(config_checkout->modname, 0, sizeof(config_checkout->modname));
	config_checkout->no_recursive = FALSE;
	config_checkout->to_stdout = FALSE;
	config_checkout->use_most_recent = FALSE;
	config_checkout->doexport = FALSE;
  config_checkout->override_checkout_dir = FALSE;
	config_checkout->case_sensitive_names = FALSE;
	config_checkout->last_checkin_time = FALSE;
  cvs_config_force_root(&config_checkout->force_cvsroot, config_checkout->cvsroot);
  memset(config_checkout->checkout_dir, 0, sizeof(config_checkout->checkout_dir));
  memset(config_checkout->keyword, 0, sizeof(config_checkout->keyword));
  config_checkout->reset_sticky = FALSE;
  config_checkout->dont_shorten_paths = FALSE;

  strcpy(config_checkout->modname, _cvs_config_module_name);

  return TRUE;
}

BOOL cvs_config_update_get(CvsMultiFiles* mf, CvsConfigUpdate* config_update)
{
  CvsMultiFilesEntry* mfi;
	CvsFileEntry* fe = NULL;

  CVSLIB_IFNULL2(mf, FALSE);
  CVSLIB_IFNULL2(config_update, FALSE);


  config_update->to_stdout = FALSE;
  config_update->no_recursive = FALSE;
  
  memset(config_update->date, 0, sizeof(config_update->date));
  memset(config_update->rev, 0, sizeof(config_update->rev));
  memset(config_update->rev1, 0, sizeof(config_update->rev1));
  memset(config_update->rev2, 0, sizeof(config_update->rev2));
  config_update->use_most_recent = FALSE;
  config_update->branch_point_merge = FALSE;
  config_update->three_way_conflicts = FALSE;
  memset(config_update->keyword, 0, sizeof(config_update->keyword));
  config_update->case_sensitive_names = FALSE;
  config_update->last_checkin_time = FALSE;

	mfi = (CvsMultiFilesEntry*)list_data(mf->dirs);
	fe = (CvsFileEntry*)list_data(mfi->files);
	if(fe != NULL && fe->curr_revision != NULL && strlen(fe->curr_revision) > 0)
	{
		cvs_functions_trim(fe->curr_revision);
		if(!iscntrl(fe->curr_revision[0]))
			strcpy(config_update->rev, fe->curr_revision);
	}

	config_update->get_clean_copy  = _cvs_config_update.get_clean_copy;
	config_update->create_miss_dir = _cvs_config_update.create_miss_dir;
	config_update->reset_sticky    = _cvs_config_update.reset_sticky;

	return TRUE;
}

int cvs_config_update_set(char* config)
{
	char* field[20];
	char* buf = NULL;
	char* p = NULL;
	char* value = NULL;
	int count = 0;
	BOOL parse_value = FALSE;

	memset(&_cvs_config_update, 0, sizeof(CvsConfigUpdate));

	CVSLIB_IFNULL2(config, cvs_config_error_msg_set("ERROR: Parametro 'config' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

  /* cria uma copia para edicao */
  buf = _strdup(config);
	cvs_functions_tolower(buf);
	cvs_functions_remove_char(buf, ' ');


	/* substitui as virgulas por \0 para trabalhar como se fosse um array */
	for(count = 0, p = (char *)buf; p != NULL && *p != '\0'; ++count, p = strchr(p, ';')) {
		if(count != 0)
			*p++ = '\0';
		field[count] = p;
	}

	for(int i=0; i<count; i++) {
		value = strchr(field[i], '=');
		if(value == NULL)
			continue;
		++value;

		parse_value = FALSE;
		if(!strncmp(value, "true", strlen("true")))
			parse_value = TRUE;
		if(!strncmp(field[i], "getcleancopy", strlen("getcleancopy"))) {
			_cvs_config_update.get_clean_copy = parse_value;
		} else if(!strncmp(field[i], "createmissingdirectories", strlen("createmissingdirectories"))) {
			_cvs_config_update.create_miss_dir = parse_value;
		} else if(!strncmp(field[i], "resetsticky", strlen("resetsticky"))) {
			_cvs_config_update.reset_sticky = parse_value;
		}
	}

  CVSLIB_FREE(buf);
	return 0;
}

BOOL cvs_config_commit_get(CvsMultiFiles* mf, CvsConfigCommit* config_commit)
{
  CVSLIB_IFNULL2(mf, FALSE);
  CVSLIB_IFNULL2(config_commit, FALSE);

	config_commit->check_valid_edits = FALSE;
	config_commit->force_commit = FALSE;
  config_commit->no_recursive = FALSE;
	config_commit->log_msg[0] = 0;
	config_commit->force_recurse = FALSE;
	config_commit->rev[0] = 0;
	config_commit->no_module_program = FALSE;
	config_commit->check_valid_edits = FALSE;	

  return TRUE;
}

void cvs_config_cvs_path_set(char* path)
{
	CVSLIB_IFNULL1(path);
	sprintf(_cvs_config_cvsnt_path, path);
}

char* cvs_config_cvs_path_get(char* path)
{
  CVSLIB_IFNULL2(path, NULL);

	if(strlen(_cvs_config_cvsnt_path) == 0)
	{
		HKEY hkey;
		struct _stat sb;
		char cvsnt_path[MAX_PATH+1];
		DWORD path_size = MAX_PATH;

		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"Software\\CVS\\PServer",
			0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
			return NULL;

		if(RegQueryValueEx(hkey, "InstallPath", NULL, NULL, (LPBYTE)cvsnt_path, &path_size) != ERROR_SUCCESS)
		{
			RegCloseKey(hkey);
			return NULL;
		}
		RegCloseKey(hkey);
		path_size = (DWORD)strlen(cvsnt_path);
		if(cvsnt_path[path_size-1] == PATH_DELIMITER)
			sprintf(_cvs_config_cvsnt_path, "%scvs.exe", cvsnt_path);
		else
			sprintf(_cvs_config_cvsnt_path, "%s%ccvs.exe", cvsnt_path, PATH_DELIMITER);

		if(_stat(_cvs_config_cvsnt_path, &sb) == -1)
			return NULL;
	}

	sprintf(path, _cvs_config_cvsnt_path);
  return path;
}

void cvs_config_error_set(int error)
{
	_cvs_config_error = error;
	if(_cvs_config_error >= 0)
		memset(_cvs_config_str_error, 0, CVSLIB_STR_ERROR);
}

int cvs_config_error_get()
{
	return _cvs_config_error;
}

int cvs_config_error_msg_set(const char *txt, int len, int error)
{
	int size, left;

	len = len > 0 ? len : (int)strlen(txt);

	size = (int)strlen(_cvs_config_str_error);
	left = CVSLIB_STR_ERROR - size;
	if(left > 0)
	{
		if(len > left)
			strncpy(_cvs_config_str_error+size, txt, left);
		else
			strcpy(_cvs_config_str_error+size, txt);
	}

	cvs_config_error_set(error);
	cvs_config_save_errstr(txt, len);

	return error;
}

char* cvs_config_error_msg_get()
{
	return _cvs_config_str_error;
}

void cvs_config_save_errstr(const char *txt, int len)
{
  FILE* file_stderr;

  CVSLIB_IFNULL1(txt);
	if(strlen(_cvs_config_dir_log_error_name) == 0)
		return;
  file_stderr = fopen(_cvs_config_dir_log_error_name, "a");
  CVSLIB_IFNULL1(file_stderr);
  if(len > 0)
    fwrite(txt, sizeof(char), len, file_stderr);
  else
    fprintf(file_stderr, "%s\n", txt);
  fclose(file_stderr);
}

void cvs_config_save_outstr(const char *txt, int len)
{
  FILE* file_stdout;

  CVSLIB_IFNULL1(txt);
	if(strlen(_cvs_config_dir_log_name) == 0)
		return;
  file_stdout = fopen(_cvs_config_dir_log_name, "a");
  CVSLIB_IFNULL1(file_stdout);
  if(len > 0)
    fwrite(txt, sizeof(char), len, file_stdout);
  else
    fprintf(file_stdout, "%s\n", txt);
  fclose(file_stdout);
}

void cvs_config_peek_pump_idle(BOOL do_idle)
{
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
    if(GetMessage(&msg, NULL, 0, 0))
    {
  		TranslateMessage(&msg);
	  	DispatchMessage(&msg);
    }
	}

	if(do_idle)
	{
		//LONG lIdle = 0;
		//while(OnIdle(lIdle++));
	}
}

char* cvs_config_env_home_get()
{
  if(strlen(_cvs_config_home_env) == 0)
    sprintf(_cvs_config_home_env, "%s%s", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
  return _cvs_config_home_env;
}

void cvs_config_unixlf_set(BOOL lf)
{
  _cvs_config_unixlf = lf;
}

BOOL cvs_config_unixlf_get()
{
  return _cvs_config_unixlf;
}

void cvs_config_checkout_readonly_set(BOOL ro)
{
	_cvs_config_checkout_readonly = ro;
}

BOOL cvs_config_checkout_readonly_get()
{
	return _cvs_config_checkout_readonly;
}


void cvs_config_log_file_set(char* dir_log_name)
{
	CVSLIB_IFNULL1(dir_log_name);
	sprintf(_cvs_config_dir_log_name, dir_log_name);
	if(strlen(_cvs_config_dir_log_name) > 0)
	{
		FILE* file;
		file = fopen(_cvs_config_dir_log_name, "w");
		if(file)
			fclose(file);
	}
}

void cvs_config_log_error_file_set(char* dir_log_error_name)
{
	CVSLIB_IFNULL1(dir_log_error_name);
	sprintf(_cvs_config_dir_log_error_name, dir_log_error_name);

	if(strlen(_cvs_config_dir_log_error_name) > 0)
	{
		FILE* file;
		file = fopen(_cvs_config_dir_log_error_name, "w");
		if(file)
			fclose(file);
	}
}
