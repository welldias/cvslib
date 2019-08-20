#include "CvsLib.h"

#define CVSLIB_WF_TAG "Working file: "
#define CVSLIB_HEAD_TAG "head: "
#define CVSLIB_SN_TAG "symbolic names:"
#define CVSLIB_KEYS_TAG "keyword substitution:"
#define CVSLIB_REV_TAG "revision "
#define CVSLIB_DATE_TAG "date: "
#define CVSLIB_AUTOR_TAG "author: "
#define CVSLIB_LINE_TAG "----------------------------"
#define CVSLIB_END_LINE_TAG "============================"
#define CVSLIB_BRANCH_TAG "branches: "


static void   _cvs_log_file_delete_foreach(void* log_file);
static int*   _cvs_log_file_parse_revision(char* revision, int* size);
static int    _cvs_log_file_compare(void* a, void* b);
static int    _cvs_list_log_file_symbolic_names_load(CvsListLogFile* list_log_file, FILE* stream);
static void   _cvs_log_file_symbolic_name_delete_foreach(void* symbolic_name);

CvsLogFile* cvs_log_file_create(char *revision, char *date, char *user, char* description, char* branches)
{
  CvsLogFile* log_file;

  log_file = (CvsLogFile*)malloc(sizeof(CvsLogFile));
  CVSLIB_IFNULL2(log_file, NULL);
  memset(log_file, 0, sizeof(CvsLogFile));
  cvs_log_file_init(log_file, revision, date, user, description, branches);
  return log_file;
}

void cvs_log_file_init(CvsLogFile* log_file, char *revision, char *date, char *user, char* description, char* branches)
{
  CVSLIB_IFNULL1(log_file);
  if(log_file->revision && revision)
		CVSLIB_FREE(log_file->revision);
  log_file->revision = revision ? _strdup(revision) : log_file->revision;
  if(log_file->date && date)
		CVSLIB_FREE(log_file->date);
  log_file->date = date ? _strdup(date) : log_file->date;
  if(log_file->user && user)
		CVSLIB_FREE(log_file->user);
  log_file->user = user ? _strdup(user) : log_file->user;
  if(log_file->description && description)
		CVSLIB_FREE(log_file->description);
  if(log_file->branches && branches)
		CVSLIB_FREE(log_file->branches);
  log_file->description = description ? _strdup(description) : log_file->description;
  log_file->type = LOG_NODE_REVISION;
  log_file->depth = 1;
}

int cvs_log_file_compare(CvsLogFile* log_file_a, CvsLogFile* log_file_b)
{
	int* a;
	int* b;
	int size1, size2;
	int i, j;
	int result;

	CVSLIB_IFNULL2(log_file_a, 1);
	CVSLIB_IFNULL2(log_file_b, -1);
	
	a = _cvs_log_file_parse_revision(log_file_a->revision, &size1);
	if(a == NULL)
	{
		return 1;
	}
	b = _cvs_log_file_parse_revision(log_file_b->revision, &size2);
	if(b == NULL)
	{
		free(a);
		return -1;
	}

	i = j = 0;
	while(i != size1 && j != size2)
	{
		if(a[i] != b[j])
		{
			result = a[i] < b[j] ? -1 : 1;
			goto END;
		}
		++i;
		++j;
	}
	if(i == size1 && j != size2)
		result = -1;
	else if(i != size1 && j == size2)
		result = 1;
	else
		result = 0;
	
END:
	CVSLIB_FREE(a);
	CVSLIB_FREE(b);
	return result;
}

BOOL cvs_log_file_is_child_of(char* rev_a, char* rev_b)
{
	int* a;
	int* b;
	int size1, size2;
	BOOL result;
	
	CVSLIB_IFNULL2(rev_a, FALSE);
	CVSLIB_IFNULL2(rev_b, FALSE);
	
	a = _cvs_log_file_parse_revision(rev_a, &size1);
	if(a == NULL)
	{
		return FALSE;
	}
	b = _cvs_log_file_parse_revision(rev_b, &size2);
	if(b == NULL)
	{
		free(a);
		return FALSE;
	}

	if(size2 + 2 != size1)
	{
		result = FALSE;
		goto END;
	}
	result = (memcmp(a, b, size2 * sizeof(int)) == 0);
	
END:
	CVSLIB_FREE(a);
	CVSLIB_FREE(b);
	return result;
}

