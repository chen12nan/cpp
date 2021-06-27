
#include <wt_internal.h>
/* 
 * __session_create --
 *	WT_SESSION->create method.
 */ 
/*����uri��config��Ϣ����һ����Ӧ��schema����
util_create����øú���  
//����uri������Ӧ��colgroup  file  lsm index table��
*/ //session->create���ã��ο�example
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
	    !WT_PREFIX_MATCH(uri, "table:")) { //wt create��Ӧtable
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
	
    //����uri������Ӧ��colgroup  file  lsm index table��
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
//����uri������Ӧ��colgroup  file  lsm index table��
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
//����uri����colgroup  file  lsm index table��Ӧ��schema
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
	else if (WT_PREFIX_MATCH(uri, "file:")) //��Ӧbtree
		ret = __create_file(session, uri, exclusive, config);
	else if (WT_PREFIX_MATCH(uri, "lsm:")) //��Ӧlsmtree
		ret = __wt_lsm_tree_create(session, uri, exclusive, config);
	else if (WT_PREFIX_MATCH(uri, "index:"))//����index
		ret = __create_index(session, uri, exclusive, config);
	else if (WT_PREFIX_MATCH(uri, "table:")) //wt create          table:access��__wt_schema_colgroup_source�и�Ϊfile
        // __create_table->__create_colgroup->__wt_schema_colgroup_source�а�table:access��Ϊfile:access�����ߴ���file(btree����)
		ret = __create_table(session, uri, exclusive, config); //������
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
 *	Create a table. ���������
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
	const char *cfg[4] = //table_meta��Ӧ��Ĭ������
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
	WT_PREFIX_SKIP_REQUIRED(session, tablename, "table:"); //�޳�ǰ���table:�ַ���

    //test .......... uri:table:./yyz-test, tablename:./yyz-test
    //printf("test .......... uri:%s, tablename:%s\r\n", uri, tablename);
	/* Check if the table already exists. */

	/*
[1532331783:898590][5465:0x7f3f1db28740][__wt_metadata_search, 289], WT_SESSION.create: Search: key: table:access, tracking: true, not turtle
[1532331783:898600][5465:0x7f3f1db28740][__wt_cursor_set_keyv, 343], WT_CURSOR.set_key: CALL: WT_CURSOR:set_key
[1532331783:898609][5465:0x7f3f1db28740][__curfile_search, 184], file:WiredTiger.wt, WT_CURSOR.search: CALL: WT_CURSOR:search
[1532331783:898620][5465:0x7f3f1db28740][__curfile_reset, 160], file:WiredTiger.wt, WT_CURSOR.reset: CALL: WT_CURSOR:reset
	*/
    //��ȡWT_METAFILE_URI WiredTiger version������table��Ԫ���ݴ浽tableconf
	if ((ret = __wt_metadata_search(
	    session, uri, &tableconf)) != WT_NOTFOUND) { //��һ��create table�϶��᷵��WT_NOTFOUND
		if (exclusive)
			WT_TRET(EEXIST);
		goto err;
	}


//uri:table:access, tableconf:(null)[1532331783:898653][5465:0x7f3f1db28740][__wt_metadata_insert, 179], WT_SESSION.create: Insert: key: table:access, value: app_metadata=,colgroups=,collator=,columns=,key_format=S,value_format=S, tracking: true, not turtle
//printf("uri:%s, tableconf:%s", uri, tableconf);
	WT_ERR(__wt_config_gets(session, cfg, "colgroups", &cval));
	__wt_config_subinit(session, &conf, &cval);//����cval������WT_CONFIG����
	
	for (ncolgroups = 0;
	    (ret = __wt_config_next(&conf, &cgkey, &cgval)) == 0;
	    ncolgroups++)
		;
	WT_ERR_NOTFOUND_OK(ret); 
    //����cfg��ȡtableconf������Ϣ��Ҳ���ǰ�cfg��������ͬ���������ϲ����浽tableconf��
	WT_ERR(__wt_config_collapse(session, cfg, &tableconf));
	WT_ERR(__wt_metadata_insert(session, uri, tableconf)); //����uriԪ������Ϣ��Ԫ�����ļ�WiredTiger.wt

	if (ncolgroups == 0) {
		cgsize = strlen("colgroup:") + strlen(tablename) + 1;
		WT_ERR(__wt_calloc_def(session, cgsize, &cgname));
		WT_ERR(__wt_snprintf(cgname, cgsize, "colgroup:%s", tablename)); //tableͷ�滻Ϊcolgroupͷ
		WT_ERR(__create_colgroup(session, cgname, exclusive, config));
	}

	/*
	 * Open the table to check that it was setup correctly.  Keep the
	 * handle exclusive until it is released at the end of the call.
	 */
	/* ��ȡuri��Ӧ��table��Ϣ��û���򴴽� */
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
WiredTiger.basecfg�洢����������Ϣ
WiredTiger.lock���ڷ�ֹ�����������ͬһ��Wiredtiger���ݿ�
table*.wt�洢����tale�����ݿ��еı�������
WiredTiger.wt�������table�����ڴ洢��������table��Ԫ������Ϣ
WiredTiger.turtle�洢WiredTiger.wt��Ԫ������Ϣ
journal�洢Write ahead log
 */
//__wt_turtle_read��WiredTiger.turtle�ļ��л�ȡWT_METAFILE_URI����WiredTiger version��Ԫ���ݣ�
//__wt_metadata_cursor->cursor->search(cursor))��WiredTiger.wt�ļ���ȡkey��Ӧ��Ԫ����
//��ȡWT_METAFILE_URI WiredTiger version������table��Ԫ����,ͨ��valuep����
int  //__wt_metadata_search��__wt_metadata_insert��ϣ�һ��д��һ����ѯ
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
	//file:WiredTiger.wt��WiredTiger version��Ӧ��Ԫ���ݴ洢��WiredTiger.turtle�ļ�����WiredTiger.turtle�л�ȡ
		/*
		 * The returned value should only be set if ret is non-zero, but
		 * Coverity is convinced otherwise. The code path is used enough
		 * that Coverity complains a lot, add an error check to get some
		 * peace and quiet.
		 */ 
		 /*��ȡturtle "WiredTiger.turtle"�ļ�,���ҵ�key��Ӧ��valueֵ���أ� ����������䵽valuep��
           WiredTiger.turtle�洢WiredTiger.wt��Ԫ������Ϣ��Ҳ���ǲ����Ƿ���WiredTiger.wt������Ϣ
		 */ 
		//keyĬ��ΪWT_METAFILE_URI������WiredTiger version�ַ���,��ΪWT_METAFILE_URI��"WiredTiger version"��Ԫ������WiredTiger.turtle�ļ���
		WT_WITH_TURTLE_LOCK(session,
		//__wt_turtle_read��WiredTiger.turtle�ļ��л�ȡWT_METAFILE_URI����WiredTiger version��Ԫ���ݣ�
		//__wt_metadata_cursor->cursor->search(cursor))��WiredTiger.wt�ļ���ȡkey��Ӧ��Ԫ����
		    ret = __wt_turtle_read(session, key, valuep));
		if (ret != 0)
			__wt_free(session, *valuep);
		return (ret);
	}

    //��ͨtable��Ԫ���ݴ洢��WiredTiger.wt����WiredTiger.wt�л�ȡ
	/*
	 * All metadata reads are at read-uncommitted isolation.  That's
	 * because once a schema-level operation completes, subsequent
	 * operations must see the current version of checkpoint metadata, or
	 * they may try to read blocks that may have been freed from a file.
	 * Metadata updates use non-transactional techniques (such as the
	 * schema and metadata locks) to protect access to in-flight updates.
	 */
	//__wt_turtle_read��WiredTiger.turtle�ļ��л�ȡWT_METAFILE_URI����WiredTiger version��Ԫ���ݣ�
	//__wt_metadata_cursor->cursor->search(cursor))��WiredTiger.wt�ļ���ȡkey��Ӧ��Ԫ����
	//��ȡһ��file:WiredTiger.wtԪ�����ļ���Ӧ��cursor��������洢������table��Ԫ����
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
//��ȡuri��Ӧ��table handle
int
__wt_schema_get_table_uri(WT_SESSION_IMPL *session,
    const char *uri, bool ok_incomplete, uint32_t flags,
    WT_TABLE **tablep)
{
	WT_DATA_HANDLE *saved_dhandle;
	WT_DECL_RET;
	WT_TABLE *table;

    //��ʱ����ԭ����
	saved_dhandle = session->dhandle;

	*tablep = NULL;

    //����uri��ȡ��Ӧ��dhandle,Ҳ���Ƕ�Ӧ��WT_TABLE
	WT_ERR(__wt_session_get_dhandle(session, uri, NULL, NULL, flags));
	table = (WT_TABLE *)session->dhandle; //���__session_get_dhandle�Ķ�
	if (!ok_incomplete && !table->cg_complete) {
		ret = EINVAL;
		WT_TRET(__wt_session_release_dhandle(session));
		WT_ERR_MSG(session, ret, "'%s' cannot be used "
		    "until all column groups are created",
		    table->iface.name);
	}
	*tablep = table;
   // printf("yang test ......................... __wt_schema_get_table_uri\r\n");
    //�ָ�
err:	session->dhandle = saved_dhandle;
	return (ret);
}

