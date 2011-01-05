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

int CardDevice::at_send_at()
{
    return send_atcmd((char*)"AT");
}

int CardDevice::at_send_ata()
{
    return send_atcmd((char*)"ATA");
}

int CardDevice::at_send_cgmi()
{
    return send_atcmd((char*)"AT+CGMI");
}

int CardDevice::at_send_cgmm()
{
    return send_atcmd((char*)"AT+CGMM");
}

int CardDevice::at_send_cgmr()
{
    return send_atcmd((char*)"AT+CGMR");
}

int CardDevice::at_send_cgsn()
{
    return send_atcmd((char*)"AT+CGSN");
}

int CardDevice::at_send_chup()
{
    return send_atcmd((char*)"AT+CHUP");
}

int CardDevice::at_send_cimi()
{
    return send_atcmd((char*)"AT+CIMI");
}

int CardDevice::at_send_clip(int status)
{
    return send_atcmd("AT+CLIP=%d", status ? 1 : 0);
}

int CardDevice::at_send_clvl(int level)
{
    return send_atcmd("AT+CLVL=%d", level);
}

int CardDevice::at_send_cmgd(int index)
{
    return send_atcmd("AT+CMGD=%d", index);
}

int CardDevice::at_send_cmgf(int mode)
{
    return send_atcmd("AT+CMGF=%d", mode);
}

int CardDevice::at_send_cmgr(int index)
{
    return send_atcmd("AT+CMGR=%d", index);
}

int CardDevice::at_send_cmgs(const int len)
{
    return send_atcmd("AT+CMGS=%d", len);
}

int CardDevice::at_send_sms_text(const char* pdu)
{
    return send_atcmd("%s\x1a", pdu);
}

int CardDevice::at_send_cnmi()
{
    return send_atcmd((char*)"AT+CNMI=2,1,0,0,0");
}

int CardDevice::at_send_cnum()
{
    return send_atcmd((char*)"AT+CNUM");
}

int CardDevice::at_send_cops()
{
    return send_atcmd((char*)"AT+COPS?");
}

int CardDevice::at_send_cops_init(int mode, int format)
{
    return send_atcmd("AT+COPS=%d,%d", mode, format);
}

int CardDevice::at_send_cpin_test()
{
    return send_atcmd((char*)"AT+CPIN?", 8);
}

int CardDevice::at_send_cpms()
{
    return send_atcmd((char*)"AT+CPMS=\"ME\",\"ME\",\"ME\"");
}

int CardDevice::at_send_creg()
{
    return send_atcmd((char*)"AT+CREG?");
}

int CardDevice::at_send_creg_init(int level)
{
    return send_atcmd("AT+CREG=%d", level);
}

int CardDevice::at_send_csq()
{
    return send_atcmd((char*)"AT+CSQ");
}

int CardDevice::at_send_cssn(int cssi, int cssu)
{
    return send_atcmd("AT+CSSN=%d,%d", cssi, cssu);
}

int CardDevice::at_send_cusd(const char* code)
{
    ssize_t res;
    char buf[256];

    if(cusd_use_7bit_encoding)
    {
	res = char_to_hexstr_7bit(code, strlen(code), buf, sizeof(buf));
	if (res <= 0)
	{
	    Debug(DebugAll, "[%s] Error converting USSD code to PDU: %s\n", c_str(), code);
	    return -1;
	}
    }
    else if(use_ucs2_encoding)
    {
	res = utf8_to_hexstr_ucs2(code, strlen(code), buf, sizeof(buf));
	if (res <= 0)
	{
	    Debug(DebugAll, "[%s] error converting USSD code to UCS-2: %s\n", c_str(), code);
	    return -1;
	}
    }
    else
    {
	return send_atcmd("AT+CUSD=1,\"%s\",15", code);
    }

    return send_atcmd("AT+CUSD=1,\"%s\",15", buf);
}

int CardDevice::at_send_cvoice_test()
{
    return send_atcmd((char*)"AT^CVOICE?");
}

int CardDevice::at_send_atd(const char* number)
{
    return send_atcmd("ATD%s;", number);
}

int CardDevice::at_send_ddsetex()
{
    return send_atcmd((char*)"AT^DDSETEX=2");
}

int CardDevice::at_send_dtmf(char digit)
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

int CardDevice::at_send_ate0()
{
    return send_atcmd((char*)"ATE0");
}

int CardDevice::at_send_u2diag(int mode)
{
    return send_atcmd("AT^U2DIAG=%d", mode);
}

int CardDevice::at_send_atz()
{
    return send_atcmd((char*)"ATZ");
}

int CardDevice::at_send_clir(int mode)
{
    return send_atcmd("AT+CLIR=%d", mode);
}

int CardDevice::at_send_ccwa_disable()
{
    return send_atcmd((char*)"AT+CCWA=0,0,1");
}

int CardDevice::at_send_cfun(int fun, int rst)
{
    return send_atcmd("AT+CFUN=%d,%d", fun, rst);
}

int CardDevice::at_send_cmee(int level)
{
    return send_atcmd("AT+CMEE=%d", level);
}

/* vi: set ts=8 sw=4 sts=4 noet: */