BOOL cvs_log_file_is_same_branch(char* rev_a, char* rev_b)
{
	int* a;
	int* b;
	int size1, size2;
	BOOL result;
	
	CVSLIB_IFNULL2(rev_a, FALSE);
	CVSLIB_IFNULL2(rev_b, FALSE);
	
	a = _cvs_log_file_parse_revision(rev_a, &size1);
	if(a == NULL)
	{
		return FALSE;
	}

	b = _cvs_log_file_parse_revision(rev_b, &size2);
	if(b == NULL)
	{
		free(a);
		return FALSE;
	}
	if(size1 != size2)
	{
		result = FALSE;
		goto END;
	}
	
	result = (memcmp(a, b, (size1-1) * sizeof(int)) == 0);
	
END:
	CVSLIB_FREE(a);
	CVSLIB_FREE(b);
	return result;
}

BOOL cvs_log_file_is_part_of(char* rev_a, char* rev_b)
{
	int* a;
	int* b;
	int size1, size2;
	BOOL result;
	
	CVSLIB_IFNULL2(rev_a, FALSE);
	CVSLIB_IFNULL2(rev_b, FALSE);
	
	a = _cvs_log_file_parse_revision(rev_a, &size1);
	if(a == NULL)
	{
		return FALSE;
	}
	b = _cvs_log_file_parse_revision(rev_b, &size2);
	if(b == NULL)
	{
		free(a);
		return FALSE;
	}

	if((size2 & 1) != 0)
	{
		// caso especial para "1.1.1"
		if((size2 + 1) != size1)
		{
			result = FALSE;
			goto END;
		}
		result = memcmp(a, b, size2 * sizeof(int)) == 0;
		goto END;
	}

	// se 1.1.1.1.2.4 eh parte de 1.1.1.1.0.2
	if(size2 != size1 || size1 < 2)
	{
		result =  FALSE;
		goto END;
	}

	if(memcmp(a, b, (size1 - 2) * sizeof(int)) != 0)
	{
		result =  FALSE;
		goto END;
	}

	result = b[size1 - 2] == 0 && a[size1 - 2] == b[size1 - 1];
	
END:	
	CVSLIB_FREE(a);
	CVSLIB_FREE(b);
	return result;
}

BOOL cvs_log_file_is_sub_branch_of(char* rev_a, char* rev_b)
{
	int* a;
	int* b;
	int size1, size2;
	BOOL result;
	
	CVSLIB_IFNULL2(rev_a, FALSE);
	CVSLIB_IFNULL2(rev_b, FALSE);
	
	a = _cvs_log_file_parse_revision(rev_a, &size1);
	if(a == NULL)
	{
		return FALSE;
	}
	b = _cvs_log_file_parse_revision(rev_b, &size2);
	if(b == NULL)
	{
		free(a);
		return FALSE;
	}

	if((size1 & 1) != 0)
	{
		// caso especial para "1.1.1"
		if((size2 + 1) != size1)
		{
			result = FALSE;
			goto END;
		}
		result = memcmp(a, b, size2 * sizeof(int)) == 0;
		goto END;		
	}

	// se 1.4.0.2 for um sub branch de 1.4
	if((size2 + 2) != size1 || a[size2] != 0)
	{
		result = FALSE;
		goto END;
	}

	result = memcmp(a, b, size2 * sizeof(int)) == 0;
	
END:	
	CVSLIB_FREE(a);
	CVSLIB_FREE(b);
	return result;	
}

void cvs_log_file_delete(CvsLogFile* log_file)
{
  CVSLIB_IFNULL1(log_file);
	CVSLIB_FREE(log_file->revision);
	CVSLIB_FREE(log_file->date);
	CVSLIB_FREE(log_file->user);
	CVSLIB_FREE(log_file->description);
	CVSLIB_FREE(log_file->branches);
	CVSLIB_FREE(log_file);
}

static void _cvs_log_file_delete_foreach(void* log_file)
{
	CVSLIB_IFNULL1(log_file);
	cvs_log_file_delete((CvsLogFile*)log_file);
}

