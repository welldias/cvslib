#include "CvsLib.h"

typedef struct _List_Accounting List_Accounting;

struct _List_Accounting
{
   List *last;
   int  count;
};

static int _list_alloc_error = 0;
   
List *
list_insert(List *list, const void *data, unsigned int pos)
{
  List *l;
  unsigned int i;
  
  l = list;
  for(i=0; l; l = l->next, i++)
  {
    if(i == pos)
      return list_prepend_relative_list(list, data, l);
  }
  return list_append(list, data);
}

/**
 * @defgroup List_Data_Group Linked List Creation Functions
 *
 * Functions that add data to an List.
 */

/**
 * Appends the given data to the given linked list.
 *
 * The following example code demonstrates how to ensure that the
 * given data has been successfully appended.
 *
 * @code
 * List *list = NULL;
 * extern void *my_data;
 *
 * list = list_append(list, my_data);
 * if (list_alloc_error())
 *   {
 *     fprintf(stderr, "ERROR: Memory is low. List allocation failed.\n");
 *     exit(-1);
 *   }
 * @endcode
 *
 * @param   list The given list.  If @c NULL is given, then a new list
 *               is created.
 * @param   data The data to append.
 * @return  A new list pointer that should be used in place of the one
 *          given to this function if successful.  Otherwise, the old
 *          pointer is returned.
 * @ingroup List_Data_Group
 */
List *
list_append(List *list, const void *data)
{
   List *l, *new_l;

   _list_alloc_error = 0;
   new_l = (List *)malloc(sizeof(List));
   if (!new_l)
     {
	_list_alloc_error = 1;
	return list;
     }
   new_l->next = NULL;
   new_l->data = (void *)data;
   if (!list)
     {
	new_l->prev = NULL;
	new_l->accounting = malloc(sizeof(List_Accounting));
	if (!new_l->accounting)
	  {
	     _list_alloc_error = 1;
	     free(new_l);
	     return list;
	  }
	((List_Accounting *)(new_l->accounting))->last = new_l;
	((List_Accounting *)(new_l->accounting))->count = 1;
	return new_l;
     }
   l = ((List_Accounting *)(list->accounting))->last;
   l->next = new_l;
   new_l->prev = l;
   new_l->accounting = list->accounting;
   ((List_Accounting *)(list->accounting))->last = new_l;
   ((List_Accounting *)(list->accounting))->count++;
   return list;
}

/**
 * Prepends the given data to the given linked list.
 *
 * The following example code demonstrates how to ensure that the
 * given data has been successfully prepended.
 *
 * Example:
 * @code
 * List *list = NULL;
 * extern void *my_data;
 *
 * list = list_prepend(list, my_data);
 * if (list_alloc_error())
 *   {
 *     fprintf(stderr, "ERROR: Memory is low. List allocation failed.\n");
 *     exit(-1);
 *   }
 * @endcode
 *
 * @param   list The given list.
 * @param   data The given data.
 * @return  A new list pointer that should be used in place of the one
 *          given to this function, if successful.  Otherwise, the old
 *          pointer is returned.
 * @ingroup List_Data_Group
 */
List *
list_prepend(List *list, const void *data)
{
  List *new_l;
  
  _list_alloc_error = 0;
  new_l = (List *)malloc(sizeof(List));
  if (!new_l)
  {
    _list_alloc_error = 1;
    return list;
  }
  new_l->prev = NULL;
  new_l->data = (void *)data;
  if (!list)
  {
    new_l->next = NULL;
    new_l->accounting = malloc(sizeof(List_Accounting));
    if (!new_l->accounting)
    {
      _list_alloc_error = 1;
      free(new_l);
      return list;
    }
    ((List_Accounting *)(new_l->accounting))->last = new_l;
    ((List_Accounting *)(new_l->accounting))->count = 1;
    return new_l;
  }
  new_l->next = list;
  list->prev = new_l;
  new_l->accounting = list->accounting;
  ((List_Accounting *)(list->accounting))->count++;
  return new_l;
}

