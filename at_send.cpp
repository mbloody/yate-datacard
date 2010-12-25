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
#include <stdarg.h>


int CardDevice::send_atcmd(const char * fmt, ...)
{
    char buf[1024];
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf(buf, 1023, fmt, ap);
    va_end(ap);
    
    return at_write_full(buf, len);
}



/*!
 * \brief Write to data socket
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

/*!
 * \brief Send the AT command
 */

int CardDevice::at_send_at()
{
	return send_atcmd((char*)"AT");
}

/*!
 * \brief Send ATA command
 */

int CardDevice::at_send_ata()
{
	return send_atcmd((char*)"ATA");
}

/*!
 * \brief Send the AT+CGMI command
 */

int CardDevice::at_send_cgmi()
{
	return send_atcmd((char*)"AT+CGMI");
}

/*!
 * \brief Send the AT+CGMM command
 */

int CardDevice::at_send_cgmm ()
{
	return send_atcmd((char*)"AT+CGMM");
}

/*!
 * \brief Send the AT+CGMR command
 */

int CardDevice::at_send_cgmr ()
{
	return send_atcmd((char*)"AT+CGMR");
}

/*!
 * \brief Send the AT+CGSN command
 */

int CardDevice::at_send_cgsn ()
{
	return send_atcmd((char*)"AT+CGSN");
}

/*!
 * \brief Send AT+CHUP command
 */

int CardDevice::at_send_chup ()
{
	return send_atcmd((char*)"AT+CHUP");
}

/*!
 * \brief Send AT+CIMI command
 */

int CardDevice::at_send_cimi ()
{
	return send_atcmd((char*)"AT+CIMI");
}

/*!
 * \brief Enable or disable calling line identification
 * \param status -- enable or disable calling line identification (should be 1 or 0)
 */

int CardDevice::at_send_clip (int status)
{
	return send_atcmd("AT+CLIP=%d", status ? 1 : 0);
}

/*!
 * \brief Send AT+CLVL command
 * \param volume -- level to send
 */

int CardDevice::at_send_clvl (int level)
{
	return send_atcmd("AT+CLVL=%d", level);
}

/*!
 * \brief Delete an SMS message
 * \param index -- the location of the requested message
 */

int CardDevice::at_send_cmgd (int index)
{
	return send_atcmd("AT+CMGD=%d", index);
}

/*!
 * \brief Set the SMS mode
 * \param mode -- the sms mode (0 = PDU, 1 = Text)
 */

int CardDevice::at_send_cmgf (int mode)
{
	return send_atcmd("AT+CMGF=%d", mode);
}

/*!
 * \brief Read an SMS message
 * \param index -- the location of the requested message
 */

int CardDevice::at_send_cmgr (int index)
{
	return send_atcmd("AT+CMGR=%d", index);
}

/*!
 * \brief Start sending an SMS message
 * \param number -- the destination of the message
 */

int CardDevice::at_send_cmgs (const int len)		// !!!!!!!!!
{
	return send_atcmd("AT+CMGS=%d", len);
}

/*!
 * \brief Send the text of an SMS message
 * \param msg -- the text of the message
 */

int CardDevice::at_send_sms_text(const char* msg)
{
	return send_atcmd("%s\x1a", msg);
}

/*!
 * \brief Setup SMS new message indication
 */

int CardDevice::at_send_cnmi ()
{
	return send_atcmd((char*)"AT+CNMI=2,1,0,0,0");
}

/*!
 * \brief Send the AT+CNUM command
 */

int CardDevice::at_send_cnum ()
{
	return send_atcmd((char*)"AT+CNUM");
}

/*!
 * \brief Send the AT+COPS? command
 */

int CardDevice::at_send_cops ()
{
	return send_atcmd((char*)"AT+COPS?");
}

/*!
 * \brief Send the AT+COPS= command
 */

int CardDevice::at_send_cops_init (int mode, int format)
{
	return send_atcmd("AT+COPS=%d,%d", mode, format);
}

/*!
 * \brief Send AT+CPIN? to ask the datacard if a pin code is required
 */

int CardDevice::at_send_cpin_test ()
{
	return send_atcmd((char*)"AT+CPIN?", 8);
}

/*!
 * \brief Set storage location for incoming SMS
 */



int CardDevice::at_send_cpms()
{
	return send_atcmd((char*)"AT+CPMS=\"ME\",\"ME\",\"ME\"");
}

/*!
 * \brief Send the AT+CREG? command
 */

