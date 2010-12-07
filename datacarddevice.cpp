#include "datacarddevice.h"
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>


using namespace TelEngine;


static int opentty (char* dev)
{
	int		fd;
	struct termios	term_attr;

	fd = open (dev, O_RDWR | O_NOCTTY);

	if (fd < 0)
	{
		Debug("opentty",DebugAll, "Unable to open '%s'", dev);
		return -1;
	}

	if (tcgetattr (fd, &term_attr) != 0)
	{
		Debug("opentty",DebugAll, "tcgetattr() failed '%s'", dev);
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
		Debug("opentty",DebugAll,"tcsetattr() failed '%s'", dev);
	}

	return fd;
}


MonitorThread::MonitorThread(CardDevice* dev):m_device(dev)
{
}
MonitorThread::~MonitorThread()
{
}

void MonitorThread::run()
{
//    at_res_t	at_res;
    at_queue_t*	e;
    int t;
//    int res;
//    struct iovec iov[2];
//    int iovcnt;
//    size_t size;
//    size_t i = 0;

    /* start datacard initilization with the AT request */
    if (!m_device) 
        return;
    
    m_device->m_mutex.lock();

//    m_device->timeout = 10000;

    if (m_device->at_send_at() || m_device->at_fifo_queue_add(CMD_AT, RES_OK))
    {
        Debug(DebugAll, "[%s] Error sending AT", m_device->c_str());
        m_device->disconnect();
        m_device->m_mutex.unlock();
        return;
    }

    m_device->m_mutex.unlock();

    // Main loop
    while (m_device->isRunning())
    {
        m_device->m_mutex.lock();

        if (m_device->dataStatus() || m_device->audioStatus())
        {
            Debug(DebugAll, "Lost connection to Datacard %s", m_device->c_str());
            m_device->disconnect();
            m_device->m_mutex.unlock();
            return;
        }
        t = 10000;

        m_device->m_mutex.unlock();

	int res = m_device->at_wait(&t);
	if(res < 0)
	{
	    m_device->m_mutex.lock();
	    m_device->disconnect();
            m_device->m_mutex.unlock();
            return;
	}
//        if (!m_device->at_wait(&t))
        if (res == 0)
        {
            m_device->m_mutex.lock();
            if (!m_device->initialized)
            {
                Debug(DebugAll, "[%s] timeout waiting for data, disconnecting", m_device->c_str());

		if ((e = m_device->at_fifo_queue_head()))
                {
                    Debug(DebugAll, "[%s] timeout while waiting '%s' in response to '%s'", m_device->c_str(), m_device->at_res2str (e->res), m_device->at_cmd2str (e->cmd));
                }
                Debug(DebugAll, "Error initializing Datacard %s", m_device->c_str());
                m_device->disconnect();
                m_device->m_mutex.unlock();
                return;
            }
            else
            {
                m_device->m_mutex.unlock();
                continue;
            }
        }
    
        m_device->m_mutex.lock();

//	Debug(DebugAll, "m_device->handle_rd_data(); [%s]", m_device->c_str());

	if (m_device->handle_rd_data())
	{
            m_device->disconnect();
            m_device->m_mutex.unlock();
            return;
	}

//	Debug(DebugAll, "after m_device->handle_rd_data(); [%s]", m_device->c_str());

/*        if (m_device->at_read())
        {
            m_device->disconnect();
            m_device->m_mutex.unlock();
            return;
        }
        while ((iovcnt = m_device->at_read_result_iov()) > 0)
        {
            at_res = m_device->at_read_result_classification(iovcnt);
	    
            if (m_device->at_response(iovcnt, at_res))
            {
                m_device->disconnect();
                m_device->m_mutex.unlock();
                return;
            }
        }
*/
        m_device->m_mutex.unlock();
    } // End of Main loop
}

void MonitorThread::cleanup()
{
}



//MediaThread

MediaThread::MediaThread(CardDevice* dev):m_device(dev)
{
}
MediaThread::~MediaThread()
{
}

