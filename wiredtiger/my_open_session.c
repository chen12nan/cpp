
// conn_api.c  __conn_open_session  open_session

#include <wt_internal.h>

/*
 * __conn_open_session --
 *	WT_CONNECTION->open_session method.
 */ 
/*
打开一个外部session， 通过wt_sessionp 返回
获取一个session, 通过session_ret返回,同时获取cursor
*/ //从conn->sessions[]获取一个可用session并赋值,同时把session_ret->iface通过wt_sessionp返回
//util_main.c中的main就会调用该函数

static int
__conn_open_session(WT_CONNECTION *wt_conn,
    WT_EVENT_HANDLER *event_handler, const char *config,
    WT_SESSION **wt_sessionp)
{
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET; //#define	WT_DECL_RET	int ret = 0
	WT_SESSION_IMPL *session, *session_ret;

	*wt_sessionp = NULL;
	conn = (WT_CONNECTION_IMPL *)wt_conn;

    //这里面会修改session.dhandle和session.name
    /**
     * #define	CONNECTION_API_CALL(conn, s, n, config, cfg)			\
        s = (conn)->default_session;					\
        API_CALL(s, WT_CONNECTION, n, NULL, config, cfg)

    //对s进行config检查，同时重新赋值dhandle和name
    #define	API_CALL(s, h, n, dh, config, cfg) do {				\
        const char *(cfg)[] =						\
            { WT_CONFIG_BASE(s, h##_##n), config, NULL };		\
        API_SESSION_INIT(s, h, n, dh);					\
        if ((config) != NULL)						\
            WT_ERR(__wt_config_check((s),				\
                WT_CONFIG_REF(session, h##_##n), (config), 0))
     */
	CONNECTION_API_CALL(conn, session, open_session, config, cfg);
	WT_UNUSED(cfg);

	session_ret = NULL;

	//获取一个session, 通过session_ret返回,同时获取cursor赋值给session->meta_cursor
	WT_ERR(__wt_open_session(
	    conn, event_handler, config, true, &session_ret));
	*wt_sessionp = &session_ret->iface;

    //还原dhandle和name
err:	API_END_RET_NOTFOUND_MAP(session, ret);
}

/*
 * __wt_open_session --
 *	Allocate a session handle.
 */
/*创建并打开一个外部使用的session对象,如果open_metadata为ture则获取cursor赋值给session->meta_cursor*/
int
__wt_open_session(WT_CONNECTION_IMPL *conn,
    WT_EVENT_HANDLER *event_handler, const char *config,
    bool open_metadata, WT_SESSION_IMPL **sessionp)
{
	WT_DECL_RET;
	WT_SESSION *wt_session;
	WT_SESSION_IMPL *session;

	*sessionp = NULL;

	/* Acquire a session. */
	//根据conn config 填充session内容,并通过session返回
	//从conn->sessions[]获取一个可用session并赋值
	WT_RET(__open_session(conn, event_handler, config, &session));

	/*
	 * Acquiring the metadata handle requires the schema lock; we've seen
	 * problems in the past where a session has acquired the schema lock
	 * unexpectedly, relatively late in the run, and deadlocked. Be
	 * defensive, get it now.  The metadata file may not exist when the
	 * connection first creates its default session or the shared cache
	 * pool creates its sessions, let our caller decline this work.
	 */

    //#define	F_ISSET(p, mask)	        FLD_ISSET((p)->flags, mask)
	if (open_metadata) {
		WT_ASSERT(session, !F_ISSET(session, WT_SESSION_LOCKED_SCHEMA));
		//获取cursor
		if ((ret = __wt_metadata_cursor(session, NULL)) != 0) {
			wt_session = &session->iface;
			WT_TRET(wt_session->close(wt_session, NULL));
			return (ret);
		}
	}

	*sessionp = session;
	return (0);
}

/*
 * __open_session --  session_api.c:1779
 *	Allocate a session handle.
 */
