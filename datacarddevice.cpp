#include "datacarddevice.h"
#include <termios.h>
#include <fcntl.h>

using namespace TelEngine;


static int opentty (char* dev)
{
	int		fd;
	struct termios	term_attr;

	fd = open (dev, O_RDWR | O_NOCTTY);

	if (fd < 0)
	{
		Debug("opentty",DebugAll, "Unable to open '%s'\n", dev);
		return -1;
	}

	if (tcgetattr (fd, &term_attr) != 0)
	{
		Debug("opentty",DebugAll, "tcgetattr() failed '%s'\n", dev);
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
		Debug("opentty",DebugAll,"tcsetattr() failed '%s'\n", dev);
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
//    at_queue_t*	e;
//    int t;
//    int res;
//    struct iovec iov[2];
//    int iovcnt;
//    size_t size;
//    size_t i = 0;

    /* start datacard initilization with the AT request */
//    ast_mutex_lock (&pvt->lock);

//    pvt->timeout = 10000;

//    if (at_send_at (pvt) || at_fifo_queue_add (pvt, CMD_AT, RES_OK))
//    {
//	ast_log (LOG_ERROR, "[%s] Error sending AT\n", pvt->id);
//	goto e_cleanup;
//    }

//    ast_mutex_unlock (&pvt->lock);

//    while (!check_unloading ())
//    {
//	ast_mutex_lock (&pvt->lock);

//	if (device_status (pvt->data_fd) || device_status (pvt->audio_fd))
//	{
//		ast_log (LOG_ERROR, "Lost connection to Datacard %s\n", pvt->id);
//		goto e_cleanup;
//	}
//	t = pvt->timeout;

//	ast_mutex_unlock (&pvt->lock);


//	if (!at_wait (pvt, &t))
//	{
//	    ast_mutex_lock (&pvt->lock);
//	    if (!pvt->initialized)
//	    {
//		ast_debug (1, "[%s] timeout waiting for data, disconnecting\n", pvt->id);

//		if ((e = at_fifo_queue_head (pvt)))
//		{
//		    ast_debug (1, "[%s] timeout while waiting '%s' in response to '%s'\n", pvt->id,	at_res2str (e->res), at_cmd2str (e->cmd));
//		}

//		goto e_cleanup;
//	    }
//	    else
//	    {
//		ast_mutex_unlock (&pvt->lock);
//		continue;
//	    }
//	}
//	ast_mutex_lock (&pvt->lock);

//	if (at_read (pvt))
//	{
//		goto e_cleanup;
//	}
//	while ((iovcnt = at_read_result_iov (pvt)) > 0)
//	{
//	    at_res = at_read_result_classification (pvt, iovcnt);
	    
//	    if (at_response (pvt, iovcnt, at_res))
//	    {
//		goto e_cleanup;
//	    }
//	}
//	ast_mutex_unlock (&pvt->lock);
//    }

//    ast_mutex_lock (&pvt->lock);

//e_cleanup:
//    if (!pvt->initialized)
//    {
//    	ast_verb (3, "Error initializing Datacard %s\n", pvt->id);
//    }

//    disconnect_datacard (pvt);

//    ast_mutex_unlock (&pvt->lock);

}

void MonitorThread::cleanup()
{
}



CardDevice::CardDevice(String name):String(name), m_mutex(true), m_connected(false)
{
    m_data_fd = -1;
    m_audio_fd = -1;
}
bool CardDevice::startMonitor() 
{ 
    MonitorThread* m_monitor = new MonitorThread(this);
    return m_monitor->startup();
//    return true;
}

bool CardDevice::tryConnect()
{
    m_mutex.lock();
    if (!m_connected)
    {
	Debug("tryConnect",DebugAll,"Datacard %s trying to connect on %s...\n", safe(), data_tty.safe());
	if ((m_data_fd = opentty((char*)data_tty.safe())) > -1)
	{
		if ((m_audio_fd = opentty((char*)audio_tty.safe())) > -1)
		{
		    if (startMonitor())
		    {
			m_connected = true;
			Debug("tryConnect",DebugAll,"Datacard %s has connected, initializing...\n", safe());
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
    	Debug("disconnect",DebugAll,"[%s] Datacard not connected\n", safe());	
    	return m_connected;
    }
//    if (pvt->owner)
//    {
//    	Debug("disconnect",DebugAll,"[%s] Datacard disconnected, hanging up owner\n", pvt->id);
//		pvt->needchup = 0;
//		channel_queue_hangup (pvt, 0);
//    }

    close(m_data_fd);
    close(m_audio_fd);

    m_data_fd = -1;
    m_audio_fd = -1;

    m_connected	= false;
//	pvt->initialized	= 0;
//	pvt->gsm_registered	= 0;

//	pvt->incoming		= 0;
//	pvt->outgoing		= 0;
//	pvt->needring		= 0;
//	pvt->needchup		= 0;
	
//	pvt->gsm_reg_status	= -1;

//	pvt->manufacturer[0]	= '\0';
//	pvt->model[0]		= '\0';
//	pvt->firmware[0]	= '\0';
//	pvt->imei[0]		= '\0';
//	pvt->imsi[0]		= '\0';

//	ast_copy_string (pvt->provider_name,	"NONE",		sizeof (pvt->provider_name));
//	ast_copy_string (pvt->number,		"Unknown",	sizeof (pvt->number));

//	rb_init (&pvt->d_read_rb, pvt->d_read_buf, sizeof (pvt->d_read_buf));

//	at_fifo_queue_flush (pvt);

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
