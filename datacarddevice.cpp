/**
 * datacarddevice.cpp
 * This file is part of the Yate-datacard Project http://code.google.com/p/yate-datacard/
 * Yate datacard channel driver for Huawei UMTS modem
 *
 * Copyright (C) 2010-2011 MBloody
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
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include "pdu.h"


using namespace TelEngine;


static int opentty (char* dev)
{
    int fd;
    struct termios term_attr;
// To open on non block mode
//	fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    fd = open(dev, O_RDWR | O_NOCTTY);

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

    cfsetospeed(&term_attr, (speed_t)B115200);
    cfsetispeed(&term_attr, (speed_t)B115200);
    term_attr.c_cflag = (term_attr.c_cflag & ~CSIZE) | CS8;

    term_attr.c_cflag |= CLOCAL | CREAD;

    term_attr.c_cflag |= CRTSCTS;

//    term_attr.c_cflag = B115200 | CS8 | CREAD | CRTSCTS;


    term_attr.c_iflag =  IGNBRK;
//    term_attr.c_iflag = 0;
    term_attr.c_lflag = 0;
    term_attr.c_oflag = 0;
    term_attr.c_cc[VMIN] = 1;
//    term_attr.c_cc[VTIME] = 5;
    term_attr.c_cc[VTIME] = 0;

//    if (tcsetattr (fd, TCSAFLUSH, &term_attr) != 0)
    if (tcsetattr (fd, TCSANOW, &term_attr) != 0)
    {
	Debug("opentty",DebugAll,"tcsetattr() failed '%s'", dev);
    }

    return fd;
}

MonitorThread::MonitorThread(CardDevice* dev):m_device(dev) {}

MonitorThread::~MonitorThread() {}

void MonitorThread::run()
{
    if (m_device)
	m_device->processATEvents();
}

void MonitorThread::cleanup() {}

//MediaThread
MediaThread::MediaThread(CardDevice* dev):m_device(dev) {}

MediaThread::~MediaThread() {}

void MediaThread::run()
{
    struct pollfd pfd;
    char buf[1024];
    int len;
    char silence_frame[FRAME_SIZE];

    ssize_t res;

    memset(silence_frame, 0, sizeof(silence_frame));

    if (!m_device)
        return;

    m_device->m_audio_buf.clear();

    // Main loop
    while (m_device->isRunning())
    {
	m_device->m_mutex.lock();

	pfd.fd = m_device->m_audio_fd;
	pfd.events = POLLIN;

	m_device->m_mutex.unlock();

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

	if(pfd.revents & POLLIN) 
	{
	    m_device->m_mutex.lock();

	    len = read(pfd.fd, buf, FRAME_SIZE);
	    if(len)
		m_device->forwardAudio(buf, len);

//TODO: Write full data
	    unsigned int avail = m_device->m_audio_buf.length();	
	    if(avail >= FRAME_SIZE)
	    {
		char* data = (char*)m_device->m_audio_buf.data();
		write(pfd.fd, data, FRAME_SIZE);
		m_device->m_audio_buf.cut(-FRAME_SIZE);
	    }
	    else if(avail > 0)
	    {
		Debug(DebugAll, "[%s] write truncated frame", m_device->c_str());
		char* data = (char*)m_device->m_audio_buf.data();
		write(pfd.fd, data, avail);
		m_device->m_audio_buf.cut(-avail);
	    }
	    else
	    {
		Debug(DebugAll, "[%s] write silence", m_device->c_str());
		write(pfd.fd, silence_frame, FRAME_SIZE);
	    }
	    m_device->m_mutex.unlock();
	}
	else if(pfd.revents)
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

void MediaThread::cleanup() {}


DatacardConsumer::DatacardConsumer(CardDevice* dev, const char* format): DataConsumer(format), m_device(dev)
{}

DatacardConsumer::~DatacardConsumer()
{}

unsigned long DatacardConsumer::Consume(const DataBlock& data, unsigned long tStamp, unsigned long flags)
{
    if (!m_device)
	return invalidStamp();
    m_device->sendAudio((char*)data.data(), data.length());
    return 0;
}

DatacardSource::DatacardSource(CardDevice* dev, const char* format):DataSource(format), m_device(dev)
{
}

DatacardSource::~DatacardSource()
{
}


CardDevice::CardDevice(String name, DevicesEndPoint* ep):String(name), m_endpoint(ep), m_monitor(0), m_consumer(0), m_source(0), m_mutex(true), m_conn(0), m_connected(false)
{
    m_data_fd = -1;
    m_audio_fd = -1;
    m_incoming_pdu = false;

    m_state = BLT_STATE_WANT_CONTROL;

    m_cusd_use_ucs2_decoding = 1;
    m_gsm_reg_status = -1;

    m_manufacturer.clear();
    m_model.clear();
    m_firmware.clear();
    m_imei.clear();
    m_imsi.clear();

    m_provider_name = "NONE";
    m_number = "Unknown";

    m_reset_datacard = true;
    m_u2diag = -1;
    m_callingpres = -1;

    m_initialized = 0;
    m_gsm_registered = 0;

    m_incoming = 0;
    m_outgoing = 0;
    m_needring = 0;
    m_needchup = 0;

    m_simstatus = -1;
    m_pincount = 0;
    m_commandQueue.clear();
    m_lastcmd = 0;

    m_source = new DatacardSource(this,"slin");
    m_consumer = new DatacardConsumer(this,"slin");
}

CardDevice::~CardDevice()
{
    TelEngine::destruct(m_source);
    TelEngine::destruct(m_consumer);
}

bool CardDevice::startMonitor() 
{
    m_running = true;
    m_media = new MediaThread(this);
    m_monitor = new MonitorThread(this);
    return m_monitor->startup() && m_media->startup();
}

bool CardDevice::tryConnect()
{
    m_mutex.lock();
    if(!m_connected)
    {
	Debug("tryConnect",DebugAll,"Datacard %s trying to connect on %s...", safe(), m_data_tty.safe());
	if(((m_data_fd = opentty((char*)m_data_tty.safe())) > -1) && ((m_audio_fd = opentty((char*)m_audio_tty.safe())) > -1))
	    if(startMonitor())
	    {
	        m_connected = true;
	        Debug("tryConnect",DebugAll,"Datacard %s has connected, initializing...", safe());
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

    stopRunning();

    if(m_conn)
    {
	Debug("disconnect",DebugAll,"[%s] Datacard disconnected, hanging up owner", c_str());
	m_needchup = 0;
	Hangup(DATACARD_FAILURE);
    }

    close(m_data_fd);
    close(m_audio_fd);

    m_data_fd = -1;
    m_audio_fd = -1;

    m_connected	= false;
    m_initialized = 0;
    m_gsm_registered = 0;

    m_incoming = 0;
    m_outgoing = 0;
    m_needring = 0;
    m_needchup = 0;

    m_gsm_reg_status = -1;

    m_manufacturer.clear();
    m_model.clear();
    m_firmware.clear();
    m_imei.clear();
    m_imsi.clear();

    m_provider_name = "NONE";
    m_number = "Unknown";
    m_incoming_pdu = false;

    m_simstatus = -1;
    m_pincount = 0;

    m_commandQueue.clear();
    m_lastcmd = 0;

    m_initialized = 0;

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

static String decodeRegStatus(int status)
{
    String reg_status = "unknown";
    switch(status)
    {
	case 0:
	    reg_status = "not registered, not searching";
	    break;
	case 1:
	    reg_status = "registered, home network";
	    break;
	case 2:
	    reg_status = "not registered, but searching";
	    break;
	case 3:
	    reg_status = "registration denied";
	    break;
	case 5:
	    reg_status = "registered, roaming";
	    break;
	default:
	    reg_status = "unknown";
	    break;
    }
    return reg_status;
}

bool CardDevice::getParams(NamedList* list)
{
    m_mutex.lock();
    list->addParam("device",c_str());

    String reg_status = decodeRegStatus(m_gsm_reg_status);

    list->addParam("gsm_reg_status",reg_status);
    list->addParam("rssi", String(m_rssi));
    list->addParam("providername", m_provider_name);
    list->addParam("manufacturer", m_manufacturer);
    list->addParam("model", m_model);
    list->addParam("firmware", m_firmware);
    list->addParam("imei", m_imei);
    list->addParam("imsi", m_imsi);
    list->addParam("number", m_number);
    list->addParam("lar", m_location_area_code);
    list->addParam("cellid", m_cell_id);
    m_mutex.unlock();
    return true;
}

/*
 * FIXME: tempopary solution. only for testing.
 */
