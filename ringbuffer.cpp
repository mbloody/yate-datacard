/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/
#include "ringbuffer.h"


//TODO: Check if we need use unsigned char!!!!!

void RingBuffer::rb_init (char* buf, size_t size)
{
	m_buffer = buf;
	m_size   = size;
	m_used   = 0;
	m_read   = 0;
	m_write  = 0;
}

size_t RingBuffer::rb_used()
{
	return m_used;
}

size_t RingBuffer::rb_free()
{
	return m_size - m_used;
}

int RingBuffer::rb_memcmp (const char* mem, size_t len)
{
	size_t tmp;

	if (m_used > 0 && len > 0 && m_used >= len)
	{
		if ((m_read + len) > m_size)
		{
			tmp = m_size - m_read;
			if (memcmp (m_buffer + m_read, mem, tmp) == 0)
			{
				len -= tmp;
				mem += tmp;

				if (memcmp (m_buffer, mem, len) == 0)
				{
					return 0;
				}
			}
		}
		else 
		{
			if (memcmp (m_buffer + m_read, mem, len) == 0)
			{
				return 0;
			}
		}

		return 1;
	}

	return -1;
}

// ============================ READ ============================= 

int RingBuffer::rb_read_all_iov (struct iovec* iov)
{
	if (m_used > 0)
	{
		if ((m_read + m_used) > m_size)
		{
			iov[0].iov_base = m_buffer + m_read;
			iov[0].iov_len  = m_size - m_read;
			iov[1].iov_base = m_buffer;
			iov[1].iov_len  = m_used - iov[0].iov_len;
			return 2;
		}
		else
		{
			iov[0].iov_base = m_buffer + m_read;
			iov[0].iov_len  = m_used;
			iov[1].iov_len  = 0;
			return 1;
		}
	}

	return 0;
}

int RingBuffer::rb_read_n_iov (struct iovec* iov, size_t len)
{
	if (m_used < len)
	{
		return 0;
	}

	if (len > 0)
	{
		if ((m_read + len) > m_size)
		{
			iov[0].iov_base = m_buffer + m_read;
			iov[0].iov_len  = m_size - m_read;
			iov[1].iov_base = m_buffer;
			iov[1].iov_len  = len - iov[0].iov_len;
			return 2;
		}
		else
		{
			iov[0].iov_base = m_buffer + m_read;
			iov[0].iov_len  = len;
			iov[1].iov_len  = 0;
			return 1;
		}
	}

	return 0;
}

int RingBuffer::rb_read_until_char_iov (struct iovec* iov, char c)
{
	char* p;

	if (m_used > 0)
	{
		if ((m_read + m_used) > m_size)
		{
			iov[0].iov_base = m_buffer + m_read;
			iov[0].iov_len  = m_size - m_read;
			if ((p = (char*)memchr (iov[0].iov_base, c, iov[0].iov_len)) != NULL)
			{
				iov[0].iov_len = p - (char*)iov[0].iov_base;
				iov[1].iov_len = 0;
				return 1;
			}
		
			if ((p = (char*)memchr (m_buffer, c, m_used - iov[0].iov_len)) != NULL)
			{
				iov[1].iov_base = m_buffer;
				iov[1].iov_len = p - m_buffer;
				return 2;
			}
		}
		else 
		{
			iov[0].iov_base = m_buffer + m_read;
			iov[0].iov_len  = m_used;
			if ((p = (char*)memchr (iov[0].iov_base, c, iov[0].iov_len)) != NULL)
			{
				iov[0].iov_len = p - (char*)iov[0].iov_base;
				iov[1].iov_len = 0;
				return 1;
			}
		}
	}

	return 0;
}

