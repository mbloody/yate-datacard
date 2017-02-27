/**
 * atacarddevice.h
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

#ifndef DATACARDDEVICE_H
#define DATACARDDEVICE_H
#include <yatephone.h>
#include "endreasons.h"


#define FRAME_SIZE 320
#define RDBUFF_MAX 1024

using namespace TelEngine;

typedef enum {
  BLT_STATE_WANT_CONTROL = 0,
  BLT_STATE_WANT_CMD = 1,
} blt_state_t;


typedef enum {
	CMD_UNKNOWN = 0,
	CMD_AT,
	CMD_AT_A,
	CMD_AT_CCWA,
	CMD_AT_CFUN,
	CMD_AT_CGMI,
	CMD_AT_CGMM,
	CMD_AT_CGMR,
	CMD_AT_CGSN,
	CMD_AT_CHUP,
	CMD_AT_CIMI,
	CMD_AT_CLIP,
	CMD_AT_CLIR,
	CMD_AT_CLVL,
	CMD_AT_CMGD,
	CMD_AT_CMGF,
	CMD_AT_CMGR,
	CMD_AT_CMGS,
	CMD_AT_CNMI,
	CMD_AT_CNUM,
	CMD_AT_COPS,
	CMD_AT_COPS_INIT,
	CMD_AT_CPIN,
	CMD_AT_CPIN_ENTER,
	CMD_AT_CPMS,
	CMD_AT_CREG,
	CMD_AT_CREG_INIT,
	CMD_AT_CSCS,
	CMD_AT_CSQ,
	CMD_AT_CSSN,
	CMD_AT_CUSD,
	CMD_AT_CVOICE,
	CMD_AT_D,
	CMD_AT_DDSETEX,
	CMD_AT_DTMF,
	CMD_AT_E,
	CMD_AT_SMS_TEXT,
	CMD_AT_U2DIAG,
	CMD_AT_Z,
	CMD_AT_CMEE,
	CMD_AT_CSMP,
} at_cmd_t;

typedef enum {
	RES_PARSE_ERROR = -1,
	RES_UNKNOWN = 0,
	RES_BOOT,
	RES_BUSY,
	RES_CEND,
	RES_CLIP,
	RES_CMGR,
	RES_CMS_ERROR,
	RES_CMTI,
	RES_CNUM,
	RES_CONF,
	RES_CONN,
	RES_COPS,
	RES_CPIN,
	RES_CPMS,
	RES_CREG,
	RES_CSQ,
	RES_CSSI,
	RES_CSSU,
	RES_CUSD,
	RES_ERROR,
	RES_MODE,
	RES_NO_CARRIER,
	RES_NO_DIALTONE,
	RES_OK,
	RES_ORIG,
	RES_RING,
	RES_RSSI,
	RES_SMMEMFULL,
	RES_SMS_PROMPT,
	RES_SRVST,
} at_res_t;


class CardDevice;
class DevicesEndPoint;
class Connection;

class ATCommand : public GenObject
{
public:
    ATCommand(String command, at_cmd_t cmd, GenObject* obj = 0, at_res_t res = RES_OK):m_command(command),m_cmd(cmd),m_res(res),m_obj(obj)
    {
	if(m_obj)
	{
	    String* data = static_cast<String*>(m_obj);
	    Debug(DebugAll, "ATCommand %s ", data->safe());
	}
    }

    ~ATCommand()
    {
	if(m_obj)
	    m_obj->destruct();
    }

    GenObject* get()
    {
	return m_obj;
    }

    void onTimeout()
    {
	Debug(DebugAll, "Timeout for AT command %s ignoring ", m_command.safe());
    }

public:
    String m_command;
    at_cmd_t m_cmd;
    at_res_t m_res;

    GenObject* m_obj;
};

/**
 * Thread for processing data tty.
 * Sending AT command
 * Processing responses 
 */
class MonitorThread : public Thread
{
public:
    MonitorThread(CardDevice* dev);
    ~MonitorThread();
    virtual void run();
    virtual void cleanup();
private:
    CardDevice* m_device; //pointer to device
};

/**
 * Thread for handling media data.
 * Receiving audio data from tty to core
 * Sending audio data to tty from core  
 */