bool CardDevice::getNetworkStatus(NamedList *list)
{
    m_mutex.lock();
    list->addParam("device", c_str());
    String reg_status = decodeRegStatus(m_gsm_reg_status);
    list->addParam("gsm_reg_status", reg_status);
    list->addParam("rssi",String(m_rssi));
    list->addParam("provider_name",m_provider_name);
    list->addParam("lar",m_location_area_code);
    list->addParam("cellid", m_cell_id);
    m_mutex.unlock();
    return true;
}

String CardDevice::getStatus()
{
//TODO: Implement this
    m_mutex.lock();
    String ret = c_str();
    ret << "|";
    ret << decodeRegStatus(m_gsm_reg_status);
    ret << "|";
//		ast_cli (a->fd, "  State                   : %s\n",
//			(!pvt->connected) ? "Not connected" :
//			(!pvt->initialized) ? "Not initialized" :
//			(!pvt->gsm_registered) ? "GSM not registered" :
//			(pvt->outgoing || pvt->incoming) ? "Busy" : "Free"
//		);
//    list->addParam("has_voice", has_voice?"true":"false");
//    list->addParam("has_sms", has_sms?"true":"false");
    ret << String(m_rssi) <<"|";
//    list->addParam("linkmode", String(linkmode));
//    list->addParam("linksubmode", String(linksubmode));

    ret << m_provider_name <<"|";
    ret << m_manufacturer <<"|";
    ret << m_model <<"|";
    ret << m_firmware <<"|";
    ret << m_imei <<"|";
    ret << m_imsi <<"|";
    ret << m_number;

//    ast_cli (a->fd, "  Default CallingPres     : %s\n", pvt->callingpres < 0 ? "<Not set>" : ast_describe_caller_presentation (pvt->callingpres));
//    ast_cli (a->fd, "  Use UCS-2 encoding      : %s\n", pvt->use_ucs2_encoding ? "Yes" : "No");
//    ast_cli (a->fd, "  USSD use 7 bit encoding : %s\n", pvt->cusd_use_7bit_encoding ? "Yes" : "No");
//    ast_cli (a->fd, "  USSD use UCS-2 decoding : %s\n", pvt->cusd_use_ucs2_decoding ? "Yes" : "No");
//    list->addParam("lar", m_location_area_code);
//    list->addParam("cellid", m_cell_id);
//    ast_cli (a->fd, "  Auto delete SMS         : %s\n", pvt->auto_delete_sms ? "Yes" : "No");
//    ast_cli (a->fd, "  Disable SMS             : %s\n\n", pvt->disablesms ? "Yes" : "No");
    m_mutex.unlock();
    return ret;
}


