#ifndef DATACARDDEVICE_H
#define DATACARDDEVICE_H
#include <yatephone.h>
#include "ringbuffer.h"
#include "endreasons.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define FRAME_SIZE		320


using namespace TelEngine;

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

class at_queue_t : public GenObject
{
public:
    at_cmd_t		cmd;
    at_res_t		res;

    int			ptype;

    union
    {
    	void*		data;
	int		num;
    } param;
};

class CardDevice;
class DevicesEndPoint;
class Connection;

class MonitorThread : public Thread
{
public:
    MonitorThread(CardDevice* dev);
    ~MonitorThread();
    virtual void run();
    virtual void cleanup();
private:
    CardDevice* m_device;
};


class MediaThread : public Thread
{
public:
    MediaThread(CardDevice* dev);
    ~MediaThread();
    virtual void run();
    virtual void cleanup();
private:
    CardDevice* m_device;
};

class CardDevice: public String
{
public:
    CardDevice(String name, DevicesEndPoint* ep);
    bool tryConnect();
    bool disconnect();
    

    int dataStatus()
    { return devStatus(m_data_fd);}
    int audioStatus()
    { return devStatus(m_audio_fd);}

    bool getStatus(NamedList * list);

//private:
    bool startMonitor();
    int devStatus(int fd);

//	AST_LIST_HEAD_NOLOCK (at_queue, at_queue_t) at_queue;	/* queue for response we are expecting */
//	pthread_t		monitor_thread;			/* monitor thread handle */
//	Thread monitor_thread;
    DevicesEndPoint* m_endpoint;
    MonitorThread* m_monitor;
    Mutex m_mutex;
    Connection* m_conn;


    MediaThread* m_media;

    
    int m_audio_fd;			/* audio descriptor */
    int m_data_fd;			/* data  descriptor */

//	struct ast_timer*	a_timer;

    char a_write_buf[FRAME_SIZE * 5];
    RingBuffer a_write_rb;
    char a_read_buf[FRAME_SIZE * 5];
    RingBuffer a_read_rb;
	
    char d_send_buf[2*1024];
    size_t d_send_size;
    char d_read_buf[2*1024];
    RingBuffer d_read_rb;
    struct iovec d_read_iov[2];
    unsigned int d_read_result:1;
    char d_parse_buf[1024];
    int timeout;			/* used to set the timeout for data */

    unsigned int has_sms:1;
    unsigned int has_voice:1;
    unsigned int use_ucs2_encoding:1;
    unsigned int cusd_use_7bit_encoding:1;
    unsigned int cusd_use_ucs2_decoding:1;
    int gsm_reg_status;
    int rssi;
    int linkmode;
    int linksubmode;
    String m_provider_name;
    String m_manufacturer;
    String m_model;
    String m_firmware;
    String m_imei;
    String m_imsi;
    String m_number;
    String m_location_area_code;
    String m_cell_id;

    /* flags */
    bool m_connected;			/* do we have an connection to a device */
    unsigned int initialized:1;			/* whether a service level connection exists or not */
    unsigned int gsm_registered:1;		/* do we have an registration to a GSM */
    unsigned int outgoing:1;			/* outgoing call */
    unsigned int incoming:1;			/* incoming call */
    unsigned int outgoing_sms:1;			/* outgoing sms */
    unsigned int incoming_sms:1;			/* incoming sms */
    unsigned int needchup:1;			/* we need to send a CHUP */
    unsigned int needring:1;			/* we need to send a RING */
    unsigned int answered:1;			/* we sent/received an answer */
    unsigned int volume_synchronized:1;		/* we have synchronized the volume */
    unsigned int group_last_used:1;		/* mark the last used device */
    unsigned int prov_last_used:1;		/* mark the last used device */
    unsigned int sim_last_used:1;		/* mark the last used device */
	
    // TODO: Running flag. Do we need to stop every MonitorThread or set one 
    // flag on module level? Do we need syncronization?
    bool m_running;
    bool isRunning() const;
    void stopRunning();
	    
	/* Config */
    String			audio_tty;			/* tty for audio connection */
    String			data_tty;			/* tty for AT commands */
//    char			context[AST_MAX_CONTEXT];	/* the context for incoming calls */
//    int group;				/* group number for group dialling */
    int rxgain;				/* increase the incoming volume */
    int txgain;				/* increase the outgoint volume */
    int u2diag;
    int callingpres;			/* calling presentation */
    bool m_auto_delete_sms;
    bool m_reset_datacard;
    bool m_usecallingpres;
    bool m_disablesms;
		
    // AT command methods.
    int at_wait(int*);
    int at_read();
    int at_read_result_iov();
    at_res_t at_read_result_classification(int);
    
    int at_response(int, at_res_t);
    int t_response_busy();
    int at_response_cend(char*, size_t);
    int at_response_cgmi(char*, size_t);
    int at_response_cgmm(char*, size_t);
    int at_response_cgmr(char*, size_t);
    int at_response_cgsn(char*, size_t);
    int at_response_cimi(char*, size_t);
    int at_response_clip(char*, size_t);
    int at_response_cmgr(char*, size_t);
    int at_response_cmti(char*, size_t);
    int at_response_cnum(char*, size_t);
    int at_response_conn();
    int at_response_cops(char*, size_t);
    int at_response_cpin(char*, size_t);
    int at_response_creg(char*, size_t);
    int at_response_csq(char*, size_t);
    int at_response_cssi(char*, size_t);
    int at_response_cusd(char*, size_t);
    int at_response_error();
    int at_response_mode(char*, size_t);
    int at_response_no_carrier();
    int at_response_no_dialtone();
    int at_response_ok();
    int at_response_orig(char*, size_t);
    int at_response_ring();
    int at_response_rssi(char*, size_t);
    int at_response_smmemfull();
    int at_response_sms_prompt();
    int at_response_busy();

