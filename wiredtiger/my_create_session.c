
#include <wt_internal.h>
/* 
 * __session_create --
 *	WT_SESSION->create method.
 */ 
/*根据uri和config信息创建一个对应的schema对象
util_create会调用该函数  
//根据uri创建对应的colgroup  file  lsm index table等
*/ //session->create调用，参考example
static int
__session_create(WT_SESSION *wt_session, const char *uri, const char *config)
{
	WT_CONFIG_ITEM cval;
	WT_DECL_RET;
	WT_SESSION_IMPL *session;

	session = (WT_SESSION_IMPL *)wt_session;
	SESSION_API_CALL(session, create, config, cfg);
	WT_UNUSED(cfg);

	/* Disallow objects in the WiredTiger name space. */
	WT_ERR(__wt_str_name_check(session, uri));

	/*
	 * Type configuration only applies to tables, column groups and indexes.
	 * We don't want applications to attempt to layer LSM on top of their
	 * extended data-sources, and the fact we allow LSM as a valid URI is an
	 * invitation to that mistake: nip it in the bud.
	 */
	if (!WT_PREFIX_MATCH(uri, "colgroup:") &&
	    !WT_PREFIX_MATCH(uri, "index:") &&
	    !WT_PREFIX_MATCH(uri, "table:")) { //wt create对应table
		/*
		 * We can't disallow type entirely, a configuration string might
		 * innocently include it, for example, a dump/load pair.  If the
		 * underlying type is "file", it's OK ("file" is the underlying
		 * type for every type); if the URI type prefix and the type are
		 * the same, let it go. 
		 */
		if ((ret =
		    __wt_config_getones(session, config, "type", &cval)) == 0 &&
		    !WT_STRING_MATCH("file", cval.str, cval.len) &&
		    (strncmp(uri, cval.str, cval.len) != 0 ||
		    uri[cval.len] != ':'))
			WT_ERR_MSG(session, EINVAL,
			    "%s: unsupported type configuration", uri);
		WT_ERR_NOTFOUND_OK(ret);
	}
	
    //根据uri创建对应的colgroup  file  lsm index table等
	ret = __wt_session_create(session, uri, config);

err:	if (ret != 0)
		WT_STAT_CONN_INCR(session, session_table_create_fail);
	else
		WT_STAT_CONN_INCR(session, session_table_create_success);
	API_END_RET_NOTFOUND_MAP(session, ret);
}
/*
 * __wt_session_create --
 *	Internal version of WT_SESSION::create.
 */ 
//根据uri创建对应的colgroup  file  lsm index table等
int
__wt_session_create(
    WT_SESSION_IMPL *session, const char *uri, const char *config)
{
	WT_DECL_RET;

	WT_WITH_SCHEMA_LOCK(session,
	    WT_WITH_TABLE_WRITE_LOCK(session,
		ret = __wt_schema_create(session, uri, config)));
	return (ret);
}

/*
 * __wt_schema_create --
 *	Process a WT_SESSION::create operation for all supported types.
 */
//根据uri创建colgroup  file  lsm index table对应的schema
int
__wt_schema_create(
    WT_SESSION_IMPL *session, const char *uri, const char *config)
{
	WT_CONFIG_ITEM cval;
	WT_DATA_SOURCE *dsrc;
	WT_DECL_RET;
	bool exclusive;

	exclusive =
	    __wt_config_getones(session, config, "exclusive", &cval) == 0 &&
	    cval.val != 0;
   
	/*
	 * We track create operations: if we fail in the middle of creating a
	 * complex object, we want to back it all out.
	 */
	WT_RET(__wt_meta_track_on(session));
	if (WT_PREFIX_MATCH(uri, "colgroup:")) //
		ret = __create_colgroup(session, uri, exclusive, config);
	else if (WT_PREFIX_MATCH(uri, "file:")) //对应btree
		ret = __create_file(session, uri, exclusive, config);
	else if (WT_PREFIX_MATCH(uri, "lsm:")) //对应lsmtree
		ret = __wt_lsm_tree_create(session, uri, exclusive, config);
	else if (WT_PREFIX_MATCH(uri, "index:"))//索引index
		ret = __create_index(session, uri, exclusive, config);
	else if (WT_PREFIX_MATCH(uri, "table:")) //wt create          table:access在__wt_schema_colgroup_source中改为file
        // __create_table->__create_colgroup->__wt_schema_colgroup_source中把table:access改为file:access，并走创建file(btree流程)
		ret = __create_table(session, uri, exclusive, config); //创建表
	else if ((dsrc = __wt_schema_get_source(session, uri)) != NULL)
		ret = dsrc->create == NULL ?
		    __wt_object_unsupported(session, uri) :
		    __create_data_source(session, uri, config, dsrc);
	else
		ret = __wt_bad_object_type(session, uri);

	session->dhandle = NULL;
	WT_TRET(__wt_meta_track_off(session, true, ret != 0));

	return (ret);
}

/*
 * __create_table --
 *	Create a table. 创建表对象
 */
