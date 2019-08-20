#ifndef _CVSLIB_CVS_FUNCTIONS_H_
#define _CVSLIB_CVS_FUNCTIONS_H_

#define CVSLIBV_LINE_LEN 1024

int   cvs_functions_trim(char *str);
int   cvs_functions_tolower(char *str);
int   cvs_functions_one_space(char *str);
int   cvs_functions_remove_char(char *str, char c);
char* cvs_functions_line_get(FILE *stream);
int   cvs_functions_create_temp_file(char* file_name);

#endif /* _CVSLIB_CVS_FUNCTIONS_H_ */