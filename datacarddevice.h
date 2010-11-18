#include <yatephone.h>
#ifndef DATACARDDEVICE_H
#define DATACARDDEVICE_H


#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define CONFIG_FILE		"datacard.conf"
#define DEF_DISCOVERY_INT	60

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

typedef struct at_queue_t
{
//	AST_LIST_ENTRY (at_queue_t) entry;

	at_cmd_t		cmd;
	at_res_t		res;

	int			ptype;

	union
	{
		void*		data;
		int		num;
	} param;
}
at_queue_t;

class CardDevice;
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

class CardDevice: public String
{
public:
    CardDevice(String name);
    bool tryConnect();
    bool disconnect();
    

    int dataStatus()
    { return devStatus(m_data_fd);}
    int audioStatus()
    { return devStatus(m_audio_fd);}

//private:
    bool startMonitor();
    int devStatus(int fd);

//	AST_LIST_HEAD_NOLOCK (at_queue, at_queue_t) at_queue;	/* queue for response we are expecting */
//	pthread_t		monitor_thread;			/* monitor thread handle */
//	Thread monitor_thread;
    MonitorThread m_monitor;
    Mutex m_mutex;
    
    int m_audio_fd;			/* audio descriptor */
    int m_data_fd;			/* data  descriptor */

//	struct ast_channel*	owner;				/* Channel we belong to, possibly NULL */
//	struct ast_dsp*		dsp;
//	struct ast_timer*	a_timer;

	char			a_write_buf[FRAME_SIZE * 5];
//	ringbuffer_t		a_write_rb;
//	char			a_read_buf[FRAME_SIZE + AST_FRIENDLY_OFFSET];
//	struct ast_frame	a_read_frame;

	char			d_send_buf[2*1024];
	size_t			d_send_size;
	char			d_read_buf[2*1024];
//	ringbuffer_t		d_read_rb;
	struct iovec		d_read_iov[2];
	unsigned int		d_read_result:1;
	char			d_parse_buf[1024];
	int			timeout;			/* used to set the timeout for data */

	unsigned int		has_sms:1;
	unsigned int		has_voice:1;
	unsigned int		use_ucs2_encoding:1;
	unsigned int		cusd_use_7bit_encoding:1;
	unsigned int		cusd_use_ucs2_decoding:1;
	int			gsm_reg_status;
	int			rssi;
	int			linkmode;
	int			linksubmode;
	char			provider_name[32];
	char			manufacturer[32];
	char			model[32];
	char			firmware[32];
	char			imei[17];
	char			imsi[17];
	char			number[128];
	char			location_area_code[8];
	char			cell_id[8];

	/* flags */
	bool			m_connected;			/* do we have an connection to a device */
	unsigned int		initialized:1;			/* whether a service level connection exists or not */
	unsigned int		gsm_registered:1;		/* do we have an registration to a GSM */
	unsigned int		outgoing:1;			/* outgoing call */
	unsigned int		incoming:1;			/* incoming call */
	unsigned int		outgoing_sms:1;			/* outgoing sms */
	unsigned int		incoming_sms:1;			/* incoming sms */
	unsigned int		needchup:1;			/* we need to send a CHUP */
	unsigned int		needring:1;			/* we need to send a RING */
	unsigned int		answered:1;			/* we sent/received an answer */
	unsigned int		volume_synchronized:1;		/* we have synchronized the volume */
	unsigned int		group_last_used:1;		/* mark the last used device */
	unsigned int		prov_last_used:1;		/* mark the last used device */
	unsigned int		sim_last_used:1;		/* mark the last used device */

	/* Config */
	String			audio_tty;			/* tty for audio connection */
	String			data_tty;			/* tty for AT commands */
//	char			context[AST_MAX_CONTEXT];	/* the context for incoming calls */
	int			group;				/* group number for group dialling */
	int			rxgain;				/* increase the incoming volume */
	int			txgain;				/* increase the outgoint volume */
	int			u2diag;
	int			callingpres;			/* calling presentation */
	unsigned int		auto_delete_sms:1;
	unsigned int		reset_datacard:1;
	unsigned int		usecallingpres:1;
	unsigned int		disablesms:1;
};


//static AST_RWLIST_HEAD_STATIC (devices, pvt_t);

static int			discovery_interval = DEF_DISCOVERY_INT;	/* The device discovery interval */
//static pthread_t		discovery_thread   = AST_PTHREADT_NULL;	/* The discovery thread */

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


//static int			opentty			(char*);
//static int			device_status		(int);
//static int			disconnect_datacard	(pvt_t*);