static int
__create_table(WT_SESSION_IMPL *session,
    const char *uri, bool exclusive, const char *config)
{
	WT_CONFIG conf;
	WT_CONFIG_ITEM cgkey, cgval, cval;
	WT_DECL_RET;
	WT_TABLE *table;
	
	/*
	{ "table.meta",
	  "app_metadata=,colgroups=,collator=,columns=,key_format=u,"
	  "value_format=u",
	  confchk_table_meta, 6
	},
	*/
	const char *cfg[4] = //table_meta对应的默认配置
	    { WT_CONFIG_BASE(session, table_meta), config, NULL, NULL };
	const char *tablename;
	char *tableconf, *cgname;
	size_t cgsize;
	int ncolgroups;

	cgname = NULL;
	table = NULL;
	tableconf = NULL;

	WT_ASSERT(session, F_ISSET(session, WT_SESSION_LOCKED_TABLE_WRITE));

	tablename = uri;
	WT_PREFIX_SKIP_REQUIRED(session, tablename, "table:"); //剔除前面的table:字符串

    //test .......... uri:table:./yyz-test, tablename:./yyz-test
    //printf("test .......... uri:%s, tablename:%s\r\n", uri, tablename);
	/* Check if the table already exists. */

	/*
[1532331783:898590][5465:0x7f3f1db28740][__wt_metadata_search, 289], WT_SESSION.create: Search: key: table:access, tracking: true, not turtle
[1532331783:898600][5465:0x7f3f1db28740][__wt_cursor_set_keyv, 343], WT_CURSOR.set_key: CALL: WT_CURSOR:set_key
[1532331783:898609][5465:0x7f3f1db28740][__curfile_search, 184], file:WiredTiger.wt, WT_CURSOR.search: CALL: WT_CURSOR:search
[1532331783:898620][5465:0x7f3f1db28740][__curfile_reset, 160], file:WiredTiger.wt, WT_CURSOR.reset: CALL: WT_CURSOR:reset
	*/
    //获取WT_METAFILE_URI WiredTiger version，或者table的元数据存到tableconf
	if ((ret = __wt_metadata_search(
	    session, uri, &tableconf)) != WT_NOTFOUND) { //第一次create table肯定会返回WT_NOTFOUND
		if (exclusive)
			WT_TRET(EEXIST);
		goto err;
	}


//uri:table:access, tableconf:(null)[1532331783:898653][5465:0x7f3f1db28740][__wt_metadata_insert, 179], WT_SESSION.create: Insert: key: table:access, value: app_metadata=,colgroups=,collator=,columns=,key_format=S,value_format=S, tracking: true, not turtle
//printf("uri:%s, tableconf:%s", uri, tableconf);
	WT_ERR(__wt_config_gets(session, cfg, "colgroups", &cval));
	__wt_config_subinit(session, &conf, &cval);//根据cval来构建WT_CONFIG对象
	
	for (ncolgroups = 0;
	    (ret = __wt_config_next(&conf, &cgkey, &cgval)) == 0;
	    ncolgroups++)
		;
	WT_ERR_NOTFOUND_OK(ret); 
    //根据cfg获取tableconf配置信息，也就是把cfg数组中相同的配置做合并，存到tableconf中
	WT_ERR(__wt_config_collapse(session, cfg, &tableconf));
	WT_ERR(__wt_metadata_insert(session, uri, tableconf)); //更新uri元数据信息到元数据文件WiredTiger.wt

	if (ncolgroups == 0) {
		cgsize = strlen("colgroup:") + strlen(tablename) + 1;
		WT_ERR(__wt_calloc_def(session, cgsize, &cgname));
		WT_ERR(__wt_snprintf(cgname, cgsize, "colgroup:%s", tablename)); //table头替换为colgroup头
		WT_ERR(__create_colgroup(session, cgname, exclusive, config));
	}

	/*
	 * Open the table to check that it was setup correctly.  Keep the
	 * handle exclusive until it is released at the end of the call.
	 */
	/* 获取uri对应的table信息，没有则创建 */
	WT_ERR(__wt_schema_get_table_uri(
	    session, uri, true, WT_DHANDLE_EXCLUSIVE, &table));
	
	if (WT_META_TRACKING(session)) {
		WT_WITH_DHANDLE(session, &table->iface,
		    ret = __wt_meta_track_handle_lock(session, true));
		WT_ERR(ret);
		table = NULL;
	}

err:	if (table != NULL)
		WT_TRET(__wt_schema_release_table(session, table));
	__wt_free(session, cgname);
	__wt_free(session, tableconf);
	return (ret);
}

/*
 * __wt_metadata_search --
 *	Return a copied row from the metadata.
 *	The caller is responsible for freeing the allocated memory.
WiredTiger.basecfg存储基本配置信息
WiredTiger.lock用于防止多个进程连接同一个Wiredtiger数据库
table*.wt存储各个tale（数据库中的表）的数据
WiredTiger.wt是特殊的table，用于存储所有其他table的元数据信息
WiredTiger.turtle存储WiredTiger.wt的元数据信息
journal存储Write ahead log
 */
