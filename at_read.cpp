/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

/*!
 * \brief Wait for activity on an socket
 * \param pvt -- pvt struct
 * \param ms  -- pointer to an int containing a timeout in ms
 * \return 0 on timeout and the socket fd (non-zero) otherwise
 * \retval 0 timeout
 */
#include "datacarddevice.h"
#include <stdlib.h>
#include <poll.h>


#if 0
// Old Asterisk implementation. Found at 
// https://issues.asterisk.org/file_download.php?file_id=6792&type=bug
int ast_fdisset(struct pollfd *pfds, int fd, int max, int *start)
{
 	int x;
    int dummy=0;

    if (fd < 0)
        return 0;
    if (start == NULL)
        start = &dummy;
    for (x = *start; x<max; x++)
 		if (pfds[x].fd == fd) {
            if (x == *start)
                (*start)++;
 			return pfds[x].revents;
 		}
 	return 0;
}

int ast_waitfor_n_fd(int *fds, int n, int *ms, int *exception)
{
    struct timeval start = { 0 , 0 };
    int res;
    int x, y;
 	int winner = -1;
    int spoint;
    struct pollfd *pfds;
    
    pfds = (struct pollfd*)malloc(sizeof(struct pollfd) * n);
    if (!pfds) {
        // ast_log(LOG_ERROR, "Out of memory\n");
        return -1;
    }
    //if (*ms > 0)
    //    start = ast_tvnow();
    
    y = 0;
    for (x=0; x < n; x++) {
        if (fds[x] > -1) {
            pfds[y].fd = fds[x];
            pfds[y].events = POLLIN | POLLPRI;
            y++;
        }
    }
    res = poll(pfds, y, *ms);
    if (res < 0) {
        /* Simulate a timeout if we were interrupted */
        if (errno != EINTR)
            *ms = -1;
        else
            *ms = 0;
        return -1;
    }
    spoint = 0;
    for (x=0; x < n; x++) {
        if (fds[x] > -1) {
            if ((res = ast_fdisset(pfds, fds[x], y, &spoint))) {
                winner = fds[x];
                if (exception) {
                    if (res & POLLPRI)
                        *exception = -1;
                    else
                    *exception = 0;
                }
            }
        }
    }
    //if (*ms > 0) {
    //    *ms -= ast_tvdiff_ms(ast_tvnow(), start);
    //    if (*ms < 0)
    //    *ms = 0;
    //}

 	return winner;
}
#endif

int CardDevice::at_wait (int* ms)
{

    struct pollfd fds;
    fds.fd = m_data_fd;
    fds.events = POLLIN | POLLPRI ;
    fds.revents = 0;

    int res = poll(&fds, 1, *ms);
    if (res < 0) {
        /* Simulate a timeout if we were interrupted */
        if (errno != EINTR)
            *ms = -1;
        else
            *ms = 0;
	return 0;
    }
    else if(res == 0)
    {
	*ms = -1;
	return 0;
    }
    {
	if((fds.revents & POLLIN))
	{
	    return fds.fd;
	}	
	if (fds.revents & (POLLRDHUP|POLLERR|POLLHUP|POLLNVAL|POLLPRI))
	{
	    return fds.fd;
	}
    }

#if 0
	int exception, outfd;

	outfd = ast_waitfor_n_fd (&m_data_fd, 1, ms, &exception);
	

	if (outfd < 0)
	{
		outfd = 0;
	}

	return outfd;
#endif
}

int CardDevice::at_read ()
{
	int	iovcnt;
	ssize_t	n;

	iovcnt = d_read_rb.rb_write_iov(d_read_iov);

	if (iovcnt > 0)
	{
		n = readv (m_data_fd, d_read_iov, iovcnt);

		if (n < 0)
		{
			if (errno != EINTR && errno != EAGAIN)
			{
				Debug(DebugAll, "[%s] readv() error: %d\n", c_str(), errno);
				return -1;
			}

			return 0;
		}
		else if (n == 0)
		{
			return -1;
		}

		d_read_rb.rb_write_upd (n);


		Debug(DebugAll, "[%s] receive %zu byte, used %zu, free %zu, read %zu, write %zu\n", c_str(), n,
			d_read_rb.rb_used(), d_read_rb.rb_free(), d_read_rb.m_read, d_read_rb.m_write);

		iovcnt = d_read_rb.rb_read_all_iov(d_read_iov);

		if (iovcnt > 0)
		{
			if (iovcnt == 2)
			{
				Debug(DebugAll, "[%s] [%.*s%.*s]\n", c_str(),
						(int) d_read_iov[0].iov_len, (char*) d_read_iov[0].iov_base,
							(int) d_read_iov[1].iov_len, (char*) d_read_iov[1].iov_base);
			}
			else
			{
				Debug(DebugAll, "[%s] [%.*s]\n", c_str(),
						(int) d_read_iov[0].iov_len, (char*) d_read_iov[0].iov_base);
			}
		}

		return 0;
	}

	Debug(DebugAll, "[%s] receive buffer overflow\n", c_str());

	return -1;
}