void MediaThread::run()
{
    struct pollfd pfd;
    char buf[1024];
    int len;
    char silence_frame[FRAME_SIZE];

    size_t used;
    int iovcnt;
    struct iovec iov[3];
    ssize_t res;
    size_t count;

    
    memset(silence_frame, 0, sizeof(silence_frame));
    
    if (!m_device) 
        return;

    m_device->a_write_rb.rb_init(m_device->a_write_buf, sizeof(m_device->a_write_buf));

    // Main loop
    while (m_device->isRunning())
    {
//        m_device->m_mutex.lock();

//        if (m_device->dataStatus() || m_device->audioStatus())
//        {
//            Debug(DebugAll, "Lost connection to Datacard %s", m_device->c_str());
//		goto e_cleanup;
//            m_device->disconnect();
//            m_device->m_mutex.unlock();
//            return;
//        }
//        m_device->m_mutex.unlock();


        m_device->m_mutex.lock();

	pfd.fd = m_device->m_audio_fd;
	pfd.events = POLLIN;

        m_device->m_mutex.unlock();

//	res = poll(&pfd, 1, 50);
	res = poll(&pfd, 1, 1000);

	if (res == -1 && errno != EINTR) 
	{
	    Debug(DebugAll, "MediaThread poll() error datacard [%s]", m_device->c_str());
	    m_device->m_mutex.lock();
	    m_device->disconnect();
	    m_device->m_mutex.unlock();
	    return;
	}

	if (res == 0)
    	    continue;


	if (pfd.revents & POLLIN) 
	{
	    m_device->m_mutex.lock();

///	    Debug(DebugAll, "POLLIN");

	    len = read(pfd.fd, buf, FRAME_SIZE);

	    if (len) 
	    {
		    m_device->forwardAudio(buf, len);
///		
///    		write(pfd.fd, buf, len);
///
	    }
	    used = m_device->a_write_rb.rb_used ();
        if (used >= FRAME_SIZE)
	    {
            iovcnt = m_device->a_write_rb.rb_read_n_iov(iov, FRAME_SIZE);
            m_device->a_write_rb.rb_read_upd(FRAME_SIZE);
        }
        else if (used > 0)
        {
            Debug(DebugAll, "[%s] write truncated frame", m_device->c_str());

            iovcnt = m_device->a_write_rb.rb_read_all_iov(iov);
            m_device->a_write_rb.rb_read_upd(used);

            iov[iovcnt].iov_base = silence_frame;
            iov[iovcnt].iov_len	= FRAME_SIZE - used;
		    iovcnt++;
	    }
	    else
	    {
		    Debug(DebugAll, "[%s] write silence", m_device->c_str());

    		iov[0].iov_base = silence_frame;
	    	iov[0].iov_len = FRAME_SIZE;
	    	iovcnt = 1;
	    }
        count = 0;
        while ((res = writev(pfd.fd, iov, iovcnt)) < 0 && (errno == EINTR || errno == EAGAIN))
        {
    	    if (count++ > 10)
    	    {
		        Debug(DebugAll,"Deadlock avoided for write!");
		        break;
	        }
	        usleep (1);
	    }

	    if (res < 0 || res != FRAME_SIZE)
	    {
	        Debug(DebugAll,"[%s] Write error!",m_device->c_str());
	    }
    
	    m_device->m_mutex.unlock();
	} 
	else if (pfd.revents) 
	{
	    Debug(DebugAll, "MediaThread poll exception datacard [%s]", m_device->c_str());
	    m_device->m_mutex.lock();
        m_device->disconnect();
        m_device->m_mutex.unlock();
        return;
	} 
	else 
	{
	    Debug(DebugAll, "MediaThread Unhandled poll output datacard [%s]", m_device->c_str());
	}

    } // End of Main loop

}

void MediaThread::cleanup()
{
}



