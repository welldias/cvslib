#include "CvsLib.h"

/* CvsFileEntry */

static BOOL                 _cvs_selection_handler_check(CvsSelectionHandler* handler, CvsMultiFiles** mf, BOOL force_normalize);
static char*                _cvs_commands_cleanup_log_msg(char* msg);
static BOOL                 _cvs_commands_split_path(const char* dir, char* uppath, char* folder, BOOL dont_skip_delimiter);

static CvsListLockedFiles*  _cvs_commands_list_locked_file = NULL;
static CvsListLogFile*      _cvs_commands_list_log_file = NULL;
static CvsConsole*          _cvs_commands_commit_console = NULL;
static char                 _cvs_command_log_cvs_command[CVSLIBV_LINE_LEN+1];

CvsFileEntry* cvs_file_entry_create()
{
	CvsFileEntry* cvs_file_entry;
	
	cvs_file_entry = (CvsFileEntry*)malloc(sizeof(CvsFileEntry));
	CVSLIB_IFNULL2(cvs_file_entry, NULL);
	memset(cvs_file_entry, 0, sizeof(CvsFileEntry));
	return cvs_file_entry;
}

static void _cvs_file_entry_delete_foreach(void* cvs_file_entry)
{
	CVSLIB_IFNULL1(cvs_file_entry);
	cvs_file_entry_delete((CvsFileEntry*)cvs_file_entry);
}

void cvs_file_entry_delete(CvsFileEntry* cvs_file_entry)
{
	CVSLIB_IFNULL1(cvs_file_entry);
	if(cvs_file_entry->file)
		CVSLIB_FREE(cvs_file_entry->file);
	if(cvs_file_entry->curr_revision)
		CVSLIB_FREE(cvs_file_entry->curr_revision);
	
	CVSLIB_FREE(cvs_file_entry);
}

void cvs_file_entry_file_set(CvsFileEntry* cvs_file_entry, const char* file_name)
{
	CVSLIB_IFNULL1(cvs_file_entry);
	CVSLIB_IFNULL1(file_name);

	if(cvs_file_entry->file)
		CVSLIB_FREE(cvs_file_entry->file);
	cvs_file_entry->file = _strdup(file_name);
}

void cvs_file_entry_curr_rev_set(CvsFileEntry* cvs_file_entry, const char* curr_revision)
{
	CVSLIB_IFNULL1(cvs_file_entry);
	CVSLIB_IFNULL1(curr_revision);

	if(cvs_file_entry->curr_revision)
		CVSLIB_FREE(cvs_file_entry->curr_revision);
	cvs_file_entry->curr_revision = _strdup(curr_revision);
}

void cvs_file_entry_spec_set(CvsFileEntry* cvs_file_entry, const UFSSpec spec)
{
	CVSLIB_IFNULL1(cvs_file_entry);
	
	cvs_file_entry->spec = spec;
}


/* CvsFileEntry */

CvsMultiFilesEntry* cvs_multi_files_entry_create(const char* path)
{
	CvsMultiFilesEntry* cvs_multi_files_entry;
	
	cvs_multi_files_entry = (CvsMultiFilesEntry*)malloc(sizeof(CvsMultiFilesEntry));
	CVSLIB_IFNULL2(cvs_multi_files_entry, NULL);
	memset(cvs_multi_files_entry, 0, sizeof(CvsMultiFilesEntry));
	cvs_multi_files_entry_dir_set(cvs_multi_files_entry, path);
	return cvs_multi_files_entry;
}

static void _cvs_multi_files_entry_delete_foreach(void* cvs_multi_files_entry)
{
	CVSLIB_IFNULL1(cvs_multi_files_entry);
	cvs_multi_files_entry_delete((CvsMultiFilesEntry*)cvs_multi_files_entry);
}

void cvs_multi_files_entry_delete(CvsMultiFilesEntry* cvs_multi_files_entry)
{
	CVSLIB_IFNULL1(cvs_multi_files_entry);
	cvs_multi_files_entry->files = list_free_foreach(cvs_multi_files_entry->files, _cvs_file_entry_delete_foreach);
	if(cvs_multi_files_entry->dir)
		CVSLIB_FREE(cvs_multi_files_entry->dir);
	CVSLIB_FREE(cvs_multi_files_entry);	
}

void cvs_multi_files_entry_dir_set(CvsMultiFilesEntry* cvs_multi_files_entry, const char* newdir)
{
	CVSLIB_IFNULL1(cvs_multi_files_entry);
	CVSLIB_IFNULL1(newdir);
	if(cvs_multi_files_entry->dir)
		CVSLIB_FREE(cvs_multi_files_entry->dir);
	cvs_multi_files_entry->dir = _strdup(newdir);
}

void cvs_multi_files_entry_add(CvsMultiFilesEntry* cvs_multi_files_entry, const char* file, const UFSSpec spec, const char* curr_revision)
{
	CvsFileEntry* cvs_file_entry;
	
	CVSLIB_IFNULL1(cvs_multi_files_entry);
	CVSLIB_IFNULL1(file);
	cvs_file_entry = cvs_file_entry_create();
	if(!cvs_file_entry)
		return;
	cvs_file_entry_file_set(cvs_file_entry, file);
	cvs_file_entry_spec_set(cvs_file_entry, spec);
	cvs_file_entry_curr_rev_set(cvs_file_entry, curr_revision);
	cvs_multi_files_entry->files = list_append(cvs_multi_files_entry->files, cvs_file_entry);
}

CvsFileEntry* cvs_multi_files_entry_get(CvsMultiFilesEntry* cvs_multi_files_entry, int index)
{
	List *l;
	int i = 0;

	if(index < 0)
		return NULL;
	if(cvs_multi_files_entry_files_count(cvs_multi_files_entry) > index)
	{
		for(l=cvs_multi_files_entry->files; l; l=list_next(l))
		{
			if(i == index)
				return (CvsFileEntry*)list_data(l);
		}
	}
	return NULL;
}