int CardDevice::at_read_result_iov()
{
	int	iovcnt = 0;
	int	res;
	size_t	s;

	if ((s = d_read_rb.rb_used ()) > 0)
	{
		if (!d_read_result)
		{
			if ((res = d_read_rb.rb_memcmp("\r\n", 2)) == 0)
			{
				d_read_rb.rb_read_upd (2);
				d_read_result = 1;

				return at_read_result_iov ();
			}
			else if (res > 0)
			{
				if (d_read_rb.rb_memcmp ("\n", 1) == 0)
				{
					Debug(DebugAll, "[%s] multiline response\n", c_str());
					d_read_rb.rb_read_upd (1);

					return at_read_result_iov();
				}

				if (d_read_rb.rb_read_until_char_iov(d_read_iov, '\r') > 0)
				{
					s = d_read_iov[0].iov_len + d_read_iov[1].iov_len + 1;
				}

				d_read_rb.rb_read_upd(s);

				return at_read_result_iov();
			}

			return 0;
		}
		else
		{
			if (d_read_rb.rb_memcmp("+CSSI:", 6) == 0)
			{
				if ((iovcnt = d_read_rb.rb_read_n_iov(d_read_iov, 8)) > 0)
				{
					d_read_result = 0;
				}

				return iovcnt;
			}
			else if (d_read_rb.rb_memcmp("\r\n+CSSU:", 8) == 0)
			{
				d_read_rb.rb_read_upd(2);
				return at_read_result_iov();
			}
			else if (d_read_rb.rb_memcmp("> ", 2) == 0)
			{
				d_read_result = 0;
				return d_read_rb.rb_read_n_iov(d_read_iov, 2);
			}
			else if (d_read_rb.rb_memcmp("+CMGR:", 6) == 0)
			{
				if ((iovcnt = d_read_rb.rb_read_until_mem_iov(d_read_iov, "\n\r\nOK\r\n", 7)) > 0)
				{
					d_read_result = 0;
				}

				return iovcnt;
			}
			else if (d_read_rb.rb_memcmp("+CNUM:", 6) == 0)
			{
				if ((iovcnt = d_read_rb.rb_read_until_mem_iov(d_read_iov, "\n\r\nOK\r\n", 7)) > 0)
				{
					d_read_result = 0;
				}

				return iovcnt;
			}
			else if (d_read_rb.rb_memcmp("ERROR+CNUM:", 11) == 0)
			{
				if ((iovcnt = d_read_rb.rb_read_until_mem_iov(d_read_iov, "\n\r\nOK\r\n", 7)) > 0)
				{
					d_read_result = 0;
				}

				return iovcnt;
			}
			else
			{
				if ((iovcnt = d_read_rb.rb_read_until_mem_iov(d_read_iov, "\r\n", 2)) > 0)
				{
					d_read_result = 0;
					s = d_read_iov[0].iov_len + d_read_iov[1].iov_len + 1;

					return d_read_rb.rb_read_n_iov(d_read_iov, s);
				}
			}
		}
	}

	return 0;
}