/**
 * Inserts the given data into the given linked list after the specified data.
 *
 * If @p relative is not in the list, @p data is appended to the end of the
 * list.  If there are multiple instances of @p relative in the list,
 * @p data is inserted after the first instance.
 *
 * The following example code demonstrates how to ensure that the
 * given data has been successfully inserted.
 *
 * @code
 * List *list = NULL;
 * extern void *my_data;
 * extern void *relative_member;
 *
 * list = list_append(list, relative_member);
 * if (list_alloc_error())
 *   {
 *     fprintf(stderr, "ERROR: Memory is low. List allocation failed.\n");
 *     exit(-1);
 *   }
 * list = list_append_relative(list, my_data, relative_member);
 * if (list_alloc_error())
 *   {
 *     fprintf(stderr, "ERROR: Memory is low. List allocation failed.\n");
 *     exit(-1);
 *   }
 * @endcode
 *
 * @param   list The given linked list.
 * @param   data The given data.
 * @param   relative The data to insert after.
 * @return  A new list pointer that should be used in place of the one
 *          given to this function if successful.  Otherwise, the old pointer
 *          is returned.
 * @ingroup List_Data_Group
 */
List *
list_append_relative(List *list, const void *data, const void *relative)
{
  List *l;
  
  for (l = list; l; l = l->next)
  {
    if (l->data == relative)
      return list_append_relative_list(list, data, l);
  }
  return list_append(list, data);
}

List *
list_append_relative_list(List *list, const void *data, List *relative)
{
  List *new_l;
  
  if ((!list) || (!relative)) return list_append(list, data);
  _list_alloc_error = 0;
  new_l = (List *)malloc(sizeof(List));
  if (!new_l)
  {
    _list_alloc_error = 1;
    return list;
  }
  new_l->data = (void *)data;
  if (relative->next)
  {
    new_l->next = relative->next;
    relative->next->prev = new_l;
  }
  else
    new_l->next = NULL;
  
  relative->next = new_l;
  new_l->prev = relative;
  new_l->accounting = list->accounting;
  ((List_Accounting *)(list->accounting))->count++;
  if (!new_l->next)
    ((List_Accounting *)(new_l->accounting))->last = new_l;
  return list;
}

/**
 * Prepend a data pointer to a linked list before the memeber specified
 * @param list The list handle to prepend @p data too
 * @param data The data pointer to prepend to list @p list before @p relative
 * @param relative The data pointer before which to insert @p data
 * @return A new list handle to replace the old one

 * Inserts the given data into the given linked list before the member
 * specified.
 *
 * If @p relative is not in the list, @p data is prepended to the
 * start of the list.  If there are multiple instances of @p relative
 * in the list, @p data is inserted before the first instance.
 *
 * The following code example demonstrates how to ensure that the
 * given data has been successfully inserted.
 *
 * @code
 * List *list = NULL;
 * extern void *my_data;
 * extern void *relative_member;
 *
 * list = list_append(list, relative_member);
 * if (list_alloc_error())
 *   {
 *     fprintf(stderr, "ERROR: Memory is low. List allocation failed.\n");
 *     exit(-1);
 *   }
 * list = list_prepend_relative(list, my_data, relative_member);
 * if (list_alloc_error())
 *   {
 *     fprintf(stderr, "ERROR: Memory is low. List allocation failed.\n");
 *     exit(-1);
 *   }
 * @endcode
 *
 * @param   list The given linked list.
 * @param   data The given data.
 * @param   relative The data to insert before.
 * @return  A new list pointer that should be used in place of the one
 *          given to this function if successful.  Otherwise the old pointer
 *          is returned.
 * @ingroup List_Data_Group
 */
List *
list_prepend_relative(List *list, const void *data, const void *relative)
{
  List *l;
  
  _list_alloc_error = 0;
  for (l = list; l; l = l->next)
  {
    if (l->data == relative)
      return list_prepend_relative_list(list, data, l);
  }
  return list_prepend(list, data);
}

List *
list_prepend_relative_list(List *list, const void *data, List *relative)
{
  List *new_l;
  
  if ((!list) || (!relative)) return list_prepend(list, data);
  _list_alloc_error = 0;
  new_l = (List *)malloc(sizeof(List));
  if (!new_l)
  {
    _list_alloc_error = 1;
    return list;
  }
  new_l->data = (void *)data;
  new_l->prev = relative->prev;
  new_l->next = relative;
  if (relative->prev) relative->prev->next = new_l;
  relative->prev = new_l;
  new_l->accounting = list->accounting;
  ((List_Accounting *)(list->accounting))->count++;
  if (new_l->prev)
    return list;
  return new_l;
}