class MediaThread : public Thread
{
public:
    MediaThread(CardDevice* dev);
    ~MediaThread();
    virtual void run();
    virtual void cleanup();
private:
    CardDevice* m_device; //pointer to device
};


class DatacardConsumer : public DataConsumer
{
public:
    DatacardConsumer(CardDevice* dev, const char* format);
    ~DatacardConsumer();
    virtual unsigned long Consume(const DataBlock &data, unsigned long tStamp, unsigned long flags);
private:
    CardDevice* m_device;
};

class DatacardSource : public DataSource
{
public:
    DatacardSource(CardDevice* dev, const char* format);
    ~DatacardSource();
private:
    CardDevice* m_device;
};


/**
 * Device
 * Work with devise tty ports 
 */
class CardDevice: public String
{
public:
    CardDevice(String name, DevicesEndPoint* ep);
    ~CardDevice();
    bool tryConnect();
    bool disconnect();

    int dataStatus()
	{ return devStatus(m_data_fd);}
    int audioStatus()
	{ return devStatus(m_audio_fd);}

    bool getParams(NamedList* list);
    String getStatus();

	//TODO: monitor cellular network parameters
	//maybe using getStatus more correct?

	bool getNetworkStatus(NamedList *list);

    void setConnection(Connection* conn)
	{ m_conn = conn; }

    inline DatacardSource* source()
	{ return m_source; }
    inline DatacardConsumer* consumer()
	{ return m_consumer; }
	
    inline bool isBusy()
	{ Lock lock(m_mutex); return (!m_initialized || m_incoming || m_outgoing); }


private:
    bool startMonitor();
    int devStatus(int fd);

    DevicesEndPoint* m_endpoint;
    MonitorThread* m_monitor;
    MediaThread* m_media;

    DatacardConsumer* m_consumer;
    DatacardSource* m_source;

public:
    Mutex m_mutex;
    Connection* m_conn;

public:
    int m_audio_fd;	// audio descriptor
    int m_data_fd;	//data  descriptor

    DataBlock m_audio_buf;

    String getNumber()
	{ return m_number; }

    String getImei()
	{ return m_imei; }

    String getImsi()
	{ return m_imsi; }

private:
    unsigned int m_has_sms:1;
    unsigned int m_has_voice:1;
    unsigned int m_use_ucs2_encoding:1;
    unsigned int m_cusd_use_7bit_encoding:1;
    unsigned int m_cusd_use_ucs2_decoding:1;
    int m_gsm_reg_status;
    int m_rssi;
    int m_cpms;
    int m_linkmode;
    int m_linksubmode;
    String m_provider_name;
    String m_manufacturer;
    String m_model;
    String m_firmware;
    String m_imei;
    String m_imsi;
    String m_number;
    String m_location_area_code;
    String m_cell_id;

    unsigned char m_pincount;
    int m_simstatus;

//FIXME: review all his flags. Simplify or implement it.

public:
    /* flags */
    bool m_connected;			/* do we have an connection to a device */
    unsigned int m_initialized:1;			/* whether a service level connection exists or not */
    unsigned int m_gsm_registered:1;		/* do we have an registration to a GSM */
    unsigned int m_outgoing:1;			/* outgoing call */
    unsigned int m_incoming:1;			/* incoming call */
    unsigned int m_needchup:1;			/* we need to send a CHUP */
    unsigned int m_needring:1;			/* we need to send a RING */
    unsigned int m_volume_synchronized:1;		/* we have synchronized the volume */
	
private:
    // TODO: Running flag. Do we need to stop every MonitorThread or set one 
    // flag on module level? Do we need syncronization?
    bool m_running;

public:
    bool isRunning() const;
    void stopRunning();


    /* Config */
    String m_sim_pin;
    String m_audio_tty;			/* tty for audio connection */
    String m_data_tty;			/* tty for AT commands */
    int m_u2diag;
    int m_callingpres;			/* calling presentation */
    bool m_auto_delete_sms;
    bool m_reset_datacard;
    bool m_disablesms;

private:
    blt_state_t m_state;
    char m_rd_buff[RDBUFF_MAX];
    int m_rd_buff_pos;

    // AT command methods.
public:

    /**
     * Read and handle data
     * @return 0 on success or -1 on error
     */
    int handle_rd_data();

    /**
     * Process AT data
     * @return
     */
    void processATEvents();

private:

