/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

/*!
 * \brief Get the string representation of the given AT command
 * \param cmd -- the command to process
 * \return a string describing the given command
 */

static const char* at_cmd2str (at_cmd_t cmd)
{
	switch (cmd)
	{
		case CMD_AT:
			return "AT";

		case CMD_AT_A:
			return "ATA";

		case CMD_AT_CCWA:
			return "AT+CCWA";

		case CMD_AT_CFUN:
			return "AT+CFUN";

		case CMD_AT_CGMI:
			return "AT+CGMI";

		case CMD_AT_CGMM:
			return "AT+CGMM";

		case CMD_AT_CGMR:
			return "AT+CGMR";

		case CMD_AT_CGSN:
			return "AT+CGSN";

		case CMD_AT_CHUP:
			return "AT+CHUP";

		case CMD_AT_CIMI:
			return "AT+CIMI";

		case CMD_AT_CLIP:
			return "AT+CLIP";

		case CMD_AT_CLIR:
			return "AT+CLIR";

		case CMD_AT_CLVL:
			return "AT+CLVL";

		case CMD_AT_CMGD:
			return "AT+CMGD";

		case CMD_AT_CMGF:
			return "AT+CMGF";

		case CMD_AT_CMGR:
			return "AT+CMGR";

		case CMD_AT_CMGS:
			return "AT+CMGS";

		case CMD_AT_CNMI:
			return "AT+CNMI";

		case CMD_AT_CNUM:
			return "AT+CNUM";

		case CMD_AT_COPS:
			return "AT+COPS?";

		case CMD_AT_COPS_INIT:
			return "AT+COPS=";

		case CMD_AT_CPIN:
			return "AT+CPIN?";

		case CMD_AT_CPMS:
			return "AT+CPMS";

		case CMD_AT_CREG:
			return "AT+CREG?";

		case CMD_AT_CREG_INIT:
			return "AT+CREG=";

		case CMD_AT_CSCS:
			return "AT+CSCS";

		case CMD_AT_CSQ:
			return "AT+CSQ";

		case CMD_AT_CSSN:
			return "AT+CSSN";

		case CMD_AT_CUSD:
			return "AT+CUSD";

		case CMD_AT_CVOICE:
			return "AT^CVOICE";

		case CMD_AT_D:
			return "ATD";

		case CMD_AT_DDSETEX:
			return "AT^DDSETEX";

		case CMD_AT_DTMF:
			return "AT^DTMF";

		case CMD_AT_E:
			return "ATE";

		case CMD_AT_SMS_TEXT:
			return "SMS TEXT";

		case CMD_AT_U2DIAG:
			return "AT^U2DIAG";

		case CMD_AT_Z:
			return "ATZ";

		case CMD_AT_CMEE:
			return "AT+CMEE";

		case CMD_UNKNOWN:
			return "UNKNOWN";

		default:
			return "UNDEFINED";
	}
}

/*!
 * \brief Get the string representation of the given AT response
 * \param res -- the response to process
 * \return a string describing the given response
 */

static const char* at_res2str (at_res_t res)
{
	switch (res)
	{
		case RES_OK:
			return "OK";

		case RES_ERROR:
			return "ERROR";

		case RES_CMS_ERROR:
			return "+CMS ERROR";

		case RES_RING:
			return "RING";

		case RES_CSSI:
			return "+CSSI";

		case RES_CONN:
			return "^CONN";

		case RES_CEND:
			return "^CEND";

		case RES_ORIG:
			return "^ORIG";

		case RES_CONF:
			return "^CONF";

		case RES_SMMEMFULL:
			return "^SMMEMFULL";

		case RES_CSQ:
			return "+CSQ";

		case RES_RSSI:
			return "^RSSI";

		case RES_BOOT:
			return "^BOOT";

		case RES_CLIP:
			return "+CLIP";

		case RES_CMTI:
			return "+CMTI";

		case RES_CMGR:
			return "+CMGR";

		case RES_SMS_PROMPT:
			return "> ";

		case RES_CUSD:
			return "+CUSD";

		case RES_BUSY:
			return "BUSY";

		case RES_NO_DIALTONE:
			return "NO DIALTONE";

		case RES_NO_CARRIER:
			return "NO CARRIER";

		case RES_CPIN:
			return "+CPIN";

		case RES_CNUM:
			return "+CNUM";

		case RES_COPS:
			return "+COPS";

		case RES_SRVST:
			return "^SRVST";

		case RES_CREG:
			return "+CREG";

		case RES_MODE:
			return "^MODE";

		case RES_PARSE_ERROR:
			return "PARSE ERROR";

		case RES_UNKNOWN:
			return "UNKNOWN";

		default:
			return "UNDEFINED";
	}
}