int cvs_multi_files_entry_files_count(CvsMultiFilesEntry* cvs_multi_files_entry)
{
	CVSLIB_IFNULL2(cvs_multi_files_entry, -1);
	return list_count(cvs_multi_files_entry->files);
}

const char* cvs_multi_files_entry_dir_get(CvsMultiFilesEntry* cvs_multi_files_entry)
{
	CVSLIB_IFNULL2(cvs_multi_files_entry, NULL);
	return cvs_multi_files_entry->dir;
}

const char* cvs_multi_files_entry_add_byargs(CvsMultiFilesEntry* cvs_multi_files_entry, CvsArgs* args)
{
  List* l;
  CvsFileEntry* fentry;
	
	if(list_count(cvs_multi_files_entry->files) > 0)
    cvs_args_start_files(args);

	for(l = cvs_multi_files_entry->files; l; l=list_next(l))
  {
    fentry = (CvsFileEntry*)list_data(l);
    cvs_args_add_file(args, fentry->file, cvs_multi_files_entry->dir);
  }

	return cvs_multi_files_entry->dir;
}

const char* cvs_multi_files_entry_folder_add(CvsMultiFilesEntry* cvs_multi_files_entry, CvsArgs* args, char* uppath, char* folder)
{
	if(!_cvs_commands_split_path(cvs_multi_files_entry->dir, uppath, folder, FALSE))
	{
    strcpy(uppath, cvs_multi_files_entry->dir);
		strcpy(folder, ".");
	}
  cvs_args_add(args, folder);

	return uppath;
}


/* CvsMultiFiles */

CvsMultiFiles* cvs_multi_files_create()
{
	CvsMultiFiles* cvs_multi_files;
	cvs_multi_files = (CvsMultiFiles*)malloc(sizeof(CvsMultiFiles));
	CVSLIB_IFNULL2(cvs_multi_files, NULL);
	memset(cvs_multi_files, 0, sizeof(CvsMultiFiles));
  return cvs_multi_files;
}

void cvs_multi_files_delete(CvsMultiFiles* cvs_multi_files)
{
	CVSLIB_IFNULL1(cvs_multi_files);
	cvs_multi_files_reset(cvs_multi_files);
	CVSLIB_FREE(cvs_multi_files);
}

void cvs_multi_files_new_dir(CvsMultiFiles* cvs_multi_files, const char* dir)
{
	CvsMultiFilesEntry* entry;
	
	CVSLIB_IFNULL1(cvs_multi_files);
	CVSLIB_IFNULL1(dir);
	entry = cvs_multi_files_entry_create(dir);
	if(!entry)
		return;
	//cvs_multi_files_entry_dir_set(entry, dir);
	cvs_multi_files->dirs = list_append(cvs_multi_files->dirs, entry);
}

void cvs_multi_files_new_file(CvsMultiFiles* cvs_multi_files, const char* file, UFSSpec spec, const char* curr_revision)
{
	CvsMultiFilesEntry* entry;
	
	CVSLIB_IFNULL1(cvs_multi_files);
	CVSLIB_IFNULL1(file);
	entry = cvs_multi_files_entry_create(NULL);
	if(!entry)
		return;
	cvs_multi_files_entry_add(entry, file, spec, curr_revision);
	cvs_multi_files->dirs = list_append(cvs_multi_files->dirs, entry);
}

BOOL cvs_multi_files_dir_get(CvsMultiFiles* cvs_multi_files, int index, char* path)
{
	List *l;
	int i = 0;

	if(index < 0)
		return FALSE;
	if(cvs_multi_files_dirs_count(cvs_multi_files) > index)
	{
		for (l=cvs_multi_files->dirs; l; l=list_next(l))
		{
			if(i == index)
			{
				strcpy(path, ((CvsMultiFilesEntry*)list_data(l))->dir);
				return TRUE;
			}
		}
	}
	return FALSE;
}

int cvs_multi_files_dirs_count(CvsMultiFiles* cvs_multi_files)
{
	CVSLIB_IFNULL2(cvs_multi_files, -1);
	return list_count(cvs_multi_files->dirs);
}

int cvs_multi_files_num_files_count(CvsMultiFiles* cvs_multi_files)
{
	int total = 0;
	List *l;

	CVSLIB_IFNULL2(cvs_multi_files, -1);
	for (l=cvs_multi_files->dirs; l; l=list_next(l)) {
			total +=  cvs_multi_files_entry_files_count((CvsMultiFilesEntry*)list_data(l));
	}
	
	return total;
}

void cvs_multi_files_reset(CvsMultiFiles* cvs_multi_files)
{
	CVSLIB_IFNULL1(cvs_multi_files);
	cvs_multi_files->dirs = list_free_foreach(cvs_multi_files->dirs, _cvs_multi_files_entry_delete_foreach);
	cvs_multi_files->dirs = NULL;
}

