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

static inline int at_wait (pvt_t* pvt, int* ms)
{
	int exception, outfd;

	outfd = ast_waitfor_n_fd (&pvt->data_fd, 1, ms, &exception);

	if (outfd < 0)
	{
		outfd = 0;
	}

	return outfd;
}

static inline int at_read (pvt_t* pvt)
{
	int	iovcnt;
	ssize_t	n;

	iovcnt = rb_write_iov (&pvt->d_read_rb, pvt->d_read_iov);

	if (iovcnt > 0)
	{
		n = readv (pvt->data_fd, pvt->d_read_iov, iovcnt);

		if (n < 0)
		{
			if (errno != EINTR && errno != EAGAIN)
			{
				ast_debug (1, "[%s] readv() error: %d\n", pvt->id, errno);
				return -1;
			}

			return 0;
		}
		else if (n == 0)
		{
			return -1;
		}

		rb_write_upd (&pvt->d_read_rb, n);


		ast_debug (5, "[%s] receive %zu byte, used %zu, free %zu, read %zu, write %zu\n", pvt->id, n,
			rb_used (&pvt->d_read_rb), rb_free (&pvt->d_read_rb), pvt->d_read_rb.read, pvt->d_read_rb.write);

		iovcnt = rb_read_all_iov (&pvt->d_read_rb, pvt->d_read_iov);

		if (iovcnt > 0)
		{
			if (iovcnt == 2)
			{
				ast_debug (5, "[%s] [%.*s%.*s]\n", pvt->id,
						(int) pvt->d_read_iov[0].iov_len, (char*) pvt->d_read_iov[0].iov_base,
							(int) pvt->d_read_iov[1].iov_len, (char*) pvt->d_read_iov[1].iov_base);
			}
			else
			{
				ast_debug (5, "[%s] [%.*s]\n", pvt->id,
						(int) pvt->d_read_iov[0].iov_len, (char*) pvt->d_read_iov[0].iov_base);
			}
		}

		return 0;
	}

	ast_debug (1, "[%s] receive buffer overflow\n", pvt->id);

	return -1;
}

static int at_read_result_iov (pvt_t* pvt)
{
	int	iovcnt = 0;
	int	res;
	size_t	s;

	if ((s = rb_used (&pvt->d_read_rb)) > 0)
	{
		if (!pvt->d_read_result)
		{
			if ((res = rb_memcmp (&pvt->d_read_rb, "\r\n", 2)) == 0)
			{
				rb_read_upd (&pvt->d_read_rb, 2);
				pvt->d_read_result = 1;

				return at_read_result_iov (pvt);
			}
			else if (res > 0)
			{
				if (rb_memcmp (&pvt->d_read_rb, "\n", 1) == 0)
				{
					ast_debug (5, "[%s] multiline response\n", pvt->id);
					rb_read_upd (&pvt->d_read_rb, 1);

					return at_read_result_iov (pvt);
				}

				if (rb_read_until_char_iov (&pvt->d_read_rb, pvt->d_read_iov, '\r') > 0)
				{
					s = pvt->d_read_iov[0].iov_len + pvt->d_read_iov[1].iov_len + 1;
				}

				rb_read_upd (&pvt->d_read_rb, s);

				return at_read_result_iov (pvt);
			}

			return 0;
		}
		else
		{
			if (rb_memcmp (&pvt->d_read_rb, "+CSSI:", 6) == 0)
			{
				if ((iovcnt = rb_read_n_iov (&pvt->d_read_rb, pvt->d_read_iov, 8)) > 0)
				{
					pvt->d_read_result = 0;
				}

				return iovcnt;
			}
			else if (rb_memcmp (&pvt->d_read_rb, "\r\n+CSSU:", 8) == 0)
			{
				rb_read_upd (&pvt->d_read_rb, 2);
				return at_read_result_iov (pvt);
			}
			else if (rb_memcmp (&pvt->d_read_rb, "> ", 2) == 0)
			{
				pvt->d_read_result = 0;
				return rb_read_n_iov (&pvt->d_read_rb, pvt->d_read_iov, 2);
			}
			else if (rb_memcmp (&pvt->d_read_rb, "+CMGR:", 6) == 0)
			{
				if ((iovcnt = rb_read_until_mem_iov (&pvt->d_read_rb, pvt->d_read_iov, "\n\r\nOK\r\n", 7)) > 0)
				{
					pvt->d_read_result = 0;
				}

				return iovcnt;
			}
			else if (rb_memcmp (&pvt->d_read_rb, "+CNUM:", 6) == 0)
			{
				if ((iovcnt = rb_read_until_mem_iov (&pvt->d_read_rb, pvt->d_read_iov, "\n\r\nOK\r\n", 7)) > 0)
				{
					pvt->d_read_result = 0;
				}

				return iovcnt;
			}
			else if (rb_memcmp (&pvt->d_read_rb, "ERROR+CNUM:", 11) == 0)
			{
				if ((iovcnt = rb_read_until_mem_iov (&pvt->d_read_rb, pvt->d_read_iov, "\n\r\nOK\r\n", 7)) > 0)
				{
					pvt->d_read_result = 0;
				}

				return iovcnt;
			}
			else
			{
				if ((iovcnt = rb_read_until_mem_iov (&pvt->d_read_rb, pvt->d_read_iov, "\r\n", 2)) > 0)
				{
					pvt->d_read_result = 0;
					s = pvt->d_read_iov[0].iov_len + pvt->d_read_iov[1].iov_len + 1;

					return rb_read_n_iov (&pvt->d_read_rb, pvt->d_read_iov, s);
				}
			}
		}
	}

	return 0;
}

