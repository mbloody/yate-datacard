/**
 * datacard.cpp
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


using namespace TelEngine;
namespace { // anonymous


#define DEF_DISCOVERY_INT 60


static TokenDict dict_errors[] = {
    { "incomplete", DATACARD_INCOMPLETE },
    { "noroute", DATACARD_NOROUTE },
    { "noconn", DATACARD_NOCONN },
    { "nomedia", DATACARD_NOMEDIA },
    { "nocall", DATACARD_NOCALL },
    { "busy", DATACARD_BUSY },
    { "noanswer", DATACARD_NOANSWER },
    { "rejected", DATACARD_REJECTED },
    { "forbidden", DATACARD_FORBIDDEN },
    { "offline", DATACARD_OFFLINE },
    { "congestion", DATACARD_CONGESTION },
    { "failure", DATACARD_FAILURE },
    {  0,   0 },
};


static Configuration s_cfg;

//TODO: make configurable for devices
static bool s_inband_dtmf = false;

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
	m->addParam("module","datacard");
	m->addParam("text",ussd);
	dev->getParams(m);
	Engine::enqueue(m);

	Message* mu = new Message("module.update");
	mu->addParam("from",*dev + "/USSD");
	mu->addParam("text",ussd);
	Engine::enqueue(mu);
    }

    virtual void onReceiveSMS(CardDevice* dev, const String& caller, const String& udh_data, const String& sms)
    {
	Debug(DebugAll, "onReceiveSMS Got SMS from %s: '%s'\n", caller.c_str(), sms.c_str());
	Message* m = new Message("datacard.sms");
	m->addParam("type","incoming");
	m->addParam("module","datacard");
	m->addParam("caller",caller);
	if(!udh_data.null())
	    m->addParam("udh_data", udh_data);
	m->addParam("text",sms);
	dev->getParams(m);
	Engine::enqueue(m);
    }

	virtual void onUpdateNetworkStatus(CardDevice* dev)
	{
		Debug(DebugAll, "Network status updated");
		Message* m = new Message("datacard.monitor");
		m->addParam("module", "datacard");
		dev->getNetworkStatus(m);
		Engine::enqueue(m);
	}

    virtual bool onIncamingCall(CardDevice* dev, const String &caller);
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

class DatacardDriver : public Driver
{
public:
    DatacardDriver();
    ~DatacardDriver();
    virtual void initialize();
    virtual bool msgExecute(Message& msg, String& dest);
    virtual bool received(Message& msg, int id);

    bool doCommand(String& line, String& rval);
    void doComplete(const String& partLine, const String& partWord, String& rval);
private:
    YDevEndPoint* m_endpoint;
    String m_statusCmd;
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
	    s->copyParams(*msg,"caller,callername,called,billid,callto,username");
	Engine::enqueue(s);

	setSource(dev->source());
	setConsumer(dev->consumer());
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
};


bool YDevEndPoint::onIncamingCall(CardDevice* dev, const String &caller)
{
    DatacardChannel* chan = new DatacardChannel(dev);
    dev->setConnection(chan);
    chan->initChan();
    return true;
}

bool SMSHandler::received(Message &msg)
{
    String type(msg.getValue("type"));
    String protocol(msg.getValue("protocol"));
    if(type != "outgoing" || protocol != "datacard")
	return false;
    String device(msg.getValue("device"));
    CardDevice* dev = m_ep->findDevice(device);
    if (!dev)
	dev = m_ep->findDevice(msg);
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
    String device(msg.getValue("device"));
    CardDevice* dev = m_ep->findDevice(device);
    if (!dev)
	dev = m_ep->findDevice(msg);

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
    sendHangup();
    Engine::enqueue(message("chan.hangup"));
    setSource();
    setConsumer();
}


bool DatacardChannel::msgAnswered(Message& msg)
{
    status("answered");
    return sendAnswer();
}

bool DatacardChannel::msgTone(Message& msg, const char* tone)
{
    if(s_inband_dtmf)
	return dtmfInband(tone);
    return sendDTMF(*tone);
}

bool DatacardChannel::onIncoming(const String &caller)
{
    Debug(this,DebugAll,"DatacardChannel::onIncoming(%s)", caller.c_str());

    Message *m = message("call.preroute",false,true);
    m->setParam("callername", caller);
    m->setParam("caller", caller);
    m->setParam("called", m_dev->getNumber());
    m_dev->getParams(m);

    if (startRouter(m))
	return true;
    Debug(this,DebugWarn,"Error starting routing thread! [%p]",this);
    return false;
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
    if( reason == DATACARD_NORMAL)
	disconnect();
    else
	disconnect(lookup(reason,dict_errors));
    return true;
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

    if (!dev)
	dev = m_endpoint->findDevice(msg);

    if (!dev)
    {
	Debug(this,DebugWarn,"Device not found");
	return false;
    }

    if(dev->isBusy())
    {
	Debug(this,DebugWarn,"Device is busy");
	return false;
    }

    DatacardChannel* chan = new DatacardChannel(dev, &msg);
    dev->setConnection(chan);
    chan->initChan();

    CallEndpoint* ch = static_cast<CallEndpoint*>(msg.userData());
    if (ch && chan->connect(ch,msg.getValue(YSTRING("reason"))) && dev->newCall(dest))
    {
	chan->callConnect(msg);
	msg.setParam("peerid",chan->id());
	msg.setParam("targetid",chan->id());
	chan->deref();
	return true;
    }
    chan->destruct();
    return false;
}


// perform command line completion
void DatacardDriver::doComplete(const String& partLine, const String& partWord, String& rval)
{
    if (partLine.null() || (partLine == "help") || (partLine == "status"))
	Module::itemComplete(rval,"datacard",partWord);
    else if (partLine == "datacard") {
//	Module::itemComplete(rval,"config",partWord);
	Module::itemComplete(rval,"ussd",partWord);
    }
    else if ((partLine == "datacard ussd")) {
	for (unsigned int i=0;i<s_cfg.sections();i++) 
	{
	    NamedList* dev = s_cfg.getSection(i);
	    if(dev && dev->getBoolValue("enabled",true) && (*dev != "general"))
		Module::itemComplete(rval,*dev,partWord);
	}
    }

    Lock lock(this);
    if (partLine == m_statusCmd)
    {
	Module::itemComplete(rval,"devices",partWord);
    }
    lock.drop();
}

bool DatacardDriver::doCommand(String& line, String& rval)
{
    if(line.startSkip("ussd")) 
    {
	int q = line.find(' ');
	if(q >= 0) 
	{
	    String ussd = line.substr(q+1).trimBlanks();
	    line = line.substr(0,q).trimBlanks();

	    CardDevice* dev = m_endpoint->findDevice(line);
            if(dev && m_endpoint->sendUSSD(dev, ussd))
	        rval << "USSD send success!";
	    else
		rval << "Error: USSD not send.";
	}
	else 
        {
	    rval << "USSD command error";
	}
    }
    rval << "\r\n";
    return true;
}

bool DatacardDriver::received(Message& msg, int id)
{
//TODO: implement this
    if (id == Status)
    {
	String target = msg.getValue("module");
	if (!target || target == name() || target.startsWith(prefix()))
	    return Driver::received(msg,id);
	if (!target.startSkip(name(),false))
	    return false;
	target.trimBlanks();
	
	String detail;
	if(target == "devices")
	{
	    detail = m_endpoint->devicesStatus();
	}
	msg.retValue().clear();
	msg.retValue() << "module=" << name();
	msg.retValue() << "," << target;
	if (detail)
	    msg.retValue() << ";" << detail;
	msg.retValue() << "\r\n";
	return true;
    }
    else if (id == Command)
    {
	String line = msg.getValue("line");
	if (line.null()) 
	{
	    doComplete(msg.getValue("partline"), msg.getValue("partword"), msg.retValue());
	    return false;
	}
        if (!line.startSkip("datacard"))
            return false;
        if(!doCommand(line, msg.retValue()))
	    msg.retValue() = "Datacard operation failed: " + line + "\r\n";
        return true;
    }
    else if (id == Halt)
    {
	if(m_endpoint)
	{
	    m_endpoint->cleanDevices();
	    m_endpoint->stopEP();
	}
	return Driver::received(msg,id);
    }

    return Driver::received(msg,id);
}

DatacardDriver::DatacardDriver()
    : Driver("datacard", "varchans"),m_endpoint(0)
{
    Output("Loaded module Datacard");
    m_statusCmd << "status " << name();
}

DatacardDriver::~DatacardDriver()
{
    Output("Unloading module Datacard");
//    m_endpoint->cleanDevices();
}

void DatacardDriver::initialize()
{
    bool first = true;
    if(m_endpoint)
    {
        Output("DatacardChannel already initialized");
        return;
    }

    Output("Initializing module Datacard Revision %s", DTC_VER);
    s_cfg = Engine::configFile("datacard");
    s_cfg.load();

    int discovery_interval = s_cfg.getIntValue("general","discovery-interval",DEF_DISCOVERY_INT);
    Output("Discovery Interval %d", discovery_interval);

    s_inband_dtmf = s_cfg.getBoolValue("general","inband_dtmf",false);
    if(first)
	m_endpoint = new YDevEndPoint(discovery_interval);
    else
	m_endpoint->cleanDevices();
    String name;
    unsigned int n = s_cfg.sections();
    for (unsigned int i = 0; i < n; i++) 
    {
	NamedList* sect = s_cfg.getSection(i);
	if(!sect || sect->null() || *sect == "general")
	    continue;
	if(!sect->getBoolValue("enabled",true))
	    continue;
	name  = *sect;
	m_endpoint->appendDevice(name, sect);
    }

    if(first)
    {
	m_endpoint->startup();
	setup();
	installRelay(Halt);
	Engine::install(new SMSHandler(m_endpoint));
	Engine::install(new USSDHandler(m_endpoint));
    }
    Output("DatacardChannel initialized");
}

}; // anonymous namespace

/* vi: set ts=8 sw=4 sts=4 noet: */