BOOL cvs_multi_files_normalize(CvsMultiFiles* cvs_multi_files)
{
	List *l;
	List *list;
	CvsMultiFilesEntry* i;
	char dir[MAX_PATH];
	char name[MAX_PATH];
	char full_dir_path[MAX_PATH];

	CVSLIB_IFNULL2(cvs_multi_files, FALSE);
	list = cvs_multi_files->dirs;
	cvs_multi_files->dirs = NULL;
	for (l=list; l; l=list_next(l)) {
		i = (CvsMultiFilesEntry*)list_data(l);
		if (cvs_multi_files_entry_files_count(i) == 0)
		{
			if (cvs_multi_files_find_dir(cvs_multi_files, i->dir) == NULL)
			{
        cvs_multi_files_new_dir(cvs_multi_files, i->dir);
			}
		}
		else 
		{
			List *lf;
			CvsFileEntry* fe;
      CvsMultiFilesEntry *fe2;

			for (lf=i->files; lf; lf=list_next(lf))
			{
				fe = (CvsFileEntry*)list_data(lf);
				if (cvs_multi_files_split(fe->file, dir, name))
				{
					if(full_dir_path[strlen(i->dir)-1] == PATH_DELIMITER)
						sprintf(full_dir_path, "%s%s", i->dir, dir);
					else
						sprintf(full_dir_path, "%s%c%s", i->dir, PATH_DELIMITER, dir);
				}
				else
				{
					strcpy(full_dir_path, i->dir);
				}

				fe2 = cvs_multi_files_find_dir(cvs_multi_files, full_dir_path);
				if (fe2)
				{
					cvs_multi_files_entry_add(fe2, name, fe->spec, fe->curr_revision);
				}
				else
				{
					cvs_multi_files_new_dir(cvs_multi_files, full_dir_path);
					cvs_multi_files_new_file(cvs_multi_files, name, fe->spec, fe->curr_revision);
				}
			}
		}
	}

	list = list_free_foreach(list, _cvs_multi_files_entry_delete_foreach);
	
	return TRUE;
}

CvsMultiFilesEntry* cvs_multi_files_find_dir(CvsMultiFiles* cvs_multi_files, const char* dir)
{
	int total = 0;
	CvsMultiFilesEntry* entry;
	List *l;

	CVSLIB_IFNULL2(cvs_multi_files, NULL);
	CVSLIB_IFNULL2(dir, NULL);
	for (l=cvs_multi_files->dirs; l; l=list_next(l))
	{
		entry = (CvsMultiFilesEntry*)list_data(l);
		if(strcmp(entry->dir, dir) == 0)
		{
			return entry;
		}
	}
	return NULL;
}

BOOL cvs_multi_files_split(char* in_partial_path, char* out_path, char* out_name)
{
	char* ptr;
	int pos = -1;
	
	for(ptr=in_partial_path;ptr;ptr++)
	{
		if(*ptr == PATH_DELIMITER)
			pos = (int)(in_partial_path - ptr);
	}
	
	if(pos >= 0)
	{
		sprintf(out_name, "%s", in_partial_path+pos);
		sprintf(out_path, "%.*s", pos, in_partial_path);
		return TRUE;
	}
	else
	{
		strcpy(out_name, in_partial_path);
		out_path = NULL;
		return FALSE;
	}
}


/* CvsSelectionHandler */
CvsSelectionHandler* cvs_selection_handler_create(const char* str_command, BOOL need_selection, BOOL need_reset_view)
{
	CvsSelectionHandler* cvs_selection_handler;
	 
	cvs_selection_handler = (CvsSelectionHandler*)malloc(sizeof(CvsSelectionHandler));
	CVSLIB_IFNULL2(cvs_selection_handler, NULL);
	cvs_selection_handler_init(cvs_selection_handler, str_command, need_selection, need_reset_view);
	
	return cvs_selection_handler;
}

void cvs_selection_handler_init(CvsSelectionHandler* cvs_selection_handler, const char* str_command, BOOL need_selection, BOOL need_reset_view)
{
  CVSLIB_IFNULL1(cvs_selection_handler);
	memset(cvs_selection_handler, 0, sizeof(CvsSelectionHandler)); 
	cvs_selection_handler->str_command = str_command ? _strdup(str_command) : NULL;
	cvs_selection_handler->multi_files = NULL;
	cvs_selection_handler->sel_type = SEL_NONE_CMD;
	cvs_selection_handler->auto_delete = FALSE;
	cvs_selection_handler->need_selection = need_selection;
	cvs_selection_handler->need_view_reset = need_reset_view;
}

void cvs_selection_handler_delete (CvsSelectionHandler* cvs_selection_handler)
{
	CVSLIB_IFNULL1(cvs_selection_handler);
	
	if(cvs_selection_handler->str_command)
		CVSLIB_FREE(cvs_selection_handler->str_command);
	CVSLIB_FREE(cvs_selection_handler);
}


SelCmdType cvs_selection_handler_seltype_get(CvsSelectionHandler* cvs_selection_handler)
{
	return cvs_selection_handler->sel_type;
}

BOOL cvs_selection_handler_need_selection_get(CvsSelectionHandler* cvs_selection_handler)
{
	return cvs_selection_handler->need_selection;
}

BOOL cvs_selection_handler_need_view_reset_get(CvsSelectionHandler* cvs_selection_handler)
{
	return cvs_selection_handler->need_view_reset;
}

CvsMultiFiles* cvs_selection_handler_selection_get(CvsSelectionHandler* cvs_selection_handler)
{
	char* str_prompt = NULL;

	CVSLIB_IFNULL2(cvs_selection_handler, NULL);
		
	if(!cvs_selection_handler->multi_files && cvs_selection_handler->need_selection )
	{
		switch(cvs_selection_handler->sel_type)
		{
		case SEL_FILES_CMD:
			cvs_selection_handler->multi_files = cvs_multi_files_create();
			if(cvs_selection_handler->multi_files)
			{
				str_prompt = (char*)malloc(strlen(cvs_selection_handler->str_command)+40);
				if(!str_prompt)
				{
					CVSLIB_FREE(cvs_selection_handler->multi_files);
					cvs_selection_handler->multi_files = NULL;
					cvs_selection_handler->auto_delete = FALSE;
					break;
				}
				cvs_selection_handler->auto_delete = TRUE;
				sprintf(str_prompt, "Selecione os arquivos para %s", cvs_selection_handler->str_command);
				if(!cvs_prompt_multi_files_get(str_prompt, cvs_selection_handler->multi_files, FALSE, SELECT_ANY))
				{
					CVSLIB_FREE(cvs_selection_handler->multi_files);
					cvs_selection_handler->multi_files = NULL;
					cvs_selection_handler->auto_delete = FALSE;
				}
			}
			break;
		case SEL_DIR_CMD:
		{
			char sel_dir[MAX_PATH] = {0};

			cvs_selection_handler->multi_files = cvs_multi_files_create();
			if(cvs_selection_handler->multi_files)
			{
				str_prompt = (char*)malloc(strlen(cvs_selection_handler->str_command)+40);
				if(!str_prompt)
				{
					cvs_multi_files_delete(cvs_selection_handler->multi_files);
					cvs_selection_handler->multi_files = NULL;
					break;
				}
				
				sprintf(str_prompt, "Selecione o diretório para %s", cvs_selection_handler->str_command);
				
				if(cvs_prompt_directory_get(str_prompt, sel_dir))
				{
					cvs_multi_files_new_dir(cvs_selection_handler->multi_files, sel_dir);
				}
				else
				{
					cvs_multi_files_delete(cvs_selection_handler->multi_files);
					cvs_selection_handler->multi_files = NULL;
				}
			}
		}
			break;
		default:
			break;
		}
	}
	
	return cvs_selection_handler->multi_files;
}