/*创建并打开一个外部使用的session对象*/
static int
__open_session(WT_CONNECTION_IMPL *conn,
    WT_EVENT_HANDLER *event_handler, const char *config,
    WT_SESSION_IMPL **sessionp)
{
	static const WT_SESSION stds = {
		NULL,
		NULL,
		__session_close,
		__session_reconfigure,
		__wt_session_strerror,
		__session_open_cursor,
		__session_alter,
		__session_create,
		__wt_session_compact,
		__session_drop,
		__session_join,
		__session_log_flush,
		__session_log_printf,
		__session_rebalance,
		__session_rename,
		__session_reset,
		__session_salvage,
		__session_truncate,
		__session_upgrade,
		__session_verify,
		__session_begin_transaction,
		__session_commit_transaction,
		__session_rollback_transaction,
		__session_timestamp_transaction,
		__session_checkpoint,
		__session_snapshot,
		__session_transaction_pinned_range,
		__session_transaction_sync
	}, stds_readonly = { //如果F_ISSET(conn, WT_CONN_READONLY)用这个
		NULL,
		NULL,
		__session_close,
		__session_reconfigure,
		__wt_session_strerror,
		__session_open_cursor,
		__session_alter_readonly,
		__session_create_readonly,
		__wt_session_compact_readonly,
		__session_drop_readonly,
		__session_join,
		__session_log_flush_readonly,
		__session_log_printf_readonly,
		__session_rebalance_readonly,
		__session_rename_readonly,
		__session_reset,
		__session_salvage_readonly,
		__session_truncate_readonly,
		__session_upgrade_readonly,
		__session_verify,
		__session_begin_transaction,
		__session_commit_transaction,
		__session_rollback_transaction,
		__session_timestamp_transaction,
		__session_checkpoint_readonly,
		__session_snapshot,
		__session_transaction_pinned_range,
		__session_transaction_sync_readonly
	};
	WT_DECL_RET;
	WT_SESSION_IMPL *session, *session_ret;
	uint32_t i;

	*sessionp = NULL;

	session = conn->default_session;
	session_ret = NULL;

	__wt_spin_lock(session, &conn->api_lock);

	/*
	 * Make sure we don't try to open a new session after the application
	 * closes the connection.  This is particularly intended to catch
	 * cases where server threads open sessions.
	 */
	WT_ASSERT(session, !F_ISSET(conn, WT_CONN_CLOSING));

	/* Find the first inactive session slot. */
	//从sessions数组中找出一个可用的session
	for (session_ret = conn->sessions,
	    i = 0; i < conn->session_size; ++session_ret, ++i)
		if (!session_ret->active)  //找到可用的一个session
			break;
			
	if (i == conn->session_size)  /*超出connection的sessions slot范围,提示错误*/
		WT_ERR_MSG(session, WT_ERROR,
		    "out of sessions, only configured to support %" PRIu32
		    " sessions (including %d additional internal sessions)",
		    conn->session_size, WT_EXTRA_INTERNAL_SESSIONS);

	/*
	 * If the active session count is increasing, update it.  We don't worry
	 * about correcting the session count on error, as long as we don't mark
	 * this session as active, we'll clean it up on close.
	 */
	if (i >= conn->session_cnt)	/* Defend against off-by-one errors. */
		conn->session_cnt = i + 1;

	session_ret->iface =
	    F_ISSET(conn, WT_CONN_READONLY) ? stds_readonly : stds;
	session_ret->iface.connection = &conn->iface;

	session_ret->name = NULL;
	session_ret->id = i;

	if (WT_SESSION_FIRST_USE(session_ret))
		__wt_random_init(&session_ret->rnd);

	__wt_event_handler_set(session_ret,
	    event_handler == NULL ? session->event_handler : event_handler);

	TAILQ_INIT(&session_ret->cursors);
	TAILQ_INIT(&session_ret->dhandles);
	/*
	 * If we don't have one, allocate the dhandle hash array.
	 * Allocate the table hash array as well.
	 */
	if (session_ret->dhhash == NULL)
		WT_ERR(__wt_calloc(session, WT_HASH_ARRAY_SIZE,
		    sizeof(struct __dhandles_hash), &session_ret->dhhash));
	for (i = 0; i < WT_HASH_ARRAY_SIZE; i++)
		TAILQ_INIT(&session_ret->dhhash[i]);

	/* Initialize transaction support: default to read-committed. */
	session_ret->isolation = WT_ISO_READ_COMMITTED;
	WT_ERR(__wt_txn_init(session, session_ret));

	/*
	 * The session's hazard pointer memory isn't discarded during normal
	 * session close because access to it isn't serialized.  Allocate the
	 * first time we open this session.
	 */
	if (WT_SESSION_FIRST_USE(session_ret)) { 
		WT_ERR(__wt_calloc_def(session,
		    WT_SESSION_INITIAL_HAZARD_SLOTS, &session_ret->hazard));
		session_ret->hazard_size = WT_SESSION_INITIAL_HAZARD_SLOTS;
		session_ret->hazard_inuse = 0;
		session_ret->nhazard = 0;
	}

	/* Cache the offset of this session's statistics bucket. */
	session_ret->stat_bucket = WT_STATS_SLOT_ID(session);

	/*
	 * Configuration: currently, the configuration for open_session is the
	 * same as session.reconfigure, so use that function.
	 */
	if (config != NULL)
		WT_ERR(
		    __session_reconfigure((WT_SESSION *)session_ret, config));

	/*
	 * Publish: make the entry visible to server threads.  There must be a
	 * barrier for two reasons, to ensure structure fields are set before
	 * any other thread will consider the session, and to push the session
	 * count to ensure the eviction thread can't review too few slots.
	 */
	WT_PUBLISH(session_ret->active, 1);

	WT_STATIC_ASSERT(offsetof(WT_SESSION_IMPL, iface) == 0);
	*sessionp = session_ret;

	WT_STAT_CONN_INCR(session, session_open);

err:	__wt_spin_unlock(session, &conn->api_lock);
	return (ret);
}