int RingBuffer::rb_read_until_mem_iov (struct iovec* iov, const char* mem, size_t len)
{
	size_t	i;
	char*	p;

	if (len == 1)
	{
		return rb_read_until_char_iov (iov, *((char*) mem));
	}

	if (m_used > 0 && len > 0 && m_used >= len)
	{
		if ((m_read + m_used) > m_size)
		{
			iov[0].iov_base = m_buffer + m_read;
			iov[0].iov_len  = m_size - m_read;
			if (iov[0].iov_len >= len)
			{
				if ((p = (char*)memmem (iov[0].iov_base, iov[0].iov_len, mem, len)) != NULL)
				{
					iov[0].iov_len = p - (char*)iov[0].iov_base;
					iov[1].iov_len = 0;
					return 1;
				}

				i = 1;
				iov[1].iov_base = (char*)iov[0].iov_base + iov[0].iov_len - len + 1;
			}
			else
			{
				i = len - iov[0].iov_len;
				iov[1].iov_base = iov[0].iov_base;
			}


			while (i < len)
			{
				if (memcmp (iov[1].iov_base, mem, len - i) == 0 && memcmp (m_buffer, mem + i, i) == 0)
				{
					iov[0].iov_len = (char*)iov[1].iov_base - (char*)iov[0].iov_base;
					iov[1].iov_len = 0;
					return 1;
				}

				if (m_used == iov[0].iov_len + i)
				{
					return 0;
				}

                // TODO: Clean this shit
                char* p_tmp = (char*)iov[1].iov_base;
				p_tmp++;
				
				i++;
			}

			if (m_used >= iov[0].iov_len + len)
			{
				if ((p = (char*)memmem (m_buffer, m_used - iov[0].iov_len, mem, len)) != NULL)
				{
					if (p == m_buffer)
					{
						iov[1].iov_len = 0;
						return 1;
					}
				
					iov[1].iov_base = m_buffer;
					iov[1].iov_len = p - m_buffer;
					return 2;
				}
			}
		}
		else 
		{
			iov[0].iov_base = m_buffer + m_read;
			iov[0].iov_len  = m_used;
			if ((p = (char*)memmem (iov[0].iov_base, iov[0].iov_len, mem, len)) != NULL)
			{
				iov[0].iov_len = p - (char*)iov[0].iov_base;
				iov[1].iov_len = 0;

				return 1;
			}
		}
	}

	return 0;
}

size_t RingBuffer::rb_read_upd (size_t len)
{
	size_t s;

	if (m_used < len)
	{
		len = m_used;
	}

	if (len > 0)
	{
		m_used -= len;

		if (m_used == 0)
		{
			m_read  = 0;
			m_write = 0;
		}
		else
		{
			s = m_read + len;

			if (s >= m_size)
			{
				m_read = s - m_size;
			}
			else
			{
				m_read = s;
			}
		}
	}

	return len;
}

size_t RingBuffer::rb_read (char* buf, size_t len)
{
	size_t s;

	if (m_used < len)
	{
		len = m_used;
	}

	if (len > 0)
	{
		s = m_read + len;
		if (s > m_size)
		{
			memmove (buf, m_buffer + m_read, m_size - m_read);
			memmove (buf + m_size - m_read, m_buffer, s - m_size);
			m_read = s - m_size;
		}
		else
		{
			memmove (buf, m_buffer + m_read, len);
			if (s == m_size)
			{
				m_read = 0;
			}
			else
			{
				m_read = s;
			}
		}

		m_used -= len;

		if (m_used == 0)
		{
			m_read  = 0;
			m_write = 0;
		}
	}

	return len;
}

// ============================ WRITE ============================ 

int RingBuffer::rb_write_iov (struct iovec* iov)
{
	size_t free;

	free = rb_free ();
	if (free > 0)
	{
		if ((m_write + free) > m_size)
		{
			iov[0].iov_base = m_buffer + m_write;
			iov[0].iov_len  = m_size - m_write;
			iov[1].iov_base = m_buffer;
			iov[1].iov_len  = free - iov[0].iov_len;

			return 2;
		}
		else
		{
			iov[0].iov_base = m_buffer + m_write;
			iov[0].iov_len  = free;

			return 1;
		}
	}

	return 0;
}

size_t RingBuffer::rb_write_upd (size_t len)
{
	size_t free;
	size_t s;

	free = rb_free ();
	if (free < len)
	{
		len = free;
	}

	if (len > 0)
	{
		s = m_write + len;

		if (s > m_size)
		{
			m_write = s - m_size;
		}
		else
		{
			m_write = s;
		}

		m_used += len;
	}

	return len;
}

size_t RingBuffer::rb_write (char* buf, size_t len)
{
	size_t	free;
	size_t	s;

	free = rb_free ();
	if (free < len)
	{
		len = free;
	}

	if (len > 0)
	{
		s = m_write + len;

		if (s > m_size)
		{
			memmove (m_buffer + m_write, buf, m_size - m_write);
			memmove (m_buffer, buf + m_size - m_write, s - m_size);
			m_write = s - m_size;
		}
		else
		{
			memmove (m_buffer + m_write, buf, len);
			if (s == m_size)
			{
				m_write = 0;
			}
			else
			{
				m_write = s;
			}
		}

		m_used += len;
	}

	return len;
}