BOOL cvs_selection_handler_needs_normalize(CvsSelectionHandler* cvs_selection_handler)
{
  return FALSE;
}

CvsSelectionHandler* cvs_lock_handler_create()
{
  return cvs_selection_handler_create("Lock", TRUE, FALSE);
}

CvsSelectionHandler* cvs_unlock_handler_create()
{
  return cvs_selection_handler_create("Unlock", TRUE, FALSE);
}

/* executa o comando 'cvs init' */
void cvs_command_init()
{
	BOOL force_cvs_root;
	char cvs_root[MAX_PATH];
	CvsArgs args;

  cvs_args_init(&args, NULL, 0, TRUE);

  if(!cvs_config_force_root(&force_cvs_root, cvs_root))
		return;

	if(cvs_root)
	{
    cvs_args_add(&args, "-d");
		cvs_args_add(&args, cvs_root);
	}

	cvs_args_add(&args, "init");
  cvs_args_print(&args, NULL);

  cvs_processes_start();
  cvs_process_launch(FALSE, NULL, cvs_args_argc(&args), cvs_args_argv(&args), NULL, NULL);
  cvs_processes_stop();
}

/* executa o comando 'cvs login' */
void cvs_command_login(void)
{
	BOOL force_cvs_root;
	char cvs_root[MAX_PATH];
	CvsArgs args;

  cvs_args_init(&args, NULL, 0, TRUE);

  if(!cvs_config_force_root(&force_cvs_root, cvs_root))
		return;

	cvs_config_loogedin_set(TRUE);

	if(force_cvs_root)
	{
    cvs_args_add(&args, "-d");
    cvs_args_add(&args, cvs_root);
	}

  cvs_args_add(&args, "login");

#ifndef CP_USE_CVSNT_20
	{
		char* password = cvs_config_password_get();
		if(password != NULL && strlen(password) > 0)
		{
			cvs_args_add(&args, "-p");
			cvs_args_add(&args, password);
		}
	}
#endif

  cvs_args_print(&args, NULL);

  cvs_processes_start();
  cvs_process_launch(FALSE, NULL, cvs_args_argc(&args), cvs_args_argv(&args), NULL, NULL);
  cvs_processes_stop();
}

/* executa o comando 'cvs logout' */
void cvs_command_logout(void)
{
	CvsArgs args;

  cvs_args_init(&args, NULL, 0, TRUE);

  cvs_config_loogedin_set(FALSE);

	cvs_args_add(&args, "logout");
	cvs_args_print(&args, NULL);

  cvs_processes_start();
  cvs_process_launch(FALSE, NULL, cvs_args_argc(&args), cvs_args_argv(&args), NULL, NULL);
  cvs_processes_stop();
}

/* executa o comando 'cvs chekout' */
void cvs_command_checkout_module(CvsSelectionHandler* handler, char* rev)
{
	CvsMultiFiles* mf = NULL;
  CvsMultiFilesEntry* mfi;
	CvsArgs args;
  CvsConfigCheckout config_checkout;

  CVSLIB_IFNULL1(handler);
	if(!_cvs_selection_handler_check(handler, &mf, FALSE))
		return;

  mfi = (CvsMultiFilesEntry*)mf->dirs->data;
  strcpy(config_checkout.path, mfi->dir);

	if(!cvs_config_checkout_get(mf, &config_checkout))
		return;

	if(rev != NULL && strlen(rev) > 0)
	{
		char* revision = _strdup(rev);
		if(revision != NULL)
		{
			cvs_functions_trim(revision);
			if(!iscntrl(revision[0]))
				strcpy(config_checkout.rev, revision);
			CVSLIB_FREE(revision);
		}
	}
	
  cvs_args_init(&args, NULL, 0, TRUE);

	if(config_checkout.force_cvsroot)
	{
		cvs_args_add(&args, "-d");
		cvs_args_add(&args, config_checkout.cvsroot);
	}

  cvs_args_add(&args, config_checkout.doexport ? "export" : "checkout");

  if(cvs_config_prune_get() && !config_checkout.doexport)
		cvs_args_add(&args, "-P");
	
	if(config_checkout.no_recursive)
		cvs_args_add(&args, "-l");
	
  if(config_checkout.to_stdout)
		cvs_args_add(&args, "-p");
	
	if(config_checkout.case_sensitive_names)
		cvs_args_add(&args, "-S");
	
	if(config_checkout.last_checkin_time)
		cvs_args_add(&args, "-t");
	
  if(config_checkout.reset_sticky)
	{
		cvs_args_add(&args, "-A");
	}
	else
	{
		if(config_checkout.keyword[0] != 0)
		{
			cvs_args_add(&args, "-k");
			cvs_args_add(&args, config_checkout.keyword);
		}
		
		if(config_checkout.use_most_recent)
		{
			cvs_args_add(&args, "-f");
		}
		
    if(config_checkout.date[0] != 0)
		{
			cvs_args_add(&args, "-D");
			cvs_args_add(&args, config_checkout.date);
		}
		
		if(config_checkout.rev[0] != 0)
		{
			cvs_args_add(&args, "-r");
			cvs_args_add(&args, config_checkout.rev);
		}
	}

	if(config_checkout.rev1[0] != 0)
	{
    char tmp[CVSLIB_REV_LEN+4];

		if(config_checkout.branch_point_merge)
			cvs_args_add(&args, "-b");
		
    if(config_checkout.three_way_conflicts)
			cvs_args_add(&args, "-3");
		
    sprintf(tmp, "-j%s", config_checkout.rev1);
		cvs_args_add(&args, tmp);
	}
	
  if(config_checkout.rev2[0] != 0)
	{
    char tmp[CVSLIB_REV_LEN+4];

    sprintf(tmp, "-j%s", config_checkout.rev2);
		cvs_args_add(&args, tmp);
	}
	
  if(config_checkout.doexport 
    && (config_checkout.rev[0] == 0)
    && (config_checkout.date[0] == 0))
	{
		cvs_args_add(&args, "-r");
		cvs_args_add(&args, "HEAD");
	}

  if(config_checkout.override_checkout_dir)
	{
    if(config_checkout.dont_shorten_paths)
			cvs_args_add(&args, "-N");

		cvs_args_add(&args, "-d");
		cvs_args_add(&args, config_checkout.checkout_dir);
	}

  cvs_args_endopt(&args);
  cvs_args_add(&args, config_checkout.modname);
	cvs_args_print(&args, config_checkout.path);
  cvs_processes_start();
  cvs_process_launch(FALSE, config_checkout.path, cvs_args_argc(&args), cvs_args_argv(&args), NULL, NULL);
  cvs_processes_stop();
}

