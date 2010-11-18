/*
 * chan_datacard
 *
 * Copyright (C) 2009 - 2010
 *
 * Artem Makhutov <artem@makhutov.org>
 * http://www.makhutov.org
 * 
 * Dmitry Vagin <dmitry2004@yandex.ru>
 *
 * chan_datacard is based on chan_mobile by Digium
 * (Mark Spencer <markster@digium.com>)
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 *
 * \brief UMTS Voice Datacard channel driver
 *
 * \author Artem Makhutov <artem@makhutov.org>
 * \author Dave Bowerman <david.bowerman@gmail.com>
 * \author Dmitry Vagin <dmitry2004@yandex.ru>
 *
 * \ingroup channel_drivers
 */

#include <asterisk.h>

ASTERISK_FILE_VERSION(__FILE__, "$Rev: 174 $")

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

#include <iconv.h>

#include <asterisk/app.h>
#include <asterisk/callerid.h>
#include <asterisk/causes.h>
#include <asterisk/frame.h>
#include <asterisk/channel.h>
#include <asterisk/cli.h>
#include <asterisk/config.h>
#include <asterisk/devicestate.h>
#include <asterisk/dsp.h>
#include <asterisk/io.h>
#include <asterisk/linkedlists.h>
#include <asterisk/lock.h>
#include <asterisk/logger.h>
#include <asterisk/manager.h>
#include <asterisk/module.h>
#include <asterisk/musiconhold.h>
#include <asterisk/options.h>
#include <asterisk/pbx.h>
#include <asterisk/timing.h>
#include <asterisk/utils.h>
#include <asterisk/version.h>

#ifdef NO_MEMMEM 
#include "__memmem.c"
#endif

#include "__ringbuffer.h"
#include "chan_datacard.h"

#include "__char_conv.c"
#include "__helpers.c"
#include "__ringbuffer.c"

#include "__cli.c"

#ifdef __APP__
#include "__app.c"
#endif

#ifdef __MANAGER__
#include "__manager.c"
#endif

#include "__channel.c"

#include "__at_fifo_queue.c"
#include "__at_parse.c"

#include "__at_send.c"
#include "__at_read.c"
#include "__at_response.c"


static int opentty (char* dev)
{
	int		fd;
	struct termios	term_attr;

	fd = open (dev, O_RDWR | O_NOCTTY);

	if (fd < 0)
	{
		ast_log (LOG_WARNING, "Unable to open '%s'\n", dev);
		return -1;
	}

	if (tcgetattr (fd, &term_attr) != 0)
	{
		ast_log (LOG_WARNING, "tcgetattr() failed '%s'\n", dev);
		return -1;
	}

	term_attr.c_cflag = B115200 | CS8 | CREAD | CRTSCTS;
	term_attr.c_iflag = 0;
	term_attr.c_oflag = 0;
	term_attr.c_lflag = 0;
	term_attr.c_cc[VMIN] = 1;
	term_attr.c_cc[VTIME] = 0;

	if (tcsetattr (fd, TCSAFLUSH, &term_attr) != 0)
	{
		ast_log (LOG_WARNING, "tcsetattr() failed '%s'\n", dev);
	}

	return fd;
}

/*!
 * Get status of the datacard. It might happen that the device disappears
 * (e.g. due to a USB unplug).
 *
 * \return 1 if device seems ok, 0 if it seems not available
 */

static int device_status (int fd)
{
	struct termios t;

	if (fd < 0)
	{
		return -1;
	}

	return tcgetattr (fd, &t);
}