    /**
     * Convert command to result type
     * @param command -- received command
     * @return result type
     */
    at_res_t at_read_result_classification(char* command);

    /**
     * Do response
     * @param str -- response string
     * @param at_res -- result type
     * @return 0 success or -1 parse error
     */
    int at_response(char* str, at_res_t at_res);

    /**
     * Handle ^CEND response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cend(char* str, size_t len);

    /**
     * Handle AT+CGMI response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cgmi(char* str, size_t len);

    /**
     * Handle AT+CGMM response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cgmm(char* str, size_t len);

    /**
     * Handle AT+CGMR response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cgmr(char* str, size_t len);

    /**
     * Handle AT+CGSN response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cgsn(char* str, size_t len);

    /**
     * Handle AT+CIMI response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cimi(char* str, size_t len);

    /**
     * Handle +CLIP response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_clip(char* str, size_t len);

    /**
     * Handle +CMGR response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cmgr(char* str, size_t len);

    /**
     * Handle +CMTI response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cmti(char* str, size_t len);

    /**
     * Handle +CNUM response Here we get our own phone number
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cnum(char* str, size_t len);

    /**
     * Handle ^CONN response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_conn(char* str, size_t len);

    /**
     * Handle +COPS response Here we get the GSM provider name
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cops(char* str, size_t len);

    /**
     * Handle +CPIN response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cpin(char* str, size_t len);

    /**
     * Handle +CREG response Here we get the GSM registration status
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_creg(char* str, size_t len);
    
    /**
     * Handle +CSQ response Here we get the signal strength and bit error rate
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_csq(char* str, size_t len);
    
    /**
     * Handle CUSD response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_cusd(char* str, size_t len);
    
    /**
     * Handle CPMS response
     * @param str -- string containing response (null terminated)
     * @param len -- string length
     * @return 0 success or -1 parse error
     */
    int at_response_cpms(char* str, size_t len);
    
    /**
     * Handle ERROR response
     * @return 0 success or -1 parse error
     */
    int at_response_error();
    
    /**
     * Handle ^MODE response Here we get the link mode (GSM, UMTS, EDGE...).
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_mode(char* str, size_t len);
    
    /**
     * Handle NO CARRIER response
     * @return 0 success or -1 parse error
     */
    int at_response_no_carrier();
    
    /**
     * Handle NO DIALTONE response
     * @return 0 success or -1 parse error
     */
    int at_response_no_dialtone();
    
    /**
     * Handle OK response
     * @return 0 success or -1 parse error
     */
    int at_response_ok();
    
    /**
     * Handle ^ORIG response
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_orig(char* str, size_t len);
    
    /**
     * Handle RING response
     * @return 0 success or -1 parse error
     */
    int at_response_ring();
    
    /**
     * Handle ^RSSI response Here we get the signal strength.
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_rssi(char* str, size_t len);
    
    /**
     * Handle ^SMMEMFULL response This event notifies us, that the sms storage is full
     * @return 0 success or -1 parse error
     */
    int at_response_smmemfull();
    
    /**
     * Send an SMS message from the queue.
     * @return 0 success or -1 parse error
     */
    int at_response_sms_prompt();
    
    /**
     * Handle BUSY response
     * @return 0 success or -1 parse error
     */
    int at_response_busy();

    /**
     * Handle PDU for incoming SMS.
     * @param str -- string containing response (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_response_pdu(char* str, size_t len);

public:

    /**
     * Get the string representation of the given AT command
     * @param res -- the response to process
     * @return a string describing the given response
     */
    const char* at_cmd2str(at_cmd_t cmd);
    
    /**
     * Get the string representation of the given AT response
     * @param res -- the response to process
     * @return a string describing the given response
     */
    const char* at_res2str(at_res_t res);

private:

    /**
     * Parse a CLIP event
     * @param str -- string to parse (null terminated)
     * @param len -- string lenght
     * @return empty String on error (parse error) or String with caller id inforamtion on success
     */
    String at_parse_clip(char* str, size_t len);
    
    /**
     * Parse a CMGR message
     * @param str -- string to parse (null terminated)
     * @param len -- string lenght
     * @param stat -- 
     * @param pdulen -- 
     * @return 0 success or -1 parse error
     */
    int at_parse_cmgr(char* str, size_t len, int* stat, int* pdulen);