CardDevice::CardDevice(String name, DevicesEndPoint* ep):String(name), m_endpoint(ep), m_monitor(0), m_mutex(true), m_conn(0), m_connected(false)
{
    m_data_fd = -1;
    m_audio_fd = -1;

//    d_read_rb.rb_init(d_read_buf, sizeof (d_read_buf));

    state = BLT_STATE_WANT_CONTROL;
    
//    timeout = 10000;
    cusd_use_ucs2_decoding =  1;
    gsm_reg_status = -1;

    m_manufacturer.clear();
    m_model.clear();
    m_firmware.clear();
    m_imei.clear();
    m_imsi.clear();

    m_provider_name = "NONE";
    m_number = "Unknown";

    m_reset_datacard = true;
    u2diag = -1;
    callingpres = -1;

    
    m_atQueue.clear();
    
///
    initialized = 0;
    gsm_registered = 0;
    
    incoming = 0;
    outgoing = 0;
    needring = 0;
    needchup = 0;

}
bool CardDevice::startMonitor() 
{ 
    //TODO: Running flag
    m_running = true;
    m_media = new MediaThread(this);
    m_media->startup();
    m_monitor = new MonitorThread(this);
    return m_monitor->startup();
//    return true;
}

bool CardDevice::tryConnect()
{
    m_mutex.lock();
    if (!m_connected)
    {
	Debug("tryConnect",DebugAll,"Datacard %s trying to connect on %s...", safe(), data_tty.safe());
	if ((m_data_fd = opentty((char*)data_tty.safe())) > -1)
	{
		if ((m_audio_fd = opentty((char*)audio_tty.safe())) > -1)
		{
		    if (startMonitor())
		    {
			m_connected = true;
			Debug("tryConnect",DebugAll,"Datacard %s has connected, initializing...", safe());
		    }
		}
	}
    }
    m_mutex.unlock();
    return m_connected;
}

bool CardDevice::disconnect()
{
    if(!m_connected)
    {
    	Debug("disconnect",DebugAll,"[%s] Datacard not connected", safe());	
    	return m_connected;
    }
    if(isRunning()) 
	stopRunning();

    if(m_conn)
    {
    	Debug("disconnect",DebugAll,"[%s] Datacard disconnected, hanging up owner", c_str());
	needchup = 0;
	Hangup(DATACARD_FAILURE);
    }

    close(m_data_fd);
    close(m_audio_fd);

    m_data_fd = -1;
    m_audio_fd = -1;

    m_connected	= false;
    initialized = 0;
    gsm_registered = 0;

    incoming = 0;
    outgoing = 0;
    needring = 0;
    needchup = 0;
	
    gsm_reg_status = -1;

    m_manufacturer.clear();
    m_model.clear();
    m_firmware.clear();
    m_imei.clear();
    m_imsi.clear();

    m_provider_name = "NONE";
    m_number = "Unknown";

//    d_read_rb.rb_init(d_read_buf, sizeof (d_read_buf));

    m_atQueue.clear();

    Debug("disconnect",DebugAll,"Datacard %s has disconnected", c_str());
    return m_connected;
}

int CardDevice::devStatus(int fd)
{
    struct termios t;
    if (fd < 0)
	return -1;
    return tcgetattr(fd, &t);
}

//TODO: Do we need syncronization?
bool CardDevice::isRunning() const
{
    bool running;
    
    // m_mutex.lock();
    running = m_running;
    // m_mutex.unlock();
    
    return running;
}

void CardDevice::stopRunning()
{
    // m_mutex.lock();
    m_running = false;
    // m_mutex.unlock();
}