/**
 * @defgroup List_Remove_Group Linked List Remove Functions
 *
 * Functions that remove data from linked lists.
 */

/**
 * Removes the first instance of the specified data from the given list.
 *
 * If the specified data is not in the given list, nothing is done.
 *
 * @param   list The given list.
 * @param   data The specified data.
 * @return  A new list pointer that should be used in place of the one
 *          passed to this functions.
 * @ingroup List_Remove_Group
 */
List *
list_remove(List *list, const void *data)
{
  List *l;
  
  for (l = list; l; l = l->next)
  {
    if (l->data == data)
      return list_remove_list(list, l);
  }
  return list;
}

/**
 * Removes the specified data
 *
 * Remove a specified member from a list
 * @param list The list handle to remove @p remove_list from
 * @param remove_list The list node which is to be removed
 * @return A new list handle to replace the old one
 *
 * Calling this function takes the list note @p remove_list and removes it
 * from the list @p list, freeing the list node structure @p remove_list.
 *
 * Example:
 * @code
 * extern List *list;
 * List *l;
 * extern void *my_data;
 *
 * for (l = list; l; l= l->next)
 *   {
 *     if (l->data == my_data)
 *       {
 *         list = list_remove_list(list, l);
 *         break;
 *       }
 *   }
 * @endcode
 * @ingroup List_Remove_Group
 */
List *
list_remove_list(List *list, List *remove_list)
{
   List *return_l;

   if (!list) return NULL;
   if (!remove_list) return list;
   if (remove_list->next) remove_list->next->prev = remove_list->prev;
   if (remove_list->prev)
     {
	remove_list->prev->next = remove_list->next;
	return_l = list;
     }
   else
     return_l = remove_list->next;
   if (remove_list == ((List_Accounting *)(list->accounting))->last)
     ((List_Accounting *)(list->accounting))->last = remove_list->prev;
   ((List_Accounting *)(list->accounting))->count--;
   if (((List_Accounting *)(list->accounting))->count == 0)
     free(list->accounting);
   free(remove_list);
   return return_l;
}

/**
 * Moves the specified data to the head of the list
 *
 * Move a specified member to the head of the list
 * @param list The list handle to move @p inside
 * @param move_list The list node which is to be moved
 * @return A new list handle to replace the old one
 *
 * Calling this function takes the list node @p move_list and moves it
 * to the front of the @p list.
 *
 * Example:
 * @code
 * extern List *list;
 * List *l;
 * extern void *my_data;
 *
 * for (l = list; l; l= l->next)
 *   {
 *     if (l->data == my_data)
 *       {
 *         list = list_promote_list(list, l);
 *         break;
 *       }
 *   }
 * @endcode
 * @ingroup List_Promote_Group
 */
List *
list_promote_list(List *list, List *move_list)
{
   List *return_l;

   if (!list) return NULL;
   if (!move_list) return list;
   if (move_list == list) return list;
   if (move_list->next) move_list->next->prev = move_list->prev;
   if (move_list->prev)
     {
	move_list->prev->next = move_list->next;
	return_l = list;
     }
   else
     return_l = move_list->next;
   if (move_list == ((List_Accounting *)(list->accounting))->last)
     ((List_Accounting *)(list->accounting))->last = move_list->prev;
   move_list->prev = return_l->prev;
   if (return_l->prev)
     return_l->prev->next = move_list;
   return_l->prev = move_list;
   move_list->next = return_l;
   return move_list;
}



/**
 * @defgroup List_Find_Group Linked List Find Functions
 *
 * Functions that find specified data in a linked list.
 */

/**
 * Find a member of a list and return the member
 * @param list The list handle to search for @p data
 * @param data The data pointer to find in the list @p list
 * @return The found member data pointer
 *
 * A call to this function will search the list @p list from beginning to end
 * for the first member whose data pointer is @p data. If it is found, @p data
 * will be returned, otherwise NULL will be returned.
 *
 * Example:
 * @code
 * extern List *list;
 * extern void *my_data;
 *
 * if (list_find(list, my_data) == my_data)
 *   {
 *     printf("Found member %p\n", my_data);
 *   }
 * @endcode
 * @ingroup List_Find_Group
 */
void *
list_find(List *list, const void *data)
{
   List *l;

   for (l = list; l; l = l->next)
     {
	if (l->data == data) return (void *)data;
     }
   return NULL;
}