/* executa o comando 'cvs update' */
void cvs_command_update(CvsSelectionHandler* handler, BOOL query_only)
{
  CvsMultiFiles* mf = NULL;
  CvsConfigUpdate config_update;
  CvsArgs args;
  List* l;

  CVSLIB_IFNULL1(handler);
  if( !_cvs_selection_handler_check(handler, &mf, FALSE))
    return;

  cvs_args_init(&args, NULL, 0, TRUE);

  if(!query_only && !cvs_config_update_get(mf, &config_update))
    return;

  for (l = mf->dirs; l; l = list_next(l))
  {
    CvsMultiFilesEntry* mfi;
    const char* path;
    CvsArgs args;

    mfi = (CvsMultiFilesEntry*)l->data;

    cvs_args_init(&args, NULL, 0, TRUE);

    cvs_args_add(&args, "-d");
		cvs_args_add(&args, cvs_config_cvsroot_get());

    if(query_only)
      cvs_args_add(&args, "-n");

    cvs_args_add(&args, "update");

    if(!config_update.reset_sticky && config_update.keyword[0] != 0)
    {
      cvs_args_add(&args, "-k");
      cvs_args_add(&args, config_update.keyword);
    }

    if(cvs_config_prune_get())
      cvs_args_add(&args, "-P");

    if(!query_only)
    {
      if(config_update.case_sensitive_names)
        cvs_args_add(&args, "-S");

      if(config_update.last_checkin_time)
        cvs_args_add(&args, "-t");

      if(config_update.reset_sticky)
        cvs_args_add(&args, "-A");

      if(config_update.create_miss_dir)
        cvs_args_add(&args, "-d");

      if(config_update.get_clean_copy)
        cvs_args_add(&args, "-C");

      if(!config_update.reset_sticky && config_update.date[0] != 0)
      {
        cvs_args_add(&args, "-D");
        cvs_args_add(&args, config_update.date);
      }

      if(!config_update.reset_sticky && config_update.use_most_recent)
        cvs_args_add(&args, "-f");

      if(config_update.rev1[0] != 0)
      {
        char tmp[CVSLIB_REV_LEN+4];

        if(config_update.branch_point_merge)
          cvs_args_add(&args, "-b");

        if(config_update.three_way_conflicts)
          cvs_args_add(&args, "-3");

        sprintf(tmp, "-j%s", config_update.rev1);
        cvs_args_add(&args, tmp);
      }

      if(config_update.rev2[0] != 0)
      {
        char tmp[CVSLIB_REV_LEN+4];

        sprintf(tmp, "-j%s", config_update.rev2);
        cvs_args_add(&args, tmp);
      }

      if(config_update.no_recursive)
        cvs_args_add(&args, "-l");

      if(config_update.to_stdout)
        cvs_args_add(&args, "-p");

      if(config_update.rev[0] != 0)
      {
        cvs_args_add(&args, "-r");
        cvs_args_add(&args, config_update.rev);
      }
    }

    path = cvs_multi_files_entry_add_byargs(mfi, &args);
    cvs_args_print(&args, path);
    cvs_processes_start();
    cvs_process_launch(FALSE, path, cvs_args_argc(&args), cvs_args_argv(&args), NULL, NULL);
    cvs_processes_stop();
  }
}

