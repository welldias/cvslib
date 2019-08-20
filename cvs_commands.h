#ifndef CVS_COMMANDS_H_
#define CVS_COMMANDS_H_

/* enumerador para tipo de edit */
enum _EditCmdType
{
	EDIT_CMD_TYPE_NORMAL,    /* cvs edit    */
	EDIT_CMD_TYPE_RESERVED,  /* cvs edit -c */
	EDIT_CMD_TYPE_FORCE,     /* cvs edit -f */
};

/* enumerador para tipo de tag */
enum _TagCmdType
{
	TAG_CMD_TYPE_CREATE, /* create tag */
	TAG_CMD_TYPE_DELETE, /* delete tag */
	TAG_CMD_TYPE_BRANCH  /* create branch tag */
};

/* enumerador para tipos de comandos add */
enum _AddCmdType
{
	ADD_CMD_TYPE_NORMAL, /* cvs add */
	ADD_CMD_TYPE_BINARY, /* cvs add -kb */
	ADD_CMD_TYPE_UNICODE /* cvs add -ku */
};

/* enumerador para tipos de comandos diff */
enum _DiffCmdType
{
	DIFF_CMD_TYPE_DIFF_FILE,    /* Diff para arquivos */
	DIFF_CMD_TYPE_DIFF_FOLDER,  /* Diff para diretorios */
	DIFF_CMD_TYPE_DIFF_GRAPH    /* Diff para graficos */
};

/* enumerador para tipos de comandos annotate */
enum _AnnotateCmdType
{
	ANNOTATE_CMD_TYPE_FILE,
	ANNOTATE_CMD_TYPE_GRAPH,
};

/* enumerador tipo de selecao */
enum _SelCmdType
{
	SEL_NONE_CMD,   /* nao especificado */
	SEL_FILES_CMD,	/* selecao de arquivos */
	SEL_DIR_CMD     /* selecao diretorio */
};

/* enumerador tipo de controle */
enum _HandleCmdType
{
	HANDLE_CMD_SELECTION = 0,
	HANDLE_CMD_LOCK,
	HANDLE_CMD_UNLOCK,
	HANDLE_CMD_CHECKOUT
};

struct _CvsFileEntry
{
	char* file;
	char* curr_revision;
	UFSSpec spec;
};

CvsFileEntry*     cvs_file_entry_create        ();
void              cvs_file_entry_delete        (CvsFileEntry* cvs_file_entry);
void              cvs_file_entry_file_set      (CvsFileEntry* cvs_file_entry, const char* file_name);
void              cvs_file_entry_curr_rev_set  (CvsFileEntry* cvs_file_entry, const char* curr_revision);
void              cvs_file_entry_spec_set      (CvsFileEntry* cvs_file_entry, const UFSSpec spec);

struct _CvsMultiFilesEntry
{
	char* dir;   /* diretorio */
	List* files;
};

CvsMultiFilesEntry* cvs_multi_files_entry_create     (const char* path);
void                cvs_multi_files_entry_delete     (CvsMultiFilesEntry* cvs_multi_files_entry);
void                cvs_multi_files_entry_dir_set    (CvsMultiFilesEntry* cvs_multi_files_entry, const char* newdir);
void                cvs_multi_files_entry_add        (CvsMultiFilesEntry* cvs_multi_files_entry, const char* file, const UFSSpec spec, const char* curr_revision);
CvsFileEntry*       cvs_multi_files_entry_get        (CvsMultiFilesEntry* cvs_multi_files_entry, int index);
int                 cvs_multi_files_entry_files_count(CvsMultiFilesEntry* cvs_multi_files_entry);
const char*         cvs_multi_files_entry_dir_get    (CvsMultiFilesEntry* cvs_multi_files_entry);
const char*         cvs_multi_files_entry_add_byargs (CvsMultiFilesEntry* cvs_multi_files_entry, CvsArgs* args);
const char*         cvs_multi_files_entry_folder_add (CvsMultiFilesEntry* cvs_multi_files_entry, CvsArgs* args, char* uppath, char* folder);

struct _CvsMultiFiles
{
	List* dirs;
};