// SMS and USSD
bool CardDevice::sendSMS(const String &called, const String &sms)
{
    Debug(DebugAll, "[%s] sendSMS: %s", c_str(), sms.c_str());

    Lock lock(m_mutex);

    // TODO: Check called & sms
    // Check if msg will be desrtoyed

    if (m_connected && m_initialized && m_gsm_registered)
    {
	if(m_has_sms)
	{
	    PDU pdu;
	    pdu.setMessage(sms.safe());
	    pdu.setNumber(called.safe());
	    pdu.setAlphabet(PDU::UCS2);
	    pdu.generate();

	    const char* pdutext = pdu.getPDU();
	    m_commandQueue.append(new ATCommand("AT+CMGS=" + String(pdu.getMessageLen()), CMD_AT_CMGS, new String(pdutext)));
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

bool CardDevice::receiveSMS(const char* pdustr, size_t len)
{
    PDU pdu(pdustr);
    if (!pdu.parse())
	return false;

    m_endpoint->onReceiveSMS(this, String(pdu.getNumber()), String(pdu.getUDHData()), String(pdu.getMessage()));
    return true;
}

bool CardDevice::sendUSSD(const String &ussd)
{
    Debug(DebugAll, "[%s] sendUSSD: %s", c_str(), ussd.c_str());

    Lock lock(m_mutex);

    if (m_connected && m_initialized && m_gsm_registered)
    {
	String ussdenc;
	if(!encodeUSSD(ussd, ussdenc))
	    return false;
	m_commandQueue.append(new ATCommand("AT+CUSD=1,\"" + ussdenc + "\",15", CMD_AT_CUSD));
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
    if(!m_endpoint->onIncamingCall(this, caller))
    {
	Debug(DebugAll, "CardDevice::incomingCall error: onIncamingCall");
	return false;
    }
    if(!m_conn)
    {
	Debug(DebugAll, "CardDevice::incomingCall error: m_conn is NULL");
	return false;
    }
    m_mutex.lock();
    m_audio_buf.clear();
    m_mutex.unlock();
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
//TODO: Review this!!!
    if(m_needchup)
    {
	m_commandQueue.append(new ATCommand("AT+CHUP", CMD_AT_CHUP));
	m_needchup = 0;
    }
    lock.drop();
    return tmp->onHangup(reason);
}


bool CardDevice::newCall(const String &called)
{
//    isE164
//1 2 3 4 5 6 7 8 9 0 * # + A B C

    Lock lock(m_mutex);

//    if (!initialized || incoming || outgoing)
//    {
//	Debug(DebugAll, "[%s] Error. Device already in use or not initilized", c_str());
//	return false;
//    }
//    if(!m_conn)
//    {
//        Debug(DebugAll, "CardDevice::newCall error: m_conn is NULL");
//	return false;
//    }

    Debug(DebugAll, "[%s] Calling '%s'", c_str(), called.c_str());

//  0: presentation indicator is used according to the subscription of the CLIR service
//  1: CLIR invocation (hide)
//  2: CLIR suppression (show)
//  Other values not valid, Do not use callingpres
    if((m_callingpres >= 0) && (m_callingpres <= 2))
	m_commandQueue.append(new ATCommand("AT+CLIR=" + String(m_callingpres), CMD_AT_CLIR, new String(called)));
    else
        m_commandQueue.append(new ATCommand("ATD" + called + ";", CMD_AT_D));

    m_audio_buf.clear();

    m_outgoing = 1;
    m_needchup = 1;
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
}

bool CardDevice::isDTMFValid(char digit)
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
	    return true;
	default:
	    return false;
    }
}

bool CardDevice::encodeUSSD(const String& code, String& ret)
{
    ssize_t res;
    char buf[512];

    if(m_cusd_use_7bit_encoding)
    {
	res = char_to_hexstr_7bit(code.safe(), code.length(), buf, sizeof(buf));
	if (res <= 0)
	{
	    Debug(DebugAll, "[%s] Error converting USSD code to PDU: %s\n", c_str(), code.safe());
	    return false;
	}
    }
    else if(m_use_ucs2_encoding)
    {
	res = utf8_to_hexstr_ucs2(code.safe(), code.length(), buf, sizeof(buf));
	if (res <= 0)
	{
	    Debug(DebugAll, "[%s] error converting USSD code to UCS-2: %s\n", c_str(), code.safe());
	    return false;
	}
    }
    else
    {
	ret = code;
	return true;
    }
    ret = buf;
    return true;

}

//audio
void CardDevice::forwardAudio(char* data, int len)
{
    if(m_source && m_source->valid())
	m_source->Forward(DataBlock(data, len));
}

int CardDevice::sendAudio(char* data, int len)
{
//TODO:
    m_mutex.lock();
    m_audio_buf.append(data, len);
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
    Debug(DebugAll, "Datacard devices: %d", m_devices.count());
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

void DevicesEndPoint::onReceiveSMS(CardDevice* dev, const String& caller, const String& udh_data, const String& sms)
{
}

void DevicesEndPoint::onUpdateNetworkStatus(CardDevice* dev)
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

    CardDevice* dev = new CardDevice(name, this);
    dev->m_data_tty = data_tty;
    dev->m_audio_tty = audio_tty;

    dev->m_sim_pin = data->getValue("pin");

    dev->m_auto_delete_sms = data->getBoolValue("autodeletesms",true);
    dev->m_reset_datacard = data->getBoolValue("resetdatacard",true);
    dev->m_u2diag = data->getIntValue("u2diag",-1);
    if (dev->m_u2diag == 0)
	dev->m_u2diag = -1;
    dev->m_callingpres = data->getIntValue("callingpres",-1);
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

CardDevice* DevicesEndPoint::findDevice(const NamedList &list)
{
    Lock lock(m_mutex);

    const ObjList *devicesIter = &m_devices;
    while (devicesIter)
    {
	GenObject* obj = devicesIter->get();
	devicesIter = devicesIter->next();
	if (!obj) continue;
	CardDevice* dev = static_cast<CardDevice*>(obj);
	if((dev->getImei() == list["imei"]) || (dev->getImsi() == list["imsi"]))
	    return dev;
    }
    return 0;
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
	m_devices.remove(obj, true); // Remove from list and delete object
    }
    m_mutex.unlock();
}

void DevicesEndPoint::stopEP()
{
    m_run = false;
    cancel(true);
}

String DevicesEndPoint::devicesStatus()
{
    CardDevice* dev = 0;
    String ret ="";
    m_mutex.lock();
    const ObjList *devicesIter = &m_devices;
    while (devicesIter)
    {
	GenObject* obj = devicesIter->get();
	devicesIter = devicesIter->next();
	if (!obj) continue;
	dev = static_cast<CardDevice*>(obj);
	ret << dev->getStatus() <<";";
    }
    m_mutex.unlock();
    return ret;
}

bool DevicesEndPoint::onIncamingCall(CardDevice* dev, const String &caller)
{
    return false;
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
    if (m_dev->m_incoming)
	m_dev->m_commandQueue.append(new ATCommand("ATA", CMD_AT_A));
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

    m_dev = NULL;


    if (tmp->m_needchup)
    {
	tmp->m_commandQueue.append(new ATCommand("AT+CHUP", CMD_AT_CHUP));
	tmp->m_needchup = 0;
    }

    tmp->m_conn = NULL;
    tmp->m_needring = 0;

    tmp->m_mutex.unlock();

    return true;
}

bool Connection::sendDTMF(char digit)
{
    Debug(DebugAll, "[%s] Send DTMF %c", m_dev->c_str(), digit);

    m_dev->m_mutex.lock();
    if(m_dev->isDTMFValid(digit))
	m_dev->m_commandQueue.append(new ATCommand("AT^DTMF=1," + digit, CMD_AT_DTMF));
    m_dev->m_mutex.unlock();

    return true;
}

/* vi: set ts=8 sw=4 sts=4 noet: */