/*!
 * \brief Parse a CLIP event
 * \param pvt -- pvt structure
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * @note str will be modified when the CID string is parsed
 * \return NULL on error (parse error) or a pointer to the caller id inforamtion in str on success
 */

static inline char* at_parse_clip (pvt_t* pvt, char* str, size_t len)
{
	size_t	i;
	int	state;
	char*	clip = NULL;

	/*
	 * parse clip info in the following format:
	 * +CLIP: "123456789",128,...
	 */

	for (i = 0, state = 0; i < len && state < 3; i++)
	{
		switch (state)
		{
			case 0: /* search for start of the number (") */
				if (str[i] == '"')
				{
					state++;
				}
				break;

			case 1: /* mark the number */
				clip = &str[i];
				state++;
				/* fall through */

			case 2: /* search for the end of the number (") */
				if (str[i] == '"')
				{
					str[i] = '\0';
					state++;
				}
				break;
		}
	}

	if (state != 3)
	{
		return NULL;
	}

	return clip;
}

/*!
 * \brief Parse a CNUM response
 * \param pvt -- pvt structure
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * @note str will be modified when the CNUM message is parsed
 * \return NULL on error (parse error) or a pointer to the subscriber number
 */

static inline char* at_parse_cnum (pvt_t* pvt, char* str, size_t len)
{
	size_t	i;
	int	state;
	char*	number = NULL;

	/*
	 * parse CNUM response in the following format:
	 * +CNUM: "<name>","<number>",<type>
	 */

	for (i = 0, state = 0; i < len && state < 5; i++)
	{
		switch (state)
		{
			case 0: /* search for start of the name (") */
				if (str[i] == '"')
				{
					state++;
				}
				break;

			case 1: /* search for the end of the name (") */
				if (str[i] == '"')
				{
					state++;
				}
				break;

			case 2: /* search for the start of the number (") */
				if (str[i] == '"')
				{
					state++;
				}
				break;

			case 3: /* mark the number */
				number = &str[i];
				state++;
				/* fall through */

			case 4: /* search for the end of the number (") */
				if (str[i] == '"')
				{
					str[i] = '\0';
					state++;
				}
				break;
		}
	}

	if (state != 5)
	{
		return NULL;
	}

	return number;
}

/*!
 * \brief Parse a COPS response
 * \param pvt -- pvt structure
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * @note str will be modified when the COPS message is parsed
 * \return NULL on error (parse error) or a pointer to the provider name
 */

static inline char* at_parse_cops (pvt_t* pvt, char* str, size_t len)
{
	size_t	i;
	int	state;
	char*	provider = NULL;

	/*
	 * parse COPS response in the following format:
	 * +COPS: <mode>[,<format>,<oper>]
	 */

	for (i = 0, state = 0; i < len && state < 3; i++)
	{
		switch (state)
		{
			case 0: /* search for start of the provider name (") */
				if (str[i] == '"')
				{
					state++;
				}
				break;

			case 1: /* mark the provider name */
				provider = &str[i];
				state++;
				/* fall through */

			case 2: /* search for the end of the provider name (") */
				if (str[i] == '"')
				{
					str[i] = '\0';
					state++;
				}
				break;
		}
	}

	if (state != 3)
	{
		return NULL;
	}

	return provider;
}

/*!
 * \brief Parse a CREG response
 * \param pvt -- pvt structure
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \param gsm_reg -- a pointer to a int
 * \param gsm_reg_status -- a pointer to a int
 * \param lac -- a pointer to a char pointer which will store the location area code in hex format
 * \param ci  -- a pointer to a char pointer which will store the cell id in hex format
 * @note str will be modified when the CREG message is parsed
 * \retval  0 success
 * \retval -1 parse error
 */

