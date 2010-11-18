/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

/*
   Copyright (C) 2009 - 2010 Artem Makhutov
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
*/
#include "datacarddevice.h"
#include <stdio.h>
#include <string.h>


/*!
 * \brief Write to data socket
 * \param pvt -- pvt structure
 * \param buf -- buffer to write (null terminated)
 *
 * This function will write characters from buf. The buffer must be null terminated.
 *
 * \retval -1 error
 * \retval  0 success
 */

int CardDevice::at_write(char* buf)
{
	return at_write_full(buf, strlen (buf));
}


/*!
 * \brief Write to data socket
 * \param pvt -- pvt structure
 * \param buf -- buffer to write
 * \param count -- number of bytes to write
 *
 * This function will write count characters from buf. It will always write
 * count chars unless it encounters an error.
 *
 * \retval -1 error
 * \retval  0 success
 */

int CardDevice::at_write_full (char* buf, size_t count)
{
	char*	p = buf;
	ssize_t	out_count;

    Debug(DebugAll, "[%s] [%.*s]\n", c_str(), (int) count, buf);

	while (count > 0)
	{
		if ((out_count = write (m_data_fd, p, count)) == -1)
		{
			Debug(DebugAll, "[%s] write() error: %d\n", c_str(), errno);
			return -1;
		}

		count -= out_count;
		p += out_count;
	}

	return 0;
}

/*!
 * \brief Send the AT command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_at()
{
	return at_write_full("AT\r", 3);
}

/*!
 * \brief Send ATA command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_ata()
{
	return at_write_full("ATA\r", 4);
}

/*!
 * \brief Send the AT+CGMI command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cgmi()
{
	return at_write_full("AT+CGMI\r", 8);
}

/*!
 * \brief Send the AT+CGMM command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cgmm ()
{
	return at_write_full("AT+CGMM\r", 8);
}

/*!
 * \brief Send the AT+CGMR command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cgmr ()
{
	return at_write_full("AT+CGMR\r", 8);
}

/*!
 * \brief Send the AT+CGSN command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cgsn ()
{
	return at_write_full("AT+CGSN\r", 8);
}

/*!
 * \brief Send AT+CHUP command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_chup ()
{
	return at_write_full("AT+CHUP\r", 8);
}

/*!
 * \brief Send AT+CIMI command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cimi ()
{
	return at_write_full("AT+CIMI\r", 8);
}

/*!
 * \brief Enable or disable calling line identification
 * \param pvt -- pvt structure
 * \param status -- enable or disable calling line identification (should be 1 or 0)
 */

int CardDevice::at_send_clip (int status)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+CLIP=%d\r", status ? 1 : 0);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Send AT+CLVL command
 * \param pvt -- pvt structure
 * \param volume -- level to send
 */

int CardDevice::at_send_clvl (int level)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+CLVL=%d\r", level);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Delete an SMS message
 * \param pvt -- pvt structure
 * \param index -- the location of the requested message
 */

int CardDevice::at_send_cmgd (int index)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+CMGD=%d\r", index);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Set the SMS mode
 * \param pvt -- pvt structure
 * \param mode -- the sms mode (0 = PDU, 1 = Text)
 */

int CardDevice::at_send_cmgf (int mode)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+CMGF=%d\r", mode);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Read an SMS message
 * \param pvt -- pvt structure
 * \param index -- the location of the requested message
 */

int CardDevice::at_send_cmgr (int index)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+CMGR=%d\r", index);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Start sending an SMS message
 * \param pvt -- pvt structure
 * \param number -- the destination of the message
 */

int CardDevice::at_send_cmgs (const char* number)		// !!!!!!!!!
{
    //TODO:
    return 0;
/*	ssize_t	res;
	char*	p;

	memmove (d_send_buf, "AT+CMGS=\"", 9);
	p = d_send_buf + 9;

//	if (use_ucs2_encoding)
	{
		res = utf8_to_hexstr_ucs2 (number, strlen (number), p, sizeof (d_send_buf) - 9 - 3);
		if (res <= 0)
		{
			ast_log (LOG_ERROR, "[%s] Error converting SMS number to UCS-2: %s\n", id, number);
			return -1;
		}
		p += res;
		memmove (p, "\"\r", 3);
	}
//	else
	{
//		snprintf (d_send_buf, sizeof (d_send_buf), "AT+CMGS=\"%s\"\r", number);
	}

	return at_write(d_send_buf);
*/
}

/*!
 * \brief Send the text of an SMS message
 * \param pvt -- pvt structure
 * \param msg -- the text of the message
 */

int CardDevice::at_send_sms_text (const char* msg)
{
    //TODO:
    return 0;
/*
	ssize_t	res;

	if (use_ucs2_encoding)
	{
		res = utf8_to_hexstr_ucs2 (msg, strlen (msg), d_send_buf, 280 + 1);
		if (res < 0)
		{
			ast_log (LOG_ERROR, "[%s] Error converting SMS to UCS-2: '%s'\n", id, msg);
			res = 0;
		}
		d_send_buf[res] = 0x1a;
		d_send_size = res + 1;
	}
	else
	{
		d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "%.160s\x1a", msg);
	}

	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
*/
}

