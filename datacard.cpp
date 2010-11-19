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

class DatacardChannel :  public Channel
{
public:
    DatacardChannel(const char* addr = 0, const NamedList* exeMsg = 0) :
      Channel(__plugin, 0, (exeMsg != 0))
    {
	m_address = addr;
	Message* s = message("chan.startup",exeMsg);
	if (exeMsg)
	    s->copyParams(*exeMsg,"caller,callername,called,billid,callto,username");
	Engine::enqueue(s);
    };
    ~DatacardChannel();
    virtual void disconnected(bool final, const char *reason);
    inline void setTargetid(const char* targetid)
	{ m_targetid = targetid; }
};


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
    String text(msg.getValue("text"));
    return m_ep->sendSMS(dev, text);
//    bool sendUSSD(CardDevice* dev, const String &ussd);

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


void DatacardChannel::disconnected(bool final, const char *reason)
{
    Debug(DebugAll,"DatacardChannel::disconnected() '%s'",reason);
    Channel::disconnected(final,reason);
}

DatacardChannel::~DatacardChannel()
{
    Debug(this,DebugAll,"DatacardChannel::~DatacardChannel() src=%p cons=%p",getSource(),getConsumer());
    Engine::enqueue(message("chan.hangup"));
}

bool DatacardDriver::msgExecute(Message& msg, String& dest)
{
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