int CardDevice::at_send_creg()
{
	return send_atcmd((char*)"AT+CREG?");
}

/*!
 * \brief Send the AT+CREG=n command
 * \param level -- verbose level of CREG
 */

int CardDevice::at_send_creg_init (int level)
{
	return send_atcmd("AT+CREG=%d", level);
}

/*!
 * \brief Send AT+CSCS.
 * \param encoding -- encoding for SMS text mode
 */

int CardDevice::at_send_cscs (const char* encoding)
{
	return send_atcmd("AT+CSCS=\"%s\"", encoding);
}

/*!
 * \brief Send AT+CSQ.
 */

int CardDevice::at_send_csq()
{
	return send_atcmd((char*)"AT+CSQ");
}

/*!
 * \brief Manage Supplementary Service Notification.
 * \param cssi the value to send (0 = disabled, 1 = enabled)
 * \param cssu the value to send (0 = disabled, 1 = enabled)
 */

int CardDevice::at_send_cssn (int cssi, int cssu)
{
	return send_atcmd("AT+CSSN=%d,%d", cssi, cssu);
}

/*!
 * \brief Send AT+CUSD.
 * \param code the CUSD code to send
 */

int CardDevice::at_send_cusd (const char* code)
{
	ssize_t res;
	char buf[256];


	if (cusd_use_7bit_encoding)
	{
		res = char_to_hexstr_7bit (code, strlen (code), buf, sizeof (buf));
		if (res <= 0)
		{
			Debug(DebugAll, "[%s] Error converting USSD code to PDU: %s\n", c_str(), code);
			return -1;
		}
	}
	else if (use_ucs2_encoding)
	{
		res = utf8_to_hexstr_ucs2 (code, strlen (code), buf, sizeof (buf));
		if (res <= 0)
		{
			Debug(DebugAll, "[%s] error converting USSD code to UCS-2: %s\n", c_str(), code);
			return -1;
		}
	}
	else
	{
		res = MIN (strlen (code), sizeof (buf));
		memmove (buf, code, res);
	}

	return send_atcmd("AT+CUSD=1,\"%s\",15", buf);
}

/*!
 * \brief Check device for audio capabilities
 */

int CardDevice::at_send_cvoice_test()
{
	return send_atcmd((char*)"AT^CVOICE?");
}

/*!
 * \brief Send ATD command
 */

int CardDevice::at_send_atd (const char* number)
{
	return send_atcmd("ATD%s;", number);
}

/*!
 * \brief Enable transmitting of audio to the debug port (tty)
 */

int CardDevice::at_send_ddsetex()
{
	return send_atcmd((char*)"AT^DDSETEX=2");
}

/*!
 * \brief Send a DTMF command
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
			return send_atcmd("AT^DTMF=1,%c", digit);
		default:
			return -1;
	}
}

/*!
 * \brief Send the ATE0 command
 */

int CardDevice::at_send_ate0()
{
	return send_atcmd((char*)"ATE0");
}

/*!
 * \brief Set the U2DIAG mode
 * \param mode -- the U2DIAG mode (0 = Only modem functions)
 */

int CardDevice::at_send_u2diag(int mode)
{
	return send_atcmd("AT^U2DIAG=%d", mode);
}

/*!
 * \brief Send the ATZ command
 */

int CardDevice::at_send_atz()
{
	return send_atcmd((char*)"ATZ");
}

/*!
 * \brief Send the AT+CLIR command
 * \param mode -- the CLIR mode
 */

int CardDevice::at_send_clir (int mode)
{
        return send_atcmd("AT+CLIR=%d", mode);
}

/*!
 * \brief Send the AT+CCWA command (disable)
 */

int CardDevice::at_send_ccwa_disable()
{
	return send_atcmd((char*)"AT+CCWA=0,0,1");
}

/*!
 * \brief Send the AT+CFUN command (Operation Mode Setting)
 */

int CardDevice::at_send_cfun (int fun, int rst)
{
	return send_atcmd("AT+CFUN=%d,%d", fun, rst);
}

/*!
 * \brief Set error reporing verbosity level
 * \param level -- the verbosity level
 */

int CardDevice::at_send_cmee (int level)
{
	return send_atcmd("AT+CMEE=%d", level);
}


int CardDevice::at_send_csmp (int fo, int vp, int pid, int dcs)
{
    return send_atcmd("AT+CSMP=%d,%d,%d,%d", fo, vp, pid, dcs);
}

/* vi: set ts=8 sw=4 sts=4 noet: */

