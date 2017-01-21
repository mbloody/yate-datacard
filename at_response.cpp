/**
 * at_response.cpp
 * This file is part of the Yate-datacard Project http://code.google.com/p/yate-datacard/
 * Yate datacard channel driver for Huawei UMTS modem
 *
 * Copyright (C) 2010-2011 MBloody
 *
 * Based on chan_datacard module for Asterisk
 *
 * Copyright (C) 2009-2010 Artem Makhutov <artem@makhutov.org>
 * Copyright (C) 2009-2010 Dmitry Vagin <dmitry2004@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
 */


#include "datacarddevice.h"
#include <stdio.h>
#include <string.h>

int CardDevice::at_response(char* str, at_res_t at_res)
{
    size_t len = strlen(str);

    switch(at_res)
    {
	case RES_BOOT:
	case RES_CONF:
	case RES_CSSI:
	case RES_CSSU:
	case RES_SRVST:
	    return 0;

	case RES_OK:
	    return at_response_ok();

	case RES_RSSI:
	    return at_response_rssi(str, len);

	case RES_MODE:
	    /* An error here is not fatal. Just keep going. */
	    at_response_mode(str, len);
	    return 0;

	case RES_ORIG:
	    return at_response_orig(str, len);

	case RES_CEND:
	    return at_response_cend(str, len);

	case RES_CONN:
	    return at_response_conn(str, len);

	case RES_CREG:
	    /* An error here is not fatal. Just keep going. */
	    at_response_creg(str, len);
	    return 0;

	case RES_COPS:
	    /* An error here is not fatal. Just keep going. */
	    at_response_cops(str, len);
	    return 0;

	case RES_CSQ:
	    return at_response_csq(str, len);

	case RES_CMS_ERROR:
	case RES_ERROR:
	    return at_response_error();

	case RES_RING:
	    return at_response_ring();

	case RES_SMMEMFULL:
	    return at_response_smmemfull();

	case RES_CLIP:
	    return at_response_clip(str, len);

	case RES_CMTI:
	    return at_response_cmti(str, len);

	case RES_CMGR:
	    return at_response_cmgr(str, len);

	case RES_SMS_PROMPT:
	    return at_response_sms_prompt();

	case RES_CUSD:
	    return at_response_cusd(str, len);

	case RES_BUSY:
	    return at_response_busy();

	case RES_NO_DIALTONE:
	    return at_response_no_dialtone();

	case RES_NO_CARRIER:
	    return at_response_no_carrier();

	case RES_CPIN:
	    return at_response_cpin(str, len);

	case RES_CNUM:
	    /* An error here is not fatal. Just keep going. */
	    at_response_cnum(str, len);
	    return 0;

    case RES_CPMS:
      return at_response_cpms(str, len);

	case RES_PARSE_ERROR:
	    Debug(DebugAll, "[%s] Error parsing result", c_str());
	    return -1;

	case RES_UNKNOWN:
	    if (m_lastcmd)
	    {
		switch(m_lastcmd->m_cmd)
		{
		    case CMD_AT_CGMI:
			Debug(DebugAll,  "[%s] Got AT_CGMI data (manufacturer info)", c_str());
			return at_response_cgmi(str, len);
		    case CMD_AT_CGMM:
			Debug(DebugAll,  "[%s] Got AT_CGMM data (model info)", c_str());
			return at_response_cgmm(str, len);
		    case CMD_AT_CGMR:
			Debug(DebugAll,  "[%s] Got AT+CGMR data (firmware info)", c_str());
			return at_response_cgmr(str, len);
		    case CMD_AT_CGSN:
			Debug(DebugAll,  "[%s] Got AT+CGSN data (IMEI number)", c_str());
			return at_response_cgsn (str, len);
		    case CMD_AT_CIMI:
			Debug(DebugAll,  "[%s] Got AT+CIMI data (IMSI number)", c_str());
			return at_response_cimi(str, len);
		    case CMD_AT_CMGR:
			Debug(DebugAll,  "[%s] Got AT+CMGR data (SMS PDU data)", c_str());
			return at_response_pdu(str, len);
		    default:
			Debug(DebugAll, "[%s] Ignoring unknown result: '%.*s'", c_str(), (int) len, str);
			break;
		}
	    }
	    else
	    {
		Debug(DebugAll, "[%s] Ignoring unknown result: '%.*s'", c_str(), (int) len, str);
	    }
	    break;
	}
	return 0;
}

