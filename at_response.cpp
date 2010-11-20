/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

#include "datacarddevice.h"
#include <stdio.h>

/*!                             
 * \brief Do response
 * \param pvt -- pvt structure
 * \param iovcnt -- number of elements array d_read_iov
 * \param at_res -- result type
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response(int iovcnt, at_res_t at_res)
{
	char*		str;
	size_t		len;
	at_queue_t*	e;

	if (iovcnt > 0)
	{
		len = d_read_iov[0].iov_len + d_read_iov[1].iov_len - 1;

		if (iovcnt == 2)
		{
			Debug(DebugAll, "[%s] iovcnt == 2\n", c_str());

			if (len > sizeof (d_parse_buf) - 1)
			{
				Debug(DebugAll, "[%s] buffer overflow\n", c_str());
				return -1;
			}

			str = d_parse_buf;
			memmove (str,					d_read_iov[0].iov_base, d_read_iov[0].iov_len);
			memmove (str + d_read_iov[0].iov_len,	d_read_iov[1].iov_base, d_read_iov[1].iov_len);
			str[len] = '\0';
		}
		else
		{
			str = (char*)d_read_iov[0].iov_base;
			str[len] = '\0';
		}

//		Debug(DebugAll,  "[%s] [%.*s]\n", c_str(), (int) len, str);

		switch (at_res)
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
				at_response_mode (str, len);
				return 0;

			case RES_ORIG:
				return at_response_orig (str, len);

			case RES_CEND:
				return at_response_cend (str, len);

			case RES_CONN:
				return at_response_conn();

			case RES_CREG:
				/* An error here is not fatal. Just keep going. */
				at_response_creg (str, len);
				return 0;

			case RES_COPS:
				/* An error here is not fatal. Just keep going. */
				at_response_cops (str, len);
				return 0;



			case RES_CSQ:
				return at_response_csq (str, len);

			case RES_CMS_ERROR:
			case RES_ERROR:
				return at_response_error();

			case RES_RING:
				return at_response_ring();

			case RES_SMMEMFULL:
				return at_response_smmemfull();

			case RES_CLIP:
				return at_response_clip (str, len);

			case RES_CMTI:
				return at_response_cmti (str, len);

			case RES_CMGR:
				return at_response_cmgr (str, len);

			case RES_SMS_PROMPT:
				return at_response_sms_prompt();

			case RES_CUSD:
				return at_response_cusd (str, len);

			case RES_BUSY:
				return at_response_busy();

			case RES_NO_DIALTONE:
				return at_response_no_dialtone();

			case RES_NO_CARRIER:
				return at_response_no_carrier();

			case RES_CPIN:
				return at_response_cpin (str, len);

			case RES_CNUM:
				/* An error here is not fatal. Just keep going. */
				at_response_cnum (str, len);
				return 0;

			case RES_PARSE_ERROR:
				Debug(DebugAll, "[%s] Error parsing result\n", c_str());
				return -1;

			case RES_UNKNOWN:
				if ((e = at_fifo_queue_head()))
//				if ((e =  static_cast<at_queue_t*>(m_atQueue.get())))
				{
					switch (e->cmd)
					{
						case CMD_AT_CGMI:
							Debug(DebugAll,  "[%s] Got AT_CGMI data (manufacturer info)\n", c_str());
							return at_response_cgmi (str, len);

						case CMD_AT_CGMM:
							Debug(DebugAll,  "[%s] Got AT_CGMM data (model info)\n", c_str());
							return at_response_cgmm (str, len);

						case CMD_AT_CGMR:
							Debug(DebugAll,  "[%s] Got AT+CGMR data (firmware info)\n", c_str());
							return at_response_cgmr (str, len);

						case CMD_AT_CGSN:
							Debug(DebugAll,  "[%s] Got AT+CGSN data (IMEI number)\n", c_str());
							return at_response_cgsn (str, len);

						case CMD_AT_CIMI:
							Debug(DebugAll,  "[%s] Got AT+CIMI data (IMSI number)\n", c_str());
							return at_response_cimi (str, len);

						default:
							Debug(DebugAll, "[%s] Ignoring unknown result: '%.*s'\n", c_str(), (int) len, str);
							break;
					}
				}
				else
				{
					Debug(DebugAll, "[%s] Ignoring unknown result: '%.*s'\n", c_str(), (int) len, str);
				}

				break;
		}
	}

	return 0;
}