static void* do_monitor_phone (void* data)
{
	pvt_t*		pvt = (pvt_t*) data;
	at_res_t	at_res;
	at_queue_t*	e;
	int		t;
	int		res;
	struct iovec	iov[2];
	int		iovcnt;
	size_t		size;
	size_t		i = 0;

	/* start datacard initilization with the AT request */
	ast_mutex_lock (&pvt->lock);

	pvt->timeout = 10000;

	if (at_send_at (pvt) || at_fifo_queue_add (pvt, CMD_AT, RES_OK))
	{
		ast_log (LOG_ERROR, "[%s] Error sending AT\n", pvt->id);
		goto e_cleanup;
	}

	ast_mutex_unlock (&pvt->lock);

	while (!check_unloading ())
	{
		ast_mutex_lock (&pvt->lock);

		if (device_status (pvt->data_fd) || device_status (pvt->audio_fd))
		{
			ast_log (LOG_ERROR, "Lost connection to Datacard %s\n", pvt->id);
			goto e_cleanup;
		}
		t = pvt->timeout;

		ast_mutex_unlock (&pvt->lock);


		if (!at_wait (pvt, &t))
		{
			ast_mutex_lock (&pvt->lock);
			if (!pvt->initialized)
			{
				ast_debug (1, "[%s] timeout waiting for data, disconnecting\n", pvt->id);

				if ((e = at_fifo_queue_head (pvt)))
				{
					ast_debug (1, "[%s] timeout while waiting '%s' in response to '%s'\n", pvt->id,
							at_res2str (e->res), at_cmd2str (e->cmd));
				}

				goto e_cleanup;
			}
			else
			{
				ast_mutex_unlock (&pvt->lock);
				continue;
			}
		}


		ast_mutex_lock (&pvt->lock);

		if (at_read (pvt))
		{
			goto e_cleanup;
		}
		while ((iovcnt = at_read_result_iov (pvt)) > 0)
		{
			at_res = at_read_result_classification (pvt, iovcnt);

			if (at_response (pvt, iovcnt, at_res))
			{
				goto e_cleanup;
			}
		}
		ast_mutex_unlock (&pvt->lock);
	}

	ast_mutex_lock (&pvt->lock);

e_cleanup:
	if (!pvt->initialized)
	{
		ast_verb (3, "Error initializing Datacard %s\n", pvt->id);
	}

	disconnect_datacard (pvt);

	ast_mutex_unlock (&pvt->lock);

	return NULL;
}

static int disconnect_datacard (pvt_t* pvt)
{
	if (pvt->owner)
	{
		ast_debug (1, "[%s] Datacard disconnected, hanging up owner\n", pvt->id);
		pvt->needchup = 0;
		channel_queue_hangup (pvt, 0);
	}

	close (pvt->data_fd);
	close (pvt->audio_fd);

	pvt->data_fd		= -1;
	pvt->audio_fd		= -1;

	pvt->connected		= 0;
	pvt->initialized	= 0;
	pvt->gsm_registered	= 0;

	pvt->incoming		= 0;
	pvt->outgoing		= 0;
	pvt->needring		= 0;
	pvt->needchup		= 0;
	
	pvt->gsm_reg_status	= -1;

	pvt->manufacturer[0]	= '\0';
	pvt->model[0]		= '\0';
	pvt->firmware[0]	= '\0';
	pvt->imei[0]		= '\0';
	pvt->imsi[0]		= '\0';

	ast_copy_string (pvt->provider_name,	"NONE",		sizeof (pvt->provider_name));
	ast_copy_string (pvt->number,		"Unknown",	sizeof (pvt->number));

	rb_init (&pvt->d_read_rb, pvt->d_read_buf, sizeof (pvt->d_read_buf));

	at_fifo_queue_flush (pvt);

	ast_verb (3, "Datacard %s has disconnected\n", pvt->id);

#ifdef __MANAGER__
	manager_event (EVENT_FLAG_SYSTEM, "DatacardStatus", "Status: Disconnect\r\nDevice: %s\r\n", pvt->id);
#endif

	return 1;
}

static inline int start_monitor (pvt_t* pvt)
{
	if (ast_pthread_create_background (&pvt->monitor_thread, NULL, do_monitor_phone, pvt) < 0)
	{
		pvt->monitor_thread = AST_PTHREADT_NULL;
		return 0;
	}

	return 1;
}