/**
 * Find a member of a list and return the list node containing that member
 * @param list The list handle to search for @p data
 * @param data The data pointer to find in the list @p list
 * @return The found members list node
 *
 * A call to this function will search the list @p list from beginning to end
 * for the first member whose data pointer is @p data. If it is found, the
 * list node containing the specified member will be returned, otherwise NULL
 * will be returned.
 *
 * Example:
 * @code
 * extern List *list;
 * extern void *my_data;
 * List *found_node;
 *
 * found_node = list_find_list(list, my_data);
 * if (found_node)
 *   {
 *     printf("Found member %p\n", found_node->data);
 *   }
 * @endcode
 * @ingroup List_Find_Group
 */
List *
list_find_list(List *list, const void *data)
{
   List *l;

   for (l = list; l; l = l->next)
     {
	if (l->data == data) return l;
     }
   return NULL;
}

List *
list_find_data(List *list, const void *data, int(*func)(const void*, const void*))
{
  List *l;
  
  for (l = list; l; l = l->next)
  {
    if(func(data, l->data))
      return l;
  }
  return NULL;
}

/**
 * Free an entire list and all the nodes, ignoring the data contained
 * @param list The list to free
 * @return A NULL pointer
 *
 * This function will free all the list nodes in list specified by @p list.
 *
 * Example:
 * @code
 * extern List *list;
 *
 * list = list_free(list);
 * @endcode
 * @ingroup List_Remove_Group
 */
List *
list_free(List *list)
{
   List *l, *free_l;

   if (!list) return NULL;
   free(list->accounting);
   for (l = list; l;)
     {
	free_l = l;
	l = l->next;
	free(free_l);
     }
   return NULL;
}

List *
list_free_foreach(List *list, void(*func)(void*))
{
  List *l, *free_l;
  
  if (!list) return NULL;
  free(list->accounting);
  for (l = list; l;)
  {
    free_l = l;
    l = l->next;
    func(free_l->data);
    free(free_l);
  }
  return NULL;
}

/**
 * @defgroup List_Traverse_Group Linked List Traverse Functions
 *
 * Functions that you can use to traverse a linked list.
 */

/**
 * Get the last list node in the list
 * @param list The list to get the last list node from
 * @return The last list node in the list @p list
 *
 * This function will return the last list node in the list (or NULL if the
 * list is empty).
 *
 * NB: This is a order-1 operation (it takes the same short time regardless of
 * the length of the list).
 *
 * Example:
 * @code
 * extern List *list;
 * List *last, *l;
 *
 * last = list_last(list);
 * printf("The list in reverse:\n");
 * for (l = last; l; l = l->prev)
 *   {
 *     printf("%p\n", l->data);
 *   }
 * @endcode
 * @ingroup List_Traverse_Group
 */
List *
list_last(List *list)
{
   if (!list) return NULL;
   return ((List_Accounting *)(list->accounting))->last;
}

/**
 * Get the next list node after the specified list node
 * @param list The list node to get the next list node from
 * @return The next list node, or NULL if no next list node exists
 *
 * This function returns the next list node after the current one. It is
 * equivalent to list->next.
 *
 * Example:
 * @code
 * extern List *list;
 * List *l;
 *
 * printf("The list:\n");
 * for (l = list; l; l = list_next(l))
 *   {
 *     printf("%p\n", l->data);
 *   }
 * @endcode
 * @ingroup List_Traverse_Group
 */
List *
list_next(List *list)
{
   if (!list) return NULL;
   return list->next;
}

/**
 * Get the previous list node before the specified list node
 * @param list The list node to get the previous list node from
 * @return The previous list node, or NULL if no previous list node exists
 *
 * This function returns the previous list node before the current one. It is
 * equivalent to list->prev.
 *
 * Example:
 * @code
 * extern List *list;
 * List *last, *l;
 *
 * last = list_last(list);
 * printf("The list in reverse:\n");
 * for (l = last; l; l = list_prev(l))
 *   {
 *     printf("%p\n", l->data);
 *   }
 * @endcode
 * @ingroup List_Traverse_Group
 */
List *
list_prev(List *list)
{
   if (!list) return NULL;
   return list->prev;
}

/**
 * @defgroup List_General_Group Linked List General Functions
 *
 * Miscellaneous functions that work on linked lists.
 */

