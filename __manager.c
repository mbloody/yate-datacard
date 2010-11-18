/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

static int manager_show_devices (struct mansession* s, const struct message* m)
{
	const char*	id = astman_get_header (m, "ActionID");
	char		idtext[256] = "";
	pvt_t*		pvt;
	size_t		count = 0;

	if (!ast_strlen_zero (id))
	{
		snprintf (idtext, sizeof (idtext), "ActionID: %s\r\n", id);
	}

	astman_send_listack (s, m, "Device status list will follow", "start");

	AST_RWLIST_RDLOCK (&devices);
	AST_RWLIST_TRAVERSE (&devices, pvt, entry)
	{
		ast_mutex_lock (&pvt->lock);
		astman_append (s, "Event: DatacardDeviceEntry\r\n%s", idtext);
		astman_append (s, "Device: %s\r\n", pvt->id);
		astman_append (s, "Group: %d\r\n", pvt->group);
		astman_append (s, "GSM Registration Status: %s\r\n",
			(pvt->gsm_reg_status == 0) ? "Not registered, not searching" :
			(pvt->gsm_reg_status == 1) ? "Registered, home network" :
			(pvt->gsm_reg_status == 2) ? "Not registered, but searching" :
			(pvt->gsm_reg_status == 3) ? "Registration denied" :
			(pvt->gsm_reg_status == 5) ? "Registered, roaming" : "Unknown"
		);
		astman_append (s, "State: %s\r\n", 
			(!pvt->connected) ? "Not connected" :
			(!pvt->initialized) ? "Not initialized" :
			(!pvt->gsm_registered) ? "GSM not registered" :
			(pvt->outgoing || pvt->incoming) ? "Busy" :
			(pvt->outgoing_sms || pvt->incoming_sms) ? "SMS" : "Free"
		);
		astman_append (s, "Voice: %s\r\n", (pvt->has_voice) ? "Yes" : "No");
		astman_append (s, "SMS: %s\r\n", (pvt->has_sms) ? "Yes" : "No");
		astman_append (s, "RSSI: %d\r\n", pvt->rssi);
		astman_append (s, "Mode: %d\r\n", pvt->linkmode);
		astman_append (s, "Submode: %d\r\n", pvt->linksubmode);
		astman_append (s, "ProviderName: %s\r\n", pvt->provider_name);
		astman_append (s, "Manufacturer: %s\r\n", pvt->manufacturer);
		astman_append (s, "Model: %s\r\n", pvt->model);
		astman_append (s, "Firmware: %s\r\n", pvt->firmware);
		astman_append (s, "IMEI: %s\r\n", pvt->imei);
		astman_append (s, "IMSI: %s\r\n", pvt->imsi);
		astman_append (s, "Number: %s\r\n", pvt->number);
		astman_append (s, "Use CallingPres: %s\r\n", pvt->usecallingpres ? "Yes" : "No");
		astman_append (s, "Default CallingPres: %s\r\n", pvt->callingpres < 0 ? "<Not set>" : ast_describe_caller_presentation (pvt->callingpres));
		astman_append (s, "Use UCS-2 encoding: %s\r\n", pvt->use_ucs2_encoding ? "Yes" : "No");
		astman_append (s, "USSD use 7 bit encoding: %s\r\n", pvt->cusd_use_7bit_encoding ? "Yes" : "No");
		astman_append (s, "USSD use UCS-2 decoding: %s\r\n", pvt->cusd_use_ucs2_decoding ? "Yes" : "No");
		astman_append (s, "Location area code: %s\r\n", pvt->location_area_code);
		astman_append (s, "Cell ID: %s\r\n", pvt->cell_id);
		astman_append (s, "Auto delete SMS: %s\r\n", pvt->auto_delete_sms ? "Yes" : "No");
		astman_append (s, "Disable SMS: %s\r\n", pvt->disablesms ? "Yes" : "No");
		astman_append (s, "\r\n");
		ast_mutex_unlock (&pvt->lock);
		count++;
	}
	AST_RWLIST_UNLOCK (&devices);

	astman_append (s,
		"Event: DatacardShowDevicesComplete\r\n%s"
		"EventList: Complete\r\n"
		"ListItems: %zu\r\n"
		"\r\n",
		idtext, count
	);

	return 0;
}