//meta_table.c
/*
 * __wt_metadata_cursor --
 *	Returns the session's cached metadata cursor, unless it's in use, in
 * which case it opens and returns another metadata cursor.
 */

//__wt_turtle_read从WiredTiger.turtle文件中获取WT_METAFILE_URI或者WiredTiger version的元数据，
//__wt_metadata_cursor->cursor->search(cursor))从WiredTiger.wt文件获取key对应的元数据

/*打开一个meta cursor， 通过cursorp返回并记录到session->meta_cursor */
//获取一个file:WiredTiger.wt元数据文件对应的cursor，这里面存储有所有table的元数据  file对应的cursor接口赋值在__curfile_create
int
__wt_metadata_cursor(WT_SESSION_IMPL *session, WT_CURSOR **cursorp) 
{
	WT_CURSOR *cursor;

	/*
	 * If we don't have a cached metadata cursor, or it's already in use,
	 * we'll need to open a new one.
	 */
	cursor = NULL;
	//没有meta_cursor或者在使用中，则重写打开一个新的meta_cursor
	if (session->meta_cursor == NULL ||
	    F_ISSET(session->meta_cursor, WT_CURSTD_META_INUSE)) { //#define	WT_CURSTD_META_INUSE	0x00100
	    //获取一个file:WiredTiger.wt元数据文件对应的cursor，这里面存储有所有table的元数据
		WT_RET(__wt_metadata_cursor_open(session, NULL, &cursor));
		if (session->meta_cursor == NULL) {
			session->meta_cursor = cursor;
			cursor = NULL;
		}
	}

	/*
	 * If there's no cursor return, we're done, our caller should have just
	 * been triggering the creation of the session's cached cursor. There
	 * should not be an open local cursor in that case, but caution doesn't
	 * cost anything.
	 */
	if (cursorp == NULL)
		return (cursor == NULL ? 0 : cursor->close(cursor));

	/*
	 * If the cached cursor is in use, return the newly opened cursor, else
	 * mark the cached cursor in use and return it.
	 */
	if (F_ISSET(session->meta_cursor, WT_CURSTD_META_INUSE))
		*cursorp = cursor;
	else {
		*cursorp = session->meta_cursor;
		F_SET(session->meta_cursor, WT_CURSTD_META_INUSE);
	}
	return (0);
}