//__wt_turtle_read从WiredTiger.turtle文件中获取WT_METAFILE_URI或者WiredTiger version的元数据，
//__wt_metadata_cursor->cursor->search(cursor))从WiredTiger.wt文件获取key对应的元数据
//获取WT_METAFILE_URI WiredTiger version，或者table的元数据,通过valuep返回
int  //__wt_metadata_search和__wt_metadata_insert配合，一个写，一个查询
__wt_metadata_search(WT_SESSION_IMPL *session, const char *key, char **valuep)
{
	WT_CURSOR *cursor;
	WT_DECL_RET;
	const char *value;

	*valuep = NULL;

    //WT_SESSION.create: Search: key: table:access, tracking: true, not turtle
	__wt_verbose(session, WT_VERB_METADATA,
	    "Search: key: %s, tracking: %s, %s" "turtle",
	    key, WT_META_TRACKING(session) ? "true" : "false",
	    __metadata_turtle(key) ? "" : "not ");

	if (__metadata_turtle(key)) { 
	//file:WiredTiger.wt和WiredTiger version对应的元数据存储在WiredTiger.turtle文件，从WiredTiger.turtle中获取
		/*
		 * The returned value should only be set if ret is non-zero, but
		 * Coverity is convinced otherwise. The code path is used enough
		 * that Coverity complains a lot, add an error check to get some
		 * peace and quiet.
		 */ 
		 /*读取turtle "WiredTiger.turtle"文件,并找到key对应的value值返回， 返回内容填充到valuep中
           WiredTiger.turtle存储WiredTiger.wt的元数据信息，也就是查找是否有WiredTiger.wt配置信息
		 */ 
		//key默认为WT_METAFILE_URI，或者WiredTiger version字符串,因为WT_METAFILE_URI和"WiredTiger version"的元数据在WiredTiger.turtle文件中
		WT_WITH_TURTLE_LOCK(session,
		//__wt_turtle_read从WiredTiger.turtle文件中获取WT_METAFILE_URI或者WiredTiger version的元数据，
		//__wt_metadata_cursor->cursor->search(cursor))从WiredTiger.wt文件获取key对应的元数据
		    ret = __wt_turtle_read(session, key, valuep));
		if (ret != 0)
			__wt_free(session, *valuep);
		return (ret);
	}

    //普通table的元数据存储在WiredTiger.wt，从WiredTiger.wt中获取
	/*
	 * All metadata reads are at read-uncommitted isolation.  That's
	 * because once a schema-level operation completes, subsequent
	 * operations must see the current version of checkpoint metadata, or
	 * they may try to read blocks that may have been freed from a file.
	 * Metadata updates use non-transactional techniques (such as the
	 * schema and metadata locks) to protect access to in-flight updates.
	 */
	//__wt_turtle_read从WiredTiger.turtle文件中获取WT_METAFILE_URI或者WiredTiger version的元数据，
	//__wt_metadata_cursor->cursor->search(cursor))从WiredTiger.wt文件获取key对应的元数据
	//获取一个file:WiredTiger.wt元数据文件对应的cursor，这里面存储有所有table的元数据
	WT_RET(__wt_metadata_cursor(session, &cursor));
	cursor->set_key(cursor, key); //__wt_cursor_set_key
	WT_WITH_TXN_ISOLATION(session, WT_ISO_READ_UNCOMMITTED,
	    ret = cursor->search(cursor)); //__curfile_search
	WT_ERR(ret);

	WT_ERR(cursor->get_value(cursor, &value));
	WT_ERR(__wt_strdup(session, value, valuep));

err:	WT_TRET(__wt_metadata_cursor_release(session, &cursor));

	if (ret != 0)
		__wt_free(session, *valuep);
	return (ret);
}

 
/*
 * __wt_schema_get_table_uri --
 *	Get the table handle for the named table.
 */
//获取uri对应的table handle
int
__wt_schema_get_table_uri(WT_SESSION_IMPL *session,
    const char *uri, bool ok_incomplete, uint32_t flags,
    WT_TABLE **tablep)
{
	WT_DATA_HANDLE *saved_dhandle;
	WT_DECL_RET;
	WT_TABLE *table;

    //临时保存原来的
	saved_dhandle = session->dhandle;

	*tablep = NULL;

    //根据uri获取对应的dhandle,也就是对应的WT_TABLE
	WT_ERR(__wt_session_get_dhandle(session, uri, NULL, NULL, flags));
	table = (WT_TABLE *)session->dhandle; //配合__session_get_dhandle阅读
	if (!ok_incomplete && !table->cg_complete) {
		ret = EINVAL;
		WT_TRET(__wt_session_release_dhandle(session));
		WT_ERR_MSG(session, ret, "'%s' cannot be used "
		    "until all column groups are created",
		    table->iface.name);
	}
	*tablep = table;
   // printf("yang test ......................... __wt_schema_get_table_uri\r\n");
    //恢复
err:	session->dhandle = saved_dhandle;
	return (ret);
}

/*
 * __wt_session_get_dhandle --
 *	Get a data handle for the given name, set session->dhandle.
 * 创建对应的btree也是在这里面
 * table也是根据该函数获取，见__wt_schema_get_table_uri
 * file table有各自的dhandle
 * 把获取的uri和checkpoint对应的dhandle赋值给session->dhandle
 */