static inline int at_parse_creg (pvt_t* pvt, char* str, size_t len, int* gsm_reg, int* gsm_reg_status, char** lac, char** ci)
{
	size_t	i;
	int	state;
	char*	p1 = NULL;
	char*	p2 = NULL;
	char*	p3 = NULL;
	char*	p4 = NULL;

	*gsm_reg = 0;
	*gsm_reg_status = -1;
	*lac = NULL;
	*ci  = NULL;

	/*
	 * parse CREG response in the following format:
	 * +CREG: [<p1>,]<p2>[,<p3>,<p4>]
	 */

	for (i = 0, state = 0; i < len && state < 8; i++)
	{
		switch (state)
		{
			case 0:
				if (str[i] == ':')
				{
					state++;
				}
				break;

			case 1:
				if (str[i] != ' ')
				{
					p1 = &str[i];
					state++;
				}
				/* fall through */

			case 2:
				if (str[i] == ',')
				{
					str[i] = '\0';
					state++;
				}
				break;

			case 3:
				p2 = &str[i];
				state++;
				/* fall through */

			case 4:
				if (str[i] == ',')
				{
					str[i] = '\0';
					state++;
				}
				break;

			case 5:
				if (str[i] != ' ')
				{
					p3 = &str[i];
					state++;
				}
				/* fall through */

			case 6:
				if (str[i] == ',')
				{
					str[i] = '\0';
					state++;
				}
				break;

			case 7:
				if (str[i] != ' ')
				{
					p4 = &str[i];
					state++;
				}
				break;
		}
	}

	if (state < 2)
	{
		return -1;
	}

	if ((p2 && !p3 && !p4) || (p2 && p3 && p4))
	{
		p1 = p2;
	}

	if (p1)
	{
		errno = 0;
		*gsm_reg_status = (int) strtol (p1, (char**) NULL, 10);
		if (*gsm_reg_status == 0 && errno == EINVAL)
		{
			*gsm_reg_status = -1;
			return -1;
		}

		if (*gsm_reg_status == 1 || *gsm_reg_status == 5)
		{
			*gsm_reg = 1;
		}
	}

	if (p2 && p3 && !p4)
	{
		*lac = p2;
		*ci  = p3;
	}
	else if (p3 && p4)
	{
		*lac = p3;
		*ci  = p4;
	}

	return 0;
}

/*!
 * \brief Parse a CMTI notification
 * \param pvt -- pvt structure
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * @note str will be modified when the CMTI message is parsed
 * \return -1 on error (parse error) or the index of the new sms message
 */

static inline int at_parse_cmti (pvt_t* pvt, char* str, size_t len)
{
	int index = -1;

	/*
	 * parse cmti info in the following format:
	 * +CMTI: <mem>,<index> 
	 */

	if (!sscanf (str, "+CMTI: %*[^,],%d", &index))
	{
		ast_debug(2, "[%s] Error parsing CMTI event '%s'\n", pvt->id, str);
		return -1;
	}

	return index;
}

/*!
 * \brief Parse a CMGR message
 * \param pvt -- pvt structure
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \param number -- a pointer to a char pointer which will store the from number
 * \param text -- a pointer to a char pointer which will store the message text
 * @note str will be modified when the CMGR message is parsed
 * \retval  0 success
 * \retval -1 parse error
 */

static inline int at_parse_cmgr (pvt_t* pvt, char* str, size_t len, char** number, char** text)
{
	size_t	i;
	int	state;

	*number = NULL;
	*text   = NULL;

	/*
	 * parse cmgr info in the following format:
	 * +CMGR: <msg status>,"+123456789",...\r\n
	 * <message text>
	 */

	for (i = 0, state = 0; i < len && state < 6; i++)
	{
		switch (state)
		{
			case 0: /* search for start of the number section (,) */
				if (str[i] == ',')
				{
					state++;
				}
				break;

			case 1: /* find the opening quote (") */
				if (str[i] == '"')
				{
					state++;
				}
				break;

			case 2: /* mark the start of the number */
				*number = &str[i];
				state++;
				break;

			/* fall through */

			case 3: /* search for the end of the number (") */
				if (str[i] == '"')
				{
					str[i] = '\0';
					state++;
				}
				break;

			case 4: /* search for the start of the message text (\n) */
				if (str[i] == '\n')
				{
					state++;
				}
				break;

			case 5: /* mark the start of the message text */
				*text = &str[i];
				state++;
				break;
		}
	}

	if (state != 6)
	{
		return -1;
	}

	return 0;
}

 /*!
 * \brief Parse a CUSD answer
 * \param pvt -- pvt structure
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * @note str will be modified when the CUSD string is parsed
 * \retval  0 success
 * \retval -1 parse error
 */

