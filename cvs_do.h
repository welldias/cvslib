#ifndef _CVSLIB_CVS_DO_H_
#define _CVSLIB_CVS_DO_H_

extern "C"
{
	CVSAPI int   cvs_do_login();
	CVSAPI int   cvs_do_logout();
	CVSAPI int   cvs_do_checkout(char* dir, char* rev);
	CVSAPI int   cvs_do_update(char* dir, char* file, char* rev);
	CVSAPI int   cvs_do_commit(char* dir, char* file, char* comment);
	CVSAPI int   cvs_do_stop_process();
	CVSAPI int   cvs_do_lock(char* dir, char* file);
	CVSAPI int   cvs_do_unlock(char* dir, char* file);
	CVSAPI int   cvs_do_add(char* dir, char* file, char* type);
	CVSAPI int   cvs_do_edit(char* dir, char* file, char* type);
	CVSAPI int   cvs_do_unedit(char* dir, char* file);
	CVSAPI char* cvs_do_error_msg();
	CVSAPI char* cvs_do_revision_file_get(char* dir, char* file);
	CVSAPI void  cvs_do_cvsroot_set(char* cvsroot);
	CVSAPI void  cvs_do_module_name_set(char* module_name);
	CVSAPI void  cvs_do_password_set(char* password);
	CVSAPI int   cvs_do_update_config_set(char* config);
	CVSAPI void  cvs_do_cvsnt_path_set(char* dir_cvs_name);
	CVSAPI void  cvs_do_log_file_set(char* dir_log_name);
	CVSAPI void  cvs_do_log_error_file_set(char* dir_log_error_name);
	CVSAPI int   cvs_do_locked_begin(char* dir, char* file);
	CVSAPI char* cvs_do_locked_get(int index);
	CVSAPI int   cvs_do_locked_end();
	CVSAPI int   cvs_do_graph_begin(char* dir, char* file);
	CVSAPI char* cvs_do_graph_get(int index);
	CVSAPI int   cvs_do_graph_end();
	CVSAPI char* cvs_do_log_cvs_get();
}

#endif /* _CVSLIB_CVS_DO_H_ */