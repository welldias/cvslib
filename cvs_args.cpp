#include "CvsLib.h"


#define CVS_CMD "cvs"

#define USE_Z9_OPTION 0
#define USE_CHECKOUT_FILE_ATTRIBUTE 0
#define USE_MESSAGES 0
#define USE_ENCRYPT_COMMUNICATION 0
#define USE_ALWAYS_USE_CVS_ROOT 1
#define MAX_PRINT_ARG 512

static char* _cvs_args_strdup(const char *string);
static char* _cvs_args_reduce_string(const char *str, char* buf, int buf_size);
static char* _cvs_args_expand_lf(const char *str, char* buf);

CvsArgs* cvs_args_create(char* argv[], int argc, BOOL defargs)
{
  CvsArgs* cvs_args;

  cvs_args = (CvsArgs*)malloc(sizeof(CvsArgs));
  CVSLIB_IFNULL2(cvs_args, NULL);
  cvs_args_init(cvs_args, argv, argc, defargs);

  return cvs_args;
}

void cvs_args_init(CvsArgs* cvs_args, char* argv[], int argc, BOOL defargs)
{
  int i;

  CVSLIB_IFNULL1(cvs_args);
  cvs_args->argv = NULL;
  cvs_args->argc = 0;
  cvs_args->has_end_opt = FALSE;
  cvs_args_reset(cvs_args, defargs);

  for (i=0; i<argc; i++) {
    cvs_args_add(cvs_args, argv[i]);
  }
}

void cvs_args_delete(CvsArgs* cvs_args)
{
  CVSLIB_IFNULL1(cvs_args);
  cvs_args_reset(cvs_args, FALSE);
  CVSLIB_FREE(cvs_args);
}

void cvs_args_reset(CvsArgs* cvs_args, BOOL defargs)
{
  int i;

  CVSLIB_IFNULL1(cvs_args);

  if (cvs_args->argv != 0L) {
    for (i = 0; i < cvs_args->argc; i++) {
      if (cvs_args->argv[i] != 0L)
        CVSLIB_FREE(cvs_args->argv[i]);
    }

    CVSLIB_FREE(cvs_args->argv);
  }

  cvs_args->argv = NULL;
  cvs_args->argc = 0;
  cvs_args->has_end_opt = FALSE;

  if (defargs) {
    int numargs = 1;

    if (USE_Z9_OPTION)
      numargs++;

    if (USE_CHECKOUT_FILE_ATTRIBUTE)
      numargs++;

    if (USE_MESSAGES)
      numargs++;

    if (USE_ENCRYPT_COMMUNICATION)
      numargs++;

    if (USE_ALWAYS_USE_CVS_ROOT)
      numargs += 2;

    cvs_args->argv = (char**)malloc(numargs * sizeof(char*));
    cvs_args->argv[cvs_args->argc++] = _cvs_args_strdup(CVS_CMD);

    if (USE_Z9_OPTION)
      cvs_args->argv[cvs_args->argc++] = _cvs_args_strdup("-z 2");

    if (USE_CHECKOUT_FILE_ATTRIBUTE)
      cvs_args->argv[cvs_args->argc++] = _cvs_args_strdup("-r"); /* readonly */

    if (USE_MESSAGES)
      cvs_args->argv[cvs_args->argc++] = _cvs_args_strdup("-q"); /* quiet */


    if (USE_ENCRYPT_COMMUNICATION)
      cvs_args->argv[cvs_args->argc++] = _cvs_args_strdup("-x");
  }
}

void cvs_args_add(CvsArgs* cvs_args, const char* arg)
{
  CVSLIB_IFNULL1(cvs_args);

  if (cvs_args->argv == 0L)
    cvs_args->argv = (char**)malloc(2 * sizeof(char*));
  else
    cvs_args->argv = (char**)realloc(cvs_args->argv, (cvs_args->argc + 2) * sizeof(char*));

  if(arg != 0L)
    cvs_args->argv[cvs_args->argc++] = _cvs_args_strdup(arg);
  else
    cvs_args->argv[cvs_args->argc++] = 0L;
  
  cvs_args->argv[cvs_args->argc] = 0L;
}

void cvs_args_add_file(CvsArgs* cvs_args, const char* arg, const char* dir)
{
  CVSLIB_IFNULL1(cvs_args);
  CVSLIB_IFNULL1(arg);
  cvs_args_add(cvs_args, arg);
}