//根据uri和checkpoint获取对应的dhandle,如果是table则对应WT_TABLE，如果是file则对应__wt_data_handle，赋值给了session->dhandle
int
__wt_session_get_dhandle(WT_SESSION_IMPL *session,
    const char *uri, const char *checkpoint, const char *cfg[], uint32_t flags)
{
	WT_DATA_HANDLE *dhandle;
	WT_DECL_RET;
	bool is_dead;

	WT_ASSERT(session, !F_ISSET(session, WT_SESSION_NO_DATA_HANDLES));

	for (;;) {
	    //根据uri和checkpoint获取对应的dhandle放入session->dhandle，没有则创建一个dhandle
		WT_RET(__session_get_dhandle(session, uri, checkpoint));
		dhandle = session->dhandle;
		/* Try to lock the handle. */
		WT_RET(__wt_session_lock_dhandle(session, flags, &is_dead));
		if (is_dead)
			continue;

		/* If the handle is open in the mode we want, we're done. */
		if (LF_ISSET(WT_DHANDLE_LOCK_ONLY) ||
		    (F_ISSET(dhandle, WT_DHANDLE_OPEN) &&
		    !LF_ISSET(WT_BTREE_SPECIAL_FLAGS))) //已经打开了该dhandle
			break;

		WT_ASSERT(session, F_ISSET(dhandle, WT_DHANDLE_EXCLUSIVE));

		/*
		 * For now, we need the schema lock and handle list locks to
		 * open a file for real.
		 *
		 * Code needing exclusive access (such as drop or verify)
		 * assumes that it can close all open handles, then open an
		 * exclusive handle on the active tree and no other threads can
		 * reopen handles in the meantime.  A combination of the schema
		 * and handle list locks are used to enforce this.
		 */
		if (!F_ISSET(session, WT_SESSION_LOCKED_SCHEMA)) {
			dhandle->excl_session = NULL;
			dhandle->excl_ref = 0;
			F_CLR(dhandle, WT_DHANDLE_EXCLUSIVE);
			__wt_writeunlock(session, &dhandle->rwlock);

			WT_WITH_SCHEMA_LOCK(session,
			    ret = __wt_session_get_dhandle(
				session, uri, checkpoint, cfg, flags)); //递归调用

			return (ret);
		}

		/* Open the handle. 创建对应的btree或者table 并赋值*/
		if ((ret = __wt_conn_dhandle_open(session, cfg, flags)) == 0 &&
		    LF_ISSET(WT_DHANDLE_EXCLUSIVE)) {
			break;
        }

		/*
		 * If we got the handle exclusive to open it but only want
		 * ordinary access, drop our lock and retry the open.
		 */
		dhandle->excl_session = NULL;
		dhandle->excl_ref = 0;
		F_CLR(dhandle, WT_DHANDLE_EXCLUSIVE);
		__wt_writeunlock(session, &dhandle->rwlock);
		WT_RET(ret);
	}


	WT_ASSERT(session, !F_ISSET(dhandle, WT_DHANDLE_DEAD));
	WT_ASSERT(session, LF_ISSET(WT_DHANDLE_LOCK_ONLY) ||
	    F_ISSET(dhandle, WT_DHANDLE_OPEN));

	WT_ASSERT(session, LF_ISSET(WT_DHANDLE_EXCLUSIVE) ==
	    F_ISSET(dhandle, WT_DHANDLE_EXCLUSIVE) || dhandle->excl_ref > 1);

	return (0);
}

/*
 * __wt_session_get_dhandle --
 *	Get a data handle for the given name, set session->dhandle.
 * 创建对应的btree也是在这里面
 * table也是根据该函数获取，见__wt_schema_get_table_uri
 * file table有各自的dhandle
 * 把获取的uri和checkpoint对应的dhandle赋值给session->dhandle
 */
//根据uri和checkpoint获取对应的dhandle,如果是table则对应WT_TABLE，如果是file则对应__wt_data_handle，赋值给了session->dhandle
int
__wt_session_get_dhandle(WT_SESSION_IMPL *session,
    const char *uri, const char *checkpoint, const char *cfg[], uint32_t flags)
{
	WT_DATA_HANDLE *dhandle;
	WT_DECL_RET;
	bool is_dead;

	WT_ASSERT(session, !F_ISSET(session, WT_SESSION_NO_DATA_HANDLES));

	for (;;) {
	    //根据uri和checkpoint获取对应的dhandle放入session->dhandle，没有则创建一个dhandle
		WT_RET(__session_get_dhandle(session, uri, checkpoint));
		dhandle = session->dhandle;
		/* Try to lock the handle. */
		WT_RET(__wt_session_lock_dhandle(session, flags, &is_dead));
		if (is_dead)
			continue;

		/* If the handle is open in the mode we want, we're done. */
		if (LF_ISSET(WT_DHANDLE_LOCK_ONLY) ||
		    (F_ISSET(dhandle, WT_DHANDLE_OPEN) &&
		    !LF_ISSET(WT_BTREE_SPECIAL_FLAGS))) //已经打开了该dhandle
			break;

		WT_ASSERT(session, F_ISSET(dhandle, WT_DHANDLE_EXCLUSIVE));

		/*
		 * For now, we need the schema lock and handle list locks to
		 * open a file for real.
		 *
		 * Code needing exclusive access (such as drop or verify)
		 * assumes that it can close all open handles, then open an
		 * exclusive handle on the active tree and no other threads can
		 * reopen handles in the meantime.  A combination of the schema
		 * and handle list locks are used to enforce this.
		 */
		if (!F_ISSET(session, WT_SESSION_LOCKED_SCHEMA)) {
			dhandle->excl_session = NULL;
			dhandle->excl_ref = 0;
			F_CLR(dhandle, WT_DHANDLE_EXCLUSIVE);
			__wt_writeunlock(session, &dhandle->rwlock);

			WT_WITH_SCHEMA_LOCK(session,
			    ret = __wt_session_get_dhandle(
				session, uri, checkpoint, cfg, flags)); //递归调用

			return (ret);
		}

		/* Open the handle. 创建对应的btree或者table 并赋值*/
		if ((ret = __wt_conn_dhandle_open(session, cfg, flags)) == 0 &&
		    LF_ISSET(WT_DHANDLE_EXCLUSIVE)) {
			break;
        }

		/*
		 * If we got the handle exclusive to open it but only want
		 * ordinary access, drop our lock and retry the open.
		 */
		dhandle->excl_session = NULL;
		dhandle->excl_ref = 0;
		F_CLR(dhandle, WT_DHANDLE_EXCLUSIVE);
		__wt_writeunlock(session, &dhandle->rwlock);
		WT_RET(ret);
	}


	WT_ASSERT(session, !F_ISSET(dhandle, WT_DHANDLE_DEAD));
	WT_ASSERT(session, LF_ISSET(WT_DHANDLE_LOCK_ONLY) ||
	    F_ISSET(dhandle, WT_DHANDLE_OPEN));

	WT_ASSERT(session, LF_ISSET(WT_DHANDLE_EXCLUSIVE) ==
	    F_ISSET(dhandle, WT_DHANDLE_EXCLUSIVE) || dhandle->excl_ref > 1);

	return (0);
}

