/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

#ifndef ____RINGBUFFER_H__
#define ____RINGBUFFER_H__

#include <sys/uio.h>
#include <string.h>

typedef struct ringbuffer_t
{
	void*	buffer;
	size_t	size;
	size_t	used;
	size_t	read;
	size_t	write;
}
ringbuffer_t;

static inline void	rb_init			(ringbuffer_t*, char*, size_t);

static inline size_t	rb_used			(ringbuffer_t*);
static inline size_t	rb_free			(ringbuffer_t*);
static int		rb_memcmp		(ringbuffer_t*, const char*, size_t);

static int		rb_read_all_iov		(ringbuffer_t*, struct iovec*);
static int		rb_read_n_iov		(ringbuffer_t*, struct iovec*, size_t);
static int		rb_read_until_char_iov	(ringbuffer_t*, struct iovec*, char);
static int		rb_read_until_mem_iov	(ringbuffer_t*, struct iovec*, const void*, size_t);
static size_t		rb_read_upd		(ringbuffer_t*, size_t);
static size_t		rb_read			(ringbuffer_t*, char*, size_t);

static int		rb_write_iov		(ringbuffer_t*, struct iovec*);
static size_t		rb_write_upd		(ringbuffer_t*, size_t);
static size_t		rb_write		(ringbuffer_t*, char*, size_t);

#endif /* ____RINGBUFFER_H__ */