static inline at_res_t at_read_result_classification (pvt_t* pvt, int iovcnt)
{
	at_res_t at_res;

	if (iovcnt > 0)
	{
		if (rb_memcmp (&pvt->d_read_rb, "^BOOT:", 6) == 0)		// 5115
		{
			at_res = RES_BOOT;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "OK\r", 3) == 0)		// 2637
		{
			at_res = RES_OK;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "^RSSI:", 6) == 0)		// 880
		{
			at_res = RES_RSSI;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "^MODE:", 6) == 0)		// 656
		{
			at_res = RES_MODE;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "^CEND:", 6) == 0)		// 425
		{
			at_res = RES_CEND;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "+CSSI:", 6) == 0)		// 416
		{
			at_res = RES_CSSI;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "^ORIG:", 6) == 0)		// 408
		{
			at_res = RES_ORIG;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "^CONF:", 6) == 0)		// 404
		{
			at_res = RES_CONF;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "^CONN:", 6) == 0)		// 332
		{
			at_res = RES_CONN;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "+CREG:", 6) == 0)		// 56
		{
			at_res = RES_CREG;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "+COPS:", 6) == 0)		// 56
		{
			at_res = RES_COPS;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "^SRVST:", 7) == 0)	// 35
		{
			at_res = RES_SRVST;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "+CSQ:", 5) == 0)		// 28 init
		{
			at_res = RES_CSQ;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "+CPIN:", 6) == 0)		// 28 init
		{
			at_res = RES_CPIN;
		}


		else if (rb_memcmp (&pvt->d_read_rb, "RING\r", 5) == 0)		// 15 incoming
		{
			at_res = RES_RING;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "+CLIP:", 6) == 0)		// 15 incoming
		{
			at_res = RES_CLIP;
		}


		else if (rb_memcmp (&pvt->d_read_rb, "ERROR\r", 6) == 0)	// 12
		{
			at_res = RES_ERROR;
		}

		else if (rb_memcmp (&pvt->d_read_rb, "+CMTI:", 6) == 0)		// 8 SMS
		{
			at_res = RES_CMTI;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "+CMGR:", 6) == 0)		// 8 SMS
		{
			at_res = RES_CMGR;
		}


		else if (rb_memcmp (&pvt->d_read_rb, "+CSSU:", 6) == 0)		// 2
		{
			at_res = RES_CSSU;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "BUSY\r", 5) == 0)
		{
			at_res = RES_BUSY;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "NO DIALTONE\r", 12) == 0)
		{
			at_res = RES_NO_DIALTONE;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "NO CARRIER\r", 11) == 0)
		{
			at_res = RES_NO_CARRIER;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "COMMAND NOT SUPPORT\r", 20) == 0)
		{
			at_res = RES_ERROR;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "+CMS ERROR:", 11) == 0)
		{
			at_res = RES_CMS_ERROR;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "^SMMEMFULL:", 11) == 0)
		{
			at_res = RES_SMMEMFULL;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "> ", 2) == 0)
		{
			at_res = RES_SMS_PROMPT;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "+CUSD:", 6) == 0)
		{
			at_res = RES_CUSD;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "+CNUM:", 6) == 0)
		{
			at_res = RES_CNUM;
		}
		else if (rb_memcmp (&pvt->d_read_rb, "ERROR+CNUM:", 11) == 0)
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
				rb_read_upd (&pvt->d_read_rb, 2);
				break;

			case RES_CMGR:
				rb_read_upd (&pvt->d_read_rb, pvt->d_read_iov[0].iov_len + pvt->d_read_iov[1].iov_len + 7);
				break;

			case RES_CSSI:
				rb_read_upd (&pvt->d_read_rb, 8);
				break;

			default:
				rb_read_upd (&pvt->d_read_rb, pvt->d_read_iov[0].iov_len + pvt->d_read_iov[1].iov_len + 1);
				break;
		}

//		ast_debug (5, "[%s] receive result '%s'\n", pvt->id, at_res2str (at_res));

		return at_res;
	}

	return RES_PARSE_ERROR;
}