/* executa o comando 'cvs commit' */
void cvs_command_commit(CvsSelectionHandler* handler, char* comment)
{
	CvsMultiFiles* mf = NULL;
  CvsConfigCommit config_commit;
  CvsArgs args;
  List* l;

  CVSLIB_IFNULL1(handler);
	if( !_cvs_selection_handler_check(handler, &mf, FALSE))
		return;

  cvs_args_init(&args, NULL, 0, TRUE);

	if( !cvs_config_commit_get(mf, &config_commit) )
		return;

	if(_cvs_commands_commit_console != NULL)
	{
		_cvs_commands_commit_console->auto_close = TRUE;
		cvs_console_destroy(_cvs_commands_commit_console);
		_cvs_commands_commit_console = NULL;
  }
	_cvs_commands_commit_console = cvs_console_create(NULL, FALSE);
  if(_cvs_commands_commit_console == NULL)
  {
    cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
    return;
  }
	if(!cvs_console_open(_cvs_commands_commit_console))
  {
		cvs_config_error_msg_set("ERROR: Impossivel criar arquivo de log temporario.", 0, CVSLIB_ERROR_IO);
    return;
  }

	if(comment != NULL && strlen(comment) > 0)
		sprintf(config_commit.log_msg, "%.*s", CVSLIB_LOGMSG, comment);
	
  for (l = mf->dirs; l; l = list_next(l))
  {
    CvsMultiFilesEntry* mfi;
    const char* path;
		CvsArgs args;

    mfi = (CvsMultiFilesEntry*)l->data;

    cvs_args_init(&args, NULL, 0, TRUE);

    cvs_args_add(&args, "-d");
		cvs_args_add(&args, cvs_config_cvsroot_get());

		cvs_args_add(&args, "commit");

		if(config_commit.check_valid_edits)
			cvs_args_add(&args, "-c");
		
    if(config_commit.no_recursive)
			cvs_args_add(&args, "-l");
		
    if(config_commit.no_module_program)
			cvs_args_add(&args, "-n");
		
    if(config_commit.force_commit)
			cvs_args_add(&args, "-f");
		
    if(config_commit.force_recurse)
			cvs_args_add(&args, "-R");
		
		cvs_args_add(&args, "-m");
    _cvs_commands_cleanup_log_msg(config_commit.log_msg);
    cvs_args_add(&args, config_commit.log_msg);
		
		if(config_commit.rev[0] != 0)
		{
			cvs_args_add(&args, "-r");
			cvs_args_add(&args, config_commit.rev);
		}
		
    path = cvs_multi_files_entry_add_byargs(mfi, &args);
		cvs_args_print(&args, path);

    cvs_processes_start();
    cvs_process_launch(FALSE, path, cvs_args_argc(&args), cvs_args_argv(&args), _cvs_commands_commit_console, NULL);
    cvs_processes_stop();
  }
}

/* executa o comando 'cvs admin -l' */
void cvs_command_lock(CvsSelectionHandler* handler)
{	
	CvsMultiFiles* mf = NULL;
  List* l;

  CVSLIB_IFNULL1(handler);
	if( !_cvs_selection_handler_check(handler, &mf, FALSE))
		return;

  for (l = mf->dirs; l; l = list_next(l))
  {
    CvsMultiFilesEntry* mfi;
    const char* path;
		CvsArgs args;

    mfi = (CvsMultiFilesEntry*)l->data;
    cvs_args_init(&args, NULL, 0, TRUE);

		cvs_args_add(&args, "-d");
		cvs_args_add(&args, cvs_config_cvsroot_get());

		cvs_args_add(&args, "admin");
		cvs_args_add(&args, "-l");
    path = cvs_multi_files_entry_add_byargs(mfi, &args);
		cvs_args_print(&args, path);

		cvs_processes_start();
    cvs_process_launch(FALSE, path, cvs_args_argc(&args), cvs_args_argv(&args), NULL, NULL);
		cvs_processes_stop();
  }
}

/* executa o comando 'cvs admin -u' */
void cvs_command_unlock(CvsSelectionHandler* handler)
{	
	CvsMultiFiles* mf = NULL;
  List* l;

  CVSLIB_IFNULL1(handler);
	if( !_cvs_selection_handler_check(handler, &mf, FALSE))
		return;

  for (l = mf->dirs; l; l = list_next(l))
  {
    CvsMultiFilesEntry* mfi;
    const char* path;
		CvsArgs args;

    mfi = (CvsMultiFilesEntry*)l->data;

    cvs_args_init(&args, NULL, 0, TRUE);

		cvs_args_add(&args, "-d");
		cvs_args_add(&args, cvs_config_cvsroot_get());

		cvs_args_add(&args, "admin");
		cvs_args_add(&args, "-u");

    path = cvs_multi_files_entry_add_byargs(mfi, &args);
		cvs_args_print(&args, path);

    cvs_processes_start();
		cvs_process_launch(FALSE, path, cvs_args_argc(&args), cvs_args_argv(&args), NULL, NULL);
    cvs_processes_stop();
  }
}

void cvs_command_add(CvsSelectionHandler* handler, AddCmdType add_type)
{
	CvsMultiFiles* mf = NULL;
	List* l;

	CVSLIB_IFNULL1(handler);
	if(!_cvs_selection_handler_check(handler, &mf, FALSE) )
		return;

	for (l = mf->dirs; l; l = list_next(l))
  {
		CvsMultiFilesEntry* mfi;
		const char* path;
		CvsArgs args;

		mfi = (CvsMultiFilesEntry*)l->data;
		cvs_args_init(&args, NULL, 0, TRUE);

		cvs_args_add(&args, "-d");
		cvs_args_add(&args, cvs_config_cvsroot_get());

		if(list_count(mfi->files) > 0)
		{
			cvs_args_add(&args, "add");
			if(add_type == ADD_CMD_TYPE_BINARY)
				cvs_args_add(&args, "-kb");
			else if(add_type == ADD_CMD_TYPE_UNICODE)
				cvs_args_add(&args, "-ku");

			path = cvs_multi_files_entry_add_byargs(mfi, &args);
			cvs_args_print(&args, path);

			cvs_processes_start();
			cvs_process_launch(FALSE, path, cvs_args_argc(&args), cvs_args_argv(&args), NULL, NULL);
			cvs_processes_stop();
		}
		else
		{
			char uppath[MAX_PATH];
			char folder[MAX_PATH];

			cvs_args_add(&args, "add");
			cvs_args_endopt(&args);

			path = cvs_multi_files_entry_folder_add(mfi, &args, uppath, folder);

			cvs_args_print(&args, path);

			cvs_processes_start();
			cvs_process_launch(FALSE, path, cvs_args_argc(&args), cvs_args_argv(&args), NULL, NULL);
			cvs_processes_stop();
		}
	}
}

