
#include <wt_internal.h>


/*
 * __session_open_cursor --
 *	WT_SESSION->open_cursor method.
 */ //获取一个cursor通过cursorp返回
static int
__session_open_cursor(WT_SESSION *wt_session,
    const char *uri, WT_CURSOR *to_dup, const char *config, WT_CURSOR **cursorp)
{
	WT_CURSOR *cursor;
	WT_DECL_RET;
	WT_SESSION_IMPL *session;
	bool statjoin;

	cursor = *cursorp = NULL;

	session = (WT_SESSION_IMPL *)wt_session;
	SESSION_API_CALL(session, open_cursor, config, cfg);

	statjoin = (to_dup != NULL && uri != NULL &&
	    WT_STREQ(uri, "statistics:join"));
	if ((to_dup == NULL && uri == NULL) ||
	    (to_dup != NULL && uri != NULL && !statjoin))
		WT_ERR_MSG(session, EINVAL,
		    "should be passed either a URI or a cursor to duplicate, "
		    "but not both");

	if (to_dup != NULL && !statjoin) {
		uri = to_dup->uri;
		if (!WT_PREFIX_MATCH(uri, "colgroup:") &&
		    !WT_PREFIX_MATCH(uri, "index:") &&
		    !WT_PREFIX_MATCH(uri, "file:") &&
		    !WT_PREFIX_MATCH(uri, "lsm:") &&
		    !WT_PREFIX_MATCH(uri, WT_METADATA_URI) &&
		    !WT_PREFIX_MATCH(uri, "table:") &&
		    __wt_schema_get_source(session, uri) == NULL)
			WT_ERR(__wt_bad_object_type(session, uri));
	}

	WT_ERR(__session_open_cursor_int(session, uri, NULL,
	    statjoin ? to_dup : NULL, cfg, &cursor));
	if (to_dup != NULL && !statjoin)
		WT_ERR(__wt_cursor_dup_position(to_dup, cursor));

	*cursorp = cursor;

	if (0) {
err:		if (cursor != NULL)
			WT_TRET(cursor->close(cursor));
	}

	/*
	 * Opening a cursor on a non-existent data source will set ret to
	 * either of ENOENT or WT_NOTFOUND at this point. However,
	 * applications may reasonably do this inside a transaction to check
	 * for the existence of a table or index.
	 *
	 * Failure in opening a cursor should not set an error on the
	 * transaction and WT_NOTFOUND will be mapped to ENOENT.
	 */

	API_END_RET_NO_TXN_ERROR(session, ret);
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
