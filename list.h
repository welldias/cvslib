#ifndef _CVSLIB_LIST_H_
#define _CVSLIB_LIST_H_

struct _List
{
	void  *data;
	List *next;
	List *prev;
	void  *accounting;
};

List    *list_insert                 (List *list, const void *data, unsigned int pos);
List    *list_append                 (List *list, const void *data);
List    *list_prepend                (List *list, const void *data);
List    *list_append_relative        (List *list, const void *data, const void *relative);
List    *list_append_relative_list   (List *list, const void *data, List *relative);
List    *list_prepend_relative       (List *list, const void *data, const void *relative);
List    *list_prepend_relative_list  (List *list, const void *data, List *relative);
List    *list_remove                 (List *list, const void *data);
List    *list_remove_list            (List *list, List *remove_list);
List    *list_promote_list           (List *list, List *move_list);
void    *list_find                   (List *list, const void *data);
List    *list_find_list              (List *list, const void *data);
List    *list_find_data              (List *list, const void *data, int(*func)(const void*, const void*));
List    *list_free                   (List *list);
List    *list_free_foreach           (List *list, void(*func)(void*));
List    *list_last                   (List *list);
List    *list_next                   (List *list);
List    *list_prev                   (List *list);
void    *list_data                   (List *list);
int     list_count                   (List *list);
void    *list_nth                    (List *list, int n);
List    *list_nth_list               (List *list, int n);
List    *list_reverse                (List *list);
List    *list_sort                   (List *list, int size, int(*func)(void*,void*));
int     list_alloc_error             (void);

#endif /* _CVSLIB_LIST_H_ */
