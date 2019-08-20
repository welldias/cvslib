#include "CvsLib.h"
#include <commdlg.h>
#include <shlobj.h>

#ifdef WIN32
#define FILTER_SEPARATOR	"|"
#define FILTER_DLL			  "Arquivos DLL (*.dll)" FILTER_SEPARATOR "*.dll"
#define FILTER_EXECUTABLE "Arquivos EXE (*.exe)" FILTER_SEPARATOR "*.exe"
#define FILTER_ALL			  "Todos arquivos (*.*)" FILTER_SEPARATOR "*.*"
#define FILTER_END			  FILTER_SEPARATOR FILTER_SEPARATOR
#endif

BOOL cvs_prompt_directory_get(char* prompt, char* dir)
{
  LPITEMIDLIST pidl;
  BROWSEINFO bi = { 0 };
  char path_file[MAX_PATH];
  IMalloc * imalloc;

  bi.lpszTitle = prompt;
  bi.pszDisplayName = dir;
  
  pidl = SHBrowseForFolder(&bi);
  if(pidl != 0)
  {
    if(SHGetPathFromIDList(pidl, path_file))
    {
      if(path_file[strlen(path_file)-1] != PATH_DELIMITER)
        strcat(path_file, "\\");
    	strcpy(path_file, dir);
    }

    imalloc = 0;
    if(SUCCEEDED(SHGetMalloc(&imalloc )))
    {
      //imalloc->Free(pidl);
      //imalloc->Release();
    }
    return TRUE;
  }

	return FALSE;
}

BOOL cvs_prompt_multi_files_get(const char* prompt, CvsMultiFiles* mf, BOOL single,  FilesSelectionType selection_type)
{
	BOOL result = FALSE;
	List *l;
	CvsMultiFilesEntry* mfe;
	char sel_dir[MAX_PATH] = { 0 };
	char sel_file[MAX_PATH] = { 0 };
  OPENFILENAME open_file_dlg;

  memset(&open_file_dlg, 0, sizeof(open_file_dlg));
  open_file_dlg.lpstrFilter = FILTER_ALL FILTER_END;
  open_file_dlg.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if(cvs_multi_files_num_files_count(mf) > 0 )
	{
		for (l=mf->dirs; l; l=list_next(l))
		{
			mfe = (CvsMultiFilesEntry*)list_data(l);
      strcpy(sel_dir, mfe->dir);
		}
		
		if(cvs_multi_files_num_files_count(mf) == 1)
		{
			CvsFileEntry* fe;
			fe = cvs_multi_files_entry_get(mfe, 0);
      strcpy(sel_file, fe->file);
		}
	}

	cvs_multi_files_reset(mf);

	switch(selection_type)
	{
	default:
	case SELECT_ANY:
		open_file_dlg.lpstrFilter = FILTER_ALL FILTER_END;
		break;
	case SELECT_EXECUTABLE:
		open_file_dlg.lpstrFilter = FILTER_EXECUTABLE FILTER_SEPARATOR FILTER_ALL FILTER_END;
		break;
	case SELECT_DLL:
		open_file_dlg.lpstrFilter = FILTER_DLL FILTER_SEPARATOR FILTER_ALL FILTER_END;
		break;
	}

  if(strlen(sel_dir) == 0)
		GetCurrentDirectory(MAX_PATH-1, sel_dir);

	open_file_dlg.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST;
	if(!single)
		open_file_dlg.Flags |= OFN_ALLOWMULTISELECT;

	open_file_dlg.lStructSize = sizeof(open_file_dlg);
	open_file_dlg.lpstrTitle = prompt;
	open_file_dlg.lpstrInitialDir = sel_dir;
	open_file_dlg.hwndOwner = NULL; /*hwnd; */
	open_file_dlg.lpstrFile = sel_file;
	open_file_dlg.nMaxFile = sizeof(sel_file);
	open_file_dlg.nFilterIndex = 1;
	open_file_dlg.lpstrFileTitle = NULL;
	open_file_dlg.nMaxFileTitle = 0;

  if(GetOpenFileName(&open_file_dlg)==TRUE && strlen(sel_file) && strlen(sel_dir)) 
  {
		if( open_file_dlg.Flags & OFN_ALLOWMULTISELECT )
		{
			cvs_multi_files_new_dir(mf, sel_dir);
			cvs_multi_files_new_file(mf, sel_file, 0, NULL);
		}
		else
		{
      cvs_multi_files_new_dir(mf, sel_dir);
			cvs_multi_files_new_file(mf, sel_file, 0, NULL);			
		}
  	
  	return TRUE;
  }

  return FALSE;
}

BOOL cvs_prompt_saveas_file_get (const char* prompt, const FilesSelectionType selection_type, char* fullpath)
{
	return FALSE;
}