/*
 * __wt_session_get_dhandle --
 *	Get a data handle for the given name, set session->dhandle.
 * ������Ӧ��btreeҲ����������
 * tableҲ�Ǹ��ݸú�����ȡ����__wt_schema_get_table_uri
 * file table�и��Ե�dhandle
 * �ѻ�ȡ��uri��checkpoint��Ӧ��dhandle��ֵ��session->dhandle
 */
//����uri��checkpoint��ȡ��Ӧ��dhandle,�����table���ӦWT_TABLE�������file���Ӧ__wt_data_handle����ֵ����session->dhandle
int
__wt_session_get_dhandle(WT_SESSION_IMPL *session,
    const char *uri, const char *checkpoint, const char *cfg[], uint32_t flags)
{
	WT_DATA_HANDLE *dhandle;
	WT_DECL_RET;
	bool is_dead;

	WT_ASSERT(session, !F_ISSET(session, WT_SESSION_NO_DATA_HANDLES));

	for (;;) {
	    //����uri��checkpoint��ȡ��Ӧ��dhandle����session->dhandle��û���򴴽�һ��dhandle
		WT_RET(__session_get_dhandle(session, uri, checkpoint));
		dhandle = session->dhandle;
		/* Try to lock the handle. */
		WT_RET(__wt_session_lock_dhandle(session, flags, &is_dead));
		if (is_dead)
			continue;

		/* If the handle is open in the mode we want, we're done. */
		if (LF_ISSET(WT_DHANDLE_LOCK_ONLY) ||
		    (F_ISSET(dhandle, WT_DHANDLE_OPEN) &&
		    !LF_ISSET(WT_BTREE_SPECIAL_FLAGS))) //�Ѿ����˸�dhandle
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
				session, uri, checkpoint, cfg, flags)); //�ݹ����

			return (ret);
		}

		/* Open the handle. ������Ӧ��btree����table ����ֵ*/
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
 * ������Ӧ��btreeҲ����������
 * tableҲ�Ǹ��ݸú�����ȡ����__wt_schema_get_table_uri
 * file table�и��Ե�dhandle
 * �ѻ�ȡ��uri��checkpoint��Ӧ��dhandle��ֵ��session->dhandle
 */