    const char* at_cmd2str(at_cmd_t);
    const char* at_res2str(at_res_t);

    char* at_parse_clip(char*, size_t);
    int at_parse_cmgr(char*, size_t, char**, char**);
    int at_parse_cmti(char*, size_t);

    char* at_parse_cnum(char*, size_t);
    char* at_parse_cops(char*, size_t);
    int at_parse_creg(char*, size_t, int*, int*, char**, char**);
    int at_parse_cpin(char*, size_t);
    int at_parse_csq(char*, size_t, int*);
    int at_parse_cusd(char*, size_t, char**, unsigned char*);
    int at_parse_mode(char*, size_t, int*, int*);
    int at_parse_rssi(char*, size_t);


    int at_write(char*);
    int at_write_full(char*, size_t);

    int at_send_at();
    int at_send_ata();
    int at_send_atd(const char* number);
    int at_send_ate0();
    int at_send_atz();
    int at_send_cgmi();
    int at_send_cgmm();
    int at_send_cgmr();
    int at_send_cgsn();
    int at_send_chup();
    int at_send_cimi();
    int at_send_clip(int status);
    int at_send_clir(int mode);
    int at_send_clvl(int level);
    int at_send_cmgd(int index);
    int at_send_cmgf(int mode);
    int at_send_cmgr(int index);
    int at_send_cmgs(const char* number);
    int at_send_cnmi();
    int at_send_cnum();
    int at_send_cops();
    int at_send_cops_init(int mode, int format);
    int at_send_cpin_test();
    int at_send_creg();
    int at_send_creg_init(int level);
    int at_send_cscs(const char* encoding);
    int at_send_csq();
    int at_send_cssn(int cssi, int cssu);
    int at_send_cusd(const char* code);
    int at_send_cvoice_test();
    int at_send_ddsetex();
    int at_send_dtmf(char digit);
    int at_send_sms_text(const char* message);
    int at_send_u2diag(int mode);
    int at_send_ccwa_disable();
    int at_send_cfun(int, int);
    int at_send_cmee(int);
    int at_send_cpms();

    ObjList m_atQueue;
    int	at_fifo_queue_add(at_cmd_t, at_res_t);
    int	at_fifo_queue_add_ptr(at_cmd_t, at_res_t, void*);
    int	at_fifo_queue_add_num(at_cmd_t, at_res_t, int);
    void at_fifo_queue_rem();
    void at_fifo_queue_flush();
    at_queue_t* at_fifo_queue_head();

    ssize_t convert_string(const char* in, size_t in_length, char* out, size_t out_size, char* from, char* to);
    ssize_t hexstr_to_ucs2char(const char* in, size_t in_length, char* out, size_t out_size);
    ssize_t ucs2char_to_hexstr(const char* in, size_t in_length, char* out, size_t out_size);
    ssize_t hexstr_ucs2_to_utf8(const char* in, size_t in_length, char* out, size_t out_size);
    ssize_t utf8_to_hexstr_ucs2(const char* in, size_t in_length, char* out, size_t out_size);
    ssize_t char_to_hexstr_7bit(const char* in, size_t in_length, char* out, size_t out_size);
    ssize_t hexstr_7bit_to_char(const char* in, size_t in_length, char* out, size_t out_size);

// SMS and USSD
    bool sendSMS(const String &called, const String &sms);
    bool sendUSSD(const String &ussd);   

    void forwardAudio(char* data, int len);
    int sendAudio(char* data, int len);
    
    bool newCall(const String &called, void* usrData);
    
private:
    bool incomingCall(const String &caller);
    bool Hangup(int error);
    int getReason(int end_status, int cc_cause);
};


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

    virtual void forwardAudio(char* data, int len);
    int sendAudio(char* data, int len);
    
protected:
    CardDevice* m_dev;
};

class DevicesEndPoint : public Thread
{
public:
    
    DevicesEndPoint(int interval);
    virtual ~DevicesEndPoint();
    
    
    virtual void run();
    virtual void cleanup();
    
    virtual void onReceiveUSSD(CardDevice* dev, String ussd);
    virtual void onReceiveSMS(CardDevice* dev, String caller, String sms);
    
    bool sendSMS(CardDevice* dev, const String &called, const String &sms);
    bool sendUSSD(CardDevice* dev, const String &ussd);
    
    CardDevice* appendDevice(String name, NamedList* data);
    CardDevice* findDevice(const String &name);
    void cleanDevices();
    
    virtual Connection* createConnection(CardDevice* dev, void* usrData = 0);
    bool MakeCall(CardDevice* dev, const String &called, void* usrData);

private:
    Mutex m_mutex;
    ObjList m_devices;
    int m_interval;
    bool m_run;
};







//AST_MUTEX_DEFINE_STATIC (unload_mtx);
//static int			unloading_flag = 0;
//static inline int		check_unloading ();


/* Helpers */

//static pvt_t*			find_device			(const char*);
//static char*			complete_device			(const char*, const char*, int, int, int);
//static inline int		get_at_clir_value		(pvt_t*, int);


/*! Global jitterbuffer configuration - by default, jb is disabled */
/*
static struct ast_jb_conf jbconf_default = {
	.flags			= 0,
	.max_size		= -1,
	.resync_threshold	= -1,
	.impl			= "",
	.target_extra		= -1,
};

static struct ast_jb_conf jbconf_global;
*/
//AST_MUTEX_DEFINE_STATIC (round_robin_mtx);
//static pvt_t*	round_robin[256];
//static char	silence_frame[FRAME_SIZE];


//static int			device_status		(int);

#endif