int CardDevice::at_response_ok()
{
    if(m_lastcmd && (m_lastcmd->m_res == RES_OK))
    {
	switch (m_lastcmd->m_cmd)
	{
	    /* initilization stuff */
	    case CMD_AT:
		if(!m_initialized)
		{
		    if(m_reset_datacard)
			m_commandQueue.append(new ATCommand("ATZ", CMD_AT_Z));
		    else
			m_commandQueue.append(new ATCommand("ATE0", CMD_AT_E));
		}
		break;
		
	    case CMD_AT_Z:
	        m_commandQueue.append(new ATCommand("ATE0", CMD_AT_E));
		break;

	    case CMD_AT_E:
		if(!m_initialized)
		{
		    if(m_u2diag != -1)
		        m_commandQueue.append(new ATCommand("AT^U2DIAG=" + String(m_u2diag), CMD_AT_U2DIAG));
		    else
		        m_commandQueue.append(new ATCommand("AT+CGMI", CMD_AT_CGMI));
		}
		break;

	    case CMD_AT_U2DIAG:
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CGMI", CMD_AT_CGMI));
		break;

	    case CMD_AT_CGMI:
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CGMM", CMD_AT_CGMM));
		break;

	    case CMD_AT_CGMM:
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CGMR", CMD_AT_CGMR));
		break;

	    case CMD_AT_CGMR:
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CMEE=0", CMD_AT_CMEE));
		break;
		
	    case CMD_AT_CMEE:
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CGSN", CMD_AT_CGSN));
		break;

	    case CMD_AT_CGSN:
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CPIN?", CMD_AT_CPIN));
		break;

	    case CMD_AT_CIMI:
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+COPS=0,0", CMD_AT_COPS_INIT));
		break;

	    case CMD_AT_CPIN:
		if(!m_initialized)
		{
		    if(m_simstatus == 0)
		        m_commandQueue.append(new ATCommand("AT+CIMI", CMD_AT_CIMI));
		    else if(m_simstatus == 1 && (m_sim_pin.length() > 0) && (m_pincount == 0))
		    {
			m_pincount++;
			m_commandQueue.append(new ATCommand("AT+CPIN=" + m_sim_pin, CMD_AT_CPIN_ENTER));
		    }
		    else
		    {
			Debug(DebugAll, "[%s] Wrong SIM State", c_str());
			m_lastcmd->destruct();
			m_lastcmd = 0;
			return -1;
		    }

		}
		break;

	    case CMD_AT_CPIN_ENTER:
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CPIN?", CMD_AT_CPIN));
		break;

	    case CMD_AT_COPS_INIT:
		Debug(DebugAll, "[%s] Operator select parameters set", c_str());
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CREG=2", CMD_AT_CREG_INIT));
		break;

	    case CMD_AT_CREG_INIT:
		Debug(DebugAll,  "[%s] registration info enabled", c_str());
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CREG?", CMD_AT_CREG));
		break;

	    case CMD_AT_CREG:
		Debug(DebugAll, "[%s] registration query sent", c_str());
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CNUM", CMD_AT_CNUM));
		break;

	    case CMD_AT_CNUM:
		Debug(DebugAll, "[%s] Subscriber phone number query successed", c_str());
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT^CVOICE?", CMD_AT_CVOICE));
		break;

	    case CMD_AT_CVOICE:
		Debug(DebugAll, "[%s] Datacard has voice support", c_str());
		m_has_voice = 1;
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CLIP=1", CMD_AT_CLIP));
		break;

	    case CMD_AT_CLIP:
		Debug(DebugAll, "[%s] Calling line indication enabled", c_str());
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CSSN=1,1", CMD_AT_CSSN));
		break;

	    case CMD_AT_CSSN:
		Debug(DebugAll, "[%s] Supplementary Service Notification enabled successful", c_str());
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CMGF=0", CMD_AT_CMGF));
		break;

	    case CMD_AT_CMGF:
		Debug(DebugAll, "[%s] SMS PDU mode enabled", c_str());
		m_use_ucs2_encoding = 1;
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CPMS=\"ME\",\"ME\",\"ME\"", CMD_AT_CPMS));
		break;

	    case CMD_AT_CPMS:
		Debug(DebugAll,  "[%s] SMS storage location is established", c_str());
		if(!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CNMI=2,1,0,0,0", CMD_AT_CNMI));
		break;

	    case CMD_AT_CNMI:
		Debug(DebugAll, "[%s] SMS new message indication enabled", c_str());
		Debug(DebugAll, "[%s] Datacard has sms support", c_str());
		m_has_sms = 1;
		if(!m_initialized)
		{
		    m_commandQueue.append(new ATCommand("AT+CSQ", CMD_AT_CSQ));
		    m_initialized = 1;
		}
		break;
	    /* end initilization stuff */

	    case CMD_AT_A:
		Debug(DebugAll,  "[%s] Answer sent successfully", c_str());
		//FIXME: Clear audio bufer
		m_audio_buf.clear();
		m_commandQueue.append(new ATCommand("AT^DDSETEX=2", CMD_AT_DDSETEX));
		break;

	    case CMD_AT_CLIR:
		Debug(DebugAll, "[%s] CLIR sent successfully", c_str());
		if(m_lastcmd->get())
		{
		    String* number = static_cast<String*>(m_lastcmd->get());
		    m_commandQueue.append(new ATCommand("ATD" + *number  + ";", CMD_AT_D));
		}
		break;

	    case CMD_AT_D:
		Debug(DebugAll,  "[%s] Dial sent successfully", c_str());
		m_commandQueue.append(new ATCommand("AT^DDSETEX=2", CMD_AT_DDSETEX));
		break;

	    case CMD_AT_DDSETEX:
		Debug(DebugAll,  "[%s] AT^DDSETEX sent successfully", c_str());
		break;

	    case CMD_AT_CHUP:
		Debug(DebugAll,  "[%s] Successful hangup", c_str());
		break;

	    case CMD_AT_CMGS:
		Debug(DebugAll, "[%s] Successfully sent sms message", c_str());
		break;

	    case CMD_AT_DTMF:
		Debug(DebugAll, "[%s] DTMF sent successfully", c_str());
		break;

	    case CMD_AT_CUSD:
		Debug(DebugAll, "[%s] CUSD code sent successfully", c_str());
		break;

	    case CMD_AT_COPS:
		Debug(DebugAll, "[%s] Provider query successfully", c_str());
		break;

	    case CMD_AT_CMGR:
		Debug(DebugAll, "[%s] SMS message read successfully", c_str());
		if(m_auto_delete_sms)
		{
		    if(m_lastcmd->get())
		    {
			String* index = static_cast<String*>(m_lastcmd->get());
			m_commandQueue.append(new ATCommand("AT+CMGD=" + *index, CMD_AT_CMGD));
		    }
		}
		break;

	    case CMD_AT_CMGD:
		Debug(DebugAll, "[%s] SMS message deleted successfully", c_str());
		break;

	    case CMD_AT_CSQ:
		Debug(DebugAll, "[%s] Got signal strength result", c_str());
		break;
			
	    case CMD_AT_CCWA:
		Debug(DebugAll, "Call-Waiting disabled on device %s.", c_str());
		break;
			
	    case CMD_AT_CFUN:
		Debug(DebugAll, "[%s] CFUN sent successfully", c_str());
		break;

	    case CMD_AT_CLVL:
		if (m_volume_synchronized == 0)
		{
		    m_volume_synchronized = 1;
		    m_commandQueue.append(new ATCommand("AT+CLVL=5", CMD_AT_CLVL));
		}
		break;

	    default:
		Debug(DebugAll, "[%s] Received 'OK' for unhandled command '%s'", c_str(), at_cmd2str (m_lastcmd->m_cmd));
		break;
	}
	
	m_lastcmd->destruct();
	m_lastcmd = 0;

    }
    else if (m_lastcmd)
    {
	Debug(DebugAll, "[%s] Received 'OK' when expecting '%s', ignoring", c_str(), at_res2str(m_lastcmd->m_res));
    }
    else
    {
	Debug(DebugAll,  "[%s] Received unexpected 'OK'", c_str());
    }
    return 0;
}