//����uri��checkpoint��ȡ��Ӧ��dhandle,�����table���ӦWT_TABLE�������file���Ӧ__wt_data_handle����ֵ����session->dhandle
int
__wt_session_get_dhandle(WT_SESSION_IMPL *session,
    const char *uri, const char *checkpoint, const char *cfg[], uint32_t flags)
{
	WT_DATA_HANDLE *dhandle;
	WT_DECL_RET;
	bool is_dead;

	WT_ASSERT(session, !F_ISSET(session, WT_SESSION_NO_DATA_HANDLES));

	for (;;) {
	    //����uri��checkpoint��ȡ��Ӧ��dhandle����session->dhandle��û���򴴽�һ��dhandle
		WT_RET(__session_get_dhandle(session, uri, checkpoint));
		dhandle = session->dhandle;
		/* Try to lock the handle. */
		WT_RET(__wt_session_lock_dhandle(session, flags, &is_dead));
		if (is_dead)
			continue;

		/* If the handle is open in the mode we want, we're done. */
		if (LF_ISSET(WT_DHANDLE_LOCK_ONLY) ||
		    (F_ISSET(dhandle, WT_DHANDLE_OPEN) &&
		    !LF_ISSET(WT_BTREE_SPECIAL_FLAGS))) //�Ѿ����˸�dhandle
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
				session, uri, checkpoint, cfg, flags)); //�ݹ����

			return (ret);
		}

		/* Open the handle. ������Ӧ��btree����table ����ֵ*/
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
//����uri��checkpoint���Ҷ�Ӧ��handle���ҵ�ֱ�ӷ��أ����sessin��û�ҵ���
//��ͨ��__session_find_shared_dhandle�½�һ��handle, ���ͨ��__session_add_dhandle���
//tableҲ�Ǹ��ݸú�����ȡ����__wt_schema_get_table_uri
static int
__session_get_dhandle(
    WT_SESSION_IMPL *session, const char *uri, const char *checkpoint)
{
	WT_DATA_HANDLE_CACHE *dhandle_cache;
	WT_DECL_RET;

    //�ȴ�session->dhhash����
	__session_find_dhandle(session, uri, checkpoint, &dhandle_cache);
	if (dhandle_cache != NULL) { //�ҵ���ֱ�ӷ���  ��hash�����ҵ���Ӧ��dhandle��session->dhandleָ������ҵ���handle��û�ҵ��򴴽�
	//˵��session->dhandleʵ���ϴ洢���ǵ�ǰ��Ҫ�õ�dhandle�������table,��Ϊ__wt_table.iface�������file���Ϊ__wt_data_handle 
		session->dhandle = dhandle_cache->dhandle;
		return (0);
	}

	/* Sweep the handle list to remove any dead handles. */
	__session_dhandle_sweep(session); //handle����

	/*
	 * We didn't find a match in the session cache, search the shared
	 * handle list and cache the handle we find.
	 */
	//��sessin��Ӧ��conn�������dhandle��û������½�һ��dhandle
	//�ڴ�conn->dhhash����
	WT_RET(__session_find_shared_dhandle(session, uri, checkpoint));

	/*
	 * Fixup the reference count on failure (we incremented the reference
	 * count while holding the handle-list lock).
	 */
	 //���
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
 * ��sessin��Ӧ��conn�������dhandle������Ҳ����͵���__wt_conn_dhandle_alloc ����dhandle��
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

    //û�ҵ��򴴽�һ��dhandle
	WT_WITH_HANDLE_LIST_WRITE_LOCK(session,
	    if ((ret = __wt_conn_dhandle_alloc(session, uri, checkpoint)) == 0)
		    WT_DHANDLE_ACQUIRE(session->dhandle));

	return (ret);
}