    /**
     * Parse a CMTI notification
     * @param str -- string to parse (null terminated)
     * @param len -- string lenght
     * @note str will be modified when the CMTI message is parsed
     * @return -1 on error (parse error) or the index of the new sms message
     */
    int at_parse_cmti(char* str, size_t len);

    /**
     * Parse a CNUM response
     * @param str -- string to parse (null terminated)
     * @param len -- string lenght
     * @return emplty String on error (parse error) or String withsubscriber number
     */
    String at_parse_cnum(char* str, size_t len);
    
    /**
     * Parse a COPS response
     * @param str -- string to parse (null terminated)
     * @param len -- string lenght
     * @note str will be modified when the COPS message is parsed
     * @return NULL on error (parse error) or a pointer to the provider name
     */
    char* at_parse_cops(char* str, size_t len);
    
    /**
     * Parse a CREG response
     * @param str -- string to parse (null terminated)
     * @param len -- string lenght
     * @param gsm_reg -- a pointer to a int
     * @param gsm_reg_status -- a pointer to a int
     * @param lac -- a pointer to a char pointer which will store the location area code in hex format
     * @param ci  -- a pointer to a char pointer which will store the cell id in hex format
     * @note str will be modified when the CREG message is parsed
     * @return 0 success or -1 parse error
     */
    int at_parse_creg(char* str, size_t len, int* gsm_reg, int* gsm_reg_status, char** lac, char** ci);
    
    /**
     * Parse a CPIN notification
     * @param str -- string to parse (null terminated)
     * @param len -- string lenght
     * @return  4 if PUK2 required
     * @return  3 if PIN2 required
     * @return  2 if PUK required
     * @return  1 if PIN required
     * @return  0 if no PIN required
     * @return -1 on error (parse error) or card lock
     */
    int at_parse_cpin(char* str, size_t len);
    
    /**
     * Parse +CSQ response
     * @param str -- string to parse (null terminated)
     * @param len -- string lenght
     * @param rssi
     * @return 0 success or -1 parse error
     */
    int at_parse_csq(char* str, size_t len, int* rssi);

    /**
     * Parse a CUSD answer
     * @param str -- string to parse (null terminated)
     * @param len -- string lenght
     * @return 0 success or -1 parse error
     */
    int at_parse_cusd(char* str, size_t len, String &cusd, unsigned char &dcs);
    
    /**
     * Parse a ^MODE notification (link mode)
     * @param str -- string to parse (null terminated)
     * @param len -- string lenght
     * @param mode
     * @param submode
     * @return -1 on error (parse error) or the the link mode value
     */
    int at_parse_mode(char* str, size_t len, int* mode, int* submode);
    
    /**
     * Parse a ^RSSI notification
     * @param str -- string to parse (null terminated)
     * @param len -- string lenght
     * @return -1 on error (parse error) or the rssi value
     */
    int at_parse_rssi(char* str, size_t len);

    /** Parse +CPMS Response
     * @param str -- string to parse
     * @param len -- string length
     * @return count of offline received messages or -1 on error
     */
    int at_parse_cpms(char* str, size_t len);

    /**
     * Write to data socket
     * This function will write count characters from buf. It will always write
     * count chars unless it encounters an error.
     * @param buf -- buffer to write
     * @param count -- number of bytes to write
     * @return 0 success or -1 on error
     */
    int at_write_full(char* buf, size_t count);

    /**
     * Send the SMS PDU message
     * @param pdu -- SMS PDU
     */
    int at_send_sms_text(const char* pdu);

//private:

    ssize_t convert_string(const char* in, size_t in_length, char* out, size_t out_size, char* from, char* to);
    ssize_t hexstr_to_ucs2char(const char* in, size_t in_length, char* out, size_t out_size);
    ssize_t ucs2char_to_hexstr(const char* in, size_t in_length, char* out, size_t out_size);
    ssize_t hexstr_ucs2_to_utf8(const char* in, size_t in_length, char* out, size_t out_size);
    ssize_t utf8_to_hexstr_ucs2(const char* in, size_t in_length, char* out, size_t out_size);
    ssize_t char_to_hexstr_7bit(const char* in, size_t in_length, char* out, size_t out_size);
    ssize_t hexstr_7bit_to_char(const char* in, size_t in_length, char* out, size_t out_size);

public:
// SMS and USSD

