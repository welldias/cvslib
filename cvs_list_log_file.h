#ifndef _CVSLIB_LIST_LOG_FILE_H_
#define _CVSLIB_LIST_LOG_FILE_H_

enum _LogNodeType
{
	LOG_NODE_REVISION,
	LOG_NODE_TAG,
	LOG_NODE_BRANCH,
};

struct _CvsLogFile
{
	char*       revision;
	char*       date;
	char*       user;
	char*       description;
	char*       branches;
	int         depth;
	LogNodeType type;
	BOOL        reordered;
};

struct _CvsListSymbolicName
{
	char* name;
	char* revision;
	BOOL  inserted;
};


struct _CvsListLogFile
{
	char* file_name;
	char* head;
	List* symbolic_names;
	List* logs;
};

CvsLogFile*         cvs_log_file_create                    (char *revision, char *date, char *user, char* description, char* branches);
void                cvs_log_file_init                      (CvsLogFile* log_file, char *revision, char *date, char *user, char* description, char* branches);
int                 cvs_log_file_compare                   (CvsLogFile* log_file_a, CvsLogFile* log_file_b);
void                cvs_log_file_delete                    (CvsLogFile* log_file);
BOOL                cvs_log_file_is_child_of               (char* rev_a, char* rev_b);
BOOL                cvs_log_file_is_same_branch            (CvsLogFile* a, CvsLogFile* b);
BOOL                cvs_log_file_is_part_of                (CvsLogFile* a, CvsLogFile* b);
BOOL                cvs_log_file_is_sub_branch_of          (CvsLogFile* a, CvsLogFile* b);

CvsListLogFile*     cvs_list_log_file_create               ();
void                cvs_list_log_file_init                 (CvsListLogFile* list_log_file);
void                cvs_list_log_file_delete               (CvsListLogFile* list_log_file);
int                 cvs_list_log_file_load                 (CvsListLogFile* list_log_file, char* file_name);
int                 cvs_list_log_file_load                 (CvsListLogFile* list_log_file, FILE* stream);
int                 cvs_list_log_file_rehierarchy          (CvsListLogFile* list_log_file);
int                 cvs_list_log_file_rehierarchy_branch   (CvsListLogFile* list_log_file, CvsListSymbolicName* symbolic_name);
int                 cvs_list_log_file_rehierarchy_tag      (CvsListLogFile* list_log_file, CvsListSymbolicName* symbolic_name);

#endif /* _CVSLIB_LIST_LOG_FILE_H_  */