/*
 * jabberd - Jabber Open Source Server
 * Copyright (c) 2002 Jeremie Miller, Thomas Muldowney,
 *                    Ryan Eatmon, Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA02111-1307USA
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include "pool.h"
#include <windows.h>
#include "pthread.h"
#include <stdint.h>
#include "syslog.h"

#ifndef INCL_UTIL_H
#define INCL_UTIL_H

/* jabberd2 Windows DLL */
#ifndef JABBERD2_API
# ifdef _WIN32
#  ifdef JABBERD2_EXPORTS
#   define JABBERD2_API  __declspec(dllexport)
#  else /* JABBERD2_EXPORTS */
#   define JABBERD2_API  __declspec(dllimport)
#  endif /* JABBERD2_EXPORTS */
# else /* _WIN32 */
#  define JABBERD2_API extern
# endif /* _WIN32 */
#endif /* JABBERD2_API */

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------- */
/*                                                           */
/* String pools (spool) functions                            */
/*                                                           */
/* --------------------------------------------------------- */
struct spool_node
{
    char *c;
    struct spool_node *next;
};

typedef struct spool_struct
{
    pool_t p;
    int len;
    struct spool_node *last;
    struct spool_node *first;
} *spool;

JABBERD2_API spool spool_new(pool_t p); /* create a string pool */
JABBERD2_API void spooler(spool s, ...); /* append all the char * args to the pool, terminate args with s again */
JABBERD2_API char *spool_print(spool s); /* return a big string */
JABBERD2_API void spool_add(spool s, char *str); /* add a single string to the pool */
JABBERD2_API void spool_escape(spool s, char *raw, int len); /* add and xml escape a single string to the pool */
JABBERD2_API char *spools(pool_t p, ...); /* wrap all the spooler stuff in one function, the happy fun ball! */



/* logging */

typedef enum {
    log_STDOUT,
    log_SYSLOG,
    log_FILE
} log_type_t;

typedef struct log_st
{
    log_type_t  type;
    FILE        *file;
} *log_t;

typedef struct log_facility_st
{
    const char  *facility;
    int         number;
} log_facility_t;

JABBERD2_API log_t    log_new(log_type_t type, const char *ident, const char *facility);
JABBERD2_API void     log_write(log_t log, int level, const char *msgfmt, ...);
JABBERD2_API void     log_free(log_t log);

 
/*
 * rate limiting
 */

typedef struct rate_st
{
    int             total;      /* if we exceed this many events */
    int             seconds;    /* in this many seconds */
    int             wait;       /* then go bad for this many seconds */

    time_t          time;       /* time we started counting events */
    int             count;      /* event count */

    time_t          bad;        /* time we went bad, or 0 if we're not */
} *rate_t;

JABBERD2_API rate_t      rate_new(int total, int seconds, int wait);
JABBERD2_API void        rate_free(rate_t rt);
JABBERD2_API void        rate_reset(rate_t rt);

/**
 * Add a number of events to the counter.  This takes care of moving
 * the sliding window, if we've moved outside the previous window.
 */
JABBERD2_API void        rate_add(rate_t rt, int count);

/**
 * @return The amount of events we have left before we hit the rate
 *         limit.  This could be number of bytes, or number of
 *         connection attempts, etc.
 */
JABBERD2_API int         rate_left(rate_t rt);

/**
 * @return 1 if we're under the rate limit and everything is fine or
 *         0 if the rate limit has been exceeded and we should throttle
 *         something.
 */
JABBERD2_API int         rate_check(rate_t rt);


/*
 * serialisation helper functions
 */

JABBERD2_API int         ser_string_get(char **dest, int *source, const char *buf, int len);
JABBERD2_API int         ser_int_get(int *dest, int *source, const char *buf, int len);
JABBERD2_API void        ser_string_set(char *source, int *dest, char **buf, int *len);
JABBERD2_API void        ser_int_set(int source, int *dest, char **buf, int *len);

/*
 *list
 */

typedef struct _olist_node_st  *_olist_node_t;
struct _olist_node_st {
	void          *data;
	_olist_node_t  next;
	_olist_node_t  prev;
};

typedef struct _olist_st {
	pool_t          p;
	_olist_node_t   cache;
	_olist_node_t   front;
	_olist_node_t   back;

	int             size;
	time_t          init_time;
} *olist_t;


/*
 * priority queues
 */

typedef struct _jqueue_node_st  *_jqueue_node_t;
struct _jqueue_node_st {
    void            *data;

    int             priority;

    _jqueue_node_t  next;
    _jqueue_node_t  prev;
};

typedef struct _jqueue_st {
    pool_t          p;
    _jqueue_node_t  cache;

    _jqueue_node_t  front;
    _jqueue_node_t  back;

    int             size;
    char            *key;
    time_t          init_time;
} *jqueue_t;

