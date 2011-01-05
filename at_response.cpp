/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

#include "datacarddevice.h"
#include <stdio.h>

int CardDevice::at_response(char* str, at_res_t at_res)
{
    size_t len = strlen(str);
    at_queue_t*	e;

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

	case RES_PARSE_ERROR:
	    Debug(DebugAll, "[%s] Error parsing result", c_str());
	    return -1;

	case RES_UNKNOWN:
	    if ((e = at_fifo_queue_head()))
	    {
		switch(e->cmd)
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
    at_queue_t* e;

    if ((e = at_fifo_queue_head()) && (e->res == RES_OK))
    {
	switch (e->cmd)
	{
	    /* initilization stuff */
	    case CMD_AT:
		if(!initialized)
		{
		    if(m_reset_datacard)
		    {
			if(at_send_atz() || at_fifo_queue_add(CMD_AT_Z, RES_OK))
			{
			    Debug(DebugAll, "[%s] Error reset datacard", c_str());
			    goto e_return;
			}
		    }
		    else
		    {
			if(at_send_ate0() || at_fifo_queue_add(CMD_AT_E, RES_OK))
			{
			    Debug(DebugAll, "[%s] Error disabling echo", c_str());
			    goto e_return;
			}
		    }
		}
		break;
		
	    case CMD_AT_Z:
		if(at_send_ate0() || at_fifo_queue_add(CMD_AT_E, RES_OK))
		{
		    Debug(DebugAll, "[%s] Error disabling echo", c_str());
		    goto e_return;
		}
		break;

	    case CMD_AT_E:
		if(!initialized)
		{
		    if(m_u2diag != -1)
		    {
			if(at_send_u2diag(m_u2diag) || at_fifo_queue_add(CMD_AT_U2DIAG, RES_OK))
			{
			    Debug(DebugAll, "[%s] Error setting U2DIAG", c_str());
			    goto e_return;
			}
		    }
		    else
		    {
			if(at_send_cgmi() || at_fifo_queue_add(CMD_AT_CGMI, RES_OK))
			{
			    Debug(DebugAll, "[%s] Error asking datacard for manufacturer info", c_str());
			    goto e_return;
			}
		    }
		}
		break;
	    case CMD_AT_U2DIAG:
		if(!initialized)
		{
		    if(at_send_cgmi() || at_fifo_queue_add(CMD_AT_CGMI, RES_OK))
		    {
						Debug(DebugAll, "[%s] Error asking datacard for manufacturer info", c_str());
						goto e_return;
		    }
		}
		break;

	    case CMD_AT_CGMI:
		if(!initialized)
		{
		    if(at_send_cgmm() || at_fifo_queue_add(CMD_AT_CGMM, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error asking datacard for model info", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CGMM:
		if(!initialized)
		{
		    if(at_send_cgmr() || at_fifo_queue_add(CMD_AT_CGMR, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error asking datacard for firmware info", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CGMR:
		if(!initialized)
		{
		    if(at_send_cmee(0) || at_fifo_queue_add(CMD_AT_CMEE, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error setting error verbosity level", c_str());
			goto e_return;
		    }
		}
		break;
		
	    case CMD_AT_CMEE:
		if(!initialized)
		{
		    if(at_send_cgsn() || at_fifo_queue_add(CMD_AT_CGSN, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error asking datacard for IMEI number", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CGSN:
		if(!initialized)
		{
		    if(at_send_cimi() || at_fifo_queue_add(CMD_AT_CIMI, RES_OK))
		    {
			Debug(DebugAll,  "[%s] Error asking datacard for IMSI number", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CIMI:
		if(!initialized)
		{
		    if(at_send_cpin_test() || at_fifo_queue_add(CMD_AT_CPIN, RES_OK))
		    {
			Debug(DebugAll,  "[%s] Error asking datacard for PIN state", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CPIN:
		if(!initialized)
		{
		    if(at_send_cops_init(0, 0) || at_fifo_queue_add(CMD_AT_COPS_INIT, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error setting operator select parameters", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_COPS_INIT:
		Debug(DebugAll, "[%s] Operator select parameters set", c_str());
		if(!initialized)
		{
		    if(at_send_creg_init(2) || at_fifo_queue_add(CMD_AT_CREG_INIT, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error enabeling registration info", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CREG_INIT:
		Debug(DebugAll,  "[%s] registration info enabled", c_str());
		if(!initialized)
		{
		    if(at_send_creg() || at_fifo_queue_add(CMD_AT_CREG, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error sending registration query", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CREG:
		Debug(DebugAll, "[%s] registration query sent", c_str());
		if(!initialized)
		{
		    if(at_send_cnum() || at_fifo_queue_add(CMD_AT_CNUM, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error checking subscriber phone number", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CNUM:
		Debug(DebugAll, "[%s] Subscriber phone number query successed", c_str());
		if(!initialized)
		{
		    if(at_send_cvoice_test() || at_fifo_queue_add(CMD_AT_CVOICE, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error checking voice capabilities", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CVOICE:
		Debug(DebugAll, "[%s] Datacard has voice support", c_str());
		has_voice = 1;
		if(!initialized)
		{
		    if(at_send_clip(1) || at_fifo_queue_add(CMD_AT_CLIP, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error enabling calling line notification", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CLIP:
		Debug(DebugAll, "[%s] Calling line indication enabled", c_str());
		if(!initialized)
		{
		    if(at_send_cssn(1, 1) || at_fifo_queue_add(CMD_AT_CSSN, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error activating Supplementary Service Notification", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CSSN:
		Debug(DebugAll, "[%s] Supplementary Service Notification enabled successful", c_str());
		if(!initialized)
		{
		    /* set the SMS operating mode to PDU mode */
		    if (at_send_cmgf(0) || at_fifo_queue_add(CMD_AT_CMGF, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error setting CMGF", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CMGF:
		Debug(DebugAll, "[%s] SMS PDU mode enabled", c_str());
		use_ucs2_encoding = 1;
		if(!initialized)
		{
		    /* set SMS storage location */
		    if(at_send_cpms() || at_fifo_queue_add(CMD_AT_CPMS, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error setting CPMS", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CPMS:
		Debug(DebugAll,  "[%s] SMS storage location is established", c_str());
		if(!initialized)
		{
		    /* turn on SMS new message indication */
		    if(at_send_cnmi() || at_fifo_queue_add(CMD_AT_CNMI, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error sending CNMI", c_str());
			goto e_return;
		    }
		}
		break;

	    case CMD_AT_CNMI:
		Debug(DebugAll, "[%s] SMS new message indication enabled", c_str());
		Debug(DebugAll, "[%s] Datacard has sms support", c_str());
		has_sms = 1;
		if(!initialized)
		{
		    if(at_send_csq() || at_fifo_queue_add(CMD_AT_CSQ, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error querying signal strength", c_str());
			goto e_return;
		    }
		    initialized = 1;
		    Debug(DebugAll, "Datacard %s initialized and ready", c_str());
		}
		break;
	    /* end initilization stuff */

	    case CMD_AT_A:
		Debug(DebugAll,  "[%s] Answer sent successfully", c_str());
		if(at_send_ddsetex() || at_fifo_queue_add(CMD_AT_DDSETEX, RES_OK))
		{
		    Debug(DebugAll, "[%s] Error sending AT^DDSETEX", c_str());
		    goto e_return;
		}
		break;

	    case CMD_AT_CLIR:
		Debug(DebugAll, "[%s] CLIR sent successfully", c_str());
		if(e->ptype != 0 || at_send_atd((char*)(e->param.data)) || at_fifo_queue_add (CMD_AT_D, RES_OK))
		{
		    Debug(DebugAll, "[%s] Error sending ATD command", c_str());
		    goto e_return;
		}
		break;

	    case CMD_AT_D:
		Debug(DebugAll,  "[%s] Dial sent successfully", c_str());
		if(at_send_ddsetex() || at_fifo_queue_add(CMD_AT_DDSETEX, RES_OK))
		{
		    Debug(DebugAll, "[%s] Error sending AT^DDSETEX", c_str());
		    goto e_return;
		}
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
		if(m_auto_delete_sms && e->ptype == 1)
		{
		    if(at_send_cmgd(e->param.num) || at_fifo_queue_add(CMD_AT_CMGD, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error sending CMGD to delete SMS message", c_str());
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
		if (volume_synchronized == 0)
		{
		    volume_synchronized = 1;

		    if(at_send_clvl(5) || at_fifo_queue_add(CMD_AT_CLVL, RES_OK))
		    {
			Debug(DebugAll, "[%s] Error syncronizing audio level (part 2/2)", c_str());
			goto e_return;
		    }
		}
		break;

	    default:
		Debug(DebugAll, "[%s] Received 'OK' for unhandled command '%s'", c_str(), at_cmd2str (e->cmd));
		break;
	}

	at_fifo_queue_rem();
    }
    else if (e)
    {
	Debug(DebugAll, "[%s] Received 'OK' when expecting '%s', ignoring", c_str(), at_res2str (e->res));
    }
    else
    {
	Debug(DebugAll,  "[%s] Received unexpected 'OK'", c_str());
    }
    return 0;

e_return:
    at_fifo_queue_rem();
    return -1;
}

int CardDevice::at_response_error()
{
	at_queue_t* e;

	if ((e = at_fifo_queue_head()) && (e->res == RES_OK || e->res == RES_ERROR ||
			e->res == RES_CMS_ERROR || e->res == RES_SMS_PROMPT))
	{
		switch (e->cmd)
		{
        		/* initilization stuff */
			case CMD_AT:
			case CMD_AT_Z:
			case CMD_AT_E:
			case CMD_AT_U2DIAG:
				Debug(DebugAll,  "[%s] Command '%s' failed", c_str(), at_cmd2str (e->cmd));
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

			case CMD_AT_COPS_INIT:
				Debug(DebugAll,  "[%s] Error setting operator select parameters", c_str());
				goto e_return;

			case CMD_AT_CREG_INIT:
				Debug(DebugAll, "[%s] Error enableling registration info", c_str());
				goto e_return;

			case CMD_AT_CREG:
				Debug(DebugAll, "[%s] Error getting registration info", c_str());

				if (!initialized)
				{
					if (at_send_cnum() || at_fifo_queue_add (CMD_AT_CNUM, RES_OK))
					{
						Debug(DebugAll, "[%s] Error checking subscriber phone number", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CNUM:
				Debug(DebugAll, "[%s] Error checking subscriber phone number", c_str());
				Debug(DebugAll, "Datacard %s needs to be reinitialized. The SIM card is not ready yet", c_str());
				goto e_return;

			case CMD_AT_CVOICE:
				Debug(DebugAll, "[%s] Datacard has NO voice support", c_str());

				has_voice = 0;

				if (!initialized)
				{
					if (at_send_cmgf (1) || at_fifo_queue_add (CMD_AT_CMGF, RES_OK))
					{
						Debug(DebugAll, "[%s] Error setting CMGF", c_str());
						goto e_return;
					}
				}
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
				Debug(DebugAll, "[%s] Command '%s' failed", c_str(), at_cmd2str (e->cmd));
				Debug(DebugAll, "[%s] No SMS support", c_str());

				has_sms = 0;

				if (!initialized)
				{
					if (has_voice)
					{
						if (at_send_csq() || at_fifo_queue_add (CMD_AT_CSQ, RES_OK))
						{
							Debug(DebugAll, "[%s] Error querying signal strength", c_str());
							goto e_return;
						}

						initialized = 1;
						Debug(DebugAll, "Datacard %s initialized and ready", c_str());
					}

					goto e_return;
				}
				break;

			case CMD_AT_CSCS:
				Debug(DebugAll, "[%s] No UCS-2 encoding support", c_str());

				use_ucs2_encoding = 0;

				if (!initialized)
				{
					/* set SMS storage location */
					if (at_send_cpms() || at_fifo_queue_add (CMD_AT_CPMS, RES_OK))
					{
						Debug(DebugAll, "[%s] Error setting SMS storage location", c_str());
						goto e_return;
					}
				}
				break;

			/* end initilization stuff */

			case CMD_AT_A:
				Debug(DebugAll, "[%s] Answer failed", c_str());
//TODO:
//				channel_queue_hangup(0);
				Hangup(DATACARD_FAILURE);
				break;

			case CMD_AT_CLIR:
				Debug(DebugAll, "[%s] Setting CLIR failed", c_str());

				/* continue dialing */
				if (e->ptype != 0 || at_send_atd((char*)e->param.data) || at_fifo_queue_add (CMD_AT_D, RES_OK))
				{
					Debug(DebugAll, "[%s] Error sending ATD command", c_str());
					goto e_return;
				}
				break;

			case CMD_AT_D:
				Debug(DebugAll, "[%s] Dial failed", c_str());
				outgoing = 0;
				needchup = 0;
//TODO:
//				channel_queue_control (AST_CONTROL_CONGESTION);
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
				volume_synchronized = 0;
				break;

			case CMD_AT_CUSD:
				Debug(DebugAll, "[%s] Could not send USSD code", c_str());
				break;

			default:
				Debug(DebugAll, "[%s] Received 'ERROR' for unhandled command '%s'", c_str(), at_cmd2str (e->cmd));
				break;
		}

		at_fifo_queue_rem();
	}
	else if (e)
	{
		Debug(DebugAll, "[%s] Received 'ERROR' when expecting '%s', ignoring", c_str(), at_res2str (e->res));
	}
	else
	{
		Debug(DebugAll, "[%s] Received unexpected 'ERROR'", c_str());
	}

	return 0;

e_return:
	at_fifo_queue_rem();

	return -1;
}

int CardDevice::at_response_rssi(char* str, size_t len)
{
    if ((rssi = at_parse_rssi(str, len)) == -1)
    {
	return -1;
    }
    return 0;
}

int CardDevice::at_response_mode(char* str, size_t len)
{
    return at_parse_mode(str, len, &linkmode, &linksubmode);
}

int CardDevice::at_response_orig(char* str, size_t len)
{
    int call_index = 1;
    int call_type  = 0;

//TODO:
//	channel_queue_control (AST_CONTROL_PROGRESS);
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


    if (at_send_clvl(1) || at_fifo_queue_add(CMD_AT_CLVL, RES_OK))
    {
	Debug(DebugAll, "[%s] Error syncronizing audio level (part 1/2)", c_str());
    }

    volume_synchronized = 0;

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

    if (!sscanf (str, "^CEND:%d,%d,%d,%d", &call_index, &duration, &end_status, &cc_cause))
    {
    	Debug(DebugAll, "[%s] Could not parse all CEND parameters", c_str());
    }

    Debug(DebugAll, "[%s] CEND: call_index: %d", c_str(), call_index);
    Debug(DebugAll, "[%s] CEND: duration:   %d", c_str(), duration);
    Debug(DebugAll, "[%s] CEND: end_status: %d", c_str(), end_status);
    Debug(DebugAll, "[%s] CEND: cc_cause:   %d", c_str(), cc_cause);

    Debug(DebugAll, "[%s] Line disconnected", c_str());
	
    needchup = 0;
//TODO:

    if(m_conn)
    {
	Debug(DebugAll, "[%s] hanging up owner", c_str());
		
	int reason = getReason(end_status, cc_cause);
			
	if(Hangup(reason) == false)
	{
	    Debug(DebugAll, "[%s] Error queueing hangup...", c_str());
	    return -1;
	}
    }
    incoming = 0;
    outgoing = 0;
    needring = 0;
    
    return 0;
}

int CardDevice::at_response_conn(char* str, size_t len)
{
//TODO:
    int call_index =1;
    int call_type =0;
    if (!sscanf (str, "^CONN:%d,%d", &call_index, &call_type))
    {
	Debug(DebugAll, "[%s] Error parsing CONN event '%s'", c_str(), str);
	return -1;
    }

    Debug(DebugAll, "[%s] Received call_index: %d", c_str(), call_index);
    Debug(DebugAll, "[%s] Received call_type:  %d", c_str(), call_type);

    if(outgoing)
    {
	Debug(DebugAll, "[%s] Remote end answered", c_str());
	if(m_conn)
	    m_conn->onAnswered();
//		hannel_queue_control (AST_CONTROL_ANSWER);
    }
    else if(incoming)
    {
//		ast_setstate (owner, AST_STATE_UP);
    }
    return 0;
}

int CardDevice::at_response_clip(char* str, size_t len)
{
//TODO:
//
    char* clip;
    if (initialized && needring == 0)
    {
	incoming = 1;
	if ((clip = at_parse_clip(str, len)) == NULL)
	{
	    Debug(DebugAll, "[%s] Error parsing CLIP: %s", c_str(), str);
	}
	if(incomingCall(String(clip)) == false)
	{
	    Debug(DebugAll, "[%s] Unable to allocate channel for incoming call", c_str());
	    if (at_send_chup() || at_fifo_queue_add (CMD_AT_CHUP, RES_OK))
	    {
		Debug(DebugAll, "[%s] Error sending AT+CHUP command", c_str());
	    }
	    return -1;
	}
	needchup = 1;
	needring = 1;
    }
    return 0;
}

int CardDevice::at_response_ring()
{
    if(initialized)
    {
	/* We only want to syncronize volume on the first ring */
	if(!incoming)
	{
	    if(at_send_clvl(1) || at_fifo_queue_add(CMD_AT_CLVL, RES_OK))
	    {
		Debug(DebugAll, "[%s] Error syncronizing audio level (part 1/2)", c_str());
	    }
	    volume_synchronized = 0;
	}
	incoming = 1;
    }

    return 0;
}

int CardDevice::at_response_cmti(char* str, size_t len)
{
    int index = at_parse_cmti(str, len);

    if (index > -1)
    {
	Debug(DebugAll, "[%s] Incoming SMS message", c_str());
	if (m_disablesms)
	{
	    Debug(DebugAll, "[%s] SMS reception has been disabled in the configuration.", c_str());
	}
	else
	{
	    // FIXME: replace it with correct CMGR parser
	    if (at_send_cmgr(index) || at_fifo_queue_add_num(CMD_AT_CMGR, RES_OK, index))
	    {
		Debug(DebugAll, "[%s] Error sending CMGR to retrieve SMS message", c_str());
		return -1;
	    }
	}
	return 0;
    }
    else
    {
	Debug(DebugAll, "[%s] Error parsing incoming sms message alert, disconnecting", c_str());
	return -1;
    }
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
    at_queue_t* e;

    if((e = at_fifo_queue_head()) && e->res == RES_SMS_PROMPT)
    {
	if(e->ptype != 0 || !e->param.data || at_send_sms_text((char*)e->param.data) || at_fifo_queue_add(CMD_AT_CMGS, RES_OK))
	{
	    Debug(DebugAll, "[%s] Error sending sms message", c_str());
	    return -1;
	}
	at_fifo_queue_rem();
    }
    else if(e)
    {
	Debug(DebugAll,  "[%s] Received sms prompt when expecting '%s' response to '%s', ignoring", c_str(), at_res2str (e->res), at_cmd2str (e->cmd));
    }
    else
    {
	Debug(DebugAll, "[%s] Received unexpected sms prompt", c_str());
    }
    return 0;
}

int CardDevice::at_response_cusd(char* str, size_t len)
{
//TODO:
    ssize_t res;
    char* cusd;
    unsigned char dcs;
    char cusd_utf8_str[1024];

    if(at_parse_cusd (str, len, &cusd, &dcs))
    {
	Debug(DebugAll, "[%s] Error parsing CUSD: '%.*s'", c_str(), (int) len, str);
	return 0;
    }

    if((dcs == 0 || dcs == 15) && !cusd_use_ucs2_decoding)
    {
	res = hexstr_7bit_to_char(cusd, strlen (cusd), cusd_utf8_str, sizeof (cusd_utf8_str));
	if (res > 0)
	{
	    cusd = cusd_utf8_str;
	}
	else
	{
	    Debug(DebugAll, "[%s] Error parsing CUSD (convert 7bit to ASCII): %s", c_str(), cusd);
	    return -1;
	}
    }
    else
    {
	res = hexstr_ucs2_to_utf8(cusd, strlen (cusd), cusd_utf8_str, sizeof (cusd_utf8_str));
	if (res > 0)
	{
	    cusd = cusd_utf8_str;
	}
	else
	{
	    Debug(DebugAll, "[%s] Error parsing CUSD (convert UCS-2 to UTF-8): %s", c_str(), cusd);
	    return -1;
	}
    }
    Debug(DebugAll, "[%s] Got USSD response: '%s'", c_str(), cusd);
    m_endpoint->onReceiveUSSD(this, cusd);
    return 0;
}

int CardDevice::at_response_busy()
{
    needchup = 1;
//TODO:
//	channel_queue_control (AST_CONTROL_BUSY);
    Hangup(DATACARD_BUSY);
    return 0;
}
 
int CardDevice::at_response_no_dialtone()
{
    Debug(DebugAll, "[%s] Datacard reports 'NO DIALTONE'", c_str());
    needchup = 1;
//TODO:
//	channel_queue_control (AST_CONTROL_CONGESTION);
    Hangup(DATACARD_CONGESTION);
    return 0;
}

int CardDevice::at_response_no_carrier()
{
    Debug(DebugAll, "[%s] Datacard reports 'NO CARRIER'", c_str());
    needchup = 1;
//TODO:
//	channel_queue_control (AST_CONTROL_CONGESTION);
    Hangup(DATACARD_CONGESTION);
    return 0;
}

int CardDevice::at_response_cpin(char* str, size_t len)
{
    return at_parse_cpin(str, len);
}

int CardDevice::at_response_smmemfull()
{
    Debug(DebugAll, "[%s] SMS storage is full", c_str());
    return 0;
}

int CardDevice::at_response_csq(char* str, size_t len)
{
    return at_parse_csq(str, len, &rssi);
}

int CardDevice::at_response_cnum(char* str, size_t len)
{
    char* number = at_parse_cnum(str, len);
    if(number)
    {
	m_number = number;
	return 0;
    }
    m_number = "Unknown";

    return -1;
}

int CardDevice::at_response_cops(char* str, size_t len)
{
    char* provider_name = at_parse_cops (str, len);
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

    if(at_send_cops() || at_fifo_queue_add(CMD_AT_COPS, RES_OK))
    {
	Debug(DebugAll, "[%s] Error sending query for provider name", c_str());
    }
    
    if(at_parse_creg(str, len, &d, &gsm_reg_status, &lac, &ci))
    {
	Debug(DebugAll, "[%s] Error parsing CREG: '%.*s'", c_str(), (int) len, str);
	return 0;
    }

    if(d)
	gsm_registered = 1;
    else
	gsm_registered = 0;

    if(lac)
	m_location_area_code = lac;
    if(ci)
	m_cell_id = ci;

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
    if(m_model == "E1550"|| m_model == "E1750" || m_model == "E160X")
    {
	cusd_use_7bit_encoding = 1;
	cusd_use_ucs2_decoding = 0;
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
