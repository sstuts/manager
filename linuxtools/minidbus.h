/*
 * Copyright (c) 2010 Citrix Systems, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MINIDBUS_H
#define MINIDBUS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	DBUS_SIGNATURE,
	DBUS_OBJECTPATH,
	DBUS_BOOLEAN,
	DBUS_BYTE,
	DBUS_STRING,
	DBUS_INT16,
	DBUS_UINT16,
	DBUS_INT32,
	DBUS_UINT32,
	DBUS_INT64,
	DBUS_UINT64,
	DBUS_DOUBLE,
	DBUS_ARRAY,
	DBUS_STRUCT_BEGIN,
	DBUS_STRUCT_END,
	DBUS_INVALID = -1,
} dbus_type;

typedef struct {
	dbus_type a[256];
} dbus_sig;

struct dbus_reader {
	uint8_t *data;
	uint32_t align_offset;
	uint32_t offset;
	uint32_t length;
	int endianness; /* 0 = little endian, 1 = big endian */
};

struct dbus_writer {
	uint8_t *buffer;
	uint32_t offset;
	uint32_t length;
	int endianness; /* 0 = little endian, 1 = big endian */
};

typedef struct { uint32_t *ptr; uint32_t offset; } dbus_array_writer;
typedef struct { uint32_t length; uint32_t offset; } dbus_array_reader;

typedef enum {
	DBUS_FIELD_INVALID = 0,
	DBUS_FIELD_PATH = 1,
	DBUS_FIELD_INTERFACE = 2,
	DBUS_FIELD_MEMBER = 3,
	DBUS_FIELD_ERROR_NAME = 4,
	DBUS_FIELD_REPLY_SERIAL = 5,
	DBUS_FIELD_DESTINATION = 6,
	DBUS_FIELD_SENDER = 7,
	DBUS_FIELD_SIGNATURE = 8,
	DBUS_FIELD_UNIX_FDS = 9,
} dbus_field_type;

typedef enum {
	DBUS_TYPE_INVALID = 0,
	DBUS_TYPE_METHOD_CALL = 1,
	DBUS_TYPE_METHOD_RETURN = 2,
	DBUS_TYPE_ERROR = 3,
	DBUS_TYPE_SIGNAL = 4,
} dbus_msg_type;

typedef struct {
	int type;
	uint32_t serial;
	char *destination;
	char *path;
	char *interface;
	char *method;
	char *error_name;
	char *sender;
	dbus_sig signature;
	uint32_t reply_serial;
	int w;
	uint8_t *body;
	union {
		struct dbus_writer writer; /* writing body */
		struct dbus_reader reader; /* reading body */
	};
} dbus_msg;

/* vectorise IO operations so that user can decide about buffering
 * and how to read/write to whatever is providing the data (channel, handle, etc)
 */
typedef struct {
	/* return 0 if succeed to write count bytes, non-0 otherwise */
	int (*io_write)(void *priv, const void *buf, uint32_t count);
	/* return 0 if succeed to read count bytes, non-0 otherwise */
	int (*io_read)(void *priv, void *buf, uint32_t count);
	/* debugging logging */
	void (*io_debug)(void *logpriv, const char *buf);
	/* private pointer passed to write/read (eg. handle, channel, buffer, etc) */
	void *priv;
	/* private pointer passed for debugging */
	void *logpriv;
} dbus_io;

void      dbus_msg_free(dbus_msg *msg);
dbus_msg *dbus_msg_new(uint32_t serial);
dbus_msg *dbus_msg_new_method_call(uint32_t serial, const char *destination, const char *path,
                                   const char *interface, const char *method);
dbus_msg *dbus_msg_new_signal(uint32_t serial, const char *path, const char *interface, const char *name);

void dbus_msg_set_destination(dbus_msg *msg, const char *destination);
void dbus_msg_set_path(dbus_msg *msg, const char *path);
void dbus_msg_set_method(dbus_msg *msg, const char *method);
void dbus_msg_set_error_name(dbus_msg *msg, const char *error_name);
void dbus_msg_set_sender(dbus_msg *msg, const char *sender);
void dbus_msg_set_interface(dbus_msg *msg, const char *interface);
void dbus_msg_set_signature(dbus_msg *msg, dbus_sig *signature);

/* create a body buffer of specified length.
 BEWARE: need to be called before adding any elements to the body */
int dbus_msg_body_add(dbus_msg *msg, uint32_t length);

/* method to add differents types of elements to the body. */
int dbus_msg_body_add_byte       (dbus_msg *msg, uint8_t val);
int dbus_msg_body_add_boolean    (dbus_msg *msg, bool val);
int dbus_msg_body_add_int16      (dbus_msg *msg, int16_t val);
int dbus_msg_body_add_uint16     (dbus_msg *msg, uint16_t val);
int dbus_msg_body_add_int32      (dbus_msg *msg, int32_t val);
int dbus_msg_body_add_uint32     (dbus_msg *msg, uint32_t val);
int dbus_msg_body_add_int64      (dbus_msg *msg, int64_t val);
int dbus_msg_body_add_uint64     (dbus_msg *msg, uint64_t val);
int dbus_msg_body_add_double     (dbus_msg *msg, double val);
int dbus_msg_body_add_string     (dbus_msg *msg, const char *val);
int dbus_msg_body_add_objectpath (dbus_msg *msg, const char *val);
int dbus_msg_body_add_array_begin(dbus_msg *msg, dbus_array_writer *ptr);
int dbus_msg_body_add_array_end  (dbus_msg *msg, dbus_array_writer *ptr);
int dbus_msg_body_add_structure  (dbus_msg *msg);
int dbus_msg_body_add_variant    (dbus_msg *msg, dbus_sig *signature);

/* methods to introspect a received message body for all different types */
int dbus_msg_body_get_byte        (dbus_msg *msg, uint8_t *val);
int dbus_msg_body_get_boolean     (dbus_msg *msg, bool *val);
int dbus_msg_body_get_int16       (dbus_msg *msg, int16_t *val);
int dbus_msg_body_get_uint16      (dbus_msg *msg, uint16_t *val);
int dbus_msg_body_get_int32       (dbus_msg *msg, int32_t *val);
int dbus_msg_body_get_uint32      (dbus_msg *msg, uint32_t *val);
int dbus_msg_body_get_int64       (dbus_msg *msg, int64_t *val);
int dbus_msg_body_get_uint64      (dbus_msg *msg, uint64_t *val);
int dbus_msg_body_get_double      (dbus_msg *msg, double *val);
int dbus_msg_body_get_string      (dbus_msg *msg, char **val);
int dbus_msg_body_get_object_path (dbus_msg *msg, char **val);
int dbus_msg_body_get_array       (dbus_msg *msg, dbus_array_reader *ptr);
int dbus_msg_body_get_array_left  (dbus_msg *msg, dbus_array_reader *ptr);
int dbus_msg_body_get_structure   (dbus_msg *msg);
int dbus_msg_body_get_variant     (dbus_msg *msg, dbus_sig *signature);

int dbus_msg_send(dbus_io *dio, dbus_msg *msg);
int dbus_msg_recv(dbus_io *dio, dbus_msg **msg);

/* connection method */
int dbus_connect_session(void);
int dbus_connect_system (void);

/* auth handshake */
int dbus_auth(dbus_io *dio, char *auth);

#endif