bool CardDevice::getStatus(NamedList * list)
{
    m_mutex.lock();
    list->addParam("device",c_str());
    
//		ast_cli (a->fd, "  Group                   : %d\n", pvt->group);
//		ast_cli (a->fd, "  GSM Registration Status : %s\n",
//			(pvt->gsm_reg_status == 0) ? "Not registered, not searching" :
//			(pvt->gsm_reg_status == 1) ? "Registered, home network" :
//			(pvt->gsm_reg_status == 2) ? "Not registered, but searching" :
//			(pvt->gsm_reg_status == 3) ? "Registration denied" :
//			(pvt->gsm_reg_status == 5) ? "Registered, roaming" : "Unknown"
//		);
//		ast_cli (a->fd, "  State                   : %s\n",
//			(!pvt->connected) ? "Not connected" :
//			(!pvt->initialized) ? "Not initialized" :
//			(!pvt->gsm_registered) ? "GSM not registered" :
//			(pvt->outgoing || pvt->incoming) ? "Busy" :
//			(pvt->outgoing_sms || pvt->incoming_sms) ? "SMS" : "Free"
//		);
//		ast_cli (a->fd, "  Voice                   : %s\n", (pvt->has_voice) ? "Yes" : "No");
//		ast_cli (a->fd, "  SMS                     : %s\n", (pvt->has_sms) ? "Yes" : "No");
//		ast_cli (a->fd, "  RSSI                    : %d\n", pvt->rssi);
//		ast_cli (a->fd, "  Mode                    : %d\n", pvt->linkmode);
//		ast_cli (a->fd, "  Submode                 : %d\n", pvt->linksubmode);
    list->addParam("providername", m_provider_name);
    list->addParam("manufacturer", m_manufacturer);
    list->addParam("model", m_model);
    list->addParam("firmware", m_firmware);
    list->addParam("imei", m_imei);
    list->addParam("imsi", m_imsi);
    list->addParam("number", m_number);

//    ast_cli (a->fd, "  Use CallingPres         : %s\n", pvt->usecallingpres ? "Yes" : "No");
//    ast_cli (a->fd, "  Default CallingPres     : %s\n", pvt->callingpres < 0 ? "<Not set>" : ast_describe_caller_presentation (pvt->callingpres));
//    ast_cli (a->fd, "  Use UCS-2 encoding      : %s\n", pvt->use_ucs2_encoding ? "Yes" : "No");
//    ast_cli (a->fd, "  USSD use 7 bit encoding : %s\n", pvt->cusd_use_7bit_encoding ? "Yes" : "No");
//    ast_cli (a->fd, "  USSD use UCS-2 decoding : %s\n", pvt->cusd_use_ucs2_decoding ? "Yes" : "No");
    list->addParam("lar", m_location_area_code);
    list->addParam("cellid", m_cell_id);
//    ast_cli (a->fd, "  Auto delete SMS         : %s\n", pvt->auto_delete_sms ? "Yes" : "No");
//    ast_cli (a->fd, "  Disable SMS             : %s\n\n", pvt->disablesms ? "Yes" : "No");
    m_mutex.unlock();
    return true;
}


// SMS and USSD
bool CardDevice::sendSMS(const String &called, const String &sms)
{
    Debug(DebugAll, "[%s] sendSMS: %s", c_str(), sms.c_str());
    
    Lock lock(m_mutex);
    
    // TODO: Check called & sms
    // Check if msg will be desrtoyed
    char *msg;

    if (m_connected && initialized && gsm_registered)
    {
        if (has_sms)
        {
            msg = strdup(sms.safe());
            if (at_send_cmgs(called.safe()) || at_fifo_queue_add_ptr(CMD_AT_CMGS, RES_SMS_PROMPT, msg))
            {
                free(msg);
                Debug(DebugAll, "[%s] Error sending SMS message", c_str());
                return false;
		    }
	    }
        else
        {
            Debug(DebugAll, "Datacard %s doesn't handle SMS -- SMS will not be sent", c_str());
            return false;
        }
    }
    else
    {
        Debug(DebugAll, "Device %s not connected / initialized / registered", c_str());
        return false;
    }
    return true;
}

bool CardDevice::sendUSSD(const String &ussd)
{
    Debug(DebugAll, "[%s] sendUSSD: %s", c_str(), ussd.c_str());

    Lock lock(m_mutex);
    
    if (m_connected && initialized && gsm_registered)
    {
        if (at_send_cusd(ussd.c_str()) || at_fifo_queue_add(CMD_AT_CUSD, RES_OK))
        {
            Debug(DebugAll, "[%s] Error sending USSD command", c_str());
            return false;
        }
    }
    else
    {
        Debug(DebugAll, "Device %s not connected / initialized / registered", c_str());
        return false;
    }
    return true;
}


bool CardDevice::incomingCall(const String &caller)
{
    m_conn = m_endpoint->createConnection(this);
    if(!m_conn)
    {
        Debug(DebugAll, "CardDevice::incomingCall error: m_conn is NULL");
	return false;
    }
    a_write_rb.rb_init(a_write_buf, sizeof(a_write_buf));
    return m_conn->onIncoming(caller);
}

