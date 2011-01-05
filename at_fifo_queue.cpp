/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

#include "datacarddevice.h"
#include <stdlib.h>


int CardDevice::at_fifo_queue_add(at_cmd_t cmd, at_res_t res)
{
    return at_fifo_queue_add_ptr(cmd, res, NULL);
}

int CardDevice::at_fifo_queue_add_ptr(at_cmd_t cmd, at_res_t res, void* data)
{
    at_queue_t* e = new at_queue_t();
	
    e->cmd = cmd;
    e->res = res;
    e->ptype = 0;
    e->param.data = data;

    m_atQueue.append(e);

    Debug(DebugAll, "[%s] add command '%s' expected response '%s'\n", c_str(), at_cmd2str (e->cmd), at_res2str (e->res));
    return 0;
}

int CardDevice::at_fifo_queue_add_num(at_cmd_t cmd, at_res_t res, int num)
{
    at_queue_t* e = new at_queue_t();

    e->cmd = cmd;
    e->res = res;
    e->ptype = 1;
    e->param.num = num;

    m_atQueue.append(e);

    Debug(DebugAll, "[%s] add command '%s' expected response '%s'\n", c_str(), at_cmd2str(e->cmd), at_res2str(e->res));
    return 0;
}

void CardDevice::at_fifo_queue_rem()
{
    at_queue_t* e = static_cast<at_queue_t*>(m_atQueue.get());

    if(e)
    {
	Debug(DebugAll,"[%s] remove command '%s' expected response '%s'\n", c_str(), at_cmd2str (e->cmd), at_res2str (e->res));

	if (e->ptype == 0 && e->param.data)
	{
	    free(e->param.data);
	}
	m_atQueue.remove(e);
    }
}

void CardDevice::at_fifo_queue_flush()
{
    m_atQueue.clear();
}

at_queue_t* CardDevice::at_fifo_queue_head()
{
    return static_cast<at_queue_t*>(m_atQueue.get());
}

/* vi: set ts=8 sw=4 sts=4 noet: */
