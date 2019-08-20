#ifndef _CVSLIB_CVS_ARGS_H_
#define _CVSLIB_CVS_ARGS_H_

struct _CvsArgs
{
	char** argv;
	int argc;
	BOOL has_end_opt;
};

CvsArgs* cvs_args_create      (char* argv[], int argc, BOOL defargs);
void     cvs_args_init        (CvsArgs* cvs_args, char* argv[], int argc, BOOL defargs);
void     cvs_args_delete      (CvsArgs* cvs_args);
void     cvs_args_add         (CvsArgs* cvs_args, const char* arg);
void     cvs_args_add_file    (CvsArgs* cvs_args, const char* arg, const char* dir);
int      cvs_args_parse       (CvsArgs* cvs_args, const char* arg);
void     cvs_args_endopt      (CvsArgs* cvs_args);
void     cvs_args_start_files (CvsArgs* cvs_args);
void     cvs_args_reset       (CvsArgs* cvs_args, BOOL defargs);
void     cvs_args_print       (CvsArgs* cvs_args, const char* in_directory);
char**   cvs_args_argv        (CvsArgs* cvs_args);
int      cvs_args_argc        (CvsArgs* cvs_args);

#endif /*_CVSLIB_CVS_ARGS_H_*/