bool CardDevice::Hangup(int reason)
{
    Lock lock(m_mutex);
    Connection* tmp = m_conn;
    if(!tmp)
    {
        Debug(DebugAll, "CardDevice::Hangup error: m_conn is NULL");
	return false;
    }
    m_conn = 0;
    return tmp->onHangup(reason);
}


bool CardDevice::newCall(const String &called, void* usrData)
{
    int clir = 0;

    m_conn = m_endpoint->createConnection(this, usrData);
    if(!m_conn)
    {
        Debug(DebugAll, "CardDevice::newCall error: m_conn is NULL");
	return false;
    }

    Lock lock(m_mutex);

    if (!initialized || incoming || outgoing)
    {
	Debug(DebugAll, "[%s] Error device already in use", c_str());
	Hangup(DATACARD_BUSY);
	return false;
    }

    Debug(DebugAll, "[%s] Calling '%s'", c_str(), called.c_str());

    if (m_usecallingpres)
    {
//    	Hangup(DATACARD_FAILURE);
//    	return false;
    //TODO:
    /*
	AT+CLIR=[<n>]

	<n>: (this setting effects CLI status for following calls)

	    0 presentation indicator is used according to the subscription of the CLIR service
	    1 CLIR invocation (hide)
	    2 CLIR suppression (show)
	    
	if (pvt->callingpres < 0)
	{
		#if ASTERISK_VERSION_NUM >= 10800
		clir = channel->connected.id.number.presentation;
		#else
		clir = channel->cid.cid_pres;
		#endif
	}
	else
	{
		clir = pvt->callingpres;
	}

	clir = get_at_clir_value (pvt, clir);
	*/
	if(callingpres < 0)
	    clir = 0;
	else
	    clir = callingpres;
	char* dest_number = strdup(called.c_str());
	if (at_send_clir(clir) || at_fifo_queue_add_ptr (CMD_AT_CLIR, RES_OK, dest_number))
	{
		Debug(DebugAll, "[%s] Error sending AT+CLIR command", c_str());
		Hangup(DATACARD_FAILURE);
		return false;
	}
    }
    else
    {
	if (at_send_atd(called.c_str()) || at_fifo_queue_add(CMD_AT_D, RES_OK))
	{
		Debug(DebugAll, "[%s] Error sending ATD command", c_str());
		Hangup(DATACARD_FAILURE);
		return false;
	}
    }

    a_write_rb.rb_init(a_write_buf, sizeof(a_write_buf));

    outgoing = 1;
    needchup = 1;
    return true;
}

