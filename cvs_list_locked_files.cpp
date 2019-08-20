#include "CvsLib.h"

#define CVSLIB_WF_TAG "Working file: "
#define CVSLIB_AL_TAG "access list:"
#define CVSLIB_LK_TAG "locks: "

static void _cvs_locked_file_delete_foreach(void* locked_file);

CvsLockedFile* cvs_locked_file_create(char *file_name, char *user, char *revision)
{
  CvsLockedFile* locked_file;

  locked_file = (CvsLockedFile*)malloc(sizeof(CvsLockedFile));
  CVSLIB_IFNULL2(locked_file, NULL);
  memset(locked_file, 0, sizeof(CvsLockedFile));
  cvs_locked_file_init(locked_file, file_name, user, revision);
  return locked_file;
}

void cvs_locked_file_init(CvsLockedFile* locked_file, char *file_name, char *user, char *revision)
{
  CVSLIB_IFNULL1(locked_file);
  if(locked_file->file_name && file_name)
		CVSLIB_FREE(locked_file->file_name);
  locked_file->file_name = file_name ? _strdup(file_name) : locked_file->file_name;
  if(locked_file->user && user)
		CVSLIB_FREE(locked_file->user);
  locked_file->user = user ? _strdup(user) : locked_file->user;
  if(locked_file->revision && revision)
		CVSLIB_FREE(locked_file->revision);
  locked_file->revision = revision ? _strdup(revision) : locked_file->revision;
}

void cvs_locked_file_delete(CvsLockedFile* locked_file)
{
  CVSLIB_IFNULL1(locked_file);
	CVSLIB_FREE(locked_file->file_name);
	CVSLIB_FREE(locked_file->user);
	CVSLIB_FREE(locked_file->revision);
	CVSLIB_FREE(locked_file);
}

static void _cvs_locked_file_delete_foreach(void* locked_file)
{
	CVSLIB_IFNULL1(locked_file);
	cvs_locked_file_delete((CvsLockedFile*)locked_file);
}

CvsListLockedFiles* cvs_list_locked_files_create()
{
  CvsListLockedFiles* list_locked_file;

  list_locked_file = (CvsListLockedFiles*)malloc(sizeof(CvsListLockedFiles));
  CVSLIB_IFNULL2(list_locked_file, NULL);
  memset(list_locked_file, 0, sizeof(CvsListLockedFiles));
  cvs_list_locked_files_init(list_locked_file);
  return list_locked_file;
}

void cvs_list_locked_files_init(CvsListLockedFiles* list_locked_file)
{
	CVSLIB_IFNULL1(list_locked_file);
}

void cvs_list_locked_files_delete(CvsListLockedFiles* list_locked_file)
{
	CVSLIB_IFNULL1(list_locked_file);
	list_locked_file->files = list_free_foreach(list_locked_file->files, _cvs_locked_file_delete_foreach);
	CVSLIB_FREE(list_locked_file);
}

int cvs_list_locked_files_load(CvsListLockedFiles* list_locked_file, char* file_name)
{
	FILE *stream = NULL;	
	
	CVSLIB_IFNULL2(list_locked_file, cvs_config_error_msg_set("ERROR: Parametro 'list_locked_file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
  CVSLIB_IFNULL2(stream, cvs_config_error_msg_set("ERROR: Parametro 'stream' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

  stream = fopen(file_name, "r");
	if(stream == NULL)
	{
		return cvs_config_error_msg_set("ERROR: Impossivel criar lista temporaria dos arquivos bloqueados.", 0, CVSLIB_ERROR_IO);
	}

	cvs_list_locked_files_load(list_locked_file, stream);
		
	fclose(stream);

	if(list_locked_file != NULL && list_count(list_locked_file->files) > 0)
		return list_count(list_locked_file->files);
	return cvs_config_error_get();
}

int cvs_list_locked_files_load(CvsListLockedFiles* list_locked_file, FILE* stream)
{
  char *line;
	char *ptr;
  char file_name[MAX_PATH];
  BOOL parse_locks  = FALSE;

  CVSLIB_IFNULL2(list_locked_file, cvs_config_error_msg_set("ERROR: Parametro 'list_locked_file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
	CVSLIB_IFNULL2(stream, cvs_config_error_msg_set("ERROR: Parametro 'stream' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

	rewind(stream);

  while(!feof(stream))
  {
    line = cvs_functions_line_get(stream);
		if(line == NULL || strlen(line) == 0)
    {
			CVSLIB_FREE(line);
			continue;
		}    

    if(CVSLIB_MATCH(line, CVSLIB_WF_TAG))
    {
			strcpy(file_name, line+strlen(CVSLIB_WF_TAG));
			parse_locks = FALSE;
    }
    
    if(parse_locks)
    {
			if(CVSLIB_MATCH(line, CVSLIB_AL_TAG))
			{
				parse_locks = FALSE;
			}
			else
			{
				CvsLockedFile* locked_file = NULL;
				char *user = NULL;
				char *revision = NULL;
				
				cvs_functions_trim(line);
				user = line;
				ptr = strchr(line, ':');
				if(ptr)
				{
					*ptr++ = '\0';
					revision = ++ptr;
				}
				locked_file = cvs_locked_file_create(file_name, user, revision);
				if(locked_file == NULL)
				{
					cvs_list_locked_files_delete(list_locked_file);
					CVSLIB_FREE(line);
					return cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
				}
				list_locked_file->files =  list_append(list_locked_file->files, (void*)locked_file);
			}
			
			continue;
    }

    if(CVSLIB_MATCH(line, CVSLIB_LK_TAG))
    {
			parse_locks = TRUE;
    }
      
    CVSLIB_FREE(line);
  }
  
  return list_count(list_locked_file->files);
}