at_res_t CardDevice::at_read_result_classification (int iovcnt)
{
	at_res_t at_res;

	if (iovcnt > 0)
	{
		if (d_read_rb.rb_memcmp("^BOOT:", 6) == 0)		// 5115
		{
			at_res = RES_BOOT;
		}
		else if (d_read_rb.rb_memcmp("OK\r", 3) == 0)		// 2637
		{
			at_res = RES_OK;
		}
		else if (d_read_rb.rb_memcmp("^RSSI:", 6) == 0)		// 880
		{
			at_res = RES_RSSI;
		}
		else if (d_read_rb.rb_memcmp("^MODE:", 6) == 0)		// 656
		{
			at_res = RES_MODE;
		}
		else if (d_read_rb.rb_memcmp("^CEND:", 6) == 0)		// 425
		{
			at_res = RES_CEND;
		}
		else if (d_read_rb.rb_memcmp("+CSSI:", 6) == 0)		// 416
		{
			at_res = RES_CSSI;
		}
		else if (d_read_rb.rb_memcmp("^ORIG:", 6) == 0)		// 408
		{
			at_res = RES_ORIG;
		}
		else if (d_read_rb.rb_memcmp("^CONF:", 6) == 0)		// 404
		{
			at_res = RES_CONF;
		}
		else if (d_read_rb.rb_memcmp("^CONN:", 6) == 0)		// 332
		{
			at_res = RES_CONN;
		}
		else if (d_read_rb.rb_memcmp("+CREG:", 6) == 0)		// 56
		{
			at_res = RES_CREG;
		}
		else if (d_read_rb.rb_memcmp("+COPS:", 6) == 0)		// 56
		{
			at_res = RES_COPS;
		}
		else if (d_read_rb.rb_memcmp("^SRVST:", 7) == 0)	// 35
		{
			at_res = RES_SRVST;
		}
		else if (d_read_rb.rb_memcmp("+CSQ:", 5) == 0)		// 28 init
		{
			at_res = RES_CSQ;
		}
		else if (d_read_rb.rb_memcmp("+CPIN:", 6) == 0)		// 28 init
		{
			at_res = RES_CPIN;
		}


		else if (d_read_rb.rb_memcmp("RING\r", 5) == 0)		// 15 incoming
		{
			at_res = RES_RING;
		}
		else if (d_read_rb.rb_memcmp("+CLIP:", 6) == 0)		// 15 incoming
		{
			at_res = RES_CLIP;
		}


		else if (d_read_rb.rb_memcmp("ERROR\r", 6) == 0)	// 12
		{
			at_res = RES_ERROR;
		}

		else if (d_read_rb.rb_memcmp("+CMTI:", 6) == 0)		// 8 SMS
		{
			at_res = RES_CMTI;
		}
		else if (d_read_rb.rb_memcmp("+CMGR:", 6) == 0)		// 8 SMS
		{
			at_res = RES_CMGR;
		}


		else if (d_read_rb.rb_memcmp("+CSSU:", 6) == 0)		// 2
		{
			at_res = RES_CSSU;
		}
		else if (d_read_rb.rb_memcmp("BUSY\r", 5) == 0)
		{
			at_res = RES_BUSY;
		}
		else if (d_read_rb.rb_memcmp("NO DIALTONE\r", 12) == 0)
		{
			at_res = RES_NO_DIALTONE;
		}
		else if (d_read_rb.rb_memcmp("NO CARRIER\r", 11) == 0)
		{
			at_res = RES_NO_CARRIER;
		}
		else if (d_read_rb.rb_memcmp("COMMAND NOT SUPPORT\r", 20) == 0)
		{
			at_res = RES_ERROR;
		}
		else if (d_read_rb.rb_memcmp("+CMS ERROR:", 11) == 0)
		{
			at_res = RES_CMS_ERROR;
		}
		else if (d_read_rb.rb_memcmp("^SMMEMFULL:", 11) == 0)
		{
			at_res = RES_SMMEMFULL;
		}
		else if (d_read_rb.rb_memcmp("> ", 2) == 0)
		{
			at_res = RES_SMS_PROMPT;
		}
		else if (d_read_rb.rb_memcmp("+CUSD:", 6) == 0)
		{
			at_res = RES_CUSD;
		}
		else if (d_read_rb.rb_memcmp("+CNUM:", 6) == 0)
		{
			at_res = RES_CNUM;
		}
		else if (d_read_rb.rb_memcmp("ERROR+CNUM:", 11) == 0)
		{
			at_res = RES_CNUM;
		}
		else
		{
			at_res = RES_UNKNOWN;
		}

		switch (at_res)
		{
			case RES_SMS_PROMPT:
				d_read_rb.rb_read_upd (2);
				break;

			case RES_CMGR:
				d_read_rb.rb_read_upd (d_read_iov[0].iov_len + d_read_iov[1].iov_len + 7);
				break;

			case RES_CSSI:
				d_read_rb.rb_read_upd (8);
				break;

			default:
				d_read_rb.rb_read_upd (d_read_iov[0].iov_len + d_read_iov[1].iov_len + 1);
				break;
		}

//		ast_debug (5, "[%s] receive result '%s'\n", id, at_res2str (at_res));

		return at_res;
	}

	return RES_PARSE_ERROR;
}