/*
static inline int		at_wait			(pvt_t*, int*);
static inline int		at_read			(pvt_t*);
static int			at_read_result_iov	(pvt_t*);
static inline at_res_t		at_read_result_classification (pvt_t*, int);


static inline int		at_response		(pvt_t*, int, at_res_t);
static inline int		at_response_busy	(pvt_t*);
static inline int		at_response_cend	(pvt_t*, char*, size_t);
static inline int		at_response_cgmi	(pvt_t*, char*, size_t);
static inline int		at_response_cgmm	(pvt_t*, char*, size_t);
static inline int		at_response_cgmr	(pvt_t*, char*, size_t);
static inline int		at_response_cgsn	(pvt_t*, char*, size_t);
static inline int		at_response_cimi	(pvt_t*, char*, size_t);
static inline int		at_response_clip	(pvt_t*, char*, size_t);
static inline int		at_response_cmgr	(pvt_t*, char*, size_t);
static inline int		at_response_cmti	(pvt_t*, char*, size_t);
static inline int		at_response_cnum	(pvt_t*, char*, size_t);
static inline int		at_response_conn	(pvt_t*);
static inline int		at_response_cops	(pvt_t*, char*, size_t);
static inline int		at_response_cpin	(pvt_t*, char*, size_t);
static inline int		at_response_creg	(pvt_t*, char*, size_t);
static inline int		at_response_csq		(pvt_t*, char*, size_t);
static inline int		at_response_cssi	(pvt_t*, char*, size_t);
static inline int		at_response_cusd	(pvt_t*, char*, size_t);
static inline int		at_response_error	(pvt_t*);
static inline int		at_response_mode	(pvt_t*, char*, size_t);
static inline int		at_response_no_carrier	(pvt_t*);
static inline int		at_response_no_dialtone	(pvt_t*);
static inline int		at_response_ok		(pvt_t*);
static inline int		at_response_orig	(pvt_t*, char*, size_t);
static inline int		at_response_ring	(pvt_t*);
static inline int		at_response_rssi	(pvt_t*, char*, size_t);
static inline int		at_response_smmemfull	(pvt_t*);
static inline int		at_response_sms_prompt	(pvt_t*);


static const char*		at_cmd2str		(at_cmd_t);
static const char*		at_res2str		(at_res_t);

static char*			at_parse_clip		(pvt_t*, char*, size_t);
static int			at_parse_cmgr		(pvt_t*, char*, size_t, char**, char**);
static int			at_parse_cmti		(pvt_t*, char*, size_t);

static inline char*		at_parse_cnum		(pvt_t*, char*, size_t);
static inline char*		at_parse_cops		(pvt_t*, char*, size_t);
static inline int		at_parse_creg		(pvt_t*, char*, size_t, int*, int*, char**, char**);
static inline int		at_parse_cpin		(pvt_t*, char*, size_t);
static inline int		at_parse_csq		(pvt_t*, char*, size_t, int*);
static inline int		at_parse_cusd		(pvt_t*, char*, size_t, char**, unsigned char*);
static inline int		at_parse_mode		(pvt_t*, char*, size_t, int*, int*);
static inline int		at_parse_rssi		(pvt_t*, char*, size_t);


static inline int		at_write		(pvt_t*, char*);
static int			at_write_full		(pvt_t*, char*, size_t);

static inline int		at_send_at		(pvt_t*);
static inline int		at_send_ata		(pvt_t*);
static inline int		at_send_atd		(pvt_t*, const char* number);
static inline int		at_send_ate0		(pvt_t*);
static inline int		at_send_atz		(pvt_t*);
static inline int		at_send_cgmi		(pvt_t*);
static inline int		at_send_cgmm		(pvt_t*);
static inline int		at_send_cgmr		(pvt_t*);
static inline int		at_send_cgsn		(pvt_t*);
static inline int		at_send_chup		(pvt_t*);
static inline int		at_send_cimi		(pvt_t*);
static inline int		at_send_clip		(pvt_t*, int status);
static inline int		at_send_clir		(pvt_t*, int mode);
static inline int		at_send_clvl		(pvt_t*, int level);
static inline int		at_send_cmgd		(pvt_t*, int index);
static inline int		at_send_cmgf		(pvt_t*, int mode);
static inline int		at_send_cmgr		(pvt_t*, int index);
static inline int		at_send_cmgs		(pvt_t*, const char* number);
static inline int		at_send_cnmi		(pvt_t*);
static inline int		at_send_cnum		(pvt_t*);
static inline int		at_send_cops		(pvt_t*);
static inline int		at_send_cops_init	(pvt_t*, int mode, int format);
static inline int		at_send_cpin_test	(pvt_t*);
static inline int		at_send_creg		(pvt_t*);
static inline int		at_send_creg_init	(pvt_t*, int level);
static inline int		at_send_cscs		(pvt_t*, const char* encoding);
static inline int		at_send_csq		(pvt_t*);
static inline int		at_send_cssn		(pvt_t*, int cssi, int cssu);
static inline int		at_send_cusd		(pvt_t*, const char* code);
static inline int		at_send_cvoice_test	(pvt_t*);
static inline int		at_send_ddsetex		(pvt_t*);
static inline int		at_send_dtmf		(pvt_t*, char digit);
static inline int		at_send_sms_text	(pvt_t*, const char* message);
static inline int		at_send_u2diag		(pvt_t*, int mode);
static inline int		at_send_ccwa_disable	(pvt_t*);
static inline int		at_send_cfun		(pvt_t*, int, int);
static inline int		at_send_cmee		(pvt_t*, int);


static inline int		at_fifo_queue_add	(pvt_t*, at_cmd_t, at_res_t);
static int			at_fifo_queue_add_ptr	(pvt_t*, at_cmd_t, at_res_t, void*);
static int			at_fifo_queue_add_num	(pvt_t*, at_cmd_t, at_res_t, int);
static inline void		at_fifo_queue_rem	(pvt_t*);
static inline void		at_fifo_queue_flush	(pvt_t*);
static inline at_queue_t*	at_fifo_queue_head	(pvt_t*);
*/



#endif