/*
 * __wt_metadata_cursor_open --
 *	Opens a cursor on the metadata.
 */
 //获取一个cursor,通过cursorp返回  
 //获取一个file:WiredTiger.wt元数据文件对应的cursor，这里面存储有所有table的元数据
int
__wt_metadata_cursor_open(
    WT_SESSION_IMPL *session, const char *config, WT_CURSOR **cursorp)
{
	WT_BTREE *btree;
	WT_DECL_RET;

	/*
   	{ "WT_SESSION.open_cursor",
	  "append=false,bulk=false,checkpoint=,checkpoint_wait=true,dump=,"
	  "next_random=false,next_random_sample_size=0,overwrite=true,"
	  "raw=false,readonly=false,skip_sort_check=false,statistics=,"
	  "target=",
	  confchk_WT_SESSION_open_cursor, 13
	},
	*/
	const char *open_cursor_cfg[] = { //WT_CONFIG_ENTRY_WT_SESSION_open_cursor配置
	    WT_CONFIG_BASE(session, WT_SESSION_open_cursor), config, NULL };

	/**
	 * WiredTiger.wt是特殊的table，用于存储所有其他table的元数据信息，如表的位置，表的配置信息等，写入见__wt_metadata_insert
	 * #define	WT_METAFILE_URI		"file:WiredTiger.wt"	 
	 */
	WT_WITHOUT_DHANDLE(session, ret = __wt_open_cursor(
	    session, WT_METAFILE_URI, NULL, open_cursor_cfg, cursorp));
	WT_RET(ret);

	/*
	 * Retrieve the btree from the cursor, rather than the session because
	 * we don't always switch the metadata handle in to the session before
	 * entering this function.
	 */
	btree = ((WT_CURSOR_BTREE *)(*cursorp))->btree;

	/*
	 * Special settings for metadata: skew eviction so metadata almost
	 * always stays in cache and make sure metadata is logged if possible.
	 *
	 * Test before setting so updates can't race in subsequent opens (the
	 * first update is safe because it's single-threaded from
	 * wiredtiger_open).
	 */
#define	WT_EVICT_META_SKEW	10000
	if (btree->evict_priority == 0)
		WT_WITH_BTREE(session, btree,
		    __wt_evict_priority_set(session, WT_EVICT_META_SKEW));
	if (F_ISSET(btree, WT_BTREE_NO_LOGGING))
		F_CLR(btree, WT_BTREE_NO_LOGGING);

	return (0);
}
/*
 * __wt_open_cursor --
 *	Internal version of WT_SESSION::open_cursor.
 根据uri打开一个对应的cursor对象返回
 */
int
__wt_open_cursor(WT_SESSION_IMPL *session,
    const char *uri, WT_CURSOR *owner, const char *cfg[], WT_CURSOR **cursorp)
{
	return (__session_open_cursor_int(session, uri, owner, NULL, cfg,
	    cursorp));
}

/*
 * __session_open_cursor_int --
 *	Internal version of WT_SESSION::open_cursor, with second cursor arg.
 */ // 根据uri打开一个对应的cursor对象 
 /*获取cursor返回，不同的table file lsm等有不同的cursor操作接口 */
