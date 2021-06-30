
#include <wiredtiger.h>
#include <bson.h>
#include <iostream>

#define error_check(func) func

int main(int argc, char *argv[])
{
    std::cout << "Hello WiredTiger1" << std::endl;
    const char *home = "/Users/sisi/Public/Xingkd/learn_wt/cpp/build/WT_HOME";
    /*! [access example connection] */
    WT_CONNECTION *conn;
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
    error_check(session->create(session, "table:bison", "key_format=u,value_format=u"));
    /*! [access example table create] */

    session->create(session, "config:_test_config", "key_format=u, value_format=u");

    session->create(session, "index:_test_index", "key_format=u, value_format=u");
    session->create(session, "join:_test_join", "key_format=u, value_format=u");
    session->create(session, "log:_test_log", "key_format=u, value_format=u");
    session->create(session, "statistics:_test_statistics", "key_format=u, value_format=u");

    /*! [access example cursor open] */
    error_check(session->open_cursor(session, "table:bison", NULL, NULL, &cursor));
    // error_check(session->open_cursor(session, "index:access:key", NULL, NULL, &cursor));
    /*! [access example cursor open] */

    
    /*! [access example cursor insert] */
    // for (int i = 0; i < 1000000; i++) {
    //     std::string id = "key_" + std::to_string(i);
    //     std::string val = "value_" + std::to_string(i);
    //     cursor->set_key(cursor, id.c_str()); /* Insert a record. */
    //     cursor->set_value(cursor, val.c_str());
    //     error_check(cursor->insert(cursor));
    // }
    /*! [access example cursor insert] */

    for (int i = 0; i < 3; i ++) {
        break;
        bson_t *b = bson_new();
        BSON_APPEND_BOOL(b, "k_bool", false);
        BSON_APPEND_INT32(b, "k_in32", i);
        BSON_APPEND_DOUBLE(b, "k_double", 3.44);
        BSON_APPEND_UTF8(b, "k_utf8", "test_bson");
        
        bson_oid_t oid;
        bson_oid_init(&oid, nullptr);

        char str[25];
        bson_oid_to_string(&oid, str);
        std::cout<<"generate id = "<<str<<std::endl;
        WT_ITEM kitem;
        kitem.data = oid.bytes;
        kitem.size = 12;
        
        WT_ITEM vitem;
        vitem.data = static_cast<const void*>(bson_get_data(b));
        vitem.size = b->len;

        std::cout<<"kitem size = "<<kitem.size<<std::endl;
        cursor->set_key(cursor, &kitem);
        cursor->set_value(cursor, &vitem);
        cursor->insert(cursor);
        bson_free(b);
    }

    // /*! [access example cursor list] */
    error_check(cursor->reset(cursor)); /* Restart the scan. */
    WT_CURSOR* get_cur;
    error_check(session->open_cursor(session, "table:bison", NULL, NULL, &get_cur));
    get_cur->reset(get_cur);

    while ((ret = get_cur->next(get_cur)) == 0)
    {
        WT_ITEM val;
        WT_ITEM kitem;
        char str[25];
        error_check(get_cur->get_key(get_cur, &kitem));
        error_check(get_cur->get_value(get_cur, &val));
        bson_oid_t dt;
        bson_oid_init_from_data(&dt, (uint8_t*)(kitem.data));
        bson_oid_to_string(&dt, str);

        uint8_t* data = reinterpret_cast<uint8_t*>(const_cast<void*>(val.data));
        bson_t *tb = bson_new_from_data(data, val.size);

        std::cout<<"get oid = "<<str<<std::endl;
        std::cout<<"value = "<<bson_as_json(tb, nullptr)<<std::endl;
    }

    // scan_end_check(ret == WT_NOTFOUND); /* Check for end-of-table. */
    /*! [access example cursor list] */

    error_check(session->create(session, "table:order_test", "key_format=i,value_format=u"));
    WT_CURSOR *order_cur;
    session->open_cursor(session, "table:order_test", nullptr, nullptr, &order_cur);
    for (int i = i; i < 2000000; i++) {
        int x = rand()%2000000;
        std::cout<<"x = "<<x<<std::endl;
        order_cur->set_key(order_cur, x);
        bson_t *b = bson_new();
        BSON_APPEND_BOOL(b, "k_bool", false);
        BSON_APPEND_INT32(b, "k_in32", 10009);
        BSON_APPEND_DOUBLE(b, "k_double", 3.44);
        BSON_APPEND_UTF8(b, "k_utf8", "test_bsonadsfasdfsadfdasfasdfsdfdsafdsafdsafsdfdsaffffsafd");
        WT_ITEM vitem;
        vitem.data = static_cast<const void*>(bson_get_data(b));
        vitem.size = b->len;
        order_cur->set_value(order_cur, &vitem);
        order_cur->insert(order_cur);

        order_cur->set_key(order_cur, 2);
        order_cur->set_value(order_cur, &vitem);
        order_cur->insert(order_cur);
    }

    while ((ret = order_cur->next(order_cur)) == 0)
    {
        WT_ITEM val;
        WT_ITEM kitem;
        int i = -1;
        error_check(order_cur->get_key(order_cur, &i));
        error_check(order_cur->get_value(order_cur, &val));

        uint8_t* data = reinterpret_cast<uint8_t*>(const_cast<void*>(val.data));
        bson_t *tb = bson_new_from_data(data, val.size);

        std::cout<<"order i = "<<i<<std::endl;
        // std::cout<<"value = "<<bson_as_json(tb, nullptr)<<std::endl;
    }


    /*! [access example close] */
    error_check(conn->close(conn, NULL)); /* Close all handles. */
                                          /*! [access example close] */
    return 0;
}