static void* do_discovery (void* data)
{
	pvt_t* pvt;

	while (!check_unloading ())
	{
		AST_RWLIST_RDLOCK (&devices);
		AST_RWLIST_TRAVERSE (&devices, pvt, entry)
		{
			ast_mutex_lock (&pvt->lock);

			if (!pvt->connected)
			{
				ast_verb (3, "Datacard %s trying to connect on %s...\n", pvt->id, pvt->data_tty);

				if ((pvt->data_fd = opentty (pvt->data_tty)) > -1)
				{
					if ((pvt->audio_fd = opentty (pvt->audio_tty)) > -1)
					{
						if (start_monitor (pvt))
						{
							pvt->connected = 1;
#ifdef __MANAGER__
							manager_event (EVENT_FLAG_SYSTEM, "DatacardStatus", "Status: Connect\r\nDevice: %s\r\n", pvt->id);
#endif
							ast_verb (3, "Datacard %s has connected, initializing...\n", pvt->id);
						}
					}
				}
			}

			ast_mutex_unlock (&pvt->lock);
		}
		AST_RWLIST_UNLOCK (&devices);

		/* Go to sleep (only if we are not unloading) */
		if (!check_unloading ())
		{
			sleep (discovery_interval);
		}
	}

	return NULL;
}



/* Module */

/*!
 * \brief Load a device from the configuration file.
 * \param cfg the config to load the device from
 * \param cat the device to load
 * \return NULL on error, a pointer to the device that was loaded on success
 */

static pvt_t* load_device (struct ast_config* cfg, const char* cat)
{
	pvt_t*			pvt;
	const char*		audio_tty;
	const char*		data_tty;
	struct ast_variable*	v;

	ast_debug (1, "Reading configuration for device %s\n", cat);

	audio_tty = ast_variable_retrieve (cfg, cat, "audio");
	data_tty  = ast_variable_retrieve (cfg, cat, "data");

	if (ast_strlen_zero (audio_tty) || ast_strlen_zero (data_tty))
	{
		ast_log (LOG_ERROR, "Skipping device %s. Missing required audio_tty or data_tty setting\n", cat);
		return NULL;
	}

	/* create and initialize our pvt structure */

	pvt = ast_calloc (1, sizeof (*pvt));
	if (!pvt)
	{
		ast_log (LOG_ERROR, "Skipping device %s. Error allocating memory\n", cat);
		return NULL;
	}

	ast_mutex_init (&pvt->lock);

	AST_LIST_HEAD_INIT_NOLOCK (&pvt->at_queue);

	rb_init (&pvt->d_read_rb, pvt->d_read_buf, sizeof (pvt->d_read_buf));


	/* populate the pvt structure */

	ast_copy_string (pvt->id,		cat,		sizeof (pvt->id));
	ast_copy_string (pvt->data_tty,		data_tty,	sizeof (pvt->data_tty));
	ast_copy_string (pvt->audio_tty,	audio_tty,	sizeof (pvt->audio_tty));

	/* set some defaults */

	pvt->monitor_thread		= AST_PTHREADT_NULL;
	pvt->audio_fd			= -1;
	pvt->data_fd			= -1;
	pvt->timeout			= 10000;
	pvt->cusd_use_ucs2_decoding	=  1;
	pvt->gsm_reg_status		= -1;

	ast_copy_string (pvt->provider_name,	"NONE",		sizeof (pvt->provider_name));
	ast_copy_string (pvt->number,		"Unknown",	sizeof (pvt->number));
	ast_copy_string (pvt->context,		"default",	sizeof (pvt->context));

	pvt->reset_datacard		=  1;
	pvt->u2diag			= -1;
	pvt->callingpres		= -1;


	/* setup the dsp */

	pvt->dsp = ast_dsp_new ();
	if (!pvt->dsp)
	{
		ast_log(LOG_ERROR, "Skipping device %s. Error setting up dsp for dtmf detection\n", cat);
		ast_free (pvt);
		return NULL;
	}

	ast_dsp_set_features (pvt->dsp, DSP_FEATURE_DIGIT_DETECT);
	ast_dsp_set_digitmode (pvt->dsp, DSP_DIGITMODE_DTMF | DSP_DIGITMODE_RELAXDTMF);

	for (v = ast_variable_browse (cfg, cat); v; v = v->next)
	{
		if (!strcasecmp (v->name, "context"))
		{
			ast_copy_string (pvt->context, v->value, sizeof (pvt->context));
		}
		else if (!strcasecmp (v->name, "group"))
		{
			pvt->group = (int) strtol (v->value, (char**) NULL, 10);		/* group is set to 0 if invalid */
		}
		else if (!strcasecmp (v->name, "rxgain"))
		{
			pvt->rxgain = (int) strtol (v->value, (char**) NULL, 10);		/* rxgain is set to 0 if invalid */
		}
		else if (!strcasecmp (v->name, "txgain"))
		{
			pvt->txgain = (int) strtol (v->value, (char**) NULL, 10);		/* txgain is set to 0 if invalid */
		}
		else if (!strcasecmp (v->name, "autodeletesms"))
		{
			pvt->auto_delete_sms = ast_true (v->value);				/* auto_delete_sms is set to 0 if invalid */

		}
		else if (!strcasecmp (v->name, "resetdatacard"))
		{
			pvt->reset_datacard = ast_true (v->value);				/* reset_datacard is set to 0 if invalid */
		}
		else if (!strcasecmp (v->name, "u2diag"))
		{
			errno = 0;
			pvt->u2diag = (int) strtol (v->value, (char**) NULL, 10);		/* u2diag is set to -1 if invalid */
			if (pvt->u2diag == 0 && errno == EINVAL)
			{
				pvt->u2diag = -1;
			}
		}
		else if (!strcasecmp (v->name, "usecallingpres"))
		{
			pvt->usecallingpres = ast_true (v->value);				/* usecallingpres is set to 0 if invalid */
		}
		else if (!strcasecmp (v->name, "callingpres"))
		{
			pvt->callingpres = ast_parse_caller_presentation (v->value);
			if (pvt->callingpres == -1)
			{
				errno = 0;
				pvt->callingpres = (int) strtol (v->value, (char**) NULL, 10);	/* callingpres is set to -1 if invalid */
				if (pvt->callingpres == 0 && errno == EINVAL)
				{
					pvt->callingpres = -1;
				}
			}
		}
		else if (!strcasecmp (v->name, "disablesms"))
		{
			pvt->disablesms = ast_true (v->value);				/* disablesms is set to 0 if invalid */
		}
	}

	ast_debug (1, "[%s] Loaded device\n", pvt->id);
	ast_log (LOG_NOTICE, "Loaded device %s\n", pvt->id);

	AST_RWLIST_WRLOCK (&devices);
	AST_RWLIST_INSERT_HEAD (&devices, pvt, entry);
	AST_RWLIST_UNLOCK (&devices);

	return pvt;
}

