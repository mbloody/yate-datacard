#include "datacarddevice.h"


using namespace TelEngine;
namespace { // anonymous

static Configuration s_cfg;


class YDevEndPoint : public DevicesEndPoint
{
public:
    YDevEndPoint(int interval):DevicesEndPoint(interval){}
    ~YDevEndPoint(){}
    virtual void onReceiveUSSD(CardDevice* dev, String ussd)
    {
	Debug(DebugAll, "onReceiveUSSD Got USSD response: '%s'\n", ussd.c_str());
	Message* m = new Message("datacard.ussd");
	m->addParam("type","incoming");
	m->addParam("protocol","datacard");
	m->addParam("line",*dev);
	m->addParam("text",ussd);
	Engine::enqueue(m);
    }
    virtual void onReceiveSMS(CardDevice* dev, String caller, String sms)
    {
	Debug(DebugAll, "onReceiveSMS Got SMS from %s: '%s'\n", caller.c_str(), sms.c_str());
	Message* m = new Message("datacard.sms");
	m->addParam("type","incoming");
	m->addParam("protocol","datacard");
	m->addParam("line",*dev);
	m->addParam("caller",caller);
	m->addParam("text",sms);
	Engine::enqueue(m);
    }
    virtual Connection* createConnection(CardDevice* dev, void* usrData);
};

class SMSHandler : public MessageHandler
{
public:
    SMSHandler(YDevEndPoint* ep) : MessageHandler("datacard.sms"), m_ep(ep) { }
    virtual bool received(Message& msg);
private:
    YDevEndPoint* m_ep;
};

class USSDHandler : public MessageHandler
{
public:
    USSDHandler(YDevEndPoint* ep) : MessageHandler("datacard.ussd"), m_ep(ep) { }
    virtual bool received(Message& msg);
private:
    YDevEndPoint* m_ep;
};


class DatacardChannel;

class DatacardConsumer : public DataConsumer
{
public:
    DatacardConsumer(DatacardChannel* conn, const char* format);
    ~DatacardConsumer();
    virtual unsigned long Consume(const DataBlock &data, unsigned long tStamp, unsigned long flags);
private:
    DatacardChannel* m_connection;
};


class DatacardSource : public DataSource
{
public:
    DatacardSource(DatacardChannel* conn, const char* format);
    ~DatacardSource();
private:
    DatacardChannel* m_connection;
};


class DatacardDriver : public Driver
{
public:
    DatacardDriver();
    ~DatacardDriver();
    virtual void initialize();
    virtual bool msgExecute(Message& msg, String& dest);

private:
    YDevEndPoint* m_endpoint;
};

INIT_PLUGIN(DatacardDriver);

class DatacardChannel :  public Channel, public Connection
{
public:
    DatacardChannel(CardDevice* dev, Message* msg = 0) :
      Channel(__plugin, 0, (msg != 0)), Connection(dev)
    {
	m_address = 0;
	Message* s = message("chan.startup",msg);
	
	if (msg)
	{
	    s->copyParams(*msg,"caller,callername,called,billid,callto,username");
	    CallEndpoint* ch = YOBJECT(CallEndpoint,msg->userData());
	    if (ch && ch->connect(this,msg->getValue("reason"))) 
	    {
		callConnect(*msg);
		m_targetid = msg->getValue("id");
		msg->setParam("peerid",id());
		msg->setParam("targetid",id());
		deref();
	    }
	}
	Engine::enqueue(s);

	setSource(new DatacardSource(this,"slin"));
	getSource()->deref();
	setConsumer(new DatacardConsumer(this,"slin"));
	getConsumer()->deref();
		
    };
    ~DatacardChannel();
    
    
    virtual bool msgAnswered(Message& msg);
    virtual bool msgTone(Message& msg, const char* tone);

    
    virtual void disconnected(bool final, const char *reason);
    inline void setTargetid(const char* targetid)
	{ m_targetid = targetid; }

