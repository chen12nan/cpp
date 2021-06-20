
#include <wiredtiger.h>
#include <iostream>

#define error_check(func) func

int main(int argc, char *argv[])
{
    std::cout << "Hello WiredTiger" << std::endl;
    const char *home = "/Users/xingkd/WT_HOME";
    /*! [access example connection] */
    WT_CONNECTION *
        conn;
    WT_CURSOR *cursor;
    WT_SESSION *session;
    const char *key, *value;
    int ret;

    /* Open a connection to the database, creating it if necessary. */
    error_check(wiredtiger_open(home, NULL, "create", &conn));

    /* Open a session handle for the database. */
    error_check(conn->open_session(conn, NULL, NULL, &session));
    /*! [access example connection] */

    /*! [access example table create] */
    error_check(session->create(session, "table:access", "key_format=S,value_format=S"));
    /*! [access example table create] */

    /*! [access example cursor open] */
    error_check(session->open_cursor(session, "table:access", NULL, NULL, &cursor));
    error_check(session->open_cursor(session, "index:access:key", NULL, NULL, &cursor));
    /*! [access example cursor open] */

    /*! [access example cursor insert] */
    cursor->set_key(cursor, "key1"); /* Insert a record. */
    cursor->set_value(cursor, "value1");
    error_check(cursor->insert(cursor));
    /*! [access example cursor insert] */

    /*! [access example cursor list] */
    error_check(cursor->reset(cursor)); /* Restart the scan. */
    while ((ret = cursor->next(cursor)) == 0)
    {
        error_check(cursor->get_key(cursor, &key));
        error_check(cursor->get_value(cursor, &value));

        printf("Got record: %s : %s\n", key, value);
    }
    // scan_end_check(ret == WT_NOTFOUND); /* Check for end-of-table. */
    /*! [access example cursor list] */

    /*! [access example close] */
    error_check(conn->close(conn, NULL)); /* Close all handles. */
                                          /*! [access example close] */
    return 0;
}