/**
 * Get the list node data member
 * @param list The list node to get the data member of
 * @return The data member from the list node @p list
 *
 * This function returns the data member of the specified list node @p list.
 * It is equivalent to list->data.
 *
 * Example:
 * @code
 * extern List *list;
 * List *l;
 *
 * printf("The list:\n");
 * for (l = list; l; l = list_next(l))
 *   {
 *     printf("%p\n", list_data(l));
 *   }
 * @endcode
 * @ingroup List_General_Group
 */
void *
list_data(List *list)
{
   if (!list) return NULL;
   return list->data;
}

/**
 * Get the count of the number of items in a list
 * @param list The list whose count to return
 * @return The number of members in the list @p list
 *
 * This function returns how many members in the specified list: @p list. If
 * the list is empty (NULL), 0 is returned.
 *
 * NB: This is an order-1 operation and takes the same tiem regardless of the
 * length of the list.
 *
 * Example:
 * @code
 * extern List *list;
 *
 * printf("The list has %i members\n", list_count(list));
 * @endcode
 * @ingroup List_General_Group
 */
int
list_count(List *list)
{
   if (!list) return 0;
   return ((List_Accounting *)(list->accounting))->count;
}

/**
 * Get the nth member's data pointer in a list
 * @param list The list to get member number @p n from
 * @param n The number of the element (0 being the first)
 * @return The data pointer stored in the specified element
 *
 * This function returns the data pointer of element number @p n, in the list
 * @p list. The first element in the array is element number 0. If the element
 * number @p n does not exist, NULL will be returned.
 *
 * Example:
 * @code
 * extern List *list;
 * extern int number;
 * void *data;
 *
 * data = list_nth(list, number);
 * if (data)
 *   printf("Element number %i has data %p\n", number, data);
 * @endcode
 * @ingroup List_Find_Group
 */
void *
list_nth(List *list, int n)
{
   List *l;
   
   l = list_nth_list(list, n);
   return l ? l->data : NULL;
}

/**
 * Get the nth member's list node in a list
 * @param list The list to get member number @p n from
 * @param n The number of the element (0 being the first)
 * @return The list node stored in the numbered element
 *
 * This function returns the list node of element number @p n, in the list
 * @p list. The first element in the array is element number 0. If the element
 * number @p n does not exist, NULL will be returned.
 *
 * Example:
 * @code
 * extern List *list;
 * extern int number;
 * List *nth_list;
 *
 * nth_list = list_nth_list(list, number);
 * if (nth_list)
 *   printf("Element number %i has data %p\n", number, nth_list->data);
 * @endcode
 * @ingroup List_Find_Group
 */
List *
list_nth_list(List *list, int n)
{
   int i;
   List *l;

   /* check for non-existing nodes */
   if ((!list) || (n < 0) || 
       (n > ((List_Accounting *)(list->accounting))->count - 1))
     return NULL;

   /* if the node is in the 2nd half of the list, search from the end
    * else, search from the beginning.
    */
   if (n > (((List_Accounting *)(list->accounting))->count / 2))
     {
	for (i = ((List_Accounting *)(list->accounting))->count - 1,
	     l = ((List_Accounting *)(list->accounting))->last; 
	     l; 
	     l = l->prev, i--)
	  {
	     if (i == n) return l;
	  }
     }
   else
     {
	for (i = 0, l = list; l; l = l->next, i++)
	  {
	     if (i == n) return l;
	  }
     }
   return NULL;
}

/**
 * @defgroup List_Ordering_Group Linked List Ordering Functions
 *
 * Functions that change the ordering of data in a linked list.
 */

/**
 * Reverse all the elements in the list
 * @param list The list to reverse
 * @return The list after it has been reversed
 *
 * This takes a list @p list, and reverses the order of all elements in the
 * list, so the last member is now first, and so on.
 *
 * Example:
 * @code
 * extern List *list;
 *
 * list = list_reverse(list);
 * @endcode
 * @ingroup List_Ordering_Group
 */
List *
list_reverse(List *list)
{
   List *l1, *l2;

   if (!list) return NULL;
   l1 = list;
   l2 = ((List_Accounting *)(list->accounting))->last;
   while (l1 != l2)
     {
	void *data;

	data = l1->data;
	l1->data = l2->data;
	l2->data = data;
	l1 = l1->next;
	if (l1 == l2) break;
	l2 = l2->prev;
     }
   return list;
}

