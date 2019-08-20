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
Derived from cvsgui_wire.h in WinCVS
*/

#ifndef _CVSLIB_CVS_WIRE_H_
#define _CVSLIB_CVS_WIRE_H_


typedef void (*WireReadFunc) (HANDLE fd, WireMessage* msg);
typedef void (*WireWriteFunc) (HANDLE fd, WireMessage* msg);
typedef void (*WireDestroyFunc) (WireMessage* msg);
typedef int  (*WireIOFunc) (HANDLE fd, char* buf, unsigned long count);
typedef int  (*WireFlushFunc) (HANDLE fd);

struct _WireMessage
{
	unsigned int type;
	void* data;
};

void wire_register(unsigned int type, WireReadFunc read_func, WireWriteFunc write_func, WireDestroyFunc destroy_func);

void wire_set_reader(WireIOFunc read_func);
void wire_set_writer(WireIOFunc write_func);
void wire_set_flusher(WireFlushFunc flush_func);

int wire_read(HANDLE fd, unsigned char *buf, unsigned long count);
int wire_write(HANDLE fd, unsigned char *buf, unsigned long count);
int wire_flush(HANDLE fd);

int wire_error(void);
void wire_clear_error(void);

int wire_read_msg(HANDLE fd, WireMessage* msg);
int wire_write_msg(HANDLE fd, WireMessage* msg);
void wire_destroy(WireMessage* msg);

int wire_read_int32(HANDLE fd, unsigned int* data, int count);
int wire_read_int16(HANDLE fd, unsigned short* data, int count);
int wire_read_int8(HANDLE fd, unsigned char* data, int count);
int wire_read_double(HANDLE fd, double* data, int count);
int wire_read_string(HANDLE fd, char** data, int count);

int wire_write_int32(HANDLE fd, unsigned int* data, int count);
int wire_write_int16(HANDLE fd, unsigned short* data, int count);
int wire_write_int8(HANDLE fd, unsigned char* data, int count);
int wire_write_double(HANDLE fd, double* data, int count);
int wire_write_string(HANDLE fd, char** data, int count, int len);


#endif /* _CVSLIB_CVS_WIRE_H_ */