    /**
     * Send SMS message
     * @param called - number of recepient
     * @param sms - sms text body
     * @return true on success or false on error
     */
    bool sendSMS(const String &called, const String &sms);

    /**
     * Send USSD
     * @param ussd - cusd
     * @return true on success or false on error
     */
    bool sendUSSD(const String &ussd);

    void forwardAudio(char* data, int len);
    int sendAudio(char* data, int len);

    /**
     * Create new call
     * @param called - called party number
     * @param usrData - additional user data
     * @return true on success or false on error
     */
    bool newCall(const String &called);

private:
    bool receiveSMS(const char* pdustr, size_t len);
    bool incomingCall(const String &caller);
    bool Hangup(int error);
    int getReason(int end_status, int cc_cause);
    bool m_incoming_pdu;

    ATCommand* m_lastcmd;
public:
    bool isDTMFValid(char dtmf);
    bool encodeUSSD(const String& code, String& ret);
    ObjList m_commandQueue;
};

/**
 * Call connection
 * Process all needed call actions
 */
class Connection
{
public:
    Connection(CardDevice* dev);
    virtual bool onIncoming(const String &caller);
    virtual bool onProgress();
    virtual bool onAnswered();
    virtual bool onHangup(int reason);

    bool sendAnswer();
    bool sendHangup();

    bool sendDTMF(char digit);

protected:
    CardDevice* m_dev;
};

/**
 * Holds all currently created devices
 * Process incoming connections, SMS and USSD.
 * Make calls throw selected device 
 */
class DevicesEndPoint : public Thread
{
public:

    DevicesEndPoint(int interval);
    virtual ~DevicesEndPoint();

    /**
     * Thread methods for device discovery.
     * @param
     * @return
     */
    virtual void run();
    virtual void cleanup();

    /**
     * Call when CUSD response received.
     * @param dev - pointer to current device
     * @param ussd - cusd string
     * @return
     */
    virtual void onReceiveUSSD(CardDevice* dev, String ussd);

    /**
     * Call when new SMS message received.
     * @param dev - pointer to current device
     * @param caller - number of SMS sender
     * @param sms - SMS body
     * @return
     */
    virtual void onReceiveSMS(CardDevice* dev, const String& caller, const String& udh_data, const String& sms);

	/**
	 * Call when network status (rssi, lac, etc) changed. 
	 * @param dev - pointer to current dev
	 * @return
	 */
	virtual void onUpdateNetworkStatus(CardDevice* dev);

    /**
     * Send SMS message.
     * @param dev - pointer to current device
     * @param called - number of SMS recepient
     * @param sms - SMS body
     * @return
     */    
    bool sendSMS(CardDevice* dev, const String &called, const String &sms);

    /**
     * Send CUSD request.
     * @param dev - pointer to current device
     * @param ussd - ussd text. For example *100#
     * @return
     */    
    bool sendUSSD(CardDevice* dev, const String &ussd);

    /**
     * Append new device to endpoint.
     * @param name - unique device name for future access
     * @param data - list of configuration parameters
     * @return pointer of newly created device object
     */        
    CardDevice* appendDevice(String name, NamedList* data);

    /**
     * Find device by name
     * @param name - device name
     * @return device pointer or NULL if not found
     */
    CardDevice* findDevice(const String &name);

    /**
     * Find device by params
     * @param list - device params
     * @return device pointer or NULL if not found
     */
    CardDevice* findDevice(const NamedList &list);

    /**
     * Remove all devices from endpoint
     * @param
     * @return
     */            
    void cleanDevices();

    /**
     * Stop devices endpoints
     * @param
     * @return
     */
    void stopEP();

    /**
     * Get status of all devices
     * @param
     * @return
     */
    String devicesStatus();

    /**
     * Called on new incoming for call
     * @param dev - pointer to calling device
     * @param caller - additional user information
     * @return incaming call handling state
     */    
    virtual bool onIncamingCall(CardDevice* dev, const String &caller);

private:
    Mutex m_mutex;
    ObjList m_devices; //devices list
    int m_interval;  //discovery interval
    bool m_run;
};

#endif

/* vi: set ts=8 sw=4 sts=4 noet: */