/*
 * __session_get_dhandle --
 *	Search for a data handle, first in the session cache, then in the
 *	connection.
 */
//根据uri和checkpoint查找对应的handle，找到直接返回，如果sessin中没找到，
//则通过__session_find_shared_dhandle新建一个handle, 最后通过__session_add_dhandle添加
//table也是根据该函数获取，见__wt_schema_get_table_uri
static int
__session_get_dhandle(
    WT_SESSION_IMPL *session, const char *uri, const char *checkpoint)
{
	WT_DATA_HANDLE_CACHE *dhandle_cache;
	WT_DECL_RET;

    //先从session->dhhash查找
	__session_find_dhandle(session, uri, checkpoint, &dhandle_cache);
	if (dhandle_cache != NULL) { //找到，直接返回  从hash表中找到对应的dhandle，session->dhandle指向这个找到的handle，没找到则创建
	//说明session->dhandle实际上存储的是当前需要用的dhandle，如果是table,则为__wt_table.iface，如果是file则就为__wt_data_handle 
		session->dhandle = dhandle_cache->dhandle;
		return (0);
	}

	/* Sweep the handle list to remove any dead handles. */
	__session_dhandle_sweep(session); //handle清理

	/*
	 * We didn't find a match in the session cache, search the shared
	 * handle list and cache the handle we find.
	 */
	//从sessin对应的conn上面查找dhandle，没有则会新建一个dhandle
	//在从conn->dhhash查找
	WT_RET(__session_find_shared_dhandle(session, uri, checkpoint));

	/*
	 * Fixup the reference count on failure (we incremented the reference
	 * count while holding the handle-list lock).
	 */
	 //添加
	if ((ret = __session_add_dhandle(session)) != 0) {
		WT_DHANDLE_RELEASE(session->dhandle);
		session->dhandle = NULL;
	}

	return (ret);
}

/*
 * __session_find_shared_dhandle --
 *	Search for a data handle in the connection and add it to a session's
 *	cache.  We must increment the handle's reference count while holding
 *	the handle list lock.
 * 从sessin对应的conn上面查找dhandle，如果找不到就调用__wt_conn_dhandle_alloc 创建dhandle，
 */
static int
__session_find_shared_dhandle(
    WT_SESSION_IMPL *session, const char *uri, const char *checkpoint)
{
	WT_DECL_RET;

	WT_WITH_HANDLE_LIST_READ_LOCK(session,
	    if ((ret = __wt_conn_dhandle_find(session, uri, checkpoint)) == 0)
		    WT_DHANDLE_ACQUIRE(session->dhandle));

	if (ret != WT_NOTFOUND)
		return (ret);

    //没找到则创建一个dhandle
	WT_WITH_HANDLE_LIST_WRITE_LOCK(session,
	    if ((ret = __wt_conn_dhandle_alloc(session, uri, checkpoint)) == 0)
		    WT_DHANDLE_ACQUIRE(session->dhandle));

	return (ret);
}

/*
 * __wt_conn_dhandle_alloc --
 *	Allocate a new data handle and return it linked into the connection's
 *	list.
 */ //创建dhandle，并加入到dhash 和dqh中， session->dhandle = dhandle