static int load_config ()
{
	struct ast_config*	cfg;
	const char*		cat;
	struct ast_variable*	v;
	struct ast_flags	config_flags = { 0 };

	if ((cfg = ast_config_load (CONFIG_FILE, config_flags)) == NULL)
	{
		return -1;
	}

	/* parse [general] section */
	for (v = ast_variable_browse (cfg, "general"); v; v = v->next)
	{
		/* handle jb conf */
		if (!ast_jb_read_conf (&jbconf_global, v->name, v->value))
		{
			continue;
		}

		if (!strcasecmp (v->name, "interval"))
		{
			errno = 0;
			discovery_interval = (int) strtol (v->value, (char**) NULL, 10);
			if (discovery_interval == 0 && errno == EINVAL)
			{
				ast_log (LOG_NOTICE, "Error parsing 'interval' in general section, using default value\n");
				discovery_interval = DEF_DISCOVERY_INT;
			}
		}
	}

	/* now load devices */
	for (cat = ast_category_browse (cfg, NULL); cat; cat = ast_category_browse (cfg, cat))
	{
		if (strcasecmp (cat, "general"))
		{
			load_device (cfg, cat);
		}
	}

	ast_config_destroy (cfg);

	return 0;
}


/*!
 * \brief Check if the module is unloading.
 * \retval 0 not unloading
 * \retval 1 unloading
 */

static inline int check_unloading ()
{
	int res;

	ast_mutex_lock (&unload_mtx);
	res = unloading_flag;
	ast_mutex_unlock (&unload_mtx);

	return res;
}

