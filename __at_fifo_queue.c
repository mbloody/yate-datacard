/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

/*!
 * \brief Add an item to the back of the queue
 * \param pvt -- pvt structure
 * \param cmd -- the command that was sent to generate the response
 * \param res -- the expected response
 */

static inline int at_fifo_queue_add (pvt_t* pvt, at_cmd_t cmd, at_res_t res)
{
	return at_fifo_queue_add_ptr (pvt, cmd, res, NULL);
}

/*!
 * \brief Add an item to the back of the queue with pointer data
 * \param pvt -- pvt structure
 * \param cmd -- the command that was sent to generate the response
 * \param res -- the expected response
 * \param data -- pointer data associated with this entry, it will be freed when the message is freed
 */

static int at_fifo_queue_add_ptr (pvt_t* pvt, at_cmd_t cmd, at_res_t res, void* data)
{
	at_queue_t* e;

	if (!(e = ast_calloc (1, sizeof(*e))))
	{
		return -1;
	}

	e->cmd		= cmd;
	e->res		= res;
	e->ptype	= 0;
	e->param.data	= data;

	AST_LIST_INSERT_TAIL (&pvt->at_queue, e, entry);

	ast_debug (4, "[%s] add command '%s' expected response '%s'\n", pvt->id,
							 at_cmd2str (e->cmd), at_res2str (e->res));

	return 0;
}

/*!
 * \brief Add an item to the back of the queue with pointer data
 * \param pvt -- pvt structure
 * \param cmd -- the command that was sent to generate the response
 * \param res -- the expected response
 * \param num -- numeric data
 */

static int at_fifo_queue_add_num (pvt_t* pvt, at_cmd_t cmd, at_res_t res, int num)
{
	at_queue_t* e;

	if (!(e = ast_calloc (1, sizeof(*e))))
	{
		return -1;
	}

	e->cmd		= cmd;
	e->res		= res;
	e->ptype	= 1;
	e->param.num	= num;

	AST_LIST_INSERT_TAIL (&pvt->at_queue, e, entry);

	ast_debug (4, "[%s] add command '%s' expected response '%s'\n", pvt->id,
							 at_cmd2str (e->cmd), at_res2str (e->res));

	return 0;
}

/*!
 * \brief Remove an item from the front of the queue, and free it
 * \param pvt -- pvt structure
 */

static inline void at_fifo_queue_rem (pvt_t* pvt)
{
	at_queue_t* e = AST_LIST_REMOVE_HEAD (&pvt->at_queue, entry);

	if (e)
	{
		ast_debug (4, "[%s] remove command '%s' expected response '%s'\n", pvt->id,
							at_cmd2str (e->cmd), at_res2str (e->res));

		if (e->ptype == 0 && e->param.data)
		{
			ast_free (e->param.data);
		}

		ast_free (e);
	}
}

/*!
 * \brief Remove all itmes from the queue and free them
 * \param pvt -- pvt structure
 */

static inline void at_fifo_queue_flush (pvt_t* pvt)
{
	at_queue_t* e;

	while ((e = at_fifo_queue_head (pvt)))
	{
		at_fifo_queue_rem (pvt);
	}
}

/*!
 * \brief Get the head of a queue
 * \param pvt -- pvt structure
 * \return a pointer to the head of the given queue
 */

static inline at_queue_t* at_fifo_queue_head (pvt_t* pvt)
{
	return AST_LIST_FIRST (&pvt->at_queue);
}