/*
 * __wt_conn_dhandle_alloc --
 *	Allocate a new data handle and return it linked into the connection's
 *	list.
 */ //����dhandle�������뵽dhash ��dqh�У� session->dhandle = dhandle
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
	//�ҵ�ֱ�ӷ���
	if ((ret =
	     __wt_conn_dhandle_find(session, uri, checkpoint)) != WT_NOTFOUND)
		return (ret);

    //��Щ�½���ͬ�ļ�xx.wt��Ӧ�ĵ�dhandle��ͨ�������WT_CONN_DHANDLE_INSERT����conn->dhhash
	if (WT_PREFIX_MATCH(uri, "file:")) {
		WT_RET(__wt_calloc_one(session, &dhandle));
		dhandle->type = WT_DHANDLE_TYPE_BTREE;
	} else if (WT_PREFIX_MATCH(uri, "table:")) { //����Ǳ������ﴴ������table��table�ĵ�һ����Ա����dhandle
		WT_RET(__wt_calloc_one(session, &table));
		dhandle = &table->iface;
		dhandle->type = WT_DHANDLE_TYPE_TABLE;
	} else
		return (__wt_illegal_value(session, NULL));

	/* Btree handles keep their data separate from the interface. */
	if (dhandle->type == WT_DHANDLE_TYPE_BTREE) { //��Ӧ�ļ�xxx.wt 
	    //��Щ�½���ͬ�ļ�xx.wt��Ӧ�ĵ�dhandle(btree��֮��Ӧ)��ͨ�������WT_CONN_DHANDLE_INSERT����conn->dhhash
	    //�Ӷ��Ϳ���ͨ��������dhhash�ҵ����е�btree��Ȼ�����btree->root�ҵ����е�internal page��leaf page
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

    //�´�����dhandle��ֵ
	session->dhandle = dhandle;
	return (0);

err:	WT_TRET(__conn_dhandle_destroy(session, dhandle));
	return (ret);
}

/*
 * __wt_conn_dhandle_open --
 *	Open the current data handle.
 * ��Ҫ����Ϊ__wt_btree_open��������Ӧ��btree�ļ�
 */ //��ȡsession->dhandle,�����table���ӦWT_TABLE�������file���ӦWT_DATA_HANDLE 
