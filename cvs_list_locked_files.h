#ifndef _CVSLIB_LIST_LOCK_FILES_H_
#define _CVSLIB_LIST_LOCK_FILES_H_

struct _CvsLockedFile
{
	char* file_name;
	char* user;
	char* revision;
};

struct _CvsListLockedFiles
{
	List *files;
};

CvsLockedFile*      cvs_locked_file_create        (char *file_name, char *user, char *revision);
void                cvs_locked_file_init          (CvsLockedFile* locked_file, char *file_name, char *user, char *revision);
void                cvs_locked_file_delete        (CvsLockedFile* locked_file);

CvsListLockedFiles* cvs_list_locked_files_create  ();
void                cvs_list_locked_files_init    (CvsListLockedFiles* list_locked_file);
void                cvs_list_locked_files_delete  (CvsListLockedFiles* list_locked_file);
int                 cvs_list_locked_files_load    (CvsListLockedFiles* list_locked_file, char* file_name);
int                 cvs_list_locked_files_load    (CvsListLockedFiles* list_locked_file, FILE* stream);

#endif /* _CVSLIB_LIST_LOCK_FILES_H_  */