int
__wt_conn_dhandle_alloc(
    WT_SESSION_IMPL *session, const char *uri, const char *checkpoint)
{
	WT_BTREE *btree;
	WT_DATA_HANDLE *dhandle;
	WT_DECL_RET;
	WT_TABLE *table;
	uint64_t bucket;

	/*
	 * Ensure no one beat us to creating the handle now that we hold the
	 * write lock.
	 */
	//找到直接返回
	if ((ret =
	     __wt_conn_dhandle_find(session, uri, checkpoint)) != WT_NOTFOUND)
		return (ret);

    //这些新建不同文件xx.wt对应的的dhandle会通过下面的WT_CONN_DHANDLE_INSERT放入conn->dhhash
	if (WT_PREFIX_MATCH(uri, "file:")) {
		WT_RET(__wt_calloc_one(session, &dhandle));
		dhandle->type = WT_DHANDLE_TYPE_BTREE;
	} else if (WT_PREFIX_MATCH(uri, "table:")) { //如果是表，则这里创建的是table，table的第一个成员就是dhandle
		WT_RET(__wt_calloc_one(session, &table));
		dhandle = &table->iface;
		dhandle->type = WT_DHANDLE_TYPE_TABLE;
	} else
		return (__wt_illegal_value(session, NULL));

	/* Btree handles keep their data separate from the interface. */
	if (dhandle->type == WT_DHANDLE_TYPE_BTREE) { //对应文件xxx.wt 
	    //这些新建不同文件xx.wt对应的的dhandle(btree与之对应)会通过下面的WT_CONN_DHANDLE_INSERT放入conn->dhhash
	    //从而就可以通过遍历该dhhash找到所有的btree，然后遍历btree->root找到所有的internal page和leaf page
		WT_ERR(__wt_calloc_one(session, &btree));
		dhandle->handle = btree;
		btree->dhandle = dhandle;
	}

	if (strcmp(uri, WT_METAFILE_URI) == 0)
		F_SET(dhandle, WT_DHANDLE_IS_METADATA);

	WT_ERR(__wt_rwlock_init(session, &dhandle->rwlock));
	dhandle->name_hash = __wt_hash_city64(uri, strlen(uri));
	WT_ERR(__wt_strdup(session, uri, &dhandle->name));
	WT_ERR(__wt_strdup(session, checkpoint, &dhandle->checkpoint));

	WT_ERR(__wt_spin_init(
	    session, &dhandle->close_lock, "data handle close"));

	/*
	 * We are holding the data handle list lock, which protects most
	 * threads from seeing the new handle until that lock is released.
	 *
	 * However, the sweep server scans the list of handles without holding
	 * that lock, so we need a write barrier here to ensure the sweep
	 * server doesn't see a partially filled in structure.
	 */
	WT_WRITE_BARRIER();

	/*
	 * Prepend the handle to the connection list, assuming we're likely to
	 * need new files again soon, until they are cached by all sessions.
	 */
	bucket = dhandle->name_hash % WT_HASH_ARRAY_SIZE;
	WT_CONN_DHANDLE_INSERT(S2C(session), dhandle, bucket);

    //新创建的dhandle赋值
	session->dhandle = dhandle;
	return (0);

err:	WT_TRET(__conn_dhandle_destroy(session, dhandle));
	return (ret);
}

/*
 * __wt_conn_dhandle_open --
 *	Open the current data handle.
 * 主要函数为__wt_btree_open，创建对应的btree文件
 */ //获取session->dhandle,如果是table则对应WT_TABLE，如果是file则对应WT_DATA_HANDLE 
int
__wt_conn_dhandle_open( //分table和file两种情况
    WT_SESSION_IMPL *session, const char *cfg[], uint32_t flags)
{
	WT_BTREE *btree;
	WT_DATA_HANDLE *dhandle;
	WT_DECL_RET;

	dhandle = session->dhandle;
	btree = dhandle->handle;

	WT_ASSERT(session,
	    F_ISSET(dhandle, WT_DHANDLE_EXCLUSIVE) &&
	    !LF_ISSET(WT_DHANDLE_LOCK_ONLY));

	WT_ASSERT(session,
	     !F_ISSET(S2C(session), WT_CONN_CLOSING_NO_MORE_OPENS));

	/* Turn off eviction. */
	if (dhandle->type == WT_DHANDLE_TYPE_BTREE)
		WT_RET(__wt_evict_file_exclusive_on(session));

	/*
	 * If the handle is already open, it has to be closed so it can be
	 * reopened with a new configuration.
	 *
	 * This call can return EBUSY if there's an update in the tree that's
	 * not yet globally visible. That's not a problem because it can only
	 * happen when we're switching from a normal handle to a "special" one,
	 * so we're returning EBUSY to an attempt to verify or do other special
	 * operations. The reverse won't happen because when the handle from a
	 * verify or other special operation is closed, there won't be updates
	 * in the tree that can block the close.
	 */
	if (F_ISSET(dhandle, WT_DHANDLE_OPEN))
		WT_ERR(__wt_conn_dhandle_close(session, false, false));

	/* Discard any previous configuration, set up the new configuration. */
	//设置handle配置
	__conn_dhandle_config_clear(session);
	WT_ERR(__conn_dhandle_config_set(session)); //调用__wt_metadata_search获取dhandle->name对应的配置信息

	switch (dhandle->type) {
	case WT_DHANDLE_TYPE_BTREE:
		/* Set any special flags on the btree handle. */
		F_SET(btree, LF_MASK(WT_BTREE_SPECIAL_FLAGS));

		/*
		 * Allocate data-source statistics memory. We don't allocate
		 * that memory when allocating the data handle because not all
		 * data handles need statistics (for example, handles used for
		 * checkpoint locking).  If we are reopening the handle, then
		 * it may already have statistics memory, check to avoid the
		 * leak.
		 */
		if (dhandle->stat_array == NULL)
			WT_ERR(__wt_stat_dsrc_init(session, dhandle));

        /*打开一个btree及其对应的文件*/
		WT_ERR(__wt_btree_open(session, cfg));
		break;
	case WT_DHANDLE_TYPE_TABLE: //table成员赋值
		WT_ERR(__wt_schema_open_table(session, cfg));
		break;
	}

	/*
	 * Bulk handles require true exclusive access, otherwise, handles
	 * marked as exclusive are allowed to be relocked by the same
	 * session.
	 */
	if (F_ISSET(dhandle, WT_DHANDLE_EXCLUSIVE) &&
	    !LF_ISSET(WT_BTREE_BULK)) {
		dhandle->excl_session = session;
		dhandle->excl_ref = 1;
	}
	F_SET(dhandle, WT_DHANDLE_OPEN);

	/*
	 * Checkpoint handles are read-only, so eviction calculations based on
	 * the number of btrees are better to ignore them.
	 */
	if (dhandle->checkpoint == NULL)
		++S2C(session)->open_btree_count;

	if (0) {
err:		if (btree != NULL)
			F_CLR(btree, WT_BTREE_SPECIAL_FLAGS);
	}

	if (dhandle->type == WT_DHANDLE_TYPE_BTREE)
		__wt_evict_file_exclusive_off(session);

	return (ret);
}