static inline int at_parse_cusd (pvt_t* pvt, char* str, size_t len, char** cusd, unsigned char* dcs)
{
	size_t	i;
	int	state;
	char*	p = NULL;

	*cusd = NULL;
	*dcs  = 0;

	/*
	 * parse cusd message in the following format:
	 * +CUSD: 0,"100,00 EURO, valid till 01.01.2010, you are using tariff "Mega Tariff". More informations *111#.",15
	 */

	for (i = 0, state = 0; i < len && state < 5; i++)
	{
		switch (state)
		{
			case 0:
				if (str[i] == '"')
				{
					state++;
				}
				break;

			case 1:
				*cusd = &str[i];
				state++;
				break;

			case 2:
				if (str[i] == '"')
				{
					str[i] = '\0';
					state++;
				}
				break;

			case 3:
				if (str[i] == ',')
				{
					state++;
				}
				break;

			case 4:
				p = &str[i];
				state++;
				break;
		}
	}

	if (state != 5)
	{
		return -1;
	}

	errno = 0;
	*dcs = (unsigned char) strtol (p, (char**) NULL, 10);
	if (errno == EINVAL)
	{
		return -1;
	}

	return 0;
}

/*!
 * \brief Parse a CPIN notification
 * \param pvt -- pvt structure
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \return  2 if PUK required
 * \return  1 if PIN required
 * \return  0 if no PIN required
 * \return -1 on error (parse error) or card lock
 */

static inline int at_parse_cpin (pvt_t* pvt, char* str, size_t len)
{
	if (memmem (str, len, "READY", 5))
	{
		return 0;
	}
	if (memmem (str, len, "SIM PIN", 7))
	{
		ast_log (LOG_ERROR, "Datacard %s needs PIN code!\n", pvt->id);
		return 1;
	}
	if (memmem (str, len, "SIM PUK", 7))
	{
		ast_log (LOG_ERROR, "Datacard %s needs PUK code!\n", pvt->id);
		return 2;
	}

	ast_log (LOG_ERROR, "[%s] Error parsing +CPIN message: %s\n", pvt->id, str);

	return -1;
}

/*!
 * \brief Parse +CSQ response
 * \param pvt -- pvt struct
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static inline int at_parse_csq (pvt_t* pvt, char* str, size_t len, int* rssi)
{
	/*
	 * parse +CSQ response in the following format:
	 * +CSQ: <RSSI>,<BER>
	 */

	*rssi = -1;

	if (!sscanf (str, "+CSQ: %2d,", rssi))
	{
		ast_debug (2, "[%s] Error parsing +CSQ result '%s'\n", pvt->id, str);
		return -1;
	}

	return 0;
}

/*!
 * \brief Parse a ^RSSI notification
 * \param pvt -- pvt structure
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \return -1 on error (parse error) or the rssi value
 */

static inline int at_parse_rssi (pvt_t* pvt, char* str, size_t len)
{
	int rssi = -1;

	/*
	 * parse RSSI info in the following format:
	 * ^RSSI:<rssi>
	 */

	if (!sscanf (str, "^RSSI:%d", &rssi))
	{
		ast_debug (2, "[%s] Error parsing RSSI event '%s'\n", pvt->id, str);
		return -1;
	}

	return rssi;
}

/*!
 * \brief Parse a ^MODE notification (link mode)
 * \param pvt -- pvt structure
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \return -1 on error (parse error) or the the link mode value
 */

static inline int at_parse_mode (pvt_t* pvt, char* str, size_t len, int* mode, int* submode)
{
	/*
	 * parse RSSI info in the following format:
	 * ^MODE:<mode>,<submode>
	 */

	*mode    = -1;
	*submode = -1;

	if (!sscanf (str, "^MODE:%d,%d", mode, submode))
	{
		ast_debug (2, "[%s] Error parsing MODE event '%.*s'\n", pvt->id, (int) len, str);
		return -1;
	}

	return 0;
}