static int manager_send_ussd (struct mansession* s, const struct message* m)
{
	const char*	device	= astman_get_header (m, "Device");
	const char*	ussd	= astman_get_header (m, "USSD");
	const char*	id	= astman_get_header (m, "ActionID");

	char		idtext[256] = "";
	pvt_t*		pvt = NULL;
	char		buf[256];

	if (ast_strlen_zero (device))
	{
		astman_send_error (s, m, "Device not specified");
		return 0;
	}

	if (ast_strlen_zero (ussd))
	{
		astman_send_error (s, m, "USSD not specified");
		return 0;
	}

	if (!ast_strlen_zero (id))
	{
		snprintf (idtext, sizeof (idtext), "ActionID: %s\r\n", id);
	}

	pvt = find_device (device);
	if (pvt)
	{
		ast_mutex_lock (&pvt->lock);
		if (pvt->connected && pvt->initialized && pvt->gsm_registered)
		{
			if (at_send_cusd (pvt, ussd) || at_fifo_queue_add (pvt, CMD_AT_CUSD, RES_OK))
			{
				ast_log (LOG_ERROR, "[%s] Error sending USSD command\n", pvt->id);
			}
			else
			{
				astman_send_ack (s, m, "USSD code send successful");
			}
		}
		else
		{
			snprintf (buf, sizeof (buf), "Device %s not connected / initialized./ registered", device);
			astman_send_error (s, m, buf);
		}
		ast_mutex_unlock (&pvt->lock);
	}
	else
	{
		snprintf (buf, sizeof (buf), "Device %s not found.", device);
		astman_send_error (s, m, buf);
	}

	return 0;
}

static int manager_send_sms (struct mansession* s, const struct message* m)
{
	const char*	device	= astman_get_header (m, "Device");
	const char*	number	= astman_get_header (m, "Number");
	const char*	message	= astman_get_header (m, "Message");
	const char*	id	= astman_get_header (m, "ActionID");

	char		idtext[256] = "";
	pvt_t*		pvt = NULL;
	char*		msg;
	char		buf[256];

	if (ast_strlen_zero (device))
	{
		astman_send_error (s, m, "Device not specified");
		return 0;
	}

	if (ast_strlen_zero (number))
	{
		astman_send_error (s, m, "Number not specified");
		return 0;
	}

	if (ast_strlen_zero (message))
	{
		astman_send_error (s, m, "Message not specified");
		return 0;
	}

	if (!ast_strlen_zero (id))
	{
		snprintf (idtext, sizeof(idtext), "ActionID: %s\r\n", id);
	}

	pvt = find_device (device);
	if (pvt)
	{
		ast_mutex_lock (&pvt->lock);
		if (pvt->connected && pvt->initialized && pvt->gsm_registered)
		{
			if (pvt->has_sms)
			{
				msg = ast_strdup (message);
				if (at_send_cmgs (pvt, number) || at_fifo_queue_add_ptr (pvt, CMD_AT_CMGS, RES_SMS_PROMPT, msg))
				{
					ast_free (msg);
					ast_log (LOG_ERROR, "[%s] Error sending SMS message\n", pvt->id);
					astman_send_error (s, m, "SMS will not be sent");
				}
				else
				{
					astman_send_ack (s, m, "SMS send successful");
				}
			}
			else
			{
				snprintf (buf, sizeof (buf), "Device %s doesn't handle SMS -- SMS will not be sent", device);
				astman_send_error (s, m, buf);
			}
		}
		else
		{
			snprintf (buf, sizeof (buf), "Device %s not connected / initialized / registered -- SMS will not be sent", device);
			astman_send_error (s, m, buf);
		}
		ast_mutex_unlock (&pvt->lock);
	}
	else
	{
		snprintf (buf, sizeof(buf), "Device %s not found -- SMS will not be sent", device);
		astman_send_error (s, m, buf);
	}

	return 0;
}

/*!
 * \brief Send a DatacardNewUSSD event to the manager
 * This function splits the message in multiple lines, so multi-line
 * USSD messages can be send over the manager API.
 * \param pvt a pvt structure
 * \param message a null terminated buffer containing the message
 */

static void manager_event_new_ussd (pvt_t* pvt, char* message)
{
	struct ast_str*	buf;
	char*		s = message;
	char*		sl;
	size_t		linecount = 0;

	buf = ast_str_create (256);

	while (sl = strsep (&s, "\r\n"))
	{
		if (*sl != '\0')
		{
			ast_str_append (&buf, 0, "MessageLine%zu: %s\r\n", linecount, sl);
			linecount++;
		}
	}

	manager_event (EVENT_FLAG_CALL, "DatacardNewUSSD",
		"Device: %s\r\n"
		"LineCount: %zu\r\n"
		"%s\r\n",
		pvt->id, linecount, ast_str_buffer (buf)
	);

	ast_free (buf);
}

/*!
 * \brief Send a DatacardNewUSSD event to the manager
 * This function splits the message in multiple lines, so multi-line
 * USSD messages can be send over the manager API.
 * \param pvt a pvt structure
 * \param message a null terminated buffer containing the message
 */

static void manager_event_new_ussd_base64 (pvt_t* pvt, char* message)
{
        manager_event (EVENT_FLAG_CALL, "DatacardNewUSSDBase64",
                "Device: %s\r\n"
                "Message: %s\r\n",
                pvt->id, message
        );
}