void cvs_command_edit(CvsSelectionHandler* handler, EditCmdType edit_type)
{
	CvsMultiFiles* mf = NULL;
	List* l;

	CVSLIB_IFNULL1(handler);
	if(!_cvs_selection_handler_check(handler, &mf, FALSE) )
		return;

	for (l = mf->dirs; l; l = list_next(l))
  {
		CvsMultiFilesEntry* mfi;
		const char* path;
		CvsArgs args;

		mfi = (CvsMultiFilesEntry*)l->data;
		cvs_args_init(&args, NULL, 0, TRUE);

		cvs_args_add(&args, "-d");
		cvs_args_add(&args, cvs_config_cvsroot_get());

		cvs_args_add(&args, "edit");

		switch(edit_type)
		{
		case EDIT_CMD_TYPE_RESERVED:
			cvs_args_add(&args, "-c");
			break;
		case EDIT_CMD_TYPE_FORCE:
			cvs_args_add(&args, "-f");
			break;
		default:
		case EDIT_CMD_TYPE_NORMAL:
			break;
		}

    path = cvs_multi_files_entry_add_byargs(mfi, &args);
		cvs_args_print(&args, path);

    cvs_processes_start();
		cvs_process_launch(FALSE, path, cvs_args_argc(&args), cvs_args_argv(&args), NULL, NULL);
    cvs_processes_stop();

	}
}

void cvs_command_unedit(CvsSelectionHandler* handler)
{
	CvsMultiFiles* mf = NULL;
	List* l;

	CVSLIB_IFNULL1(handler);
	if(!_cvs_selection_handler_check(handler, &mf, FALSE) )
		return;

	for (l = mf->dirs; l; l = list_next(l))
  {
		CvsMultiFilesEntry* mfi;
		const char* path;
		CvsArgs args;

		mfi = (CvsMultiFilesEntry*)l->data;
		cvs_args_init(&args, NULL, 0, TRUE);

		cvs_args_add(&args, "-d");
		cvs_args_add(&args, cvs_config_cvsroot_get());

		cvs_args_add(&args, "unedit");

		path = cvs_multi_files_entry_add_byargs(mfi, &args);
		cvs_args_print(&args, path);

    cvs_processes_start();
		cvs_process_launch(FALSE, path, cvs_args_argc(&args), cvs_args_argv(&args), NULL, NULL);
    cvs_processes_stop();
	}
}

void cvs_command_list_locked_file(CvsSelectionHandler* handler, int* count)
{
	CvsMultiFiles* mf = NULL;
  CvsArgs args;
  CvsConsole* cvs_console;
  List* l;

	CVSLIB_IFNULL1(count);
  CVSLIB_IFNULL1(handler);
	cvs_list_locked_files_delete(_cvs_commands_list_locked_file);
	_cvs_commands_list_locked_file = NULL;
	if( !_cvs_selection_handler_check(handler, &mf, FALSE))
		return;
  cvs_args_init(&args, NULL, 0, TRUE);
	_cvs_commands_list_locked_file = cvs_list_locked_files_create();
	
	cvs_console = cvs_console_create(NULL, FALSE);
  if(cvs_console == NULL)
  {
    cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
    return;
  }
	if(!cvs_console_open(cvs_console))
  {
		cvs_config_error_msg_set("ERROR: Impossivel criar arquivo de log temporario.", 0, CVSLIB_ERROR_IO);
    return;
  }
  	
	*count = 0;
  for (l = mf->dirs; l; l = list_next(l))
  {
    CvsMultiFilesEntry* mfi;
    const char* path;
		CvsArgs args;

    mfi = (CvsMultiFilesEntry*)l->data;

    cvs_args_init(&args, NULL, 0, TRUE);
    cvs_args_add(&args, "-d");
		cvs_args_add(&args, cvs_config_cvsroot_get());
		cvs_args_add(&args, "-Q");
		cvs_args_add(&args, "-n");
		cvs_args_add(&args, "log");
		cvs_args_add(&args, "-h");
		cvs_args_add(&args, "-N");
		
    path = cvs_multi_files_entry_add_byargs(mfi, &args);
		cvs_args_print(&args, path);

    cvs_processes_start();
    cvs_process_launch(FALSE, path, cvs_args_argc(&args), cvs_args_argv(&args), cvs_console, NULL);
    cvs_processes_stop();
    *count += cvs_list_locked_files_load(_cvs_commands_list_locked_file, cvs_console->file_out);
    cvs_console->auto_close = TRUE;
    cvs_console_destroy(cvs_console);
    if(*count < 0)
    {
			cvs_list_locked_files_delete(_cvs_commands_list_locked_file);
			_cvs_commands_list_locked_file = NULL;
			return;
    }
  }
  
  if(*count == 0)
  {
		cvs_list_locked_files_delete(_cvs_commands_list_locked_file);
		_cvs_commands_list_locked_file = NULL;
  }
}

