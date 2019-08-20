#ifndef _CVSLIB_CVS_CONFIG_H_
#define _CVSLIB_CVS_CONFIG_H_

#define CVSLIB_DATE_LEN 25
#define CVSLIB_REV_LEN 30
#define CVSLIB_MODULE_NAME 256
#define CVSLIB_STR_ERROR 512
#define CVSLIB_KEYWORD 100
#define CVSLIB_LOGMSG 512

struct _CvsConfigCheckout
{
	char modname[CVSLIB_MODULE_NAME+1];
	char path[MAX_PATH];
	BOOL no_recursive;
	BOOL to_stdout;
	char date[CVSLIB_DATE_LEN+1];
	char rev[CVSLIB_REV_LEN+1];
	char rev1[CVSLIB_REV_LEN+1];
	char rev2[CVSLIB_REV_LEN+1];
	BOOL use_most_recent; 
	BOOL branch_point_merge; 
	BOOL three_way_conflicts; 
	BOOL doexport; 
	BOOL force_cvsroot;
	char cvsroot[MAX_PATH]; 
	BOOL override_checkout_dir;
	char checkout_dir[MAX_PATH];
	char keyword[CVSLIB_KEYWORD+1];
	BOOL reset_sticky;
	BOOL dont_shorten_paths;
	BOOL case_sensitive_names;
	BOOL last_checkin_time;
};

struct _CvsConfigUpdate
{
	BOOL to_stdout;
	BOOL no_recursive;
	BOOL reset_sticky;
	char date[CVSLIB_DATE_LEN+1];
	char rev[CVSLIB_REV_LEN+1];
	char rev1[CVSLIB_REV_LEN+1];
	char rev2[CVSLIB_REV_LEN+1];
	BOOL use_most_recent;
	BOOL branch_point_merge;
	BOOL three_way_conflicts;
	BOOL create_miss_dir;
	BOOL get_clean_copy;
	char keyword[CVSLIB_KEYWORD+1];
	BOOL case_sensitive_names;
	BOOL last_checkin_time;
};

struct _CvsConfigCommit
{
	BOOL no_recursive;
	char log_msg[CVSLIB_LOGMSG+1];
	BOOL force_commit;
	BOOL force_recurse;
	char rev[CVSLIB_REV_LEN+1];
	BOOL no_module_program;
	BOOL check_valid_edits;	
};

void         cvs_config_cvsroot_set(char* cvsroot);
char*        cvs_config_cvsroot_get();
void         cvs_config_module_name_set(char* module_name);
BOOL         cvs_config_prune_get();
BOOL         cvs_config_console();
int          cvs_config_show_cvs();
char*        cvs_config_password_get();
void         cvs_config_password_set(char* password);
void         cvs_config_active_cvs_process_set(CvsProcess* cvs_process);
CvsProcess*  cvs_config_active_cvs_process_get();
void         cvs_config_cvs_process_running_set(BOOL running);
BOOL         cvs_config_cvs_process_running_get();
void         cvs_config_peek_pump_idle(BOOL idle);
BOOL         cvs_config_enable_cmd_line_limit();
int          cvs_config_cmd_line_limit();
BOOL         cvs_config_checkout_get(CvsMultiFiles* mf, CvsConfigCheckout* config_checkout);
BOOL         cvs_config_update_get(CvsMultiFiles* mf, CvsConfigUpdate* config_update);
int          cvs_config_update_set(char* config);
BOOL         cvs_config_commit_get(CvsMultiFiles* mf, CvsConfigCommit* config_commit);
BOOL         cvs_config_force_root(BOOL* force_cvsroot, char* cvsroot);
BOOL         cvs_config_has_loogedin();
void         cvs_config_loogedin_set(BOOL state);
char*        cvs_config_cvs_path_get(char* path);
void         cvs_config_cvs_path_set(char* path);
void         cvs_config_error_set(int error);
int          cvs_config_error_get();
int          cvs_config_error_msg_set(const char *txt, int len, int error);
char*        cvs_config_error_msg_get();
void         cvs_config_save_errstr(const char *txt, int len);
void         cvs_config_save_outstr(const char *txt, int len);
char*        cvs_config_env_home_get();
void         cvs_config_unixlf_set(BOOL lf);
BOOL         cvs_config_unixlf_get();
void         cvs_config_checkout_readonly_set(BOOL ro);
BOOL         cvs_config_checkout_readonly_get();
void         cvs_config_log_file_set(char* dir_log_name);
void         cvs_config_log_error_file_set(char* dir_log_error_name);
void         cvs_config_running_background_set(BOOL background);
BOOL         cvs_config_running_background_get();

#endif /* _CVSLIB_CVS_CONFIG_H_ */