int CardDevice::getReason(int end_status, int cc_cause)
{
    //TODO: review this!!!!!!!!!!!!!!
    switch(end_status)
    {
    
	case CM_CALL_END_OFFLINE:
	case CM_CALL_END_NO_SRV:
	case CM_CALL_END_INTERCEPT:
	case CM_CALL_END_REORDER:
	case CM_CALL_END_ALERT_STOP:
	case CM_CALL_END_ACTIVATION:
	case CM_CALL_END_MC_ABORT:
	case CM_CALL_END_RUIM_NOT_PRESENT:
	case CM_CALL_END_NDSS_FAIL:
	case CM_CALL_END_CONF_FAILED:
	case CM_CALL_END_NO_GW_SRV:
	case CM_CALL_END_INCOM_CALL:
	    return DATACARD_FORBIDDEN;

	case CM_CALL_END_REL_SO_REJ:
	case CM_CALL_END_INCOM_REJ:
	case CM_CALL_END_SETUP_REJ:
	case CM_CALL_END_NO_FUNDS:
	    return DATACARD_REJECTED;
	    
	case CM_CALL_END_REL_NORMAL:	
	case CM_CALL_END_CLIENT_END:
	case CM_CALL_END_FADE:
	    return DATACARD_NORMAL;

	case CM_CALL_END_LL_CAUSE:
	case CM_CALL_END_NETWORK_END:
	default:
	    switch(cc_cause)
	    {
	    
		case NORMAL_CALL_CLEARING:
		case NORMAL_UNSPECIFIED:		
		    return DATACARD_NORMAL;
		    
		case NO_ROUTE_TO_DEST:
		case DESTINATION_OUT_OF_ORDER:
		    return DATACARD_NOROUTE;
		    
		case USER_BUSY:		    
		    return DATACARD_BUSY;
		    
		case NO_USER_RESPONDING:
		case USER_ALERTING_NO_ANSWER:
		    return DATACARD_NOANSWER;
		    
		case CALL_REJECTED:
		case FACILITY_REJECTED:
		case REJ_UNSPECIFIED:
		case AS_REJ_RR_REL_IND:
		case AS_REJ_RR_RANDOM_ACCESS_FAILURE:
		case AS_REJ_RRC_REL_IND:
		case AS_REJ_RRC_CLOSE_SESSION_IND:
		case AS_REJ_RRC_OPEN_SESSION_FAILURE:
		case AS_REJ_LOW_LEVEL_FAIL:
		case AS_REJ_LOW_LEVEL_FAIL_REDIAL_NOT_ALLOWD:
		case MM_REJ_INVALID_SIM:
		case MM_REJ_NO_SERVICE:
		case MM_REJ_TIMER_T3230_EXP:
		case MM_REJ_NO_CELL_AVAILABLE:
		case MM_REJ_WRONG_STATE:
		case MM_REJ_ACCESS_CLASS_BLOCKED:
		case CNM_REJ_TIMER_T303_EXP:
		case CNM_REJ_NO_RESOURCES:		    
		    return DATACARD_REJECTED;

		case CHANNEL_UNACCEPTABLE:		    
		case NETWORK_OUT_OF_ORDER:
		case NO_CIRCUIT_CHANNEL_AVAILABLE:
		case REQUESTED_CIRCUIT_CHANNEL_NOT_AVAILABLE:
		case RESOURCES_UNAVAILABLE_UNSPECIFIED:
		case SERVICE_OR_OPTION_NOT_AVAILABLE:
		case ONLY_RESTRICTED_DIGITAL_INFO_BC_AVAILABLE:
		case QUALITY_OF_SERVICE_UNAVAILABLE:
		case SWITCHING_EQUIPMENT_CONGESTION:
		    return DATACARD_CONGESTION;

// Not sortet 
		case NUMBER_CHANGED:
		case NON_SELECTED_USER_CLEARING:
		case INVALID_NUMBER_FORMAT:
		case RESPONSE_TO_STATUS_ENQUIRY:
		case USER_NOT_MEMBER_OF_CUG:
		case INTERWORKING_UNSPECIFIED:
		case ABORT_MSG_RECEIVED:
		case UNASSIGNED_CAUSE:
		case OPERATOR_DETERMINED_BARRING:
		case OTHER_CAUSE:
		case CNM_MM_REL_PENDING:
		case ACM_GEQ_ACMMAX:
		case ACCESS_INFORMATION_DISCARDED:
		case INCOMING_CALL_BARRED_WITHIN_CUG:
		case BEARER_CAPABILITY_NOT_AUTHORISED:
		case BEARER_CAPABILITY_NOT_PRESENTLY_AVAILABLE:
//		    
		case INCOMPATIBLE_DESTINATION:
		case TEMPORARY_FAILURE:
		case REQUESTED_FACILITY_NOT_SUBSCRIBED:
		case INVALID_TRANSACTION_ID_VALUE:
		case INVALID_TRANSIT_NETWORK_SELECTION:
		case SEMANTICALLY_INCORRECT_MESSAGE:
		case INVALID_MANDATORY_INFORMATION:
		case IE_NON_EXISTENT_OR_NOT_IMPLEMENTED:
		case MESSAGE_TYPE_NON_EXISTENT:
		case MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROT_STATE:
		case CONDITIONAL_IE_ERROR:
		case MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE:
		case RECOVERY_ON_TIMER_EXPIRY:		
		case PROTOCOL_ERROR_UNSPECIFIED:		
		case CNM_INVALID_USER_DATA:
		case BEARER_SERVICE_NOT_IMPLEMENTED:
		case SERVICE_OR_OPTION_NOT_IMPLEMENTED:
		case REQUESTED_FACILITY_NOT_IMPLEMENTED:
		default:
		    return DATACARD_FAILURE;
	    }
    }
    return DATACARD_FAILURE;
    
    
/*	
//Datacard reasons
#define DATACARD_NORMAL 0
#define DATACARD_INCOMPLETE 1
#define DATACARD_NOROUTE 2
#define DATACARD_NOCONN 3
#define DATACARD_NOMEDIA 4
#define DATACARD_NOCALL 5
#define DATACARD_BUSY 6
#define DATACARD_NOANSWER 7
#define DATACARD_REJECTED 8
#define DATACARD_FORBIDDEN 9
#define DATACARD_OFFLINE 10
#define DATACARD_CONGESTION 11
#define DATACARD_FAILURE 12
*/    
}


