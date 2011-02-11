/* 
   Copyright (C) 2009 - 2010
      
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

#include "datacarddevice.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


int CardDevice::send_atcmd(const char* fmt, ...)
{
    char buf[1024];
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf(buf, 1023, fmt, ap);
    va_end(ap);    
    return at_write_full(buf, len);
}

int CardDevice::at_write_full(char* buf, size_t count)
{
    char*	p = buf;
    ssize_t	out_count;

    Debug(DebugAll, "[%s] [%.*s]", c_str(), (int) count, buf);

    while (count > 0)
    {
	if ((out_count = write(m_data_fd, p, count)) == -1)
	{
	    Debug(DebugAll, "[%s] write() error: %d", c_str(), errno);
	    return -1;
	}

	count -= out_count;
	p += out_count;
    }
    write(m_data_fd, "\r", 1);
    
    return 0;
}

int CardDevice::at_send_sms_text(const char* pdu)
{
    return send_atcmd("%s\x1a", pdu);
}

/* vi: set ts=8 sw=4 sts=4 noet: */