static int* _cvs_log_file_parse_revision(char* revision, int* size)
{
	char* ptr;
	int* result;
	int* aux;
	
	CVSLIB_IFNULL2(revision, NULL);
	
	for(*size = 0, ptr = revision; ptr != NULL && *ptr != '\0'; (*size)++, ptr = strchr(ptr, '.'))
		ptr++;
	if(*size == 0)
		return NULL;
	result = (int*)malloc((*size)*sizeof(int));
	if(result == NULL)
	{
		cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
		return NULL;
	}
	memset(result, 0, (*size)*sizeof(int));
	for(aux = result, ptr = revision; ptr != NULL && *ptr != '\0'; ++aux, ptr = strchr(ptr, '.'))
	{
		sscanf(ptr, (ptr - revision == 0 ? "%d." :".%d") , aux);
		++ptr;
	}

	return result;
}

static int _cvs_log_file_compare(void* a, void* b)
{
	return cvs_log_file_compare((CvsLogFile*)a, (CvsLogFile*)b);
}

CvsListSymbolicName* cvs_log_file_symbolic_name_create(char* name, char *revision)
{
  CvsListSymbolicName* symbolic_name;

  symbolic_name = (CvsListSymbolicName*)malloc(sizeof(CvsListSymbolicName));
  CVSLIB_IFNULL2(symbolic_name, NULL);
  memset(symbolic_name, 0, sizeof(CvsListSymbolicName));
  if(symbolic_name->name && name)
		CVSLIB_FREE(symbolic_name->name);
  symbolic_name->name = name ? _strdup(name) : symbolic_name->name;
  if(symbolic_name->revision && revision)
		CVSLIB_FREE(symbolic_name->revision);
  symbolic_name->revision = revision ? _strdup(revision) : symbolic_name->revision;
  return symbolic_name;
}

void cvs_log_file_symbolic_name_delete(CvsListSymbolicName* symbolic_name)
{
	CVSLIB_IFNULL1(symbolic_name);
	CVSLIB_FREE(symbolic_name->name);
	CVSLIB_FREE(symbolic_name->revision);
	CVSLIB_FREE(symbolic_name);	
}

static void _cvs_log_file_symbolic_name_delete_foreach(void* symbolic_name)
{
	CVSLIB_IFNULL1(symbolic_name);
	cvs_log_file_symbolic_name_delete((CvsListSymbolicName*)symbolic_name);
}


CvsListLogFile* cvs_list_log_file_create()
{
  CvsListLogFile* list_log_file;

  list_log_file = (CvsListLogFile*)malloc(sizeof(CvsListLogFile));
  CVSLIB_IFNULL2(list_log_file, NULL);
  memset(list_log_file, 0, sizeof(CvsListLogFile));
  cvs_list_log_file_init(list_log_file);
  return list_log_file;
}

void cvs_list_log_file_init(CvsListLogFile* list_log_file)
{
	CVSLIB_IFNULL1(list_log_file);
}

void cvs_list_log_file_delete(CvsListLogFile* list_log_file)
{
	CVSLIB_IFNULL1(list_log_file);
	CVSLIB_FREE(list_log_file->file_name);
	CVSLIB_FREE(list_log_file->head);
	list_log_file->symbolic_names = list_free_foreach(list_log_file->symbolic_names, _cvs_log_file_symbolic_name_delete_foreach);
	list_log_file->logs = list_free_foreach(list_log_file->logs, _cvs_log_file_delete_foreach);
	CVSLIB_FREE(list_log_file);
}

