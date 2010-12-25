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

class RingBuffer
{
public:
//Methods
    void rb_init (char*, size_t);

    size_t rb_used();
    size_t rb_free();
    
    size_t read();
    size_t write();
    
    int rb_memcmp (const char*, size_t);

    int rb_read_all_iov(struct iovec*);
    int	rb_read_n_iov(struct iovec*, size_t);
    int	rb_read_until_char_iov(struct iovec*, char);
    int	rb_read_until_mem_iov(struct iovec*, const char*, size_t);
    size_t rb_read_upd(size_t);
    size_t rb_read(char*, size_t);

    int	rb_write_iov(struct iovec*);
    size_t rb_write_upd(size_t);
    size_t rb_write(char*, size_t);

// Data
private:
    char*	m_buffer;
    size_t	m_size;
    size_t	m_used;
    size_t	m_read;
    size_t	m_write;
};

#endif /* ____RINGBUFFER_H__ */

/* vi: set ts=8 sw=4 sts=4 noet: */