static int
__session_open_cursor_int(WT_SESSION_IMPL *session, const char *uri,
    WT_CURSOR *owner, WT_CURSOR *other, const char *cfg[], WT_CURSOR **cursorp)
{
	WT_COLGROUP *colgroup;
	WT_DATA_SOURCE *dsrc;
	WT_DECL_RET;

	*cursorp = NULL;

	/*
	 * Open specific cursor types we know about, or call the generic data
	 * source open function.
	 *
	 * Unwind a set of string comparisons into a switch statement hoping
	 * the compiler can make it fast, but list the common choices first
	 * instead of sorting so if/else patterns are still fast.
	 */
	switch (uri[0]) {
	/*
	 * Common cursor types.
	 */
	case 't':
		if (WT_PREFIX_MATCH(uri, "table:"))//wt create
			WT_RET(__wt_curtable_open(
			    session, uri, owner, cfg, cursorp));
		break;
	case 'c':
		if (WT_PREFIX_MATCH(uri, "colgroup:")) {
			/*
			 * Column groups are a special case: open a cursor on
			 * the underlying data source.
			 */
			WT_RET(__wt_schema_get_colgroup(
			    session, uri, false, NULL, &colgroup));
			WT_RET(__wt_open_cursor(
			    session, colgroup->source, owner, cfg, cursorp));
		} else if (WT_PREFIX_MATCH(uri, "config:"))
			WT_RET(__wt_curconfig_open(
			    session, uri, cfg, cursorp));
		break;
	case 'i':
		if (WT_PREFIX_MATCH(uri, "index:"))
			WT_RET(__wt_curindex_open(
			    session, uri, owner, cfg, cursorp));
		break;
	case 'j':
		if (WT_PREFIX_MATCH(uri, "join:"))
			WT_RET(__wt_curjoin_open(
			    session, uri, owner, cfg, cursorp));
		break;
	case 'l':
		if (WT_PREFIX_MATCH(uri, "lsm:"))
			WT_RET(__wt_clsm_open(
			    session, uri, owner, cfg, cursorp));
		else if (WT_PREFIX_MATCH(uri, "log:"))
			WT_RET(__wt_curlog_open(session, uri, cfg, cursorp));
		break;

	/*
	 * Less common cursor types.
	 */
	case 'f':  //获取uri对应的cursor，通过cursorp返回
		if (WT_PREFIX_MATCH(uri, "file:"))
		    //file对应的cursor接口见__curfile_create
			WT_RET(__wt_curfile_open(
			    session, uri, owner, cfg, cursorp));
		break;
	case 'm':
		if (WT_PREFIX_MATCH(uri, WT_METADATA_URI))
			WT_RET(__wt_curmetadata_open(
			    session, uri, owner, cfg, cursorp));
		break;
	case 'b':
		if (WT_PREFIX_MATCH(uri, "backup:"))
			WT_RET(__wt_curbackup_open(
			    session, uri, cfg, cursorp));
		break;
	case 's':
		if (WT_PREFIX_MATCH(uri, "statistics:"))
			WT_RET(__wt_curstat_open(session, uri, other, cfg,
			    cursorp));
		break;
	default:
		break;
	}

	if (*cursorp == NULL &&
	    (dsrc = __wt_schema_get_source(session, uri)) != NULL)
		WT_RET(dsrc->open_cursor == NULL ?
		    __wt_object_unsupported(session, uri) :
		    __wt_curds_open(session, uri, owner, cfg, dsrc, cursorp));

	if (*cursorp == NULL)
		return (__wt_bad_object_type(session, uri));

	/*
	 * When opening simple tables, the table code calls this function on the
	 * underlying data source, in which case the application's URI has been
	 * copied.
	 */
	if ((*cursorp)->uri == NULL &&
	    (ret = __wt_strdup(session, uri, &(*cursorp)->uri)) != 0) {
		WT_TRET((*cursorp)->close(*cursorp));
		*cursorp = NULL;
	}

	return (ret);
}

/*
 * __wt_curfile_open --
 *	WT_SESSION->open_cursor method for the btree cursor type.
 */
 //获取uri对应的cursor，通过cursorp返回
