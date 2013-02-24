#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifndef COMPAT_GLIB_H
#define COMPAT_GLIB_H

typedef unsigned char guint8;
typedef char gchar;
typedef unsigned char guchar;
typedef unsigned short gushort;

typedef unsigned long guint32;
typedef unsigned short guint16;
typedef int gint;
typedef unsigned int guint;
typedef bool gboolean;
typedef double gdouble;
typedef long glong;

#define G_BEGIN_DECLS
#define G_END_DECLS

#define g_new0(t,size) ((t*)(calloc(sizeof(t), size)))
#define g_new(t,size) ((t*)(calloc(sizeof(t), size)))
#define g_free free
#define g_strdup strdup
#define g_ascii_strcasecmp _stricmp
#define g_malloc (gchar*)malloc
#define g_ascii_isspace isspace
#define g_snprintf _snprintf

#define GUINT16_TO_BE ntohs

typedef   int   ssize_t;

typedef enum {
  G_IO_IN,
  G_IO_OUT,
  G_IO_ERR
} GIOCondition;



#endif
