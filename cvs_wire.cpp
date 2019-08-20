/*
** The cvsgui protocol used by WinCvs
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


/*!
	Derived from cvsgui_wire.c in WinCVS
*/

#include "CvsLib.h"
#include <winsock2.h>

#ifndef WIN32
#	include <unistd.h>
#	include <sys/param.h>
#	include <netinet/in.h>
#else
#	pragma warning (disable : 4786)
#endif

/*!
	Read the data from pipe
	\param fd Pipe
	\param buf Buffer for data read
	\param count Number of bytes to read
	\return The number of bytes read, -1 on error
*/
static int cvs_w_read(HANDLE fd, void* buf, int count)
{
	DWORD read;
	if(!ReadFile(fd, buf, count, &read, NULL))
	{
		DWORD err = GetLastError();
		errno = EINVAL;
	
		return -1;
	}

	return read;
}

/*!
	Write the data to the pipe
	\param fd Pipe
	\param buf Buffer for data to write
	\param count Number of bytes to write
	\return The number of bytes written, -1 on error
*/
static int cvs_w_write(HANDLE fd, const void* buf, int count)
{
	DWORD dwrite;
	if(!WriteFile(fd, buf, count, &dwrite, NULL))
	{
		errno = EINVAL;
		return -1;
	}

	return dwrite;
}

typedef struct _WireHandler  WireHandler;

struct _WireHandler
{
	unsigned int type;
	WireReadFunc read_func;
	WireWriteFunc write_func;
	WireDestroyFunc destroy_func;
};


static WireIOFunc wire_read_func = NULL;	  /*!< Read function */
static WireIOFunc wire_write_func = NULL;	  /*!< Write function */
static WireFlushFunc wire_flush_func = NULL;  /*!< Flush function */
static int wire_error_val = FALSE;			  /*!< Error value */
static List* _call_handlers;


static void wire_handler_delete(void* wire_handler)
{
	CVSLIB_IFNULL1(wire_handler);
	CVSLIB_FREE(wire_handler);
}

static void call_handlers_destroy(List* call_handlers)
{
  CVSLIB_IFNULL1(call_handlers);
  call_handlers = list_free_foreach(call_handlers, wire_handler_delete);
}

static WireHandler* call_handlers_find(List* call_handlers, unsigned int type)
{
  List *l;
  WireHandler* wire_handler;

  CVSLIB_IFNULL2(call_handlers, NULL);
  for(l = call_handlers; l; l = list_next(l))
  {
    wire_handler = (WireHandler*)l->data;
    if(wire_handler->type == type)
      return wire_handler;
  }

  return NULL;
}

void wire_register(unsigned int type, WireReadFunc read_func, WireWriteFunc write_func, WireDestroyFunc destroy_func)
{
	WireHandler* handler;

	handler = call_handlers_find(_call_handlers, type);
	if(handler == NULL)
  {
		handler = (WireHandler*)malloc(sizeof(WireHandler));
    _call_handlers = list_append(_call_handlers, handler);
  }

  handler->type = type;
  handler->read_func = read_func;
  handler->write_func = write_func;
  handler->destroy_func = destroy_func;
}

/*!
	Set the read function
	\param read_func Read function
*/
void wire_set_reader(WireIOFunc read_func)
{
	wire_read_func = read_func;
}

/*!
	Set the write function
	\param write_func Write function
*/
void wire_set_writer(WireIOFunc write_func)
{
	wire_write_func = write_func;
}

/*!
	Set the flush function
	\param flush_func Flush function
*/
void wire_set_flusher(WireFlushFunc flush_func)
{
	wire_flush_func = flush_func;
}

/*!
	Read from the pipe
	\param fd Pipe
	\param buf Buffer for data read
	\param count Count of data read
	\return TRUE on success, FALSE otherwise
*/
int wire_read(HANDLE fd, unsigned char* buf, unsigned long count)
{
	if(wire_read_func)
	{
		if(!(*wire_read_func)(fd, (char*)buf, count))
		{
			//g_print("wire_read: error\n");
			wire_error_val = TRUE;
			return FALSE;
		}
	}
	else
	{
		int bytes;
		
		while(count > 0)
		{
			do
			{
				bytes = cvs_w_read(fd, (char*)buf, count);
			}while((bytes == -1) && ((errno == EAGAIN) || (errno == EINTR))) ;
			
			if(bytes == -1)
			{
				//g_print("wire_read: error\n");
				wire_error_val = TRUE;
				return FALSE;
			}
			
			if(bytes == 0)
			{
				//g_print("wire_read: unexpected EOF (plug-in crashed?)\n");
				wire_error_val = TRUE;
				return FALSE;
			}
			
			count -= bytes;
			buf += bytes;
		}
	}
	
	return TRUE;
}