int
__wt_curfile_open(WT_SESSION_IMPL *session, const char *uri,
    WT_CURSOR *owner, const char *cfg[], WT_CURSOR **cursorp) //file对应的cursor接口见__curfile_create
{
	WT_CONFIG_ITEM cval;
	WT_DECL_RET;
	uint32_t flags;
	bool bitmap, bulk, checkpoint_wait;

	bitmap = bulk = false;
	checkpoint_wait = true;
	flags = 0;

	/*
	 * Decode the bulk configuration settings. In memory databases
	 * ignore bulk load.
	 */
	if (!F_ISSET(S2C(session), WT_CONN_IN_MEMORY)) {
		WT_RET(__wt_config_gets_def(session, cfg, "bulk", 0, &cval));
		if (cval.type == WT_CONFIG_ITEM_BOOL || //配置中带有bulk就会走到这里
		    (cval.type == WT_CONFIG_ITEM_NUM &&
		    (cval.val == 0 || cval.val == 1))) {
			bitmap = false;
			bulk = cval.val != 0;
		} else if (WT_STRING_MATCH("bitmap", cval.str, cval.len))
			bitmap = bulk = true;
			/*
			 * Unordered bulk insert is a special case used
			 * internally by index creation on existing tables. It
			 * doesn't enforce any special semantics at the file
			 * level. It primarily exists to avoid some locking
			 * problems between LSM and index creation.
			 */
		else if (!WT_STRING_MATCH("unordered", cval.str, cval.len))
			WT_RET_MSG(session, EINVAL,
			    "Value for 'bulk' must be a boolean or 'bitmap'");

		if (bulk) {
			WT_RET(__wt_config_gets(session,
			    cfg, "checkpoint_wait", &cval));
			checkpoint_wait = cval.val != 0;
		}
	}

	/* Bulk handles require exclusive access. */
	if (bulk)
		LF_SET(WT_BTREE_BULK | WT_DHANDLE_EXCLUSIVE);

	WT_ASSERT(session, WT_PREFIX_MATCH(uri, "file:"));

	/* Get the handle and lock it while the cursor is using it. */
	/*
	 * If we are opening exclusive and don't want a bulk cursor
	 * open to fail with EBUSY due to a database-wide checkpoint,
	 * get the handle while holding the checkpoint lock.
	 */
	if (LF_ISSET(WT_DHANDLE_EXCLUSIVE) && checkpoint_wait)
		WT_WITH_CHECKPOINT_LOCK(session,
		    ret = __wt_session_get_btree_ckpt(
		    session, uri, cfg, flags));
	else
	    //获取的uri和checkpoint对应的dhandle赋值给session->dhandle
		ret = __wt_session_get_btree_ckpt(
		    session, uri, cfg, flags);
	WT_RET(ret);

	WT_ERR(__curfile_create(session, owner, cfg, bulk, bitmap, cursorp));

	return (0);

err:	/* If the cursor could not be opened, release the handle. */
	WT_TRET(__wt_session_release_dhandle(session));
	return (ret);
}
/*
 * __curfile_create --
 *	Open a cursor for a given btree handle.
 */ 