JABBERD2_API jqueue_t    jqueue_new(void);
JABBERD2_API void        jqueue_free(jqueue_t q);
JABBERD2_API void        jqueue_push(jqueue_t q, void *data, int pri);
JABBERD2_API void        *jqueue_pull(jqueue_t q);
JABBERD2_API int         jqueue_size(jqueue_t q);
JABBERD2_API time_t      jqueue_age(jqueue_t q);
JABBERD2_API void        *jqueue_first(jqueue_t q);
JABBERD2_API void        *jqueue_next(jqueue_t q, void *node);
JABBERD2_API void        *jqueue_last(jqueue_t q);

JABBERD2_API void        *jqueue_iter_first(jqueue_t q);
JABBERD2_API void        *jqueue_iter_next(jqueue_t q, void *node);
#define NODEVAL(p)       (((_jqueue_node_t)p)->data)
/* thread safe ver */
typedef struct _jsqueue_st {
	jqueue_t                queue;
	pthread_mutex_t         lock;
}*jsqueue_t;
jsqueue_t   jsqueue_new(void);
void        jsqueue_free(jsqueue_t q);      
void        jsqueue_push(jsqueue_t q, void *data, int pri);
void        *jsqueue_pull(jsqueue_t q);
int         jsqueue_size(jsqueue_t q);

/* ISO 8601 / JEP-0082 date/time manipulation */
typedef enum {
    dt_DATE     = 1,
    dt_TIME     = 2,
    dt_DATETIME = 3,
    dt_LEGACY   = 4,
	dt_LDATETIME = 5,
	dt_SHORTDT   = 6
} datetime_t;

JABBERD2_API time_t  datetime_in(char *date);
JABBERD2_API void    datetime_out(time_t t, datetime_t type, char *date, int datelen);


/* base64 functions */
JABBERD2_API int apr_base64_decode_len(const char *bufcoded, int buflen);
JABBERD2_API int apr_base64_decode(char *bufplain, const char *bufcoded, int buflen);
JABBERD2_API int apr_base64_encode_len(int len);
JABBERD2_API int apr_base64_encode(char *encoded, const char *string, int len);

/* convenience, result string must be free()'d by caller */
JABBERD2_API char *b64_encode(char *buf, int len);
JABBERD2_API char *b64_decode(char *buf);

typedef struct _jfile_st {
#ifdef _WIN32/* _WIN32 */
	HANDLE hfilemap;
	HANDLE hfile;
	LPBYTE fhead;
#else 
	char   *fhead;
	int    hfile;
#endif 
	LARGE_INTEGER filesize;
} *jfile_t;

JABBERD2_API jfile_t j_fopen(const char *filename, const char *mode);
JABBERD2_API size_t  j_fread(void *buffer, size_t offset, size_t size, size_t count, jfile_t jfile);
JABBERD2_API size_t  j_fwrite(void *buffer, size_t offset, size_t size, size_t count, jfile_t jfile);
JABBERD2_API int     j_flush(jfile_t jfile);
JABBERD2_API void    j_fclose(jfile_t jfile);


/* debug logging */
JABBERD2_API int get_debug_flag(void);
JABBERD2_API void set_debug_flag(int v);
JABBERD2_API void debug_log(const char *file, int line, const char *msgfmt, ...);
#define ZONE __FILE__,__LINE__
#define MAX_DEBUG 8192

/* if no debug, basically compile it out */
#ifdef DEBUG
#define log_debug if(get_debug_flag()) debug_log
#else
#define log_debug if(0) debug_log
#endif

/* Portable signal function */
typedef void jsighandler_t(int);
JABBERD2_API jsighandler_t* jabber_signal(int signo,  jsighandler_t *func);

#ifdef _WIN32
/* Windows service wrapper function */
typedef int (jmainhandler_t)(int argc, char** argv);
JABBERD2_API int jabber_wrap_service(int argc, char** argv, jmainhandler_t *wrapper, LPCTSTR name, LPCTSTR display, LPCTSTR description, LPCTSTR depends);
#define JABBER_MAIN(name, display, description, depends) jabber_main(int argc, char** argv); \
                    main(int argc, char** argv) { return jabber_wrap_service(argc, argv, jabber_main, name, display, description, depends); } \
                    jabber_main(int argc, char** argv)
#else /* _WIN32 */
#define JABBER_MAIN(name, display, description, depends) int main(int argc, char** argv)
#endif /* _WIN32 */

unsigned long long UNGetTickCount();
int64_t  gettickcount();
#ifdef _WIN32
#define I64d_t "%I64d"
# define snprintf64 _snprintf
#else
#define I64d_t "%lld"
# define snprintf64 snprintf
#endif

#ifdef __cplusplus
}
#endif

#endif    /* INCL_UTIL_H */