/*!
	Write to the pipe
	\param fd Pipe
	\param buf Buffer of data to be written
	\param count Count of data to be written
	\return TRUE on success, FALSE otherwise
*/
int wire_write(HANDLE fd, unsigned char* buf, unsigned long count)
{
	if(wire_write_func)
	{
		if(!(*wire_write_func)(fd, (char*)buf, count))
		{
			//g_print("wire_write: error\n");
			wire_error_val = TRUE;
			return FALSE;
		}
	}
	else
	{
		int bytes;

		while(count > 0)
		{
			do 
			{
				bytes = cvs_w_write(fd, (char*)buf, count);
			}while((bytes == -1) && ((errno == EAGAIN) || (errno == EINTR)));

			if(bytes == -1)
			{
				//g_print("wire_write: error\n");
				wire_error_val = TRUE;
				return FALSE;
			}

			count -= bytes;
			buf += bytes;
		}
	}

	return TRUE;
}

/*!
	Flush the pipe
	\param fd Pipe
	\return TRUE on success, FALSE otherwise
	\note 
*/
int wire_flush(HANDLE fd)
{
	if(wire_flush_func)
		return (*wire_flush_func)(fd);

	return FALSE;
}

/*!
	Get the wire error
	\return The wire error value
*/
int wire_error()
{
	return wire_error_val;
}

/*!
	Clear the wire error
	\note Set the wire error value to FALSE
*/
void wire_clear_error()
{
	wire_error_val = FALSE;
}

/*!
	Read the message from the pipe
	\param fd Pipe
	\param msg Message read from the pipe
	\return TRUE on success, FALSE otherwise
*/
int wire_read_msg(HANDLE fd, WireMessage* msg)
{
	WireHandler* handler;

	if(wire_error_val)
		return !wire_error_val;

	if(!wire_read_int32(fd, &msg->type, 1))
		return FALSE;

	handler = call_handlers_find(_call_handlers, msg->type);
	if(handler == NULL)
		return FALSE;

	(*handler->read_func)(fd, msg);

	return !wire_error_val;
}

/*!
	Write the message to the pipe
	\param fd Pipe
	\param msg Message to write to the pipe
	\return TRUE on success, FALSE otherwise
*/
int wire_write_msg(HANDLE fd, WireMessage* msg)
{
	WireHandler* handler;

	if(wire_error_val)
		return !wire_error_val;

	handler = call_handlers_find(_call_handlers, msg->type);
	if(handler == NULL)
		return FALSE;

	if(!wire_write_int32(fd, &msg->type, 1))
		return FALSE;

	(*handler->write_func)(fd, msg);

	return !wire_error_val;
}

/*!
	Destroy wire message
	\param msg Message to be destroyed
*/
void wire_destroy(WireMessage* msg)
{
	WireHandler* handler;

	handler = call_handlers_find(_call_handlers, msg->type);
	if(handler == NULL)
		return;

	(*handler->destroy_func)(msg);
}

/*!
	Read 32 bit integers from the pipe
	\param fd Pipe
	\param data Data to be read
	\param count Count of integers to be read
	\return TRUE on success, FALSE otherwise
*/
int wire_read_int32(HANDLE fd, unsigned int* data, int count)
{
	if(count > 0)
    {
		if(!wire_read_int8(fd, (unsigned char*)data, count * 4))
			return FALSE;

		while(count--)
		{
			*data = ntohl(*data);
			data++;
		}
    }

	return TRUE;
}

/*!
	Read 16 bit integers from the pipe
	\param fd Pipe
	\param data Data to be read
	\param count Count of integers to be read
	\return TRUE on success, FALSE otherwise
*/
int wire_read_int16(HANDLE fd, unsigned short* data, int count)
{
	if(count > 0)
    {
		if(!wire_read_int8(fd, (unsigned char*)data, count * 2))
			return FALSE;

		while(count--)
		{
			*data = ntohs(*data);
			data++;
		}
    }

	return TRUE;
}