static int unload_module ()
{
	pvt_t* pvt;

	/* First, take us out of the channel loop */
	ast_channel_unregister (&channel_tech);

	/* Unregister the CLI & APP & MANAGER */
	ast_cli_unregister_multiple (cli, sizeof (cli) / sizeof (cli[0]));

#ifdef __APP__
	ast_unregister_application (app_status);
	ast_unregister_application (app_send_sms);
#endif

#ifdef __MANAGER__
	ast_manager_unregister ("DatacardShowDevices");
	ast_manager_unregister ("DatacardSendUSSD");
	ast_manager_unregister ("DatacardSendSMS");
#endif

	/* signal everyone we are unloading */
	ast_mutex_lock (&unload_mtx);
	unloading_flag = 1;
	ast_mutex_unlock (&unload_mtx);

	/* Kill the discovery thread */
	if (discovery_thread != AST_PTHREADT_NULL)
	{
		pthread_kill (discovery_thread, SIGURG);
		pthread_join (discovery_thread, NULL);
	}

	/* Destroy the device list */
	AST_RWLIST_WRLOCK (&devices);
	while ((pvt = AST_RWLIST_REMOVE_HEAD (&devices, entry)))
	{
		if (pvt->monitor_thread != AST_PTHREADT_NULL)
		{
			pthread_kill (pvt->monitor_thread, SIGURG);
			pthread_join (pvt->monitor_thread, NULL);
		}

		close (pvt->audio_fd);
		close (pvt->data_fd);

		at_fifo_queue_flush (pvt);

		ast_dsp_free (pvt->dsp);
		ast_free (pvt);
	}
	AST_RWLIST_UNLOCK (&devices);

	return 0;
}

static int load_module ()
{
	/* Copy the default jb config over global jbconf */
	memmove (&jbconf_global, &jbconf_default, sizeof (jbconf_global));

	memset (silence_frame, 0, sizeof (silence_frame));

	if (load_config ())
	{
		ast_log (LOG_ERROR, "Errors reading config file " CONFIG_FILE ", Not loading module\n");
		return AST_MODULE_LOAD_DECLINE;
	}

	/* Spin the discovery thread */
	if (ast_pthread_create_background (&discovery_thread, NULL, do_discovery, NULL) < 0)
	{
		ast_log (LOG_ERROR, "Unable to create discovery thread\n");
		return AST_MODULE_LOAD_FAILURE;
	}

	/* register our channel type */
	if (ast_channel_register (&channel_tech))
	{
		ast_log (LOG_ERROR, "Unable to register channel class %s\n", "Datacard");
		return AST_MODULE_LOAD_FAILURE;
	}

	ast_cli_register_multiple (cli, sizeof (cli) / sizeof (cli[0]));

#ifdef __APP__
	ast_register_application (app_status,   app_status_exec,   app_status_synopsis,   app_status_desc);
	ast_register_application (app_send_sms, app_send_sms_exec, app_send_sms_synopsis, app_send_sms_desc);
#endif

#ifdef __MANAGER__
	ast_manager_register2 (
		"DatacardShowDevices",
		EVENT_FLAG_SYSTEM | EVENT_FLAG_CONFIG | EVENT_FLAG_REPORTING,
		manager_show_devices,
		"List Datacard devices",
		manager_show_devices_desc
	);

	ast_manager_register2 (
		"DatacardSendUSSD",
		EVENT_FLAG_SYSTEM | EVENT_FLAG_CONFIG | EVENT_FLAG_REPORTING,
		manager_send_ussd,
		"Send a ussd command to the datacard.",
		manager_send_ussd_desc
	);

	ast_manager_register2 (
		"DatacardSendSMS",
		EVENT_FLAG_SYSTEM | EVENT_FLAG_CONFIG | EVENT_FLAG_REPORTING,
		manager_send_sms,
		"Send a sms message.",
		manager_send_sms_desc
	);
	
	ast_manager_register2 (
		"DatacardCCWADisable",
		EVENT_FLAG_SYSTEM | EVENT_FLAG_CONFIG | EVENT_FLAG_REPORTING,
		manager_ccwa_disable,
		"Disabled Call-Waiting on a datacard.",
		manager_ccwa_disable_desc
	);
	
	ast_manager_register2 (
		"DatacardReset",
		EVENT_FLAG_SYSTEM | EVENT_FLAG_CONFIG | EVENT_FLAG_REPORTING,
		manager_reset,
		"Reset a datacard.",
		manager_reset_desc
	);
#endif

	return AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD (ASTERISK_GPL_KEY, "Datacard Channel Driver");