static List *
list_combine(List *l, List *ll, int (*func)(void *, void*))
{
   List *result = NULL;
   List *l_head = NULL, *ll_head = NULL;

   l_head = l;
   ll_head = ll;
   while (l && ll)
     {
	int cmp;

	cmp = func(l->data, ll->data);
	if (cmp < 0)
	  {
	     result = list_append(result, l->data);
	     l = list_next(l);
	  }
	else if (cmp == 0)
	  {
	     result = list_append(result, l->data);
	     l = list_next(l);
	     result = list_append(result, ll->data);
	     ll = list_next(ll);
	  }
	else if (cmp > 0)
	  {
	     result = list_append(result, ll->data);
	     ll = list_next(ll);
	  }
	else
	  {
	     l = ll = NULL;
	  }
     }
   while (l)
     {
	result = list_append(result, l->data);
	l = list_next(l);
     }
   list_free(l_head);
   while (ll)
     {
	result = list_append(result, ll->data);
	ll = list_next(ll);
     }
   list_free(ll_head);
   return (result);
}

/**
 * Sort a list according to the ordering func will return
 * @param list The list handle to sort
 * @param size The length of the list to sort
 * @param func A function pointer that can handle comparing the list data
 * nodes
 * @return A new sorted list
 *
 * This function sorts your list.  The data in your nodes can be arbitrary,
 * you just have to be smart enough to know what kind of data is in your
 * lists
 *
 * In the event of a memory allocation failure, It might segv.
 *
 * Example:
 * @code
 * int
 * sort_cb(void *d1, void *d2)
 * {
 *   const char *txt = NULL;
 *    const char *txt2 = NULL;
 *
 *    if(!d1) return(1);
 *    if(!d2) return(-1);
 *
 *    return(strcmp((const char*)d1, (const char*)d2));
 * }
 * extern List *list;
 *
 * list = list_sort(list, list_count(list), sort_cb);
 * if (list_alloc_error())
 *   {
 *     fprintf(stderr, "ERROR: Memory is low. List Sorting failed.\n");
 *     exit(-1);
 *   }
 * @endcode
 * @ingroup List_Ordering_Group
 */
List *
list_sort(List *list, int size, int (*func)(void *, void *))
{
   List *l = NULL, *ll = NULL, *llast;
   int mid;

   if (!list || !func)
     return NULL;

   /* FIXME: this is really inefficient - calling list_nth is not
    * fast as it has to walk the list */
   
   /* if the caller specified an invalid size, sort the whole list */
   if ((size <= 0) ||
       (size > ((List_Accounting *)(list->accounting))->count))
     size = ((List_Accounting *)(list->accounting))->count;

   mid = size / 2;
   if (mid < 1) return list;

   /* bleh evas list splicing */
   llast = ((List_Accounting *)(list->accounting))->last;
   ll = list_nth_list(list, mid);
   if (ll->prev)
     {
	((List_Accounting *)(list->accounting))->last = ll->prev;
	((List_Accounting *)(list->accounting))->count = mid;
	ll->prev->next = NULL;
	ll->prev = NULL;
     }
   ll->accounting = malloc(sizeof(List_Accounting));
   ((List_Accounting *)(ll->accounting))->last = llast;
   ((List_Accounting *)(ll->accounting))->count = size - mid;

   /* merge sort */
   l = list_sort(list, mid, func);
   ll = list_sort(ll, size - mid, func);
   list = list_combine(l, ll, func);

   return(list);
}
/**
 * Return the memory allocation failure flag after any operation needin allocation
 * @return The state of the allocation flag
 *
 * This function returns the state of the memory allocation flag. This flag is
 * set if memory allocations during list_append(), list_prepend(),
 * list_append_relative(), or list_prepend_relative() fail. If they
 * do fail, 1 will be returned, otherwise 0 will be returned. The flag will
 * remain in its current state until the next call that requires allocation
 * is called, and is then reset.
 *
 * Example:
 * @code
 * List *list = NULL;
 * extern void *my_data;
 *
 * list = list_append(list, my_data);
 * if (list_alloc_error())
 *   {
 *     fprintf(stderr, "ERROR: Memory is low. List allocation failed.\n");
 *     exit(-1);
 *   }
 * @endcode
 * @ingroup List_General_Group
 */
int
list_alloc_error(void)
{
   return _list_alloc_error;
}
