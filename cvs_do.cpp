#include "CvsLib.h"

#define CVS_DO_ENTRIES_NAME "CVS\\Entries"
#define CVS_DO_TEMPCHAR_LEN 1024

static char  _cvs_do_temp_char[CVS_DO_TEMPCHAR_LEN+1];
static char* _cvs_do_temp_buf = NULL;

static BOOL _cvs_do_compare_str(char* str1, char* str2);

int cvs_do_login()
{
  cvs_command_login();
	return cvs_config_error_get();
}

int cvs_do_logout()
{
  cvs_command_logout();
	return cvs_config_error_get();
}

int cvs_do_checkout(char* dir, char* rev)
{
  CvsSelectionHandler selection_handler;
  CvsMultiFiles* mult_files;

	CVSLIB_IFNULL2(dir, cvs_config_error_msg_set("ERROR: Parametro 'dir' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

  cvs_selection_handler_init(&selection_handler, "Checkout", TRUE, TRUE);
  selection_handler.sel_type = SEL_DIR_CMD;

  selection_handler.auto_delete = TRUE;
  mult_files = cvs_multi_files_create();
  cvs_multi_files_new_dir(mult_files, dir);
  selection_handler.multi_files = mult_files;

  cvs_command_checkout_module(&selection_handler, rev);

	return cvs_config_error_get();
}

int cvs_do_update(char* dir, char* file, char* rev)
{
  CvsSelectionHandler selection_handler;
  CvsMultiFiles* mult_files;
	CvsMultiFilesEntry* entry;
	char* tmp = NULL;

	CVSLIB_IFNULL2(dir, cvs_config_error_msg_set("ERROR: Parametro 'dir' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

	if(file != NULL && strlen(file) > 0)
	{
		tmp = _strdup(file);
		if(tmp)
		{
			cvs_functions_trim(tmp);
			if(iscntrl(tmp[0]))
				file = NULL;
			CVSLIB_FREE(tmp);
		}
	}

  cvs_selection_handler_init(&selection_handler, "Update", TRUE, TRUE);
  selection_handler.sel_type = SEL_FILES_CMD;

  selection_handler.auto_delete = TRUE;
  mult_files = cvs_multi_files_create();
  cvs_multi_files_new_dir(mult_files, dir);
	entry = cvs_multi_files_find_dir(mult_files, dir);
	if(entry)
		cvs_multi_files_entry_add(entry, file, 0, rev);
	else
		cvs_multi_files_new_file(mult_files, file, 0, rev);
  selection_handler.multi_files = mult_files;

	cvs_command_update(&selection_handler, FALSE);

	return cvs_config_error_get();
}

int cvs_do_commit(char* dir, char* file, char* comment)
{
  CvsSelectionHandler selection_handler;
  CvsMultiFiles* mult_files;
	CvsMultiFilesEntry* entry;

	CVSLIB_IFNULL2(dir, cvs_config_error_msg_set("ERROR: Parametro 'dir' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

	//cvs_config_running_background_set(TRUE);
  cvs_selection_handler_init(&selection_handler, "Commit", TRUE, TRUE);
  selection_handler.sel_type = SEL_FILES_CMD;

  selection_handler.auto_delete = TRUE;
  mult_files = cvs_multi_files_create();
  cvs_multi_files_new_dir(mult_files, dir);
	entry = cvs_multi_files_find_dir(mult_files, dir);
	if(entry)
		cvs_multi_files_entry_add(entry, file, 0, "0");
	else
		cvs_multi_files_new_file(mult_files, file, 0, "0");
  selection_handler.multi_files = mult_files;

	cvs_command_commit(&selection_handler,comment);

	return cvs_config_error_get();
}

int cvs_do_stop_process()
{
	CvsProcess* proc = cvs_config_active_cvs_process_get();
	if(proc)
		cvs_process_stop(proc);
	return 0;
}

int cvs_do_lock(char* dir, char* file)
{
  CvsSelectionHandler selection_handler;
  CvsMultiFiles* mult_files;
	CvsMultiFilesEntry* entry;

	CVSLIB_IFNULL2(dir, cvs_config_error_msg_set("ERROR: Parametro 'dir' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

  cvs_selection_handler_init(&selection_handler, "Lock", TRUE, FALSE);
  selection_handler.sel_type = SEL_FILES_CMD;
  
  selection_handler.auto_delete = TRUE;
  mult_files = cvs_multi_files_create();
  cvs_multi_files_new_dir(mult_files, dir);
	entry = cvs_multi_files_find_dir(mult_files, dir);
	if(entry)
		cvs_multi_files_entry_add(entry, file, 0, "0");
	else
		cvs_multi_files_new_file(mult_files, file, 0, "0");
  selection_handler.multi_files = mult_files;

	cvs_command_lock(&selection_handler);

	return cvs_config_error_get();
}

int cvs_do_unlock(char* dir, char* file)
{
  CvsSelectionHandler selection_handler;
  CvsMultiFiles* mult_files;
	CvsMultiFilesEntry* entry;

	CVSLIB_IFNULL2(dir, cvs_config_error_msg_set("ERROR: Parametro 'dir' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

  cvs_selection_handler_init(&selection_handler, "Unlock", TRUE, FALSE);
  selection_handler.sel_type = SEL_FILES_CMD;

  selection_handler.auto_delete = TRUE;
  mult_files = cvs_multi_files_create();
  cvs_multi_files_new_dir(mult_files, dir);
	entry = cvs_multi_files_find_dir(mult_files, dir);
	if(entry)
		cvs_multi_files_entry_add(entry, file, 0, "0");
	else
		cvs_multi_files_new_file(mult_files, file, 0, "0");
  selection_handler.multi_files = mult_files;

  cvs_command_unlock(&selection_handler);

	return cvs_config_error_get();
}

int cvs_do_add(char* dir, char* file, char* type)
{
  CvsSelectionHandler selection_handler;
  CvsMultiFiles* mult_files;
	CvsMultiFilesEntry* entry;
	AddCmdType add_type = ADD_CMD_TYPE_NORMAL;

	CVSLIB_IFNULL2(dir, cvs_config_error_msg_set("ERROR: Parametro 'dir' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

  cvs_selection_handler_init(&selection_handler, "Add", TRUE, FALSE);
  selection_handler.sel_type = SEL_FILES_CMD;

  selection_handler.auto_delete = TRUE;
  mult_files = cvs_multi_files_create();
  cvs_multi_files_new_dir(mult_files, dir);
	entry = cvs_multi_files_find_dir(mult_files, dir);
	if(entry)
		cvs_multi_files_entry_add(entry, file, 0, "0");
	else
		cvs_multi_files_new_file(mult_files, file, 0, "0");
  selection_handler.multi_files = mult_files;

	if(type != NULL && strlen(type) > 0)
	{
		if(tolower(*type) == 'u')
			add_type  = ADD_CMD_TYPE_UNICODE;
		else if(tolower(*type) == 'b')
			add_type  = ADD_CMD_TYPE_BINARY;
	}

	cvs_command_add(&selection_handler, add_type);

	return cvs_config_error_get();
}

char* cvs_do_error_msg()
{
	if(cvs_config_error_get() == 0)
		return "";
	return cvs_config_error_msg_get();
}

char* cvs_do_revision_file_get(char* dir, char* file)
{
	FILE *stream = NULL;
	char entries_path_name[MAX_PATH];
	char buf[MAX_PATH];
	char *aux, *aux2;

	memset(_cvs_do_temp_char, 0,sizeof(_cvs_do_temp_char));

	if(!dir)
	{
		cvs_config_error_msg_set("ERROR: Parametro 'dir' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER);
		goto fim;
	}

	if(!file)
	{
		cvs_config_error_msg_set("ERROR: Parametro 'file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER);
		goto fim;
	}

	if(dir[strlen(dir)-1] != PATH_DELIMITER)
		sprintf(entries_path_name, "%s%c%s", dir, PATH_DELIMITER, CVS_DO_ENTRIES_NAME);
	else
		sprintf(entries_path_name, "%s%s", dir, CVS_DO_ENTRIES_NAME);

	stream = fopen(entries_path_name, "r");
  if(stream == NULL)
	{
		cvs_config_error_msg_set("ERROR: Impossivel verificar revisao do arquivo.", 0, CVSLIB_ERROR_IO);
    goto fim;
	}

	while(!feof(stream))
	{
		fgets(buf, sizeof(buf), stream);
		if((aux = strchr(buf, '\n')) != NULL)
		{
			*aux = '\0';
			if(*(aux-1) == '\r')
				*(aux-1) = '\0';
		}

		if(buf[0] == 'D')
			continue;
		if(buf[0] != '/' )
			continue;

		aux = buf + 1;
		if((aux2 = strchr(aux, '/')) == NULL )
			continue;
		*aux2++ = '\0';

		if(_cvs_do_compare_str(file, aux))
		{
			aux = aux2;
			if((aux2 = strchr(aux, '/')) == NULL )
			{
				cvs_config_error_msg_set("ERROR: Impossivel verificar revisao do arquivo.", 0, CVSLIB_ERROR_IO);
				goto fim;
			}
			*aux2++ = '\0';
			strcpy(_cvs_do_temp_char, aux);
			goto fim;
		}
	}

	cvs_config_error_msg_set("ERROR: Arquivo nao localizado no repositorio CVS.", 0, CVSLIB_ERROR_NOT_FOUND);

fim:

	if(stream)
		fclose(stream);

	return _cvs_do_temp_char;
}

int cvs_do_edit(char* dir, char* file, char* type)
{
	CvsSelectionHandler selection_handler;
	CvsMultiFiles* mult_files;
	CvsMultiFilesEntry* entry;
	EditCmdType edit_type = EDIT_CMD_TYPE_NORMAL;

	CVSLIB_IFNULL2(dir, cvs_config_error_msg_set("ERROR: Parametro 'dir' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
	CVSLIB_IFNULL2(file, cvs_config_error_msg_set("ERROR: Parametro 'file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

	cvs_selection_handler_init(&selection_handler, "Edit", TRUE, FALSE);
	selection_handler.sel_type = SEL_FILES_CMD;

	selection_handler.auto_delete = TRUE;
	mult_files = cvs_multi_files_create();
	cvs_multi_files_new_dir(mult_files, dir);
	entry = cvs_multi_files_find_dir(mult_files, dir);
	if(entry)
		cvs_multi_files_entry_add(entry, file, 0, "0");
	else
		cvs_multi_files_new_file(mult_files, file, 0, "0");
	selection_handler.multi_files = mult_files;

	if(type != NULL && strlen(type) > 0)
	{
		if(tolower(*type) == 'r')
			edit_type  = EDIT_CMD_TYPE_RESERVED;
		else if(tolower(*type) == 'f')
			edit_type  = EDIT_CMD_TYPE_FORCE;
	}

	cvs_command_edit(&selection_handler, edit_type);

	return cvs_config_error_get();
}

int cvs_do_unedit(char* dir, char* file)
{
	CvsSelectionHandler selection_handler;
	CvsMultiFiles* mult_files;
	CvsMultiFilesEntry* entry;
	EditCmdType edit_type = EDIT_CMD_TYPE_NORMAL;

	CVSLIB_IFNULL2(dir, cvs_config_error_msg_set("ERROR: Parametro 'dir' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
	CVSLIB_IFNULL2(file, cvs_config_error_msg_set("ERROR: Parametro 'file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

	cvs_selection_handler_init(&selection_handler, "Edit", TRUE, FALSE);
	selection_handler.sel_type = SEL_FILES_CMD;

	selection_handler.auto_delete = TRUE;
	mult_files = cvs_multi_files_create();
	cvs_multi_files_new_dir(mult_files, dir);
	entry = cvs_multi_files_find_dir(mult_files, dir);
	if(entry)
		cvs_multi_files_entry_add(entry, file, 0, "0");
	else
		cvs_multi_files_new_file(mult_files, file, 0, "0");
	selection_handler.multi_files = mult_files;

	cvs_command_unedit(&selection_handler);
	return cvs_config_error_get();
}

void cvs_do_cvsroot_set(char* cvsroot)
{
	cvs_config_cvsroot_set(cvsroot);
}

void cvs_do_module_name_set(char* module_name)
{
	cvs_config_module_name_set(module_name);
}

void cvs_do_password_set(char* password)
{
	cvs_config_password_set(password);
}


static BOOL _cvs_do_compare_str(char* str1, char* str2)
{
	size_t i;

	CVSLIB_IFNULL2(str1, FALSE);
	CVSLIB_IFNULL2(str2, FALSE);

	if(strlen(str1) != strlen(str2))
		return FALSE;

	for(i=0;i<strlen(str1);i++)
		if(tolower(str1[i]) != tolower(str2[i]))
			return FALSE;
	return TRUE;
}

int cvs_do_update_config_set(char* config)
{
	return cvs_config_update_set(config);
}

void cvs_do_cvsnt_path_set(char* dir_cvs_name)
{
	CVSLIB_IFNULL1(dir_cvs_name);
	cvs_config_cvs_path_set(dir_cvs_name);
}

void cvs_do_log_file_set(char* dir_log_name)
{
	CVSLIB_IFNULL1(dir_log_name);
	cvs_config_log_file_set(dir_log_name);
}

void cvs_do_log_error_file_set(char* dir_log_error_name)
{
	CVSLIB_IFNULL1(dir_log_error_name);
	cvs_config_log_error_file_set(dir_log_error_name);
}

int cvs_do_locked_begin(char* dir, char* file)
{
	int count;
  CvsSelectionHandler selection_handler;
  CvsMultiFiles* mult_files;
  
	CVSLIB_IFNULL2(dir, cvs_config_error_msg_set("ERROR: Parametro 'dir' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
  cvs_selection_handler_init(&selection_handler, "ListLockedFiles", TRUE, FALSE);
  selection_handler.sel_type = SEL_FILES_CMD;
  selection_handler.auto_delete = TRUE;
  mult_files = cvs_multi_files_create();
  cvs_multi_files_new_dir(mult_files, dir);
  if(strlen(file) > 0 && strcmp(file, "*") != 0 && strcmp(file, "*.*") != 0)
  {
	  CvsMultiFilesEntry* entry;
		entry = cvs_multi_files_find_dir(mult_files, dir);
		if(entry)
			cvs_multi_files_entry_add(entry, file, 0, "0");
		else
			cvs_multi_files_new_file(mult_files, file, 0, "0");
	}
  selection_handler.multi_files = mult_files;
	count = 0;
	cvs_command_list_locked_file(&selection_handler, &count);

	if(count >= 0)
		return count;
	return cvs_config_error_get();
}

char* cvs_do_locked_get(int index)
{
	CvsLockedFile* locked_file = NULL;

	locked_file = cvs_command_list_locked_files_get(index);
	if(locked_file == NULL)
		return NULL;
	memset(_cvs_do_temp_char, 0, sizeof(_cvs_do_temp_char));
	sprintf(_cvs_do_temp_char, "%s;%s;%s", locked_file->file_name, locked_file->user, locked_file->revision);
	return _cvs_do_temp_char;
}

int cvs_do_locked_end()
{
	cvs_command_list_locked_files_end();
	return 0;
}

int cvs_do_graph_begin(char* dir, char* file)
{
	int count;
  CvsSelectionHandler selection_handler;
  CvsMultiFiles* mult_files;
	CvsMultiFilesEntry* entry;
  
	CVSLIB_IFNULL2(dir, cvs_config_error_msg_set("ERROR: Parametro 'dir' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
	CVSLIB_IFNULL2(dir, cvs_config_error_msg_set("ERROR: Parametro 'file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

  cvs_selection_handler_init(&selection_handler, "ListLogFile", TRUE, FALSE);
  selection_handler.sel_type = SEL_FILES_CMD;
  selection_handler.auto_delete = TRUE;
  mult_files = cvs_multi_files_create();
  cvs_multi_files_new_dir(mult_files, dir);
	entry = cvs_multi_files_find_dir(mult_files, dir);
	if(entry)
	  cvs_multi_files_entry_add(entry, file, 0, "0");
	else
		cvs_multi_files_new_file(mult_files, file, 0, "0");
  selection_handler.multi_files = mult_files;
	count = 0;
	cvs_command_list_log_file(&selection_handler, &count);

	if(count >= 0)
		return count;
	return cvs_config_error_get();
}

char* cvs_do_graph_get(int index)
{
	char type;
	size_t size = 0;
	CvsLogFile* log_file = NULL;

	log_file = cvs_command_list_log_file_get(index);
	if(log_file == NULL)
		return NULL;
	CVSLIB_FREE(_cvs_do_temp_buf);
	size = sizeof(log_file->depth);
	size += sizeof(char);
	if(log_file->type == LOG_NODE_REVISION)
		size += (log_file->revision ? strlen(log_file->revision) : 0);
	else 
		size += (log_file->description ? strlen(log_file->description) : 0);
	size += (log_file->revision ? strlen(log_file->revision) : 0);
	size += (log_file->date ? strlen(log_file->date) : 0);
	size += (log_file->user ? strlen(log_file->user) : 0);
	if(log_file->type == LOG_NODE_REVISION)
		size += (log_file->type == LOG_NODE_REVISION ? strlen(log_file->description) : 0);
	size +=  7 + 1; /* ponto-virgulas e fim de string (\0) */
	_cvs_do_temp_buf = (char*)malloc(size);
	if(_cvs_do_temp_buf == NULL)
	{
		cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
		return NULL;
	}
	if(log_file->type == LOG_NODE_REVISION)
		type = 'R';
	else if(log_file->type == LOG_NODE_BRANCH)
		type = 'B';
	else
		type = 'T';
	/* DEPTH;TIPO;LABEL;REVISION;DATE;USER;DESCRIPTION */
	if(log_file->type == LOG_NODE_REVISION)
	{
		sprintf(_cvs_do_temp_buf, "%d;%c;%s;%s;%s;%s;%s", 
			log_file->depth, 
			type,
			(log_file->revision ? log_file->revision : ""),
			(log_file->revision ? log_file->revision : ""),
			(log_file->date ? log_file->date : ""),
			(log_file->user ? log_file->user : ""),
			(log_file->description ? log_file->description : ""));
	}
	else
	{
		sprintf(_cvs_do_temp_buf, "%d;%c;%s;%s;;;", 
			log_file->depth, 
			type,
			(log_file->description ? log_file->description : ""),
			(log_file->revision ? log_file->revision : ""));
	}
	return _cvs_do_temp_buf;
}

int cvs_do_graph_end()
{
	cvs_command_list_log_file_end();
	CVSLIB_FREE(_cvs_do_temp_buf);
	return 0;
}

char* cvs_do_log_cvs_get()
{
	return cvs_command_log_cvs_get();
}