/*!
 * \brief Handle OK response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_ok()
{
	at_queue_t* e;

	if ((e = at_fifo_queue_head()) && (e->res == RES_OK || e->res == RES_CMGR))
//	if ((e = static_cast<at_queue_t*>(m_atQueue.get())) && (e->res == RES_OK || e->res == RES_CMGR))
	{
		switch (e->cmd)
		{
			/* initilization stuff */
			case CMD_AT:
				if (!initialized)
				{
					if (reset_datacard == 1)
					{
						if (at_send_atz() || at_fifo_queue_add(CMD_AT_Z, RES_OK))
						{
							Debug(DebugAll, "[%s] Error reset datacard\n", c_str());
							goto e_return;
						}
					}
					else
					{
						if (at_send_ate0() || at_fifo_queue_add (CMD_AT_E, RES_OK))
						{
							Debug(DebugAll, "[%s] Error disabling echo\n", c_str());
							goto e_return;
						}
					}
				}
				break;

			case CMD_AT_Z:
				if (at_send_ate0() || at_fifo_queue_add (CMD_AT_E, RES_OK))
				{
					Debug(DebugAll, "[%s] Error disabling echo\n", c_str());
					goto e_return;
				}
				break;

			case CMD_AT_E:
				if (!initialized)
				{
					if (u2diag != -1)
					{
						if (at_send_u2diag (u2diag) || at_fifo_queue_add (CMD_AT_U2DIAG, RES_OK))
						{
							Debug(DebugAll, "[%s] Error setting U2DIAG\n", c_str());
							goto e_return;
						}
					}
					else
					{
						if (at_send_cgmi() || at_fifo_queue_add (CMD_AT_CGMI, RES_OK))
						{
							Debug(DebugAll, "[%s] Error asking datacard for manufacturer info\n", c_str());
							goto e_return;
						}
					}
				}
				break;

			case CMD_AT_U2DIAG:
				if (!initialized)
				{
					if (at_send_cgmi() || at_fifo_queue_add (CMD_AT_CGMI, RES_OK))
					{
						Debug(DebugAll, "[%s] Error asking datacard for manufacturer info\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CGMI:
				if (!initialized)
				{
					if (at_send_cgmm() || at_fifo_queue_add (CMD_AT_CGMM, RES_OK))
					{
						Debug(DebugAll, "[%s] Error asking datacard for model info\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CGMM:
				if (!initialized)
				{
					if (at_send_cgmr() || at_fifo_queue_add (CMD_AT_CGMR, RES_OK))
					{
						Debug(DebugAll, "[%s] Error asking datacard for firmware info\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CGMR:
				if (!initialized)
				{
					if (at_send_cmee (0) || at_fifo_queue_add (CMD_AT_CMEE, RES_OK))
					{
						Debug(DebugAll, "[%s] Error setting error verbosity level\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CMEE:
				if (!initialized)
				{
					if (at_send_cgsn() || at_fifo_queue_add (CMD_AT_CGSN, RES_OK))
					{
						Debug(DebugAll, "[%s] Error asking datacard for IMEI number\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CGSN:
				if (!initialized)
				{
					if (at_send_cimi() || at_fifo_queue_add (CMD_AT_CIMI, RES_OK))
					{
						Debug(DebugAll,  "[%s] Error asking datacard for IMSI number\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CIMI:
				if (!initialized)
				{
					if (at_send_cpin_test() || at_fifo_queue_add (CMD_AT_CPIN, RES_OK))
					{
						Debug(DebugAll,  "[%s] Error asking datacard for PIN state\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CPIN:
				if (!initialized)
				{
					if (at_send_cops_init (0, 0) || at_fifo_queue_add (CMD_AT_COPS_INIT, RES_OK))
					{
						Debug(DebugAll, "[%s] Error setting operator select parameters\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_COPS_INIT:
				Debug(DebugAll, "[%s] Operator select parameters set\n", c_str());

				if (!initialized)
				{
					if (at_send_creg_init (2) || at_fifo_queue_add (CMD_AT_CREG_INIT, RES_OK))
					{
						Debug(DebugAll, "[%s] Error enabeling registration info\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CREG_INIT:
				Debug(DebugAll,  "[%s] registration info enabled\n", c_str());

				if (!initialized)
				{
					if (at_send_creg() || at_fifo_queue_add (CMD_AT_CREG, RES_OK))
					{
						Debug(DebugAll, "[%s] Error sending registration query\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CREG:
				Debug(DebugAll, "[%s] registration query sent\n", c_str());

				if (!initialized)
				{
					if (at_send_cnum() || at_fifo_queue_add (CMD_AT_CNUM, RES_OK))
					{
						Debug(DebugAll, "[%s] Error checking subscriber phone number\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CNUM:
				Debug(DebugAll, "[%s] Subscriber phone number query successed\n", c_str());

				if (!initialized)
				{
					if (at_send_cvoice_test() || at_fifo_queue_add (CMD_AT_CVOICE, RES_OK))
					{
						Debug(DebugAll, "[%s] Error checking voice capabilities\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CVOICE:
				Debug(DebugAll, "[%s] Datacard has voice support\n", c_str());

				has_voice = 1;

				if (!initialized)
				{
					if (at_send_clip (1) || at_fifo_queue_add (CMD_AT_CLIP, RES_OK))
					{
						Debug(DebugAll, "[%s] Error enabling calling line notification\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CLIP:
				Debug(DebugAll, "[%s] Calling line indication enabled\n", c_str());

				if (!initialized)
				{
					if (at_send_cssn (1, 1) || at_fifo_queue_add (CMD_AT_CSSN, RES_OK))
					{
						Debug(DebugAll, "[%s] Error activating Supplementary Service Notification\n", c_str());
						goto e_return;
					}
				}

				break;

			case CMD_AT_CSSN:
				Debug(DebugAll, "[%s] Supplementary Service Notification enabled successful\n", c_str());

				if (!initialized)
				{
					/* set the SMS operating mode to text mode */
					if (at_send_cmgf (1) || at_fifo_queue_add (CMD_AT_CMGF, RES_OK))
					{
						Debug(DebugAll, "[%s] Error setting CMGF\n", c_str());
						goto e_return;
					}
				}

				break;

			case CMD_AT_CMGF:
				Debug(DebugAll, "[%s] SMS text mode enabled\n", c_str());

				if (!initialized)
				{
					/* set text encoding to UCS-2 */
					if (at_send_cscs ("UCS2") || at_fifo_queue_add (CMD_AT_CSCS, RES_OK))
					{
						Debug(DebugAll, "[%s] Error setting CSCS (text encoding)\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CSCS:
				Debug(DebugAll,  "[%s] UCS-2 text encoding enabled\n", c_str());

				use_ucs2_encoding = 1;

				if (!initialized)
				{
					/* set SMS storage location */
					if (at_send_cpms() || at_fifo_queue_add (CMD_AT_CPMS, RES_OK))
					{
						Debug(DebugAll, "[%s] Error setting CPMS\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CPMS:
				Debug(DebugAll,  "[%s] SMS storage location is established\n", c_str());

				if (!initialized)
				{
					/* turn on SMS new message indication */
					if (at_send_cnmi() || at_fifo_queue_add (CMD_AT_CNMI, RES_OK))
					{
						Debug(DebugAll, "[%s] Error sending CNMI\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CNMI:
				Debug(DebugAll, "[%s] SMS new message indication enabled\n", c_str());
				Debug(DebugAll, "[%s] Datacard has sms support\n", c_str());

				has_sms = 1;

				if (!initialized)
				{
					if (at_send_csq() || at_fifo_queue_add (CMD_AT_CSQ, RES_OK))
					{
						Debug(DebugAll, "[%s] Error querying signal strength\n", c_str());
						goto e_return;
					}

					timeout = 10000;
					initialized = 1;
					Debug(DebugAll, "Datacard %s initialized and ready\n", c_str());
				}
				break;

			/* end initilization stuff */

			case CMD_AT_A:
				Debug(DebugAll,  "[%s] Answer sent successfully\n", c_str());

				if (at_send_ddsetex() || at_fifo_queue_add (CMD_AT_DDSETEX, RES_OK))
				{
					Debug(DebugAll, "[%s] Error sending AT^DDSETEX\n", c_str());
					goto e_return;
				}

//				if (a_timer)
//				{
//					ast_timer_set_rate (a_timer, 50);
//				}

				break;

			case CMD_AT_CLIR:
				Debug(DebugAll, "[%s] CLIR sent successfully\n", c_str());

				if (e->ptype != 0 || at_send_atd((char*)(e->param.data)) || at_fifo_queue_add (CMD_AT_D, RES_OK))
				{
					Debug(DebugAll, "[%s] Error sending ATD command\n", c_str());
					goto e_return;
				}
				break;

			case CMD_AT_D:
				Debug(DebugAll,  "[%s] Dial sent successfully\n", c_str());

				if (at_send_ddsetex() || at_fifo_queue_add (CMD_AT_DDSETEX, RES_OK))
				{
					Debug(DebugAll, "[%s] Error sending AT^DDSETEX\n", c_str());
					goto e_return;
				}

//				if (a_timer)
//				{
//					ast_timer_set_rate (a_timer, 50);
//				}

				break;

			case CMD_AT_DDSETEX:
				Debug(DebugAll,  "[%s] AT^DDSETEX sent successfully\n", c_str());
				break;

			case CMD_AT_CHUP:
				Debug(DebugAll,  "[%s] Successful hangup\n", c_str());
				break;

			case CMD_AT_CMGS:
				Debug(DebugAll, "[%s] Successfully sent sms message\n", c_str());
				outgoing_sms = 0;
				break;

			case CMD_AT_DTMF:
				Debug(DebugAll, "[%s] DTMF sent successfully\n", c_str());
				break;

			case CMD_AT_CUSD:
				Debug(DebugAll, "[%s] CUSD code sent successfully\n", c_str());
				break;

			case CMD_AT_COPS:
				Debug(DebugAll, "[%s] Provider query successfully\n", c_str());
				break;

			case CMD_AT_CMGR:
				Debug(DebugAll, "[%s] SMS message see later\n", c_str());
				break;

			case CMD_AT_CMGD:
				Debug(DebugAll, "[%s] SMS message deleted successfully\n", c_str());
				break;

			case CMD_AT_CSQ:
				Debug(DebugAll, "[%s] Got signal strength result\n", c_str());
				break;
			
			case CMD_AT_CCWA:
				Debug(DebugAll, "Call-Waiting disabled on device %s.\n", c_str());
				break;
			
			case CMD_AT_CFUN:
				Debug(DebugAll, "[%s] CFUN sent successfully\n", c_str());
				break;

			case CMD_AT_CLVL:
				if (volume_synchronized == 0)
				{
					volume_synchronized = 1;

					if (at_send_clvl (5) || at_fifo_queue_add (CMD_AT_CLVL, RES_OK))
					{
						Debug(DebugAll, "[%s] Error syncronizing audio level (part 2/2)\n", c_str());
						goto e_return;
					}
				}
				break;

			default:
				Debug(DebugAll, "[%s] Received 'OK' for unhandled command '%s'\n", c_str(), at_cmd2str (e->cmd));
				break;
		}

		at_fifo_queue_rem();
	}
	else if (e)
	{
		Debug(DebugAll, "[%s] Received 'OK' when expecting '%s', ignoring\n", c_str(), at_res2str (e->res));
	}
	else
	{
		Debug(DebugAll,  "[%s] Received unexpected 'OK'\n", c_str());
	}

	return 0;

e_return:
	at_fifo_queue_rem();

	return -1;
}

/*!
 * \brief Handle ERROR response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_error()
{
	at_queue_t* e;

	if ((e = at_fifo_queue_head()) && (e->res == RES_OK || e->res == RES_ERROR ||
//	if ((e = static_cast<at_queue_t*>(m_atQueue.get())) && (e->res == RES_OK || e->res == RES_ERROR ||
			e->res == RES_CMS_ERROR || e->res == RES_CMGR || e->res == RES_SMS_PROMPT))
	{
		switch (e->cmd)
		{
        		/* initilization stuff */
			case CMD_AT:
			case CMD_AT_Z:
			case CMD_AT_E:
			case CMD_AT_U2DIAG:
				Debug(DebugAll,  "[%s] Command '%s' failed\n", c_str(), at_cmd2str (e->cmd));
				goto e_return;

			case CMD_AT_CGMI:
				Debug(DebugAll, "[%s] Getting manufacturer info failed\n", c_str());
				goto e_return;

			case CMD_AT_CGMM:
				Debug(DebugAll,  "[%s] Getting model info failed\n", c_str());
				goto e_return;

			case CMD_AT_CGMR:
				Debug(DebugAll,  "[%s] Getting firmware info failed\n", c_str());
				goto e_return;

			case CMD_AT_CMEE:
				Debug(DebugAll,  "[%s] Setting error verbosity level failed\n", c_str());
				goto e_return;

			case CMD_AT_CGSN:
				Debug(DebugAll,  "[%s] Getting IMEI number failed\n", c_str());
				goto e_return;

			case CMD_AT_CIMI:
				Debug(DebugAll,  "[%s] Getting IMSI number failed\n", c_str());
				goto e_return;

			case CMD_AT_CPIN:
				Debug(DebugAll,  "[%s] Error checking PIN state\n", c_str());
				goto e_return;

			case CMD_AT_COPS_INIT:
				Debug(DebugAll,  "[%s] Error setting operator select parameters\n", c_str());
				goto e_return;

			case CMD_AT_CREG_INIT:
				Debug(DebugAll, "[%s] Error enableling registration info\n", c_str());
				goto e_return;

			case CMD_AT_CREG:
				Debug(DebugAll, "[%s] Error getting registration info\n", c_str());

				if (!initialized)
				{
					if (at_send_cnum() || at_fifo_queue_add (CMD_AT_CNUM, RES_OK))
					{
						Debug(DebugAll, "[%s] Error checking subscriber phone number\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CNUM:
				Debug(DebugAll, "[%s] Error checking subscriber phone number\n", c_str());
				Debug(DebugAll, "Datacard %s needs to be reinitialized. The SIM card is not ready yet\n", c_str());
				goto e_return;

			case CMD_AT_CVOICE:
				Debug(DebugAll, "[%s] Datacard has NO voice support\n", c_str());

				has_voice = 0;

				if (!initialized)
				{
					if (at_send_cmgf (1) || at_fifo_queue_add (CMD_AT_CMGF, RES_OK))
					{
						Debug(DebugAll, "[%s] Error setting CMGF\n", c_str());
						goto e_return;
					}
				}
				break;

			case CMD_AT_CLIP:
				Debug(DebugAll, "[%s] Error enabling calling line indication\n", c_str());
				goto e_return;

			case CMD_AT_CSSN:
				Debug(DebugAll, "[%s] Error Supplementary Service Notification activation failed\n", c_str());
				goto e_return;

			case CMD_AT_CMGF:
			case CMD_AT_CPMS:
			case CMD_AT_CNMI:
				Debug(DebugAll, "[%s] Command '%s' failed\n", c_str(), at_cmd2str (e->cmd));
				Debug(DebugAll, "[%s] No SMS support\n", c_str());

				has_sms = 0;

				if (!initialized)
				{
					if (has_voice)
					{
						if (at_send_csq() || at_fifo_queue_add (CMD_AT_CSQ, RES_OK))
						{
							Debug(DebugAll, "[%s] Error querying signal strength\n", c_str());
							goto e_return;
						}

						timeout = 10000;
						initialized = 1;
						Debug(DebugAll, "Datacard %s initialized and ready\n", c_str());
					}

					goto e_return;
				}
				break;

			case CMD_AT_CSCS:
				Debug(DebugAll, "[%s] No UCS-2 encoding support\n", c_str());

				use_ucs2_encoding = 0;

				if (!initialized)
				{
					/* set SMS storage location */
					if (at_send_cpms() || at_fifo_queue_add (CMD_AT_CPMS, RES_OK))
					{
						Debug(DebugAll, "[%s] Error setting SMS storage location\n", c_str());
						goto e_return;
					}
				}
				break;

			/* end initilization stuff */

			case CMD_AT_A:
				Debug(DebugAll, "[%s] Answer failed\n", c_str());
//TODO:
//				channel_queue_hangup (0);
				Hangup(0);
				break;

			case CMD_AT_CLIR:
				Debug(DebugAll, "[%s] Setting CLIR failed\n", c_str());

				/* continue dialing */
				if (e->ptype != 0 || at_send_atd((char*)e->param.data) || at_fifo_queue_add (CMD_AT_D, RES_OK))
				{
					Debug(DebugAll, "[%s] Error sending ATD command\n", c_str());
					goto e_return;
				}
				break;

			case CMD_AT_D:
				Debug(DebugAll, "[%s] Dial failed\n", c_str());
				outgoing = 0;
				needchup = 0;
//TODO:
//				channel_queue_control (AST_CONTROL_CONGESTION);
				Hangup(0);
				break;

			case CMD_AT_DDSETEX:
				Debug(DebugAll, "[%s] AT^DDSETEX failed\n", c_str());
				break;

			case CMD_AT_CHUP:
				Debug(DebugAll, "[%s] Error sending hangup, disconnecting\n", c_str());
				goto e_return;

			case CMD_AT_CMGR:
				Debug(DebugAll, "[%s] Error reading SMS message\n", c_str());
				incoming_sms = 0;
				break;

			case CMD_AT_CMGD:
				Debug(DebugAll, "[%s] Error deleting SMS message\n", c_str());
				incoming_sms = 0;
				break;

			case CMD_AT_CMGS:
				Debug(DebugAll, "[%s] Error sending SMS message\n", c_str());
				outgoing_sms = 0;
				break;

			case CMD_AT_DTMF:
				Debug(DebugAll, "[%s] Error sending DTMF\n", c_str());
				break;

			case CMD_AT_COPS:
				Debug(DebugAll, "[%s] Could not get provider name\n", c_str());
				break;

			case CMD_AT_CLVL:
				Debug(DebugAll, "[%s] Error syncronizing audio level\n", c_str());
				volume_synchronized = 0;
				break;

			case CMD_AT_CUSD:
				Debug(DebugAll, "[%s] Could not send USSD code\n", c_str());
				break;

			default:
				Debug(DebugAll, "[%s] Received 'ERROR' for unhandled command '%s'\n", c_str(), at_cmd2str (e->cmd));
				break;
		}

		at_fifo_queue_rem();
	}
	else if (e)
	{
		Debug(DebugAll, "[%s] Received 'ERROR' when expecting '%s', ignoring\n", c_str(), at_res2str (e->res));
	}
	else
	{
		Debug(DebugAll, "[%s] Received unexpected 'ERROR'\n", c_str());
	}

	return 0;

e_return:
	at_fifo_queue_rem();

	return -1;
}

/*!
 * \brief Handle ^RSSI response Here we get the signal strength.
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_rssi (char* str, size_t len)
{
	if ((rssi = at_parse_rssi (str, len)) == -1)
	{
		return -1;
	}

	return 0;
}

/*!
 * \brief Handle ^MODE response Here we get the link mode (GSM, UMTS, EDGE...).
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_mode(char* str, size_t len)
{
	return at_parse_mode (str, len, &linkmode, &linksubmode);
}

/*!
 * \brief Handle ^ORIG response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_orig (char* str, size_t len)
{
	int call_index = 1;
	int call_type  = 0;

//TODO:
//	channel_queue_control (AST_CONTROL_PROGRESS);

	/*
	 * parse ORIG info in the following format:
	 * ^ORIG:<call_index>,<call_type>
	 */

	if (!sscanf (str, "^ORIG:%d,%d", &call_index, &call_type))
	{
		Debug(DebugAll, "[%s] Error parsing ORIG event '%s'\n", c_str(), str);
		return -1;
	}

	Debug(DebugAll, "[%s] Received call_index: %d\n", c_str(), call_index);
	Debug(DebugAll, "[%s] Received call_type:  %d\n", c_str(), call_type);

	if (at_send_clvl (1) || at_fifo_queue_add (CMD_AT_CLVL, RES_OK))
	{
		Debug(DebugAll, "[%s] Error syncronizing audio level (part 1/2)\n", c_str());
	}

	volume_synchronized = 0;

	return 0;
}

/*!
 * \brief Handle ^CEND response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cend (char* str, size_t len)
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
		Debug(DebugAll, "[%s] Could not parse all CEND parameters\n", c_str());
	}

	Debug(DebugAll, "[%s] CEND: call_index: %d\n", c_str(), call_index);
	Debug(DebugAll, "[%s] CEND: duration:   %d\n", c_str(), duration);
	Debug(DebugAll, "[%s] CEND: end_status: %d\n", c_str(), end_status);
	Debug(DebugAll, "[%s] CEND: cc_cause:   %d\n", c_str(), cc_cause);

	Debug(DebugAll, "[%s] Line disconnected\n", c_str());

	needchup = 0;
//TODO:

	if (m_conn)
	{
		Debug(DebugAll, "[%s] hanging up owner\n", c_str());
		
//		if (m_conn->onHangup(cc_cause) == false)
		if (Hangup(0, cc_cause) == false)
		{
			Debug(DebugAll, "[%s] Error queueing hangup...\n", c_str());
			return -1;
		}
	}
	incoming = 0;
	outgoing = 0;
	needring = 0;

	return 0;
}

/*!
 * \brief Handle ^CONN response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_conn ()
{
//TODO:
/*
	if (outgoing)
	{
		Debug(DebugAll, "[%s] Remote end answered\n", c_str());
		channel_queue_control (AST_CONTROL_ANSWER);
	}
	else if (incoming && answered)
	{
		ast_setstate (owner, AST_STATE_UP);
	}
*/
	return 0;
}

/*!
 * \brief Handle +CLIP response
 * \param pvt -- pvt structure
 * \param str -- null terminated strfer containing response
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_clip(char* str, size_t len)
{
//TODO:
//

//	struct ast_channel*	channel;
	char*			clip;

	if (initialized && needring == 0)
	{
		incoming = 1;

		if ((clip = at_parse_clip (str, len)) == NULL)
		{
			Debug(DebugAll, "[%s] Error parsing CLIP: %s\n", c_str(), str);
		}
//		if (!(channel = channel_new (AST_STATE_RING, clip)))
		if(incomingCall(String(clip)) == false)
		{
			Debug(DebugAll, "[%s] Unable to allocate channel for incoming call\n", c_str());

			if (at_send_chup() || at_fifo_queue_add (CMD_AT_CHUP, RES_OK))
			{
				Debug(DebugAll, "[%s] Error sending AT+CHUP command\n", c_str());
			}
			return -1;
		}

		needchup = 1;
		needring = 1;
/*
		if (ast_pbx_start (channel))
		{
			Debug(DebugAll, "[%s] Unable to start pbx on incoming call\n", c_str());
			channel_ast_hangup();

			return -1;
		}
*/
	}

	return 0;
}

/*!
 * \brief Handle RING response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_ring ()
{
	if (initialized)
	{
		/* We only want to syncronize volume on the first ring */
		if (!incoming)
		{
			if (at_send_clvl (1) || at_fifo_queue_add (CMD_AT_CLVL, RES_OK))
			{
				Debug(DebugAll, "[%s] Error syncronizing audio level (part 1/2)\n", c_str());
			}

			volume_synchronized = 0;
		}

		incoming = 1;
	}

	return 0;
}

/*!
 * \brief Handle +CMTI response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cmti(char* str, size_t len)
{
	int index = at_parse_cmti (str, len);

	if (index > -1)
	{
		Debug(DebugAll, "[%s] Incoming SMS message\n", c_str());

		if (disablesms)
		{
			Debug(DebugAll, "[%s] SMS reception has been disabled in the configuration.\n", c_str());
		}
		else
		{
			if (at_send_cmgr (index) || at_fifo_queue_add_num (CMD_AT_CMGR, RES_CMGR, index))
			{
				Debug(DebugAll, "[%s] Error sending CMGR to retrieve SMS message\n", c_str());
				return -1;
			}

			incoming_sms = 1;
		}

		return 0;
	}
	else
	{
		Debug(DebugAll, "[%s] Error parsing incoming sms message alert, disconnecting\n", c_str());
		return -1;
	}
}

/*!
 * \brief Handle +CMGR response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cmgr(char* str, size_t len)
{
//TODO:
//

	at_queue_t*	e;
	ssize_t		res;
	char		sms_utf8_str[4096];
	char		from_number_utf8_str[1024];
	char*		from_number = NULL;
	char*		text = NULL;
//	char		text_base64[16384];

	if ((e = at_fifo_queue_head()) && e->res == RES_CMGR)
	{
		if (auto_delete_sms && e->ptype == 1)
		{
			if (at_send_cmgd (e->param.num) || at_fifo_queue_add (CMD_AT_CMGD, RES_OK))
			{
				Debug(DebugAll, "[%s] Error sending CMGD to delete SMS message\n", c_str());
			}
		}

		at_fifo_queue_rem();

		if (at_parse_cmgr(str, len, &from_number, &text))
		{
			Debug(DebugAll, "[%s] Error parsing SMS message, disconnecting\n", c_str());
			return -1;
		}

		Debug(DebugAll, "[%s] Successfully read SMS message\n", c_str());

		incoming_sms = 0;

		if (use_ucs2_encoding)
		{
			res = hexstr_ucs2_to_utf8 (text, strlen (text), sms_utf8_str, sizeof (sms_utf8_str));
			if (res > 0)
			{
				text = sms_utf8_str;
			}
			else
			{
				Debug(DebugAll, "[%s] Error parsing SMS (convert UCS-2 to UTF-8): %s\n", c_str(), text);
			}

			res = hexstr_ucs2_to_utf8 (from_number, strlen (from_number), from_number_utf8_str, sizeof (from_number_utf8_str));
			if (res > 0)
			{
				from_number = from_number_utf8_str;
			}
			else
			{
				Debug(DebugAll, "[%s] Error parsing SMS from_number (convert UCS-2 to UTF-8): %s\n", c_str(), from_number);
			}
		}
		
//		String from(text_base64);
//		String to(text);
		
//		decodeBase64(String(text), String(text_base64));
//		decodeBase64(to, from);
//		ast_base64encode (text_base64, text, strlen(text), sizeof(text_base64));
		Debug(DebugAll, "[%s] Got SMS from %s: '%s'\n", c_str(), from_number, text);
		m_endpoint->onReceiveSMS(this, from_number, text);

/*
#ifdef __MANAGER__
		struct ast_channel* channel;

		snprintf (d_send_buf, sizeof (d_send_buf), "sms@%s", context);

		if (channel = channel_local_request (d_send_buf, c_str(), from_number, "en"))
		{
			pbx_builtin_setvar_helper (channel, "SMS", text);
			pbx_builtin_setvar_helper (channel, "SMS_BASE64", text_base64);

			if (ast_pbx_start (channel))
			{
				ast_hangup (channel);
				Debug(DebugAll, "[%s] Unable to start pbx on incoming sms\n", c_str());
			}
		}
#endif
*/
	}
	else if (e)
	{
		Debug(DebugAll, "[%s] Received '+CMGR' when expecting '%s' response to '%s', ignoring\n", c_str(),
				at_res2str (e->res), at_cmd2str (e->cmd));
	}
	else
	{
		Debug(DebugAll, "[%s] Received unexpected '+CMGR'\n", c_str());
	}
	return 0;
}

/*!
 * \brief Send an SMS message from the queue.
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_sms_prompt()
{
	at_queue_t* e;

    Debug(DebugAll, "[%s] 1\n", c_str());
	if ((e = at_fifo_queue_head()) && e->res == RES_SMS_PROMPT)
//	if ((e = static_cast<at_queue_t*>(m_atQueue.get())) && e->res == RES_SMS_PROMPT)
	{
	    Debug(DebugAll, "[%s] 2\n", c_str());
		if (e->ptype != 0 || !e->param.data || at_send_sms_text((char*)e->param.data) || at_fifo_queue_add (CMD_AT_CMGS, RES_OK))
		{
			Debug(DebugAll, "[%s] Error sending sms message\n", c_str());
			return -1;
		}

		at_fifo_queue_rem();
	}
	else if (e)
	{
		Debug(DebugAll,  "[%s] Received sms prompt when expecting '%s' response to '%s', ignoring\n", c_str(),
				at_res2str (e->res), at_cmd2str (e->cmd));
	}
	else
	{
		Debug(DebugAll, "[%s] Received unexpected sms prompt\n", c_str());
	}

	return 0;
}

/*!
 * \brief Handle CUSD response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cusd (char* str, size_t len)
{
//TODO:
//

	ssize_t		res;
	char*		cusd;
	unsigned char	dcs;
	char		cusd_utf8_str[1024];
//	char		text_base64[16384];

	if (at_parse_cusd (str, len, &cusd, &dcs))
	{
		Debug(DebugAll, "[%s] Error parsing CUSD: '%.*s'\n", c_str(), (int) len, str);
		return 0;
	}

	if ((dcs == 0 || dcs == 15) && !cusd_use_ucs2_decoding)
	{
		res = hexstr_7bit_to_char (cusd, strlen (cusd), cusd_utf8_str, sizeof (cusd_utf8_str));
		if (res > 0)
		{
			cusd = cusd_utf8_str;
		}
		else
		{
			Debug(DebugAll, "[%s] Error parsing CUSD (convert 7bit to ASCII): %s\n", c_str(), cusd);
			return -1;
		}
	}
	else
	{
		res = hexstr_ucs2_to_utf8 (cusd, strlen (cusd), cusd_utf8_str, sizeof (cusd_utf8_str));
		if (res > 0)
		{
			cusd = cusd_utf8_str;
		}
		else
		{
			Debug(DebugAll, "[%s] Error parsing CUSD (convert UCS-2 to UTF-8): %s\n", c_str(), cusd);
			return -1;
		}
	}

	Debug(DebugAll, "[%s] Got USSD response: '%s'\n", c_str(), cusd);
//	ast_base64encode (text_base64, cusd, strlen(cusd), sizeof(text_base64));
//	String from(text_base64);
//	String to(cusd);
//	decodeBase64(to, from);
	Debug(DebugAll, "[%s] Got USSD response: '%s'\n", c_str(), cusd);
	m_endpoint->onReceiveUSSD(this, cusd);
//

/*
#ifdef __MANAGER__
	struct ast_channel* channel;

	snprintf (d_send_buf, sizeof (d_send_buf), "ussd@%s", context);

	if (channel = channel_local_request (d_send_buf, c_str(), "ussd", "en"))
	{
		pbx_builtin_setvar_helper (channel, "USSD", cusd);
		pbx_builtin_setvar_helper (channel, "USSD_BASE64", text_base64);

		if (ast_pbx_start (channel))
		{
			ast_hangup (channel);
			Debug(DebugAll,  "[%s] Unable to start pbx on incoming ussd\n", c_str());
		}
	}
#endif
*/
	return 0;
}

/*!
 * \brief Handle BUSY response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_busy ()
{
	needchup = 1;
//TODO:
//	channel_queue_control (AST_CONTROL_BUSY);
	Hangup(0);

	return 0;
}
 
/*!
 * \brief Handle NO DIALTONE response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_no_dialtone()
{
	Debug(DebugAll, "[%s] Datacard reports 'NO DIALTONE'\n", c_str());

	needchup = 1;
//TODO:
//	channel_queue_control (AST_CONTROL_CONGESTION);
	Hangup(0);

	return 0;
}

/*!
 * \brief Handle NO CARRIER response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_no_carrier()
{
	Debug(DebugAll, "[%s] Datacard reports 'NO CARRIER'\n", c_str());

	needchup = 1;
//TODO:
//	channel_queue_control (AST_CONTROL_CONGESTION);
	Hangup(0);

	return 0;
}

/*!
 * \brief Handle +CPIN response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cpin (char* str, size_t len)
{
	return at_parse_cpin (str, len);
}

/*!
 * \brief Handle ^SMMEMFULL response This event notifies us, that the sms storage is full
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_smmemfull()
{
	Debug(DebugAll,  "[%s] SMS storage is full\n", c_str());
	return 0;
}

/*!
 * \brief Handle +CSQ response Here we get the signal strength and bit error rate
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */
int CardDevice::at_response_csq (char* str, size_t len)
{
	return at_parse_csq (str, len, &rssi);
}

/*!
 * \brief Handle +CNUM response Here we get our own phone number
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cnum(char* str, size_t len)
{
	char* number = at_parse_cnum (str, len);

	if (number)
	{
	    m_number = number;
	    return 0;
	}

	m_number = "Unknown";

	return -1;
}

/*!
 * \brief Handle +COPS response Here we get the GSM provider name
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cops(char* str, size_t len)
{
    char* provider_name = at_parse_cops (str, len);

    if (provider_name)
    {
	m_provider_name = provider_name;
	return 0;
    }

    m_provider_name = "NONE";
    return -1;
}

/*!
 * \brief Handle +CREG response Here we get the GSM registration status
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_creg (char* str, size_t len)
{
	int	d;
	char*	lac;
	char*	ci;

	if (at_send_cops() || at_fifo_queue_add (CMD_AT_COPS, RES_OK))
	{
		Debug(DebugAll, "[%s] Error sending query for provider name\n", c_str());
	}

	if (at_parse_creg (str, len, &d, &gsm_reg_status, &lac, &ci))
	{
		Debug(DebugAll, "[%s] Error parsing CREG: '%.*s'\n", c_str(), (int) len, str);
		return 0;
	}

	if (d)
	{
		gsm_registered = 1;
	}
	else
	{
		gsm_registered = 0;
	}

	if (lac)
	{
		//ast_copy_string (location_area_code, lac, sizeof (location_area_code));
		m_location_area_code = lac;
	}

	if (ci)
	{
		//ast_copy_string (cell_id, ci, sizeof (cell_id));
		m_cell_id = ci;
	}

	return 0;
}

/*!
 * \brief Handle AT+CGMI response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cgmi(char* str, size_t len)
{
	m_manufacturer.assign(str,len);
	return 0;
}

/*!
 * \brief Handle AT+CGMM response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cgmm (char* str, size_t len)
{
	m_model.assign(str,len);;
	
	if(m_model == "E1550"|| m_model == "E1750" || m_model == "E160X")
	{
		cusd_use_7bit_encoding = 1;
		cusd_use_ucs2_decoding = 0;
	}
	return 0;
}

/*!
 * \brief Handle AT+CGMR response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cgmr(char* str, size_t len)
{
	m_firmware.assign(str,len);;
	return 0;
}

/*!
 * \brief Handle AT+CGSN response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cgsn (char* str, size_t len)
{
	m_imei.assign(str,len);;
	return 0;
}

/*!
 * \brief Handle AT+CIMI response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

int CardDevice::at_response_cimi(char* str, size_t len)
{
    m_imsi.assign(str,len);;
    return 0;
}