//audio
void CardDevice::forwardAudio(char* data, int len)
{
    if(!m_conn)
    {
        Debug(DebugAll, "CardDevice::forwardAudio error: m_conn is NULL");
	return;
    }
    m_conn->forwardAudio(data, len);
}

int CardDevice::sendAudio(char* data, int len)
{
//    TODO:
    size_t count = a_write_rb.rb_free();
    
    m_mutex.lock();
    if (count < (size_t) len)
    {
	    a_write_rb.rb_read_upd(len - count);
    }
    a_write_rb.rb_write(data, len);
    m_mutex.unlock();
    return 0;
}

//EndPoint

DevicesEndPoint::DevicesEndPoint(int interval):Thread("DeviceEndPoint"),m_mutex(true),m_interval(interval),m_run(true)
{
    m_devices.clear();
}
DevicesEndPoint::~DevicesEndPoint()
{
}

void DevicesEndPoint::run()
{
    while(m_run)
    {
	CardDevice* dev = 0;
	m_mutex.lock();
        const ObjList *devicesIter = &m_devices;
	while (devicesIter)
	{
		GenObject* obj = devicesIter->get();
		devicesIter = devicesIter->next();
		if (!obj) continue;	
    		dev = static_cast<CardDevice*>(obj);
    		dev->tryConnect();
	}
	m_mutex.unlock();
	
	if (m_run)
	{
	    Thread::sleep(m_interval);
        }
    }
}

void DevicesEndPoint::cleanup()
{
}    

void DevicesEndPoint::onReceiveUSSD(CardDevice* dev, String ussd)
{
}

void DevicesEndPoint::onReceiveSMS(CardDevice* dev, String caller, String sms)
{
}

bool DevicesEndPoint::sendSMS(CardDevice* dev, const String &called, const String &sms)
{
    if (!dev)
    {
        Debug(DebugAll, "DevicesEndPoint::sendSMS() error: dev is NULL");
        return false;
    }
    return dev->sendSMS(called, sms);
}

bool DevicesEndPoint::sendUSSD(CardDevice* dev, const String &ussd)
{
    if (!dev)
    {
        Debug(DebugAll, "DevicesEndPoint::sendUSSD() error: dev is NULL");
        return false;
    }
    return dev->sendUSSD(ussd);
}

CardDevice* DevicesEndPoint::appendDevice(String name, NamedList* data)
{
    String audio_tty = data->getValue("audio");
    String data_tty = data->getValue("data");
    
    CardDevice * dev = new CardDevice(name, this);
    dev->data_tty = data_tty;
    dev->audio_tty = audio_tty;

    dev->rxgain = data->getIntValue("rxgain",0);
    dev->txgain = data->getIntValue("txgain",0);
    dev->m_auto_delete_sms = data->getBoolValue("autodeletesms",true);
    dev->m_reset_datacard = data->getBoolValue("resetdatacard",true);
    dev->u2diag = data->getIntValue("u2diag",-1);
    if (dev->u2diag == 0)
	dev->u2diag = -1;
    dev->m_usecallingpres = data->getBoolValue("usecallingpres",false);
    dev->callingpres = data->getIntValue("callingpres",-1);

//TODO:	
//		else if (!strcasecmp (v->name, "callingpres"))
//		{
//			dev->callingpres = ast_parse_caller_presentation (v->value);
//			if (dev->callingpres == -1)
//			{
//				errno = 0;
//				dev->callingpres = (int) strtol (v->value, (char**) NULL, 10);	/* callingpres is set to -1 if invalid */
//				if (pvt->callingpres == 0 && errno == EINVAL)
//				{
//					dev->callingpres = -1;
//				}
//			}
//		}
//    dev->callingpres = -1;
    dev->m_disablesms = data->getBoolValue("disablesms",false);  
    
    m_mutex.lock();
    m_devices.append(dev);
    m_mutex.unlock();
    return dev;
}

