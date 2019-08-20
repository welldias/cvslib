#ifndef _CVS_PROMPT_FILES_H_
#define _CVS_PROMPT_FILES_H_

enum _FilesSelectionType
{
	SELECT_ANY,        /* selecionar qualquer arquivo */
	SELECT_EXECUTABLE, /* executaveis podem ser selecionados */
	SELECT_DLL         /* DLLs podem ser selecionadas */
};

BOOL cvs_prompt_directory_get   (char* prompt, char* dir);
BOOL cvs_prompt_multi_files_get (const char* prompt, CvsMultiFiles* mf, BOOL single,  const FilesSelectionType selection_type);
BOOL cvs_prompt_saveas_file_get (const char* prompt, const FilesSelectionType selection_type, char* fullpath);


#endif /* _CVS_PROMPT_FILES_H_ */