int
__wt_conn_dhandle_open( //��table��file�������
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
	//����handle����
	__conn_dhandle_config_clear(session);
	WT_ERR(__conn_dhandle_config_set(session)); //����__wt_metadata_search��ȡdhandle->name��Ӧ��������Ϣ

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

        /*��һ��btree�����Ӧ���ļ�*/
		WT_ERR(__wt_btree_open(session, cfg));
		break;
	case WT_DHANDLE_TYPE_TABLE: //table��Ա��ֵ
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
 */ //��ȡ��Ӧ��table����table��Ա��ֵ
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
 */ //table��Ա��ֵ
static int
__schema_open_table(WT_SESSION_IMPL *session, const char *cfg[])
{
	WT_CONFIG cparser;
	WT_CONFIG_ITEM ckey, cval;
	WT_DECL_RET;
	WT_TABLE *table;

	/* ����wtperfĬ��table����
    table_configĬ������:
    DEF_OPT_AS_CONFIG_STRING(table_config,
    "key_format=S,value_format=S,type=lsm,exclusive=true,"
    "allocation_size=4kb,internal_page_max=64kb,leaf_page_max=4kb,"
    "split_pct=100",
	*/
	const char **table_cfg;
	const char *tablename;

    //��ȡ��table��Ա��config
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

	//��ֹһ��
	while ((ret = __wt_config_next(&cparser, &ckey, &cval)) == 0)
		table->is_simple = false;
	WT_RET_NOTFOUND_OK(ret);

	/* Check that the columns match the key and value formats. */
	if (!table->is_simple) //��ֹһ�������format���
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
	/*��һ�������е�column group����*/
	WT_RET(__wt_schema_open_colgroups(session, table));

	return (0);
}

/*
 * __wt_btree_open --
 *	Open a Btree.
 */
//����btree�ļ���ͬʱ����checkpoint���ô�checkpoint�ļ��ж�ȡ������Ϣ���ص��ڴ�
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
	//���session��Ӧ��btree
	WT_RET(__btree_clear(session));
	memset(btree, 0, WT_BTREE_CLEAR_SIZE);
	F_CLR(btree, ~WT_BTREE_SPECIAL_FLAGS);

	/* Set the data handle first, our called functions reasonably use it. */
	btree->dhandle = dhandle;

	/* Checkpoint files are readonly. */
	/*�ж��Ƿ���ֻ�����ԣ����SESSION�Ѿ�������CHECKPOINT����ô���ܶ����BTREE�ļ������޸�*/
	readonly = dhandle->checkpoint != NULL ||
	    F_ISSET(S2C(session), WT_CONN_READONLY);

	/* Get the checkpoint information for this name/checkpoint pair. */
	WT_CLEAR(ckpt);
	/*���checkpoint��Ϣ*/
	WT_RET(__wt_meta_checkpoint(
	    session, dhandle->name, dhandle->checkpoint, &ckpt));

	/*
	 * Bulk-load is only permitted on newly created files, not any empty
	 * file -- see the checkpoint code for a discussion.
	 */
	creation = ckpt.raw.size == 0; //��Ҫ����checkpoint�ļ�
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
	//����ckpt��btree->dhandle->cfg����session��Ӧ��btree��ֵ
	WT_ERR(__btree_conf(session, &ckpt));

	/* Connect to the underlying block manager. */
	filename = dhandle->name;
	if (!WT_PREFIX_SKIP(filename, "file:"))
		WT_ERR_MSG(session, EINVAL, "expected a 'file:' URI");

    //Open a file. ����һ��btree�ļ�
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

		//__bm_checkpoint_load    ͨ��checkpoint�ļ���ȡroot��ַ�ͳ��� 
		WT_ERR(bm->checkpoint_load(bm, session,
		    ckpt.raw.data, ckpt.raw.size, //ckpt.raw.data, ckpt.raw.size��Դ��__wt_meta_checkpoint
		    root_addr, &root_addr_size, readonly));
		    
		if (creation || root_addr_size == 0) //û�ж�Ӧ��checkpoint
			WT_ERR(__btree_tree_open_empty(session, creation));
		else {
		    /*�Ӵ����ж�ȡbtree�����ݲ���ʼ������page*/
			WT_ERR(__wt_btree_tree_open(
			    session, root_addr, root_addr_size));

			/*
			 * Rebalance uses the cache, but only wants the root
			 * page, nothing else.
			 */
			if (!F_ISSET(btree, WT_BTREE_REBALANCE)) {
				/* Warm the cache, if possible. */
				/*Ԥ�ȼ�������*/
				WT_WITH_PAGE_INDEX(session,
				    ret = __btree_preload(session));
				WT_ERR(ret);

				/*
				 * Get the last record number in a column-store
				 * file.
				 */
				/*��ȡ��ʽ�洢���ļ�¼���*/
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