/*创建一个btree cursor*/
/*对cursor进行初始化 通过cursorp返回*/
static int
__curfile_create(WT_SESSION_IMPL *session,
    WT_CURSOR *owner, const char *cfg[], bool bulk, bool bitmap,
    WT_CURSOR **cursorp)
{
	WT_CURSOR_STATIC_INIT(iface,
	    __wt_cursor_get_key,		/* get-key */
	    __wt_cursor_get_value,		/* get-value */
	    __wt_cursor_set_key,		/* set-key */
	    __wt_cursor_set_value,		/* set-value */
	    __curfile_compare,			/* compare */
	    __curfile_equals,			/* equals */
	    __curfile_next,			/* next */
	    __curfile_prev,			/* prev */
	    __curfile_reset,			/* reset */
	    __curfile_search,			/* search */
	    __curfile_search_near,		/* search-near */
	    __curfile_insert,			/* insert */
	    __wt_cursor_modify_notsup,		/* modify */
	    __curfile_update,			/* update */
	    __curfile_remove,			/* remove */
	    __curfile_reserve,			/* reserve */
	    __wt_cursor_reconfigure,		/* reconfigure */
	    __curfile_close);			/* close */
	WT_BTREE *btree;
	WT_CONFIG_ITEM cval;
	WT_CURSOR *cursor;
	WT_CURSOR_BTREE *cbt;
	WT_CURSOR_BULK *cbulk;
	WT_DECL_RET;
	size_t csize;

	WT_STATIC_ASSERT(offsetof(WT_CURSOR_BTREE, iface) == 0);

	cbt = NULL;

	btree = S2BT(session);
	WT_ASSERT(session, btree != NULL);

	csize = bulk ? sizeof(WT_CURSOR_BULK) : sizeof(WT_CURSOR_BTREE);
	WT_RET(__wt_calloc(session, 1, csize, &cbt));

	cursor = &cbt->iface;
	*cursor = iface;
	cursor->session = &session->iface;
	cursor->internal_uri = btree->dhandle->name;
	cursor->key_format = btree->key_format;
	cursor->value_format = btree->value_format;
	cbt->btree = btree;

	/*
	 * Increment the data-source's in-use counter; done now because closing
	 * the cursor will decrement it, and all failure paths from here close
	 * the cursor.
	 */
	__wt_cursor_dhandle_incr_use(session);

	if (session->dhandle->checkpoint != NULL)
		F_SET(cbt, WT_CBT_NO_TXN);

	if (bulk) { 
	//bulk功能生效在这里，实际上是为了减少btree insert写入时候的查找，但该功能前提是所有insert的key在插入的时候已经是有序的
		F_SET(cursor, WT_CURSTD_BULK);

		cbulk = (WT_CURSOR_BULK *)cbt;

		/* Optionally skip the validation of each bulk-loaded key. */
		WT_ERR(__wt_config_gets_def(
		    session, cfg, "skip_sort_check", 0, &cval));
		WT_ERR(__wt_curbulk_init(
		    session, cbulk, bitmap, cval.val == 0 ? 0 : 1));
	}

	/*
	 * Random retrieval, row-store only.
	 * Random retrieval cursors support a limited set of methods.
	 */
	WT_ERR(__wt_config_gets_def(session, cfg, "next_random", 0, &cval));
	if (cval.val != 0) {
		if (WT_CURSOR_RECNO(cursor))
			WT_ERR_MSG(session, ENOTSUP,
			    "next_random configuration not supported for "
			    "column-store objects");

		__wt_cursor_set_notsup(cursor);
		cursor->next = __wt_curfile_next_random;
		cursor->reset = __curfile_reset;

		WT_ERR(__wt_config_gets_def(
		    session, cfg, "next_random_sample_size", 0, &cval));
		if (cval.val != 0)
			cbt->next_random_sample_size = (u_int)cval.val;
	}

	/* Underlying btree initialization. */
	__wt_btcur_open(cbt);

	/*
	 * WT_CURSOR.modify supported on 'u' value formats, but the fast-path
	 * through the btree code requires log file format changes, it's not
	 * available in all versions.
	 */
	if (WT_STREQ(cursor->value_format, "u") &&
	    S2C(session)->compat_major >= WT_LOG_V2)
		cursor->modify = __curfile_modify;

	WT_ERR(__wt_cursor_init(
	    cursor, cursor->internal_uri, owner, cfg, cursorp));

	WT_STAT_CONN_INCR(session, cursor_create);
	WT_STAT_DATA_INCR(session, cursor_create);

	if (0) {
err:		/*
		 * Our caller expects to release the data handle if we fail.
		 * Disconnect it from the cursor before closing.
		 */
		if (session->dhandle != NULL)
			__wt_cursor_dhandle_decr_use(session);
		cbt->btree = NULL;
		WT_TRET(__curfile_close(cursor));
		*cursorp = NULL;
	}

	return (ret);
}