/*!
	Read 8 bit integers from the pipe
	\param fd Pipe
	\param data Data to be read
	\param count Count of integers to be read
	\return TRUE on success, FALSE otherwise
*/
int wire_read_int8(HANDLE fd, unsigned char* data, int count)
{
	return wire_read(fd, data, count);
}

/*!
	Read double values from the pipe
	\param fd Pipe
	\param data Data to be read
	\param count Count of double values to be read
	\return TRUE on success, FALSE otherwise
*/
int wire_read_double(HANDLE fd, double* data, int count)
{
	char* str;
	int i;

	for(i = 0; i < count; i++)
    {
		if(!wire_read_string(fd, &str, 1))
			return FALSE;

		sscanf(str, "%le", &data[i]);
		CVSLIB_FREE(str);
    }

	return TRUE;
}

/*!
	Read strings from the pipe
	\param fd Pipe
	\param data Data to be read
	\param count Count of strings to be read
	\return TRUE on success, FALSE otherwise
*/
int wire_read_string(HANDLE fd, char** data, int count)
{
  unsigned int tmp;
  int i;

  for(i = 0; i < count; i++)
  {
    if(!wire_read_int32(fd, &tmp, 1))
      return FALSE;

    if(tmp > 0)
    {
      data[i] = (char*)malloc(tmp * sizeof(char));
      if(!wire_read_int8(fd, (unsigned char*)data[i], tmp))
      {
        CVSLIB_FREE(data[i]);
        return FALSE;
      }
    }
    else
    {
      data[i] = NULL;
    }
  }

  return TRUE;
}

/*!
	Write 32 bit integers to the pipe
	\param fd Pipe
	\param data Data to be written
	\param count Count of integers to be written
	\return TRUE on success, FALSE otherwise
*/
int wire_write_int32(HANDLE fd, unsigned int* data, int count)
{
	unsigned int tmp;
	int i;

	if(count > 0)
    {
		for(i = 0; i < count; i++)
		{
			tmp = htonl(data[i]);
			if(!wire_write_int8(fd, (unsigned char*)&tmp, 4))
				return FALSE;
		}
    }

	return TRUE;
}

/*!
	Write 16 bit integers to the pipe
	\param fd Pipe
	\param data Data to be written
	\param count Count of integers to be written
	\return TRUE on success, FALSE otherwise
*/
int wire_write_int16(HANDLE fd, unsigned short* data, int count)
{
	unsigned short tmp;
	int i;

	if(count > 0)
    {
		for(i = 0; i < count; i++)
		{
			tmp = htons(data[i]);
			if(!wire_write_int8(fd, (unsigned char*)&tmp, 2))
				return FALSE;
		}
    }

	return TRUE;
}

/*!
	Write 8 bit integers to the pipe
	\param fd Pipe
	\param data Data to be written
	\param count Count of integers to be written
	\return TRUE on success, FALSE otherwise
*/
int wire_write_int8(HANDLE fd, unsigned char* data, int count)
{
	return wire_write(fd, data, count);
}

/*!
	Write double values to the pipe
	\param fd Pipe
	\param data Data to be written
	\param count Count of double values to be written
	\return TRUE on success, FALSE otherwise
*/
int wire_write_double(HANDLE fd, double* data, int count)
{
	char* t, buf[128];
	int i;

	t = buf;
	for(i = 0; i < count; i++)
    {
		sprintf(buf, "%0.50e", data[i]);
		if(!wire_write_string(fd, &t, 1, -1))
			return FALSE;
    }

	return TRUE;
}

/*!
	Write strings from the pipe
	\param fd Pipe
	\param data Data to be written
	\param count Count of strings to be written
	\param len Strings length, if -1 then it's NULL-terminated string, otherwise it's a binary data
	\return TRUE on success, FALSE otherwise
*/
int wire_write_string(HANDLE fd, char** data, int count, int len)
{
	unsigned int tmp;
	int i;

	for(i = 0; i < count; i++)
    {
		if(data[i])
			tmp = len == -1 ? (int)strlen(data[i]) + 1 : len + 1;
		else
			tmp = 0;

		if(!wire_write_int32(fd, &tmp, 1))
			return FALSE;

		if(tmp > 0)
			if(!wire_write_int8(fd, (unsigned char*)data[i], tmp))
				return FALSE;
    }

	return TRUE;
}