CardDevice* DevicesEndPoint::findDevice(const String &name)
{
    Lock lock(m_mutex);
    return static_cast<CardDevice*>(m_devices[name]);
}

void DevicesEndPoint::cleanDevices()
{
    CardDevice* dev = 0;
    m_mutex.lock();
    const ObjList *devicesIter = &m_devices;
    while (devicesIter)
    {
	GenObject* obj = devicesIter->get();
	devicesIter = devicesIter->next();
	if (!obj) continue;	
	dev = static_cast<CardDevice*>(obj);
	dev->disconnect();
    }
    m_mutex.unlock();
    m_run = false;
}

Connection* DevicesEndPoint::createConnection(CardDevice* dev, void* usrData)
{
    return 0;
}
bool DevicesEndPoint::MakeCall(CardDevice* dev, const String &called, void* usrData)
{
    
    if(!dev)
    {
        Debug(DebugAll, "DevicesEndPoint::MakeCall error: dev is NULL");
	return false;
    }

    return dev->newCall(called, usrData);
}

Connection::Connection(CardDevice* dev):m_dev(dev)
{
}

bool Connection::onIncoming(const String &caller)
{
    return true;
}

bool Connection::onProgress()
{
    return true;
}

bool Connection::onAnswered()
{
    return true;
}
bool Connection::onHangup(int reason)
{
    return true;
}
    
bool Connection::sendAnswer()
{

    m_dev->m_mutex.lock();

    if (m_dev->incoming)
    {
	if (m_dev->at_send_ata() || m_dev->at_fifo_queue_add (CMD_AT_A, RES_OK))
	{
	    Debug(DebugAll, "[%s] Error sending ATA command", m_dev->c_str());
	}
	m_dev->answered = 1;
    }
    m_dev->m_mutex.unlock();

    return true;
}
bool Connection::sendHangup()
{
    if (!m_dev)
    {
	Debug(DebugAll, "Asked to hangup channel not connected");
	return false;
    }
    
    CardDevice* tmp = m_dev;
     
    Debug(DebugAll, "[%s] Hanging up device", tmp->c_str());

    tmp->m_mutex.lock();

//	if (pvt->a_timer)
//	{
//		ast_timer_close (pvt->a_timer);
//		pvt->a_timer = NULL;
//	}

    if (tmp->needchup)
    {
	if (tmp->at_send_chup() || tmp->at_fifo_queue_add (CMD_AT_CHUP, RES_OK))
	{
		Debug(DebugAll, "[%s] Error sending AT+CHUP command", tmp->c_str());
	}
	tmp->needchup = 0;
    }

    tmp->m_conn = NULL;
    tmp->needring = 0;

    m_dev = NULL;

    tmp->m_mutex.unlock();

//	ast_setstate (channel, AST_STATE_DOWN);

    return true;
}

bool Connection::sendDTMF(char digit)
{

    m_dev->m_mutex.lock();
    
    if (m_dev->at_send_dtmf(digit) || m_dev->at_fifo_queue_add (CMD_AT_DTMF, RES_OK))
    {
	Debug(DebugAll, "[%s] Error sending DTMF %c", m_dev->c_str(), digit);
	m_dev->m_mutex.unlock();
	return -1;
    }
    Debug(DebugAll, "[%s] Send DTMF %c", m_dev->c_str(), digit);
    m_dev->m_mutex.unlock();

    return true;
}



void Connection::forwardAudio(char* data, int len)
{
}

int Connection::sendAudio(char* data, int len)
{
    return m_dev->sendAudio(data, len);
}
