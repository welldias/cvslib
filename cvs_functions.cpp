#include "CvsLib.h"

int cvs_functions_trim(char *str)
{
  char *p1, *p2;
  size_t i;

  if(str == NULL)
    return -1;
  p2 = str;
  while(isspace(*p2))
    if(*p2++ == 0)
      return -1;
  p1 = _strdup(str);
	sprintf(p1, p2);
  if(p1 == NULL)
    return -1;
  for(i=strlen(p1); i>0; i--)
    if(!isspace(p1[i-1]))
      break;
  p1[i] = 0;
  sprintf(str, p1);
  CVSLIB_FREE(p1);
  return 0;
}

int cvs_functions_tolower(char *str)
{
	char* ptr;

	if(str == NULL)
		return -1;

	for(ptr=str;*ptr;ptr++)
		*ptr=tolower(*ptr);
	return 0;
}

int cvs_functions_one_space(char *str)
{
  char *p1, *p2;
  unsigned int i, len;
  int just_one;

  if(str == NULL)
    return -1;
  len = (int)strlen(str);
  p1  = (char*)malloc(len+1);
  if(p1 == NULL)
    return -1;
  just_one = 0;
  p2 = p1;
  for(i=0; i<len; i++)
  {
    if(isspace(str[i]))
    {
      if(just_one)
        continue;

      just_one = 1;
      *p2++ = ' ';
    }
    else
    {
      just_one = 0;
      *p2++ = str[i];
    }
  }
  *p2 = 0;
  strcpy(str, p1);
  CVSLIB_FREE(p1);

  return 0;
}

int cvs_functions_remove_char(char *str, char c)
{
  char *p1, *p2, *p3;

  if(str == NULL)
    return -1;
  p1  = (char*)malloc(strlen(str)+1);
  if(p1 == NULL)
    return -1;
	p3 = p1;
	for(p2=str;*p2;p2++)
		if(*p2 != c)
			*p3++ = *p2;
	*p3=0;
  strcpy(str, p1);
  CVSLIB_FREE(p1);

  return 0;
}

char* cvs_functions_line_get(FILE *stream)
{
	char buf[CVSLIBV_LINE_LEN];
	char *aux;

	fgets(buf, CVSLIBV_LINE_LEN, stream);
	if((aux = strchr(buf, '\n')) != NULL)
	{
		*aux = '\0';
		if(*(aux-1) == '\r')
			*(aux-1) = '\0';
	}


	return (strlen(buf) > 0 ? _strdup(buf) : NULL);
}

int cvs_functions_create_temp_file(char* temp_name)
{
	DWORD result;
	DWORD buf_size = _CVSLIB_FILE_NAME_SIZE;
	TCHAR path_buffer[_CVSLIB_FILE_NAME_SIZE];
	
	result = GetTempPath(buf_size, path_buffer);
	if(result > buf_size || (result == 0))
		return cvs_config_error_msg_set("ERROR: Impossivel criar arquivo de log temporario.", 0, CVSLIB_ERROR_IO);
	result = GetTempFileName(path_buffer,
		"CVS",
		0,
		temp_name);
	if(result == 0)
		return cvs_config_error_msg_set("ERROR: Impossivel criar arquivo de log temporario.", 0, CVSLIB_ERROR_IO);
	return 0;
}