    virtual bool onIncoming(const String &caller);
    virtual bool onProgress();
    virtual bool onAnswered();
    virtual bool onHangup(int reason);
    
    
    virtual void forwardAudio(char* data, int len);
};


Connection* YDevEndPoint::createConnection(CardDevice* dev, void* usrData)
{
    Message* msg = static_cast<Message*>(usrData);
    DatacardChannel* channel = new DatacardChannel(dev, msg);
    if(channel)
	channel->initChan();
    return channel;
}


bool SMSHandler::received(Message &msg)
{
    String type(msg.getValue("type"));
    String protocol(msg.getValue("protocol"));
    if(type != "outgoing" || protocol != "datacard")
	return false;
    String line(msg.getValue("line"));
    CardDevice* dev = m_ep->findDevice(line);
    if(!dev)
	return false;
    String called(msg.getValue("called"));
    String text(msg.getValue("text"));
    return m_ep->sendSMS(dev, called, text);
}

bool USSDHandler::received(Message &msg)
{
    String type(msg.getValue("type"));
    String protocol(msg.getValue("protocol"));
    if(type != "outgoing" || protocol != "datacard")
	return false;
    String line(msg.getValue("line"));
    CardDevice* dev = m_ep->findDevice(line);
    if(!dev)
	return false;
    String text(msg.getValue("text"));
    return m_ep->sendUSSD(dev, text);
}


DatacardConsumer::DatacardConsumer(DatacardChannel* conn, const char* format): DataConsumer(format), m_connection(conn)
{
}

DatacardConsumer::~DatacardConsumer()
{
}

unsigned long DatacardConsumer::Consume(const DataBlock& data, unsigned long tStamp, unsigned long flags)
{
    if (!m_connection)
	return invalidStamp();
    m_connection->sendAudio((char*)data.data(), data.length());
	
    return 0;
}

DatacardSource::DatacardSource(DatacardChannel* conn, const char* format):DataSource(format), m_connection(conn)
{ 
}

DatacardSource::~DatacardSource()
{
}

void DatacardChannel::disconnected(bool final, const char *reason)
{
    Debug(DebugAll,"DatacardChannel::disconnected() '%s'",reason);
    Channel::disconnected(final,reason);
}

DatacardChannel::~DatacardChannel()
{
    Debug(this,DebugAll,"DatacardChannel::~DatacardChannel() src=%p cons=%p",getSource(),getConsumer());
    sendHangup();
    Engine::enqueue(message("chan.hangup"));
}


bool DatacardChannel::msgAnswered(Message& msg)
{
    return sendAnswer();
}

bool DatacardChannel::msgTone(Message& msg, const char* tone)
{
    return sendDTMF(*tone);               
}

bool DatacardChannel::onIncoming(const String &caller)
{
    Debug(this,DebugAll,"DatacardChannel::onIncoming(%s)", caller.c_str());
    
    Message *m = message("call.preroute",false,true);
    m->setParam("callername",caller);
    m->setParam("caller",caller);
//    called = s_cfg.getValue("incoming","called");
    m->setParam("called","123");
//TODO: enable tonedetect, must be configure????
    m->setParam("tonedetect_in","true");
//    m->addParam("formats",m_remoteFormats);
    m_dev->getStatus(m);
 
    if (startRouter(m))
	return true;
    Debug(this,DebugWarn,"Error starting routing thread! [%p]",this);
    return false;
    
//    return true;
}

bool DatacardChannel::onProgress()
{
    Debug(this,DebugAll,"DatacardChannel::onProgress()");
    status("progressing");
    Message *m = message("call.progress",false,true);
    Engine::enqueue(m);
    return true;
}
bool DatacardChannel::onAnswered()
{
    Debug(this,DebugAll,"DatacardChannel::onAnswered()");
    status("answered");
    Message *m = message("call.answered",false,true);
    Engine::enqueue(m);
    return true;
}
bool DatacardChannel::onHangup(int reason)
{
    Debug(this,DebugAll,"DatacardChannel::onHangup(%d)",reason);
    disconnect();
//    deref();
    return true;    
}


void DatacardChannel::forwardAudio(char* data, int len)
{
    DatacardSource* s = static_cast<DatacardSource*>(getSource());
    if(!s)
     return;
    s->Forward(DataBlock(data, len),0);
}



bool DatacardDriver::msgExecute(Message& msg, String& dest)
{
    if (dest.null())
        return false;
    if (!msg.userData()) 
    {
	Debug(this,DebugWarn,"Call found but no data channel!");
        return false;
    }
    
    CardDevice* dev = m_endpoint->findDevice(msg.getValue("device"));
    if(m_endpoint->MakeCall(dev, dest, &msg))
	return true;
    msg.setParam("error","congestion");
    return false;
}

DatacardDriver::DatacardDriver()
    : Driver("datacard", "varchans")
{
    Output("Loaded module DatacardChannel");
}

DatacardDriver::~DatacardDriver()
{
    Output("Unloading module DatacardChannel");
    m_endpoint->cleanDevices();
}

void DatacardDriver::initialize()
{
    Output("Initializing module DatacardChannel");
//TODO: make reload    
    s_cfg = Engine::configFile("datacard");
    s_cfg.load();
    m_endpoint = new YDevEndPoint(DEF_DISCOVERY_INT);
//    String preferred = s_cfg.getValue("formats","preferred");
//    bool def = s_cfg.getBoolValue("formats","default",true);
    String name;
    unsigned int n = s_cfg.sections();
    for (unsigned int i = 0; i < n; i++) 
    {
	NamedList* sect = s_cfg.getSection(i);
	if (!sect)
	    continue;
	name  = *sect;
	if (!name.startsWith("datacard"))
	    continue;
	m_endpoint->appendDevice(name, sect);
    }
    m_endpoint->startup();
    setup();
    Engine::install(new SMSHandler(m_endpoint));
    Engine::install(new USSDHandler(m_endpoint));
    Output("DatacardChannel initialized");
}

}; // anonymous namespace

/* vi: set ts=8 sw=4 sts=4 noet: */