/*!
 * \brief Setup SMS new message indication
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cnmi ()
{
	return at_write_full("AT+CNMI=2,1,0,0,0\r", 18);
}

/*!
 * \brief Send the AT+CNUM command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cnum ()
{
	return at_write_full("AT+CNUM\r", 8);
}

/*!
 * \brief Send the AT+COPS? command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cops ()
{
	return at_write_full("AT+COPS?\r", 9);
}

/*!
 * \brief Send the AT+COPS= command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cops_init (int mode, int format)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+COPS=%d,%d\r", mode, format);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Send AT+CPIN? to ask the datacard if a pin code is required
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cpin_test ()
{
	return at_write_full("AT+CPIN?\r", 9);
}

/*!
 * \brief Set storage location for incoming SMS
 * \param pvt -- pvt structure
 */

/*
:TODO
int CardDevice::at_send_cpms ()
{
	return at_write_full("AT+CPMS=\"ME\",\"ME\",\"ME\"\r", 23);
}
*/
/*!
 * \brief Send the AT+CREG? command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_creg()
{
	return at_write_full("AT+CREG?\r", 9);
}

/*!
 * \brief Send the AT+CREG=n command
 * \param pvt -- pvt structure
 * \param level -- verbose level of CREG
 */

int CardDevice::at_send_creg_init (int level)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+CREG=%d\r", level);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Send AT+CSCS.
 * \param pvt -- pvt structure
 * \param encoding -- encoding for SMS text mode
 */

int CardDevice::at_send_cscs (const char* encoding)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+CSCS=\"%s\"\r", encoding);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Send AT+CSQ.
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_csq()
{
	return at_write_full("AT+CSQ\r", 7);
}

/*!
 * \brief Manage Supplementary Service Notification.
 * \param pvt -- pvt structure
 * \param cssi the value to send (0 = disabled, 1 = enabled)
 * \param cssu the value to send (0 = disabled, 1 = enabled)
 */

int CardDevice::at_send_cssn (int cssi, int cssu)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+CSSN=%d,%d\r", cssi, cssu);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Send AT+CUSD.
 * \param pvt -- pvt structure
 * \param code the CUSD code to send
 */

int CardDevice::at_send_cusd (const char* code)
{
    //TODO:
    return 0;
/*
	ssize_t		res;
	char*		p;

	memmove (d_send_buf, "AT+CUSD=1,\"", 11);
	p = d_send_buf + 11;

	if (cusd_use_7bit_encoding)
	{
		res = char_to_hexstr_7bit (code, strlen (code), p, sizeof (d_send_buf) - 11 - 6);
		if (res <= 0)
		{
			ast_log (LOG_ERROR, "[%s] Error converting USSD code to PDU: %s\n", id, code);
			return -1;
		}
	}
	else if (use_ucs2_encoding)
	{
		res = utf8_to_hexstr_ucs2 (code, strlen (code), p, sizeof (d_send_buf) - 11 - 6);
		if (res <= 0)
		{
			ast_log (LOG_ERROR, "[%s] error converting USSD code to UCS-2: %s\n", id, code);
			return -1;
		}
	}
	else
	{
		res = MIN (strlen (code), sizeof (d_send_buf) - 11 - 6);
		memmove (p, code, res);
	}

	p += res;
	memmove (p, "\",15\r", 6);
	d_send_size = p - d_send_buf + 6;

	return at_write_full(d_send_buf, d_send_size);
*/
}

/*!
 * \brief Check device for audio capabilities
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cvoice_test()
{
	return at_write_full("AT^CVOICE?\r", 11);
}

/*!
 * \brief Send ATD command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_atd (const char* number)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "ATD%s;\r", number);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Enable transmitting of audio to the debug port (tty)
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_ddsetex()
{
	return at_write_full("AT^DDSETEX=2\r", 13);
}

/*!
 * \brief Send a DTMF command
 * \param pvt -- pvt structure
 * \param digit -- the dtmf digit to send
 * \return the result of at_write() or -1 on an invalid digit being sent
 */

int CardDevice::at_send_dtmf (char digit)
{
	switch (digit)
	{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '*':
		case '#':
			d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT^DTMF=1,%c\r", digit);
			return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));

		default:
			return -1;
	}
}

/*!
 * \brief Send the ATE0 command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_ate0()
{
	return at_write_full("ATE0\r", 5);
}

/*!
 * \brief Set the U2DIAG mode
 * \param pvt -- pvt structure
 * \param mode -- the U2DIAG mode (0 = Only modem functions)
 */

int CardDevice::at_send_u2diag (int mode)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT^U2DIAG=%d\r", mode);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Send the ATZ command
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_atz()
{
	return at_write_full("ATZ\r", 4);
}

/*!
 * \brief Send the AT+CLIR command
 * \param pvt -- pvt structure
 * \param mode -- the CLIR mode
 */

int CardDevice::at_send_clir (int mode)
{
        d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+CLIR=%d\r", mode);
        return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Send the AT+CCWA command (disable)
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_ccwa_disable()
{
	return at_write_full("AT+CCWA=0,0,1\r", 14);
}

/*!
 * \brief Send the AT+CFUN command (Operation Mode Setting)
 * \param pvt -- pvt structure
 */

int CardDevice::at_send_cfun (int fun, int rst)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+CFUN=%d,%d\r", fun, rst);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}

/*!
 * \brief Set error reporing verbosity level
 * \param pvt -- pvt structure
 * \param level -- the verbosity level
 */

int CardDevice::at_send_cmee (int level)
{
	d_send_size = snprintf (d_send_buf, sizeof (d_send_buf), "AT+CMEE=%d\r", level);
	return at_write_full(d_send_buf, MIN (d_send_size, sizeof (d_send_buf) - 1));
}