int cvs_args_parse(CvsArgs* cvs_args, const char* arg)
{
  int res = 0;
 
  CVSLIB_IFNULL2(cvs_args, 0);
  CVSLIB_IFNULL2(arg, 0);

  return res;
}

void cvs_args_endopt(CvsArgs* cvs_args)
{
  CVSLIB_IFNULL1(cvs_args);

  //cvs_args_add(cvs_args, "--");
  //cvs_args->has_end_opt = TRUE;
}

void cvs_args_start_files(CvsArgs* cvs_args)
{
  CVSLIB_IFNULL1(cvs_args);
  
  if (!cvs_args->has_end_opt) {
    cvs_args_endopt(cvs_args);
  }
}

void cvs_args_print(CvsArgs* cvs_args, const char* in_directory)
{
  int i;
  char buf[MAX_PRINT_ARG];
  BOOL has_space;
  CVSLIB_IFNULL1(cvs_args);
 
  for(i = 0; i < cvs_args->argc; i++)
	{
    char new_arg[MAX_PATH*2];
    BOOL has_lf;
    int len;

    strcpy(new_arg, cvs_args->argv[i]);
		if(strcmp(new_arg, "\n") == 0)
			continue;

		has_lf = strchr(new_arg, '\n') != 0L;
		len = (int)strlen(new_arg);

		if(len > MAX_PRINT_ARG)
    {
      _cvs_args_reduce_string(new_arg, buf, MAX_PRINT_ARG);
			strcpy(new_arg, buf);
    }

		if(has_lf)
    {
			_cvs_args_expand_lf(new_arg, buf);
      strcpy(new_arg , buf);
    }

		has_space = strchr(new_arg, ' ') != 0L;
		if(has_space)
			cvs_config_save_outstr("\"", 1);

		cvs_config_save_outstr(new_arg, (int)strlen(new_arg));
		
		if(has_space)
      cvs_config_save_outstr("\"", 1);

		cvs_config_save_outstr(" ", 1);
	}

	if(in_directory != NULL && strlen(in_directory))
	{
    sprintf(buf, "(no diretorio %s)", in_directory); 
    cvs_config_save_outstr(buf, (int)strlen(buf));
	}

  cvs_config_save_outstr("\n", 1);
}

char** cvs_args_argv(CvsArgs* cvs_args)
{
  CVSLIB_IFNULL2(cvs_args, NULL);

  return cvs_args->argv;
}

int cvs_args_argc(CvsArgs* cvs_args)
{
  CVSLIB_IFNULL2(cvs_args, 0);

  return cvs_args->argc;
}

static char *_cvs_args_strdup(const char *string)
{
  int size = (int)strlen(string);
  char *result = (char *)malloc((size + 1) * sizeof(char));
  if (result == NULL) {
    cvs_config_error_msg_set("ERRO: Erro ao alocar memoria\n", 0, CVSLIB_ERROR_MEMORY);
    exit(1);
  }
  strcpy(result, string);
  return result;
}

static char* _cvs_args_reduce_string(const char *str, char* buf, int buf_size)
{
  int len;

	len = (int)strlen(str);
	if(len < buf_size)
	{
		strcpy(buf, str);
	}
	else
	{
		sprintf(buf, "%.*s...", buf_size-4, str);
		/*
			char *tmp;
			memcpy((char *)buf, str, MAX_PRINT_ARG * sizeof(char));
			tmp = (char *)buf + MAX_PRINT_ARG;
			*tmp++ = '.';
			*tmp++ = '.';
			*tmp++ = '.';
			*tmp = '\0';
		*/
	}

	return buf;
}

static char* _cvs_args_expand_lf(const char *str, char* buf)
{
	int num_lfs = 0;
  int len;
	const char* runner = str;
	char *tmp = buf;
	char c;

	while((runner = strchr(runner, '\n')) != 0L)
	{
		num_lfs++;
		runner++;
	}

	len = (int)strlen(str);
	runner = str;
	while((c = *runner++) != '\0')
	{
		if(c == '\n')
		{
			*tmp++ = '\\';
			*tmp++ = 'n';
		}
		else
			*tmp++ = c;
	}
	*tmp++ = '\0';

	return buf;
}