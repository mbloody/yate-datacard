#include "datacarddevice.h"


using namespace TelEngine;
namespace { // anonymous

static Configuration s_cfg;
static ObjList s_devices;
static Mutex s_devicesMutex;
static bool s_run = true;


class DiscoveryThread: public Thread
{
public:
    DiscoveryThread();
    ~DiscoveryThread();
    virtual void run();
    virtual void cleanup();       
};

class DatacardDriver : public Driver
{
public:
    DatacardDriver();
    ~DatacardDriver();
    virtual void initialize();
    virtual bool msgExecute(Message& msg, String& dest);
    CardDevice* loadDevice(String name,NamedList* data);
    
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


//DiscoveryThread
DiscoveryThread::DiscoveryThread()
{
}

DiscoveryThread::~DiscoveryThread()
{
}

void DiscoveryThread::run()
{
    while(s_run)
    {
	CardDevice* dev = 0;
	s_devicesMutex.lock();
        const ObjList *devicesIter = &s_devices;
	while (devicesIter)
	{
		GenObject* obj = devicesIter->get();
		devicesIter = devicesIter->next();
		if (!obj) continue;	
    		dev = static_cast<CardDevice*>(obj);
    		dev->tryConnect();
	}
	s_devicesMutex.unlock();
	
	if (s_run)
	{
	    Thread::sleep(discovery_interval);
        }
    }
}
void DiscoveryThread::cleanup()
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
    CardDevice* dev = 0;
    s_devicesMutex.lock();
    const ObjList *devicesIter = &s_devices;
    while (devicesIter)
    {
	GenObject* obj = devicesIter->get();
	devicesIter = devicesIter->next();
	if (!obj) continue;	
	dev = static_cast<CardDevice*>(obj);
	dev->disconnect();
    }
    s_devicesMutex.unlock();

    Output("Unloading module DatacardChannel");
}

CardDevice* DatacardDriver::loadDevice(String name, NamedList* data)
{
    String audio_tty = data->getValue("audio");
    String data_tty = data->getValue("data");
    
    CardDevice * dev = new CardDevice(name);
    dev->data_tty = data_tty;
    dev->audio_tty = audio_tty;
    return dev;
}


void DatacardDriver::initialize()
{
    Output("Initializing module DatacardChannel");
    s_cfg = Engine::configFile("datacard");
    s_cfg.load();
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
	CardDevice* dev = loadDevice(name, sect);
	if(dev)
	{
	    s_devicesMutex.lock();
	    s_devices.append(dev);
	    s_devicesMutex.unlock();
	}
    }
    DiscoveryThread* dt = new DiscoveryThread();
    dt->startup();
    setup();
    Output("DatacardChannel initialized");
}

}; // anonymous namespace

/* vi: set ts=8 sw=4 sts=4 noet: */