CvsLockedFile* cvs_command_list_locked_files_get(int index)
{
	if(_cvs_commands_list_locked_file == NULL)
	{
		cvs_config_error_msg_set("ERROR: Lista de arquivos bloqueados nao inicializado.", 0, CVSLIB_ERROR_NOT_FOUND);
		return NULL;
	}
	if(index < 0 && index >= list_count(_cvs_commands_list_locked_file->files))
	{
		cvs_config_error_msg_set("ERROR: parametro 'index' com valor invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER);
		return NULL;
	}
	return (CvsLockedFile*)list_nth(_cvs_commands_list_locked_file->files, index);
}

void cvs_command_list_locked_files_end()
{
	cvs_list_locked_files_delete(_cvs_commands_list_locked_file);
	_cvs_commands_list_locked_file = NULL;
}

void cvs_command_list_log_file(CvsSelectionHandler* handler, int* count)
{
	CvsMultiFiles* mf = NULL;
	CvsConsole* cvs_console;
  CvsArgs args;
  List* l;

	CVSLIB_IFNULL1(count);
  CVSLIB_IFNULL1(handler);
	cvs_list_log_file_delete(_cvs_commands_list_log_file);
	_cvs_commands_list_log_file = NULL;
	if( !_cvs_selection_handler_check(handler, &mf, FALSE))
		return;
  cvs_args_init(&args, NULL, 0, TRUE);
	_cvs_commands_list_log_file = cvs_list_log_file_create();
	
	cvs_console = cvs_console_create(NULL, FALSE);
  if(cvs_console == NULL)
  {
    cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
    return;
  }
	if(!cvs_console_open(cvs_console))
  {
		cvs_config_error_msg_set("ERROR: Impossivel criar arquivo de log temporario.", 0, CVSLIB_ERROR_IO);
    return;
  }
	
	*count = 0;
  for (l = mf->dirs; l; l = list_next(l))
  {
    CvsMultiFilesEntry* mfi;
    const char* path;
		CvsArgs args;

    mfi = (CvsMultiFilesEntry*)l->data;

    cvs_args_init(&args, NULL, 0, TRUE);
    cvs_args_add(&args, "-d");
		cvs_args_add(&args, cvs_config_cvsroot_get());
		cvs_args_add(&args, "log");
		
    path = cvs_multi_files_entry_add_byargs(mfi, &args);
		cvs_args_print(&args, path);

    cvs_processes_start();
    cvs_process_launch(FALSE, path, cvs_args_argc(&args), cvs_args_argv(&args), cvs_console, NULL);
    cvs_processes_stop();
    *count += cvs_list_log_file_load(_cvs_commands_list_log_file, cvs_console->file_out);
    cvs_console->auto_close = TRUE;
    cvs_console_destroy(cvs_console);
    if(*count < 0)
    {
			cvs_list_log_file_delete(_cvs_commands_list_log_file);
			_cvs_commands_list_log_file = NULL;
			return;
    }
  }
  
  if(*count == 0)
  {
		cvs_list_locked_files_delete(_cvs_commands_list_locked_file);
		_cvs_commands_list_locked_file = NULL;
  }
}

CvsLogFile* cvs_command_list_log_file_get(int index)
{
	if(_cvs_commands_list_log_file == NULL)
	{
		cvs_config_error_msg_set("ERROR: Lista de log do arquivo nao inicializado.", 0, CVSLIB_ERROR_NOT_FOUND);
		return NULL;
	}
	if(index < 0 && index >= list_count(_cvs_commands_list_log_file->logs))
	{
		cvs_config_error_msg_set("ERROR: parametro 'index' com valor invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER);
		return NULL;
	}
	return (CvsLogFile*)list_nth(_cvs_commands_list_log_file->logs, index);
}

void cvs_command_list_log_file_end()
{
	cvs_list_log_file_delete(_cvs_commands_list_log_file);
	_cvs_commands_list_log_file = NULL;
}

char* cvs_command_log_cvs_get()
{
	FILE* log_cvs;
	char* aux;
	
	memset(_cvs_command_log_cvs_command, 0, CVSLIBV_LINE_LEN+1);
	if(cvs_config_running_background_get() == FALSE
		|| _cvs_commands_commit_console == NULL
		|| strlen(_cvs_commands_commit_console->file_name) == 0)
		return _cvs_command_log_cvs_command;
		
	log_cvs = fopen(_cvs_commands_commit_console->file_name, "r");
	if(log_cvs == NULL)
		return _cvs_command_log_cvs_command;
		
	fgets(_cvs_command_log_cvs_command, CVSLIBV_LINE_LEN, log_cvs);
	if((aux = strchr(_cvs_command_log_cvs_command, '\n')) != NULL)
	{
		*aux = '\0';
		if(*(aux-1) == '\r')
			*(aux-1) = '\0';
	}		

	return _cvs_command_log_cvs_command;
}

static BOOL _cvs_selection_handler_check(CvsSelectionHandler* handler, CvsMultiFiles** mf, BOOL force_normalize)
{
	*mf = cvs_selection_handler_selection_get(handler);
	if(!*mf && cvs_selection_handler_need_selection_get(handler))
	{
		return FALSE;
	}

  return (force_normalize || cvs_selection_handler_needs_normalize(handler)) ? cvs_multi_files_normalize(*mf) : TRUE;
}

static char* _cvs_commands_cleanup_log_msg(char* msg)
{
	char buf[CVSLIB_LOGMSG+1];
	char* tmp;
	char c;
	size_t len;

  len = strlen(msg);
  tmp = buf;

	while((c = *msg++) != '\0')
	{
		if(c == '\r')
			continue;
		*tmp++ = c;
	}
	*tmp++ = '\0';

  strcpy(msg, buf);
	return msg;
}

BOOL _cvs_commands_split_path(const char* dir, char* uppath, char* folder, BOOL dont_skip_delimiter)
{
	char new_dir[MAX_PATH * 2] = "";
	char old_dir[MAX_PATH * 2] = "";
	char new_file[MAX_PATH] = "";
  int dir_len;

	if( !dir )
		return FALSE;

	dir_len = (int)strlen(dir);
	if(dir_len > 0)
	{
		const char* tmp;
		const char* prev;

		strcpy(old_dir, dir);
		if(!dont_skip_delimiter && (dir[dir_len - 1] == PATH_DELIMITER))
		{
			old_dir[dir_len - 1] = '\0';
			--dir_len;
		}
		
		tmp = old_dir;
		prev = old_dir;
		
		while((tmp = strchr(tmp, PATH_DELIMITER)) != 0L)
		{
			prev = ++tmp;
		}
		
		strcpy(new_dir, old_dir);
		new_file[prev - old_dir] = '\0';
		strcpy(new_file, prev);
	}
	
  strcpy(uppath, new_dir);
	strcpy(folder, new_file);

	return TRUE;
}