int cvs_list_log_file_rehierarchy(CvsListLogFile* list_log_file)
{
	int result;
	List* l;
	List* cur_node = NULL;
	List* rehierarchy = NULL;
	CvsLogFile* cur_rev;
	CvsLogFile* rev;
	CvsListSymbolicName* symbolic_name;
	
	CVSLIB_IFNULL2(list_log_file, cvs_config_error_msg_set("ERROR: Parametro 'list_log_file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));

	list_log_file->logs = list_sort(list_log_file->logs, list_count(list_log_file->logs), _cvs_log_file_compare);
	for(cur_node=list_log_file->logs; cur_node; cur_node=list_next(cur_node))
	{
		cur_rev = (CvsLogFile*)list_data(cur_node);
		if(cur_rev == NULL || cur_rev->reordered == TRUE)
			continue;
		rehierarchy = list_append(rehierarchy, cur_rev);
		//cur_rev->reordered = TRUE;

		for(l=list_log_file->logs; l; l=list_next(l))
		{
			rev = (CvsLogFile*)list_data(l);
			if(rev == NULL || rev->reordered == TRUE)
				continue;
			if(cvs_log_file_is_child_of(rev->revision, cur_rev->revision))
			{
				rev->depth = cur_rev->depth + 1;
				rev->reordered = TRUE;
				list_append_relative(rehierarchy, rev, cur_rev);
			}
		}
	}
	
	for(l=list_log_file->symbolic_names; l; l=list_next(l))
	{
		symbolic_name = (CvsListSymbolicName*)list_data(l);
		if(symbolic_name == NULL || symbolic_name->inserted == TRUE)
			continue;
		result = cvs_list_log_file_rehierarchy_branch(list_log_file, symbolic_name);
		if(result == 0)
			symbolic_name->inserted = TRUE;
		else if(result != CVSLIB_ERROR_NOT_FOUND)
			return result;
	}

	for(l=list_log_file->symbolic_names; l; l=list_next(l))
	{
		symbolic_name = (CvsListSymbolicName*)list_data(l);
		if(symbolic_name == NULL || symbolic_name->inserted == TRUE)
			continue;
		result = cvs_list_log_file_rehierarchy_tag(list_log_file, symbolic_name);
		if(result == 0)
			symbolic_name->inserted = TRUE;
		else if(result != CVSLIB_ERROR_NOT_FOUND)
			return result;
	}
	
	return 0;
}

#if 0
int cvs_list_log_file_rehierarchy_branch(CvsListLogFile* list_log_file)
{
	List* l;
	CvsLogFile* log_file;
	
	CVSLIB_IFNULL2(list_log_file, cvs_config_error_msg_set("ERROR: Parametro 'list_log_file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
	
	for(l=list_log_file->logs; l; l=list_next(l))
	{
		log_file = (CvsLogFile*)list_data(l);
		if(log_file == NULL)
			continue;
		if(log_file->branches && strlen(log_file->branches) >0)
		{
			CvsLogFile* log_file_new = cvs_log_file_create(log_file->branches, NULL, NULL, log_file->branches, NULL);
			if(log_file == NULL)
				return cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
			log_file_new->type = LOG_NODE_BRANCH;
			log_file_new->depth = log_file->depth + 1;
			list_append_relative(list_log_file->logs, log_file_new, log_file);
			if (list_alloc_error())
			{
				cvs_log_file_delete(log_file_new);
				return cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
			}
		}
	}
	
	return 0;
}
#endif

int cvs_list_log_file_rehierarchy_branch(CvsListLogFile* list_log_file, CvsListSymbolicName* symbolic_name)
{
	List* l;
	CvsLogFile* log_file;

	CVSLIB_IFNULL2(list_log_file, cvs_config_error_msg_set("ERROR: Parametro 'list_log_file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
	CVSLIB_IFNULL2(list_log_file, cvs_config_error_msg_set("ERROR: Parametro 'symbolic_name' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
	
	for(l=list_log_file->logs; l; l=list_next(l))
	{
		log_file = (CvsLogFile*)list_data(l);
		if(log_file == NULL || log_file->type != LOG_NODE_REVISION)
			continue;
		//if(cvs_log_file_is_part_of(log_file->revision, symbolic_name->revision))
		if(cvs_log_file_is_sub_branch_of(symbolic_name->revision, log_file->revision))
		{
			CvsLogFile* log_file_new = cvs_log_file_create(symbolic_name->revision, NULL, NULL, symbolic_name->name, NULL);
			if(log_file == NULL)
				return cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
			log_file_new->type = LOG_NODE_BRANCH;
			log_file_new->depth = log_file->depth + 1;
			list_append_relative(list_log_file->logs, log_file_new, log_file);
			if (list_alloc_error())
			{
				cvs_log_file_delete(log_file_new);
				return cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
			}
			else
			{
				List* l_tmp;
				CvsLogFile* log_file_tmp;
				for(l_tmp=list_next(l); l_tmp; l_tmp=list_next(l_tmp))
				{
					log_file_tmp = (CvsLogFile*)list_data(l_tmp);
					if(log_file_tmp == NULL)
						continue;
					else if(log_file_tmp->depth == 1)
						break;
					if(cvs_log_file_is_part_of(log_file_tmp->revision, log_file_new->revision))
						log_file_tmp->depth = log_file_new->depth + 1;
				}
			}
			return 0;
		}
	}
	
	return CVSLIB_ERROR_NOT_FOUND;
}

int cvs_list_log_file_rehierarchy_tag(CvsListLogFile* list_log_file, CvsListSymbolicName* symbolic_name)
{
	List* l;
	CvsLogFile* log_file;

	CVSLIB_IFNULL2(list_log_file, cvs_config_error_msg_set("ERROR: Parametro 'list_log_file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
	CVSLIB_IFNULL2(list_log_file, cvs_config_error_msg_set("ERROR: Parametro 'symbolic_name' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
	
	for(l=list_log_file->logs; l; l=list_next(l))
	{
		log_file = (CvsLogFile*)list_data(l);
		if(log_file == NULL)
			continue;
		if(!strcmp(log_file->revision, symbolic_name->revision))
		{
			CvsLogFile* log_file_new = cvs_log_file_create(symbolic_name->revision, NULL, NULL, symbolic_name->name, NULL);
			if(log_file == NULL)
				return cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
			log_file_new->type = LOG_NODE_TAG;
			log_file_new->depth = log_file->depth + 1;
			list_append_relative(list_log_file->logs, log_file_new, log_file);
			if (list_alloc_error())
			{
				cvs_log_file_delete(log_file_new);
				return cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
			}
			return 0;
		}
	}
	
	return CVSLIB_ERROR_NOT_FOUND;
}

int cvs_list_log_file_load(CvsListLogFile* list_log_file, char* file_name)
{
	FILE *stream = NULL;	
	
	CVSLIB_IFNULL2(list_log_file, cvs_config_error_msg_set("ERROR: Parametro 'list_log_file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
	CVSLIB_IFNULL2(file_name, cvs_config_error_msg_set("ERROR: Parametro 'file_name' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
  stream = fopen(file_name, "r");
	if(stream == NULL)
	{
		return cvs_config_error_msg_set("ERROR: Impossivel criar lista temporaria dos arquivos bloqueados.", 0, CVSLIB_ERROR_IO);
	}

	cvs_list_log_file_load(list_log_file, stream);
		
	fclose(stream);

	if(list_log_file != NULL && list_count(list_log_file->logs) > 0)
		return list_count(list_log_file->logs);
	return cvs_config_error_get();
}

int cvs_list_log_file_load(CvsListLogFile* list_log_file, FILE* stream)
{
	char* ptr;
	char *line = NULL;
	int result, i;
	CvsLogFile* log_file = NULL;
  BOOL found_rev = FALSE;

	CVSLIB_IFNULL2(list_log_file, cvs_config_error_msg_set("ERROR: Parametro 'list_log_file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
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
    if(!found_rev)
    {
			if(CVSLIB_MATCH(line, CVSLIB_WF_TAG))
			{
				list_log_file->file_name = _strdup(line+strlen(CVSLIB_WF_TAG));
			}
			else if(CVSLIB_MATCH(line, CVSLIB_HEAD_TAG))
			{
				list_log_file->head = _strdup(line+strlen(CVSLIB_HEAD_TAG));
			}
			else if(CVSLIB_MATCH(line, CVSLIB_SN_TAG))
			{
				result = _cvs_list_log_file_symbolic_names_load(list_log_file, stream);
				if(result < 0)
				{
					cvs_list_log_file_delete(list_log_file);
					CVSLIB_FREE(line);
					return result;
				}
			}
    }
    
    if(CVSLIB_MATCH(line, CVSLIB_REV_TAG))
    {
			log_file = cvs_log_file_create(NULL, NULL, NULL, NULL, NULL);
			if(log_file == NULL)
			{
				cvs_list_log_file_delete(list_log_file);
				CVSLIB_FREE(line);
				return cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
			}
			ptr = strchr(line+strlen(CVSLIB_REV_TAG), '\t');
			if(ptr == NULL)
				ptr = strchr(line+strlen(CVSLIB_REV_TAG), ' ');
			if(ptr != NULL)
				*ptr  = 0;
			log_file->revision = _strdup(line+strlen(CVSLIB_REV_TAG));
			found_rev = TRUE;
    }
    else if(found_rev && log_file)
    {
			if(!log_file->date && CVSLIB_MATCH(line, CVSLIB_DATE_TAG))
			{
				char* fields[20];

				/* substitui as virgulas por \0 para trabalhar como se fosse um array */
				for(result = 0, ptr = (char *)line; ptr != NULL && *ptr != '\0'; ++result, ptr = strchr(ptr, ';'))
				{
					if(result == 19)
						break;
					if(result != 0)
						*ptr++ = '\0';
					fields[result] = ptr;
				}
				
				for(i=0; i<result; i++)
				{
					cvs_functions_trim(fields[i]);
					if(!log_file->date && CVSLIB_MATCH(fields[i], CVSLIB_DATE_TAG))
					{
						ptr = strchr(fields[i], '+');
						if(ptr) *ptr = 0;
						cvs_functions_trim(fields[i]);
						log_file->date = _strdup(fields[i]+strlen(CVSLIB_DATE_TAG));
					}
					else if(!log_file->user && CVSLIB_MATCH(fields[i], CVSLIB_AUTOR_TAG))
					{
						log_file->user = _strdup(fields[i]+strlen(CVSLIB_AUTOR_TAG));
					}
				}
			}
			else if(!log_file->branches && CVSLIB_MATCH(line, CVSLIB_BRANCH_TAG))
			{
				ptr = strchr(line+strlen(CVSLIB_REV_TAG), ';');
				if(ptr != NULL)
					*ptr  = 0;
				ptr = _strdup(line+strlen(CVSLIB_BRANCH_TAG));
				cvs_functions_trim(ptr);
				log_file->branches = ptr;
				continue;
			}
			else if(CVSLIB_MATCH(line, CVSLIB_LINE_TAG) || CVSLIB_MATCH(line, CVSLIB_END_LINE_TAG))
			{
				list_log_file->logs = list_insert(list_log_file->logs, (void*)log_file, 0);
				log_file = NULL;
				found_rev = FALSE;
				continue;
			}
			else /* commentarios */
			{
				if(log_file->description == NULL)
					log_file->description = _strdup(line);
				else
				{
					result = 0;
					ptr = log_file->description;
					result = (int)(strlen(ptr)+strlen(line)+4);
					log_file->description = (char*)malloc(result+1);
					if(log_file->description == NULL)
					{
						cvs_list_log_file_delete(list_log_file);
						cvs_log_file_delete(log_file);
						CVSLIB_FREE(line);
						return cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
					}
					sprintf(log_file->description, "%s<BR>%s", ptr, line);
					CVSLIB_FREE(ptr);
				}
			}
    }

    CVSLIB_FREE(line);
  }
  
  if(log_file)
		list_log_file->logs = list_insert(list_log_file->logs, (void*)log_file, 0);
	cvs_list_log_file_rehierarchy(list_log_file);
  return list_count(list_log_file->logs);
}

static int _cvs_list_log_file_symbolic_names_load(CvsListLogFile* list_log_file, FILE* stream)
{
  char *line = NULL;
	char *ptr = NULL;

	CVSLIB_IFNULL2(list_log_file, cvs_config_error_msg_set("ERROR: Parametro 'list_log_file' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
  CVSLIB_IFNULL2(stream, cvs_config_error_msg_set("ERROR: Parametro 'stream' invalido.", 0, CVSLIB_ERROR_INVALID_PARAMETER));
	
	while(!feof(stream))
  {
    line = cvs_functions_line_get(stream);
    if(line == NULL)
      return cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
      
		if(CVSLIB_MATCH(line, CVSLIB_KEYS_TAG))
		{
			break;
		}
		else
		{
			CvsListSymbolicName* symbolic_names = NULL;
			char* name = NULL;
			char* revision = NULL;
			
			cvs_functions_trim(line);
			name = line;
			ptr = strchr(line, ':');
			if(ptr)
			{
				*ptr++ = '\0';
				revision = ++ptr;
			}
			symbolic_names = cvs_log_file_symbolic_name_create(name, revision);
			if(symbolic_names == NULL)
			{
				list_log_file->symbolic_names = list_free_foreach(list_log_file->symbolic_names, _cvs_log_file_symbolic_name_delete_foreach);
				CVSLIB_FREE(line);
				return cvs_config_error_msg_set("ERROR: Erro ao alocar memoria.", 0, CVSLIB_ERROR_MEMORY);
			}
			list_log_file->symbolic_names = list_insert(list_log_file->symbolic_names, (void*)symbolic_names, 0);
		}
	}
	
	return list_count(list_log_file->symbolic_names);
}