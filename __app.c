/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

static int app_status_exec (struct ast_channel* channel, void* data)
{
	pvt_t*	pvt;
	char*	parse;
	int	stat;
	char	status[2];

	AST_DECLARE_APP_ARGS (args,
		AST_APP_ARG (device);
		AST_APP_ARG (variable);
	);

	if (ast_strlen_zero (data))
	{
		return -1;
	}

	parse = ast_strdupa (data);

	AST_STANDARD_APP_ARGS (args, parse);

	if (ast_strlen_zero (args.device) || ast_strlen_zero (args.variable))
	{
		return -1;
	}

	stat = 1;

	AST_RWLIST_RDLOCK (&devices);
	AST_RWLIST_TRAVERSE (&devices, pvt, entry)
	{
		if (!strcmp (pvt->id, args.device))
		{
			ast_mutex_lock(&pvt->lock);
			if (pvt->connected)
			{
				stat = 2;
			}
			if (pvt->owner)
			{
				stat = 3;
			}
			ast_mutex_unlock (&pvt->lock);

			break;
		}
	}
	AST_RWLIST_UNLOCK (&devices);

	snprintf (status, sizeof (status), "%d", stat);
	pbx_builtin_setvar_helper (channel, args.variable, status);

	return 0;
}

static int app_send_sms_exec (struct ast_channel* channel, void* data)
{
	pvt_t*	pvt;
	char*	parse;
	char*	msg;

	AST_DECLARE_APP_ARGS (args,
		AST_APP_ARG (device);
		AST_APP_ARG (number);
		AST_APP_ARG (message);
	);

	if (ast_strlen_zero (data))
	{
		return -1;
	}

	parse = ast_strdupa (data);

	AST_STANDARD_APP_ARGS (args, parse);

	if (ast_strlen_zero (args.device))
	{
		ast_log (LOG_ERROR, "NULL device for message -- SMS will not be sent\n");
		return -1;
	}

	if (ast_strlen_zero (args.number))
	{
		ast_log (LOG_ERROR, "NULL destination for message -- SMS will not be sent\n");
		return -1;
	}

	if (ast_strlen_zero (args.message))
	{
		ast_log (LOG_ERROR, "NULL Message to be sent -- SMS will not be sent\n");
		return -1;
	}

	pvt = find_device (args.device);
	if (pvt)
	{
		ast_mutex_lock (&pvt->lock);
		if (pvt->connected && pvt->initialized && pvt->gsm_registered)
		{
			if (pvt->has_sms)
			{
				msg = ast_strdup (args.message);
				if (at_send_cmgs (pvt, args.number) || at_fifo_queue_add_ptr (pvt, CMD_AT_CMGS, RES_SMS_PROMPT, msg))
				{
					ast_mutex_unlock (&pvt->lock);
					ast_free (msg);
					ast_log (LOG_ERROR, "[%s] Error sending SMS message\n", pvt->id);

					return -1;
				}
			}
			else
			{
				ast_log (LOG_ERROR, "Datacard %s doesn't handle SMS -- SMS will not be sent\n", args.device);
			}
		}
		else
		{
			ast_log (LOG_ERROR, "Datacard %s wasn't connected / initialized / registered -- SMS will not be sent\n", args.device);
		}
		ast_mutex_unlock (&pvt->lock);

	}
	else
	{
		ast_log (LOG_ERROR, "Datacard %s wasn't found -- SMS will not be sent\n", args.device);
		return -1;
	}

	return 0;
}