/*!
 * \brief Send a DatacardNewSMS event to the manager
 * This function splits the message in multiple lines, so multi-line
 * SMS messages can be send over the manager API.
 * \param pvt a pvt structure
 * \param number a null terminated buffer containing the from number
 * \param message a null terminated buffer containing the message
 */

static void manager_event_new_sms (pvt_t* pvt, char* number, char* message)
{
	struct ast_str* buf;
	size_t	linecount = 0;
	char*	s = message;
	char*	sl;

	buf = ast_str_create (256);

	while (sl = strsep (&s, "\r\n"))
	{
		if (*sl != '\0')
		{
			ast_str_append (&buf, 0, "MessageLine%zu: %s\r\n", linecount, sl);
			linecount++;
		}
	}

	manager_event (EVENT_FLAG_CALL, "DatacardNewSMS",
		"Device: %s\r\n"
		"From: %s\r\n"
		"LineCount: %zu\r\n"
		"%s\r\n",
		pvt->id, number, linecount, ast_str_buffer (buf)
	);

	ast_free (buf);
}

/*!
 * \brief Send a DatacardNewSMSBase64 event to the manager
 * \param pvt a pvt structure
 * \param number a null terminated buffer containing the from number
 * \param message_base64 a null terminated buffer containing the base64 encoded message
 */

static void manager_event_new_sms_base64 (pvt_t* pvt, char* number, char* message_base64)
{
	manager_event (EVENT_FLAG_CALL, "DatacardNewSMSBase64",
		"Device: %s\r\n"
		"From: %s\r\n"
		"Message: %s\r\n",
		pvt->id, number, message_base64
	);
}

static int manager_ccwa_disable (struct mansession* s, const struct message* m)
{
	const char*	device	= astman_get_header (m, "Device");
	const char*	id	= astman_get_header (m, "ActionID");

	char		idtext[256] = "";
	pvt_t*		pvt = NULL;
	char		buf[256];

	if (ast_strlen_zero (device))
	{
		astman_send_error (s, m, "Device not specified");
		return 0;
	}

	if (!ast_strlen_zero (id))
	{
		snprintf (idtext, sizeof (idtext), "ActionID: %s\r\n", id);
	}

	pvt = find_device (device);
	if (pvt)
	{
		ast_mutex_lock (&pvt->lock);
		if (pvt->connected && pvt->initialized && pvt->gsm_registered)
		{
			if (at_send_ccwa_disable (pvt) || at_fifo_queue_add (pvt, CMD_AT_CCWA, RES_OK))
			{
				ast_log (LOG_ERROR, "[%s] Error sending CCWA command\n", pvt->id);
				astman_send_error (s, m, "Call-Waiting disable failed");
			}
			else
			{
				astman_send_ack (s, m, "Call-Waiting disabled successful");
			}
		}
		else
		{
			snprintf (buf, sizeof (buf), "Device %s not connected / initialized./ registered", device);
			astman_send_error (s, m, buf);
		}
		ast_mutex_unlock (&pvt->lock);
	}
	else
	{
		snprintf (buf, sizeof (buf), "Device %s not found.", device);
		astman_send_error (s, m, buf);
	}

	return 0;
}

static int manager_reset (struct mansession* s, const struct message* m)
{
	const char*	device	= astman_get_header (m, "Device");
	const char*	id	= astman_get_header (m, "ActionID");

	char		idtext[256] = "";
	pvt_t*		pvt = NULL;
	char		buf[256];

	if (ast_strlen_zero (device))
	{
		astman_send_error (s, m, "Device not specified");
		return 0;
	}

	if (!ast_strlen_zero (id))
	{
		snprintf (idtext, sizeof (idtext), "ActionID: %s\r\n", id);
	}

	pvt = find_device (device);
	if (pvt)
	{
		ast_mutex_lock (&pvt->lock);
		if (pvt->connected)
		{
			if (at_send_cfun (pvt, 1, 1) || at_fifo_queue_add (pvt, CMD_AT_CFUN, RES_OK))
			{
				ast_log (LOG_ERROR, "[%s] Error sending reset command\n", pvt->id);
				astman_send_error (s, m, "Datacard reset failed");
			}
			else
			{
				pvt->incoming = 1;
				astman_send_ack (s, m, "Datacard reseted successful");
			}
		}
		else
		{
			snprintf (buf, sizeof (buf), "Device %s not connected", device);
			astman_send_error (s, m, buf);
		}
		ast_mutex_unlock (&pvt->lock);
	}
	else
	{
		snprintf (buf, sizeof (buf), "Device %s not found", device);
		astman_send_error (s, m, buf);
	}

	return 0;
}