/*
 * __wt_schema_open_table --
 *	Open a named table.
 */ //获取对应的table，对table成员赋值
int
__wt_schema_open_table(WT_SESSION_IMPL *session, const char *cfg[])
{
	WT_DECL_RET;

	WT_WITH_TABLE_WRITE_LOCK(session,
	    WT_WITH_TXN_ISOLATION(session, WT_ISO_READ_UNCOMMITTED,
		ret = __schema_open_table(session, cfg)));

	return (ret);
}

/*
 * __schema_open_table --
 *	Open the data handle for a table (internal version).
 */ //table成员赋值
static int
__schema_open_table(WT_SESSION_IMPL *session, const char *cfg[])
{
	WT_CONFIG cparser;
	WT_CONFIG_ITEM ckey, cval;
	WT_DECL_RET;
	WT_TABLE *table;

	/* 例如wtperf默认table配置
    table_config默认配置:
    DEF_OPT_AS_CONFIG_STRING(table_config,
    "key_format=S,value_format=S,type=lsm,exclusive=true,"
    "allocation_size=4kb,internal_page_max=64kb,leaf_page_max=4kb,"
    "split_pct=100",
	*/
	const char **table_cfg;
	const char *tablename;

    //获取该table队员的config
	table = (WT_TABLE *)session->dhandle;
	table_cfg = table->iface.cfg;
	tablename = table->iface.name;

	WT_ASSERT(session, F_ISSET(session, WT_SESSION_LOCKED_TABLE));
	WT_UNUSED(cfg);

	WT_RET(__wt_config_gets(session, table_cfg, "columns", &cval));
	WT_RET(__wt_config_gets(session, table_cfg, "key_format", &cval));
	WT_RET(__wt_strndup(session, cval.str, cval.len, &table->key_format));
	WT_RET(__wt_config_gets(session, table_cfg, "value_format", &cval));
	WT_RET(__wt_strndup(session, cval.str, cval.len, &table->value_format));

	/* Point to some items in the copy to save re-parsing. */
	WT_RET(__wt_config_gets(
	    session, table_cfg, "columns", &table->colconf));

	/*
	 * Count the number of columns: tables are "simple" if the columns
	 * are not named.
	 */
	__wt_config_subinit(session, &cparser, &table->colconf);
	table->is_simple = true;

	//不止一项
	while ((ret = __wt_config_next(&cparser, &ckey, &cval)) == 0)
		table->is_simple = false;
	WT_RET_NOTFOUND_OK(ret);

	/* Check that the columns match the key and value formats. */
	if (!table->is_simple) //不止一项则进行format检查
		WT_RET(__wt_schema_colcheck(session,
		    table->key_format, table->value_format, &table->colconf,
		    &table->nkey_columns, NULL));

	WT_RET(__wt_config_gets(
	    session, table_cfg, "colgroups", &table->cgconf));

	/* Count the number of column groups. */
	__wt_config_subinit(session, &cparser, &table->cgconf);
	table->ncolgroups = 0;
	while ((ret = __wt_config_next(&cparser, &ckey, &cval)) == 0)
		++table->ncolgroups;
	WT_RET_NOTFOUND_OK(ret);

	if (table->ncolgroups > 0 && table->is_simple)
		WT_RET_MSG(session, EINVAL,
		    "%s requires a table with named columns", tablename);

	WT_RET(__wt_calloc_def(session, WT_COLGROUPS(table), &table->cgroups));
	/*打开一个表所有的column group对象*/
	WT_RET(__wt_schema_open_colgroups(session, table));

	return (0);
}

/*
 * __wt_btree_open --
 *	Open a Btree.
 */