CvsMultiFiles*      cvs_multi_files_create         ();
void                cvs_multi_files_delete         (CvsMultiFiles* cvs_multi_files);
void                cvs_multi_files_new_dir        (CvsMultiFiles* cvs_multi_files, const char* dir);
void                cvs_multi_files_new_file       (CvsMultiFiles* cvs_multi_files, const char* file, UFSSpec spec, const char* curr_revision);
BOOL                cvs_multi_files_dir_get        (CvsMultiFiles* cvs_multi_files, int index, char* path);
int                 cvs_multi_files_dirs_count     (CvsMultiFiles* cvs_multi_files);
int                 cvs_multi_files_num_files_count(CvsMultiFiles* cvs_multi_files);
void                cvs_multi_files_reset          (CvsMultiFiles* cvs_multi_files);
BOOL                cvs_multi_files_normalize      (CvsMultiFiles* cvs_multi_files);
CvsMultiFilesEntry* cvs_multi_files_find_dir       (CvsMultiFiles* cvs_multi_files, const char* dir);
BOOL                cvs_multi_files_split          (char* in_partial_path, char* out_path, char* out_name);

struct _CvsSelectionHandler
{
	char* str_command;	        /* Nome do camando */
	CvsMultiFiles* multi_files; /* Selecao (arquivos ou diretorio) */
	SelCmdType sel_type;	      /* Command selection type */
	BOOL auto_delete;           /* indica se libera memoria alocada com o comando selecao */
	BOOL need_selection;
	BOOL need_view_reset;
	HandleCmdType handler_cmd_type;
};

CvsSelectionHandler* cvs_selection_handler_create              (const char* str_command, BOOL need_selection, BOOL need_reset_view);
void                 cvs_selection_handler_init                (CvsSelectionHandler* cvs_selection_handler, const char* str_command, BOOL need_selection, BOOL need_reset_view);
void                 cvs_selection_handler_delete              (CvsSelectionHandler* cvs_selection_handler);
SelCmdType           cvs_selection_handler_seltype_get         (CvsSelectionHandler* cvs_selection_handler);
BOOL                 cvs_selection_handler_need_selection_get  (CvsSelectionHandler* cvs_selection_handler);
BOOL                 cvs_selection_handler_need_view_reset_get (CvsSelectionHandler* cvs_selection_handler);
CvsMultiFiles*       cvs_selection_handler_selection_get       (CvsSelectionHandler* cvs_selection_handler);
BOOL                 cvs_selection_handler_needs_normalize     (CvsSelectionHandler* cvs_selection_handler);

CvsSelectionHandler* cvs_lock_handler_create                   ();
CvsSelectionHandler* cvs_unlock_handler_create                 ();

/* Commands */
void                 cvs_command_init                          ();
void                 cvs_command_login                         (void);
void                 cvs_command_logout                        (void);
void                 cvs_command_checkout_module               (CvsSelectionHandler* handler, char* rev);
void                 cvs_command_update                        (CvsSelectionHandler* handler, BOOL query_only);
void                 cvs_command_commit                        (CvsSelectionHandler* handler, char* comment);
void                 cvs_command_lock                          (CvsSelectionHandler* handler);
void                 cvs_command_unlock                        (CvsSelectionHandler* handler);
void                 cvs_command_add                           (CvsSelectionHandler* handler, AddCmdType add_type);
void                 cvs_command_unedit                        (CvsSelectionHandler* handler);
void                 cvs_command_edit                          (CvsSelectionHandler* handler, EditCmdType edit_type);
void                 cvs_command_list_locked_file              (CvsSelectionHandler* handler, int* count);
CvsLockedFile*       cvs_command_list_locked_files_get         (int index);
void                 cvs_command_list_locked_files_end         ();
void                 cvs_command_list_log_file                 (CvsSelectionHandler* handler, int* count);
CvsLogFile*          cvs_command_list_log_file_get             (int index);
void                 cvs_command_list_log_file_end             ();
char*                cvs_command_log_cvs_get                   ();

#endif /*CVS_COMMANDS_H_*/