int CardDevice::at_response_error()
{
    if(m_lastcmd && (m_lastcmd->m_res == RES_OK || m_lastcmd->m_res == RES_ERROR || m_lastcmd->m_res == RES_CMS_ERROR || m_lastcmd->m_res == RES_SMS_PROMPT))
    {
	switch (m_lastcmd->m_cmd)
	{
	    /* initilization stuff */
	    case CMD_AT:
	    case CMD_AT_Z:
	    case CMD_AT_E:
	    case CMD_AT_U2DIAG:
		Debug(DebugAll,  "[%s] Command '%s' failed", c_str(), at_cmd2str (m_lastcmd->m_cmd));
		goto e_return;

	    case CMD_AT_CGMI:
		Debug(DebugAll, "[%s] Getting manufacturer info failed", c_str());
		goto e_return;

	    case CMD_AT_CGMM:
		Debug(DebugAll,  "[%s] Getting model info failed", c_str());
		goto e_return;

	    case CMD_AT_CGMR:
		Debug(DebugAll,  "[%s] Getting firmware info failed", c_str());
		goto e_return;

	    case CMD_AT_CMEE:
		Debug(DebugAll,  "[%s] Setting error verbosity level failed", c_str());
		goto e_return;

	    case CMD_AT_CGSN:
		Debug(DebugAll,  "[%s] Getting IMEI number failed", c_str());
		goto e_return;

	    case CMD_AT_CIMI:
		Debug(DebugAll,  "[%s] Getting IMSI number failed", c_str());
		goto e_return;

	    case CMD_AT_CPIN:
		Debug(DebugAll,  "[%s] Error checking PIN state", c_str());
		goto e_return;

	    case CMD_AT_CPIN_ENTER:
		Debug(DebugAll,  "[%s] Error enter PIN code", c_str());
		goto e_return;

	    case CMD_AT_COPS_INIT:
		Debug(DebugAll,  "[%s] Error setting operator select parameters", c_str());
		goto e_return;

	    case CMD_AT_CREG_INIT:
		Debug(DebugAll, "[%s] Error enableling registration info", c_str());
		goto e_return;

	    case CMD_AT_CREG:
		Debug(DebugAll, "[%s] Error getting registration info", c_str());
		if (!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CNUM", CMD_AT_CNUM));
		break;

	    case CMD_AT_CNUM:
		Debug(DebugAll, "[%s] Error checking subscriber phone number", c_str());
		Debug(DebugAll, "Datacard %s needs to be reinitialized. The SIM card is not ready yet", c_str());
		goto e_return;

	    case CMD_AT_CVOICE:
		Debug(DebugAll, "[%s] Datacard has NO voice support", c_str());
		m_has_voice = 0;
		if (!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CMGF=0", CMD_AT_CMGF));
		break;

	    case CMD_AT_CLIP:
		Debug(DebugAll, "[%s] Error enabling calling line indication", c_str());
		goto e_return;

	    case CMD_AT_CSSN:
		Debug(DebugAll, "[%s] Error Supplementary Service Notification activation failed", c_str());
		goto e_return;

	    case CMD_AT_CMGF:
	    case CMD_AT_CPMS:
	    case CMD_AT_CNMI:
		Debug(DebugAll, "[%s] Command '%s' failed", c_str(), at_cmd2str (m_lastcmd->m_cmd));
		Debug(DebugAll, "[%s] No SMS support", c_str());
		m_has_sms = 0;
		if (!m_initialized)
		{
		    if (m_has_voice)
		    {
			m_commandQueue.append(new ATCommand("AT+CSQ", CMD_AT_CSQ));
			m_initialized = 1;
			Debug(DebugAll, "Datacard %s initialized and ready", c_str());
		    }
		    goto e_return;
		}
		break;

	    case CMD_AT_CSCS:
		Debug(DebugAll, "[%s] No UCS-2 encoding support", c_str());
		m_use_ucs2_encoding = 0;
		/* set SMS storage location */
		if (!m_initialized)
		    m_commandQueue.append(new ATCommand("AT+CPMS=\"ME\",\"ME\",\"ME\"", CMD_AT_CPMS));
		break;
	    /* end initilization stuff */

	    case CMD_AT_A:
		Debug(DebugAll, "[%s] Answer failed", c_str());
		Hangup(DATACARD_FAILURE);
		break;

	    case CMD_AT_CLIR:
		Debug(DebugAll, "[%s] Setting CLIR failed", c_str());
		/* continue dialing */
		if(m_lastcmd->get())
		{
		    String* number = static_cast<String*>(m_lastcmd->get());
		    m_commandQueue.append(new ATCommand("ATD" + *number + ";", CMD_AT_D));
		}
		break;

	    case CMD_AT_D:
		Debug(DebugAll, "[%s] Dial failed", c_str());
		m_outgoing = 0;
		m_needchup = 0;
		Hangup(DATACARD_CONGESTION);
		break;

	    case CMD_AT_DDSETEX:
		Debug(DebugAll, "[%s] AT^DDSETEX failed", c_str());
		break;

	    case CMD_AT_CHUP:
		Debug(DebugAll, "[%s] Error sending hangup, disconnecting", c_str());
		goto e_return;

	    case CMD_AT_CMGR:
		Debug(DebugAll, "[%s] Error reading SMS message", c_str());
		break;

	    case CMD_AT_CMGD:
		Debug(DebugAll, "[%s] Error deleting SMS message", c_str());
		break;

	    case CMD_AT_CMGS:
		Debug(DebugAll, "[%s] Error sending SMS message", c_str());
		break;

	    case CMD_AT_DTMF:
		Debug(DebugAll, "[%s] Error sending DTMF", c_str());
		break;

	    case CMD_AT_COPS:
		Debug(DebugAll, "[%s] Could not get provider name", c_str());
		break;

	    case CMD_AT_CLVL:
		Debug(DebugAll, "[%s] Error syncronizing audio level", c_str());
		m_volume_synchronized = 0;
		break;

	    case CMD_AT_CUSD:
		Debug(DebugAll, "[%s] Could not send USSD code", c_str());
		break;

	    default:
		Debug(DebugAll, "[%s] Received 'ERROR' for unhandled command '%s'", c_str(), at_cmd2str (m_lastcmd->m_cmd));
		break;
	}

	m_lastcmd->destruct();
	m_lastcmd = 0;
    }
    else if (m_lastcmd)
    {
	Debug(DebugAll, "[%s] Received 'ERROR' when expecting '%s', ignoring", c_str(), at_res2str (m_lastcmd->m_res));
    }
    else
    {
	Debug(DebugAll, "[%s] Received unexpected 'ERROR'", c_str());
    }

    return 0;

e_return:
    m_lastcmd->destruct();
    m_lastcmd = 0;

    return -1;
}

int CardDevice::at_response_cpms(char* str, size_t len)
{
  if ((m_cpms = at_parse_cpms(str, len)) == -1)
    {
      return -1;
    }
  Debug(DebugAll, "[%s] We have %d offline messages", c_str(), m_cpms);
  return 0;
}

int CardDevice::at_response_rssi(char* str, size_t len)
{
    if ((m_rssi = at_parse_rssi(str, len)) == -1)
    {
	return -1;
    }
	m_endpoint->onUpdateNetworkStatus(this);
    return 0;
}

int CardDevice::at_response_mode(char* str, size_t len)
{
    return at_parse_mode(str, len, &m_linkmode, &m_linksubmode);
}

int CardDevice::at_response_orig(char* str, size_t len)
{
    int call_index = 1;
    int call_type  = 0;

    /*
     * parse ORIG info in the following format:
     * ^ORIG:<call_index>,<call_type>
     */
 
    if (!sscanf(str, "^ORIG:%d,%d", &call_index, &call_type))
    {
	Debug(DebugAll, "[%s] Error parsing ORIG event '%s'", c_str(), str);
	return -1;
    }

    Debug(DebugAll, "[%s] Received call_index: %d", c_str(), call_index);
    Debug(DebugAll, "[%s] Received call_type:  %d", c_str(), call_type);
    if(m_conn)
	m_conn->onProgress();

    m_commandQueue.append(new ATCommand("AT+CLVL=1", CMD_AT_CLVL));
    m_volume_synchronized = 0;
    return 0;
}

int CardDevice::at_response_cend(char* str, size_t len)
{
    int call_index = 0;
    int duration   = 0;
    int end_status = 0;
    int cc_cause   = 0;

    /*
     * parse CEND info in the following format:
     * ^CEND:<call_index>,<duration>,<end_status>[,<cc_cause>]
     */

    if (!sscanf(str, "^CEND:%d,%d,%d,%d", &call_index, &duration, &end_status, &cc_cause))
    {
	Debug(DebugAll, "[%s] Could not parse all CEND parameters", c_str());
    }

    Debug(DebugAll, "[%s] CEND: call_index: %d", c_str(), call_index);
    Debug(DebugAll, "[%s] CEND: duration:   %d", c_str(), duration);
    Debug(DebugAll, "[%s] CEND: end_status: %d", c_str(), end_status);
    Debug(DebugAll, "[%s] CEND: cc_cause:   %d", c_str(), cc_cause);

    Debug(DebugAll, "[%s] Line disconnected", c_str());

    m_needchup = 0;
//TODO:

    if(m_conn)
    {
	Debug(DebugAll, "[%s] hanging up owner", c_str());

	int reason = getReason(end_status, cc_cause);

	if(Hangup(reason) == false)
	{
	    Debug(DebugAll, "[%s] Error on hangup...", c_str());
	    return -1;
	}
    }
    m_incoming = 0;
    m_outgoing = 0;
    m_needring = 0;

    return 0;
}

int CardDevice::at_response_conn(char* str, size_t len)
{
//TODO:
    int call_index = 1;
    int call_type =0;
    if (!sscanf (str, "^CONN:%d,%d", &call_index, &call_type))
    {
	Debug(DebugAll, "[%s] Error parsing CONN event '%s'", c_str(), str);
	return -1;
    }

    Debug(DebugAll, "[%s] Received call_index: %d", c_str(), call_index);
    Debug(DebugAll, "[%s] Received call_type:  %d", c_str(), call_type);

    if(m_outgoing)
    {
	Debug(DebugAll, "[%s] Remote end answered", c_str());
	//FIXME: Clear audio bufer
	m_audio_buf.clear();
	if(m_conn)
	    m_conn->onAnswered();
    }
    return 0;
}

int CardDevice::at_response_clip(char* str, size_t len)
{
    if (m_initialized && m_needring == 0)
    {
	m_incoming = 1;
	String clip = at_parse_clip(str, len);
	if (clip.null())
	    Debug(DebugAll, "[%s] Error parsing CLIP: %s", c_str(), str);
	if(incomingCall(clip) == false)
	{
	    Debug(DebugAll, "[%s] Unable to allocate channel for incoming call", c_str());
	    m_commandQueue.append(new ATCommand("AT+CHUP", CMD_AT_CHUP));
	    return -1;
	}
	m_needchup = 1;
	m_needring = 1;
    }
    return 0;
}

int CardDevice::at_response_ring()
{
    if(m_initialized)
    {
	/* We only want to syncronize volume on the first ring */
	if(!m_incoming)
	{
	    m_commandQueue.append(new ATCommand("AT+CLVL=1", CMD_AT_CLVL));
	    m_volume_synchronized = 0;
	}
	m_incoming = 1;
    }

    return 0;
}

int CardDevice::at_response_cmti(char* str, size_t len)
{
    int index = at_parse_cmti(str, len);

    if (index < 0)
    {
	Debug(DebugAll, "[%s] Error parsing incoming sms message alert", c_str());
	return -1;
    }

    Debug(DebugAll, "[%s] Incoming SMS message", c_str());
    if (m_disablesms)
        Debug(DebugAll, "[%s] SMS reception has been disabled in the configuration.", c_str());
    else
        m_commandQueue.append(new ATCommand("AT+CMGR=" + String(index), CMD_AT_CMGR, new String(index)));
    return 0;
}

int CardDevice::at_response_cmgr(char* str, size_t len)
{
//TODO:
    int stat = 0;
    int pdulen = 0;
    at_parse_cmgr(str, len, &stat, &pdulen);
    Debug(DebugAll, "got '%s' stat = %d, pdulen = %d", str, stat, pdulen);
    m_incoming_pdu = true;
    return 0;
}

int CardDevice::at_response_sms_prompt()
{
    if(m_lastcmd && (m_lastcmd->m_cmd == CMD_AT_CMGS))
    {
	if(m_lastcmd->get())
	{
	    String* text = static_cast<String*>(m_lastcmd->get());
	    at_send_sms_text((char*)text->safe());
	}
    }
    else
    {
	if(m_lastcmd)
	    Debug(DebugAll,  "[%s] Received sms prompt when expecting '%s' response to '%s', ignoring", c_str(), at_res2str (m_lastcmd->m_res), at_cmd2str(m_lastcmd->m_cmd));
	else
	    Debug(DebugAll, "[%s] Received unexpected sms prompt", c_str());
//FIXME: Send empty SMS text. To exit from SMS prompt.
	at_send_sms_text("");
    }
    return 0;
}

int CardDevice::at_response_cusd(char* str, size_t len)
{
//TODO:
    ssize_t res;
    String cusd;
    unsigned char dcs;
    char cusd_utf8_str[1024];

    if(at_parse_cusd(str, len, cusd, dcs))
    {
	Debug(DebugAll, "[%s] Error parsing CUSD: '%.*s'", c_str(), (int) len, str);
	return 0;
    }

    if((dcs == 0 || dcs == 15) && !m_cusd_use_ucs2_decoding)
    {
	res = hexstr_7bit_to_char(cusd.safe(), cusd.length(), cusd_utf8_str, sizeof (cusd_utf8_str));
	if (res > 0)
	{
	    cusd = cusd_utf8_str;
	}
	else
	{
	    Debug(DebugAll, "[%s] Error parsing CUSD (convert 7bit to ASCII): %s", c_str(), cusd.safe());
	    return -1;
	}
    }
    else
    {
	res = hexstr_ucs2_to_utf8(cusd.safe(), cusd.length(), cusd_utf8_str, sizeof (cusd_utf8_str));
	if (res > 0)
	{
	    cusd = cusd_utf8_str;
	}
	else
	{
	    Debug(DebugAll, "[%s] Error parsing CUSD (convert UCS-2 to UTF-8): %s", c_str(), cusd.safe());
	    return -1;
	}
    }
    Debug(DebugAll, "[%s] Got USSD response: '%s'", c_str(), cusd.safe());
    m_endpoint->onReceiveUSSD(this, cusd.safe());
    return 0;
}

int CardDevice::at_response_busy()
{
    m_needchup = 1;
    Hangup(DATACARD_BUSY);
    return 0;
}

int CardDevice::at_response_no_dialtone()
{
    Debug(DebugAll, "[%s] Datacard reports 'NO DIALTONE'", c_str());
    m_needchup = 1;
    Hangup(DATACARD_CONGESTION);
    return 0;
}

int CardDevice::at_response_no_carrier()
{
    Debug(DebugAll, "[%s] Datacard reports 'NO CARRIER'", c_str());
    m_needchup = 1;
    Hangup(DATACARD_CONGESTION);
    return 0;
}

int CardDevice::at_response_cpin(char* str, size_t len)
{
    m_simstatus = at_parse_cpin(str, len);
    return 0;
}

int CardDevice::at_response_smmemfull()
{
    Debug(DebugAll, "[%s] SMS storage is full", c_str());
    return 0;
}

int CardDevice::at_response_csq(char* str, size_t len)
{
    return at_parse_csq(str, len, &m_rssi);
}

int CardDevice::at_response_cnum(char* str, size_t len)
{
    String number = at_parse_cnum(str, len);
    if(!number.null())
    {
	m_number = number;
	return 0;
    }
    m_number = "Unknown";
    return -1;
}

int CardDevice::at_response_cops(char* str, size_t len)
{
    char* provider_name = at_parse_cops(str, len);
    if(provider_name)
    {
	m_provider_name = provider_name;
	return 0;
    }
    m_provider_name = "NONE";
    return -1;
}

int CardDevice::at_response_creg(char* str, size_t len)
{
    int d;
    char* lac;
    char* ci;

    m_commandQueue.append(new ATCommand("AT+COPS?", CMD_AT_COPS));

    if(at_parse_creg(str, len, &d, &m_gsm_reg_status, &lac, &ci))
    {
	Debug(DebugAll, "[%s] Error parsing CREG: '%.*s'", c_str(), (int) len, str);
	return 0;
    }

    if(d)
	m_gsm_registered = 1;
    else
	m_gsm_registered = 0;

    if(lac)
	m_location_area_code = lac;
    if(ci)
	m_cell_id = ci;
	m_endpoint->onUpdateNetworkStatus(this);
    return 0;
}

int CardDevice::at_response_cgmi(char* str, size_t len)
{
    m_manufacturer.assign(str,len);
    return 0;
}

int CardDevice::at_response_cgmm(char* str, size_t len)
{
    m_model.assign(str,len);
    if(m_model == "E1550"|| m_model == "E1750" || m_model == "E160X" || m_model == "E173" || m_model == "E352")
    {
	m_cusd_use_7bit_encoding = 1;
	m_cusd_use_ucs2_decoding = 0;
    }
    return 0;
}

int CardDevice::at_response_cgmr(char* str, size_t len)
{
    m_firmware.assign(str,len);
    return 0;
}

int CardDevice::at_response_cgsn(char* str, size_t len)
{
    m_imei.assign(str,len);
    return 0;
}

int CardDevice::at_response_cimi(char* str, size_t len)
{
    m_imsi.assign(str,len);
    return 0;
}

int CardDevice::at_response_pdu(char* str, size_t len)
{
    //FIXME: check error in parsing
    if(m_incoming_pdu)
    {
	m_incoming_pdu = false;
	if(!receiveSMS(str, len))
	{
	    Debug(DebugAll, "[%s] Error parse SMS message", c_str());
	    return 1;
	}
	Debug(DebugAll, "[%s] Successfully parse SMS message", c_str());
    }
    return 0;
}

/* vi: set ts=8 sw=4 sts=4 noet: */