//创建btree文件，同时根据checkpoint配置从checkpoint文件中读取镜像信息加载到内存
int
__wt_btree_open(WT_SESSION_IMPL *session, const char *op_cfg[])
{
	WT_BM *bm;
	WT_BTREE *btree; 
	WT_CKPT ckpt;
	WT_CONFIG_ITEM cval;
	WT_DATA_HANDLE *dhandle;
	WT_DECL_RET;
	size_t root_addr_size;
	uint8_t root_addr[WT_BTREE_MAX_ADDR_COOKIE];
	const char *filename;
	bool creation, forced_salvage, readonly;

	btree = S2BT(session);
	dhandle = session->dhandle;

	/*
	 * This may be a re-open, clean up the btree structure.
	 * Clear the fields that don't persist across a re-open.
	 * Clear all flags other than the operation flags (which are set by the
	 * connection handle software that called us).
	 */
	//清楚session对应的btree
	WT_RET(__btree_clear(session));
	memset(btree, 0, WT_BTREE_CLEAR_SIZE);
	F_CLR(btree, ~WT_BTREE_SPECIAL_FLAGS);

	/* Set the data handle first, our called functions reasonably use it. */
	btree->dhandle = dhandle;

	/* Checkpoint files are readonly. */
	/*判断是否是只读属性，如果SESSION已经建立了CHECKPOINT，那么不能对这个BTREE文件进行修改*/
	readonly = dhandle->checkpoint != NULL ||
	    F_ISSET(S2C(session), WT_CONN_READONLY);

	/* Get the checkpoint information for this name/checkpoint pair. */
	WT_CLEAR(ckpt);
	/*获得checkpoint信息*/
	WT_RET(__wt_meta_checkpoint(
	    session, dhandle->name, dhandle->checkpoint, &ckpt));

	/*
	 * Bulk-load is only permitted on newly created files, not any empty
	 * file -- see the checkpoint code for a discussion.
	 */
	creation = ckpt.raw.size == 0; //需要创建checkpoint文件
	if (!creation && F_ISSET(btree, WT_BTREE_BULK))
		WT_ERR_MSG(session, EINVAL,
		    "bulk-load is only supported on newly created objects");

	/* Handle salvage configuration. */
	forced_salvage = false;
	if (F_ISSET(btree, WT_BTREE_SALVAGE)) {
		WT_ERR(__wt_config_gets(session, op_cfg, "force", &cval));
		forced_salvage = cval.val != 0;
	}

	/* Initialize and configure the WT_BTREE structure. */
	//根据ckpt和btree->dhandle->cfg，给session对应的btree赋值
	WT_ERR(__btree_conf(session, &ckpt));

	/* Connect to the underlying block manager. */
	filename = dhandle->name;
	if (!WT_PREFIX_SKIP(filename, "file:"))
		WT_ERR_MSG(session, EINVAL, "expected a 'file:' URI");

    //Open a file. 创建一个btree文件
	WT_ERR(__wt_block_manager_open(session, filename, dhandle->cfg,
	    forced_salvage, readonly, btree->allocsize, &btree->bm)); 
	bm = btree->bm;

	/*
	 * !!!
	 * As part of block-manager configuration, we need to return the maximum
	 * sized address cookie that a block manager will ever return.  There's
	 * a limit of WT_BTREE_MAX_ADDR_COOKIE, but at 255B, it's too large for
	 * a Btree with 512B internal pages.  The default block manager packs
	 * a wt_off_t and 2 uint32_t's into its cookie, so there's no problem
	 * now, but when we create a block manager extension API, we need some
	 * way to consider the block manager's maximum cookie size versus the
	 * minimum Btree internal node size.
	 */
	btree->block_header = bm->block_header(bm);

	/*
	 * Open the specified checkpoint unless it's a special command (special
	 * commands are responsible for loading their own checkpoints, if any).
	 */
	if (!F_ISSET(btree,
	    WT_BTREE_SALVAGE | WT_BTREE_UPGRADE | WT_BTREE_VERIFY)) {
		/*
		 * There are two reasons to load an empty tree rather than a
		 * checkpoint: either there is no checkpoint (the file is
		 * being created), or the load call returns no root page (the
		 * checkpoint is for an empty file).
		 */

		//__bm_checkpoint_load    通过checkpoint文件获取root地址和长度 
		WT_ERR(bm->checkpoint_load(bm, session,
		    ckpt.raw.data, ckpt.raw.size, //ckpt.raw.data, ckpt.raw.size来源见__wt_meta_checkpoint
		    root_addr, &root_addr_size, readonly));
		    
		if (creation || root_addr_size == 0) //没有对应的checkpoint
			WT_ERR(__btree_tree_open_empty(session, creation));
		else {
		    /*从磁盘中读取btree的数据并初始化各个page*/
			WT_ERR(__wt_btree_tree_open(
			    session, root_addr, root_addr_size));

			/*
			 * Rebalance uses the cache, but only wants the root
			 * page, nothing else.
			 */
			if (!F_ISSET(btree, WT_BTREE_REBALANCE)) {
				/* Warm the cache, if possible. */
				/*预热加载数据*/
				WT_WITH_PAGE_INDEX(session,
				    ret = __btree_preload(session));
				WT_ERR(ret);

				/*
				 * Get the last record number in a column-store
				 * file.
				 */
				/*获取列式存储最后的记录序号*/
				if (btree->type != BTREE_ROW)
					WT_ERR(__btree_get_last_recno(session));
			}
		}
	}

	/*
	 * Eviction ignores trees until the handle's open flag is set, configure
	 * eviction before that happens.
	 *
	 * Files that can still be bulk-loaded cannot be evicted.
	 * Permanently cache-resident files can never be evicted.
	 * Special operations don't enable eviction. The underlying commands may
	 * turn on eviction (for example, verify turns on eviction while working
	 * a file to keep from consuming the cache), but it's their decision. If
	 * an underlying command reconfigures eviction, it must either clear the
	 * evict-disabled-open flag or restore the eviction configuration when
	 * finished so that handle close behaves correctly.
	 */
	if (btree->original ||
	    F_ISSET(btree, WT_BTREE_IN_MEMORY | WT_BTREE_REBALANCE |
	    WT_BTREE_SALVAGE | WT_BTREE_UPGRADE | WT_BTREE_VERIFY)) {
		WT_ERR(__wt_evict_file_exclusive_on(session));
		btree->evict_disabled_open = true;
	}

	if (0) {
err:		WT_TRET(__wt_btree_close(session));
	}
	__wt_meta_checkpoint_free(session, &ckpt);

	return (ret);
}
