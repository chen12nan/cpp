

/**
 *__wt_connection_open
 创建session， cache
 *
 * /
 *

 /** 初始化btree
  ** thread #1, queue = 'com.apple.main-thread', stop reason = breakpoint 1.1
    frame #0: 0x00000001001e819d libwiredtiger-10.0.1.dylib`__wt_conn_dhandle_alloc(session=0x000000010087e000, uri="file:WiredTiger.wt", checkpoint=0x0000000000000000) at conn_dhandle.c:208:27
   206 	    if (WT_DHANDLE_BTREE(dhandle)) {
   207 	        WT_ERR(__wt_calloc_one(session, &btree));
-> 208 	        dhandle->handle = btree;
   209 	        btree->dhandle = dhandle;
   210 	    }
   211
Target 0: (test_wt) stopped.
(lldb) bt
* thread #1, queue = 'com.apple.main-thread', stop reason = breakpoint 1.1
  * frame #0: 0x00000001001e819d libwiredtiger-10.0.1.dylib`__wt_conn_dhandle_alloc(session=0x000000010087e000, uri="file:WiredTiger.wt", checkpoint=0x0000000000000000) at conn_dhandle.c:208:27
    frame #1: 0x0000000100326b59 libwiredtiger-10.0.1.dylib`__session_find_shared_dhandle(session=0x000000010087e000, uri="file:WiredTiger.wt", checkpoint=0x0000000000000000) at session_dhandle.c:415:5
    frame #2: 0x0000000100326243 libwiredtiger-10.0.1.dylib`__session_get_dhandle(session=0x000000010087e000, uri="file:WiredTiger.wt", checkpoint=0x0000000000000000) at session_dhandle.c:445:5
    frame #3: 0x0000000100325ea9 libwiredtiger-10.0.1.dylib`__wt_session_get_dhandle(session=0x000000010087e000, uri="file:WiredTiger.wt", checkpoint=0x0000000000000000, cfg=0x00007ffeefbff620, flags=0) at session_dhandle.c:474:9
    frame #4: 0x0000000100325e10 libwiredtiger-10.0.1.dylib`__wt_session_get_btree_ckpt(session=0x000000010087e000, uri="file:WiredTiger.wt", cfg=0x00007ffeefbff620, flags=0) at session_dhandle.c:319:11
    frame #5: 0x0000000100211ff0 libwiredtiger-10.0.1.dylib`__wt_curfile_open(session=0x000000010087e000, uri="file:WiredTiger.wt", owner=0x0000000000000000, cfg=0x00007ffeefbff620, cursorp=0x00007ffeefbff660) at cur_file.c:835:15
    frame #6: 0x0000000100310b81 libwiredtiger-10.0.1.dylib`__session_open_cursor_int(session=0x000000010087e000, uri="file:WiredTiger.wt", owner=0x0000000000000000, other=0x0000000000000000, cfg=0x00007ffeefbff620, hash_value=1045034099109282882, cursorp=0x00007ffeefbff660) at session_api.c:487:13
    frame #7: 0x00000001003106c1 libwiredtiger-10.0.1.dylib`__wt_open_cursor(session=0x000000010087e000, uri="file:WiredTiger.wt", owner=0x0000000000000000, cfg=0x00007ffeefbff620, cursorp=0x00007ffeefbff660) at session_api.c:561:13
    frame #8: 0x00000001002ac503 libwiredtiger-10.0.1.dylib`__wt_metadata_cursor_open(session=0x000000010087e000, config=0x0000000000000000, cursorp=0x00007ffeefbff660) at meta_table.c:66:5
    frame #9: 0x00000001002ac6a0 libwiredtiger-10.0.1.dylib`__wt_metadata_cursor(session=0x000000010087e000, cursorp=0x0000000000000000) at meta_table.c:108:9
    frame #10: 0x00000001001d5ce6 libwiredtiger-10.0.1.dylib`wiredtiger_open(home="/Users/sisi/Public/Xingkd/WT_HOME", event_handler=0x0000000000000000, config="create", connectionp=0x00007ffeefbff970) at conn_api.c:2886:5
    frame #11: 0x0000000100003abf test_wt`main(argc=1, argv=0x00007ffeefbff9b8) at main.cpp:21:5
    frame #12: 0x00007fff20535f5d libdyld.dylib`start + 1
    frame #13: 0x00007fff20535f5d libdyld.dylib`start + 1

*/

/**
 * * thread #1, queue = 'com.apple.main-thread', stop reason = breakpoint 1.1
  * frame #0: 0x00000001001e819d libwiredtiger-10.0.1.dylib`__wt_conn_dhandle_alloc(session=0x0000000100881260, uri="file:bison.wt", checkpoint=0x0000000000000000) at conn_dhandle.c:208:27
    frame #1: 0x0000000100326b59 libwiredtiger-10.0.1.dylib`__session_find_shared_dhandle(session=0x0000000100881260, uri="file:bison.wt", checkpoint=0x0000000000000000) at session_dhandle.c:415:5
    frame #2: 0x0000000100326243 libwiredtiger-10.0.1.dylib`__session_get_dhandle(session=0x0000000100881260, uri="file:bison.wt", checkpoint=0x0000000000000000) at session_dhandle.c:445:5
    frame #3: 0x0000000100325ea9 libwiredtiger-10.0.1.dylib`__wt_session_get_dhandle(session=0x0000000100881260, uri="file:bison.wt", checkpoint=0x0000000000000000, cfg=0x00007ffeefbff890, flags=0) at session_dhandle.c:474:9
    frame #4: 0x0000000100325e10 libwiredtiger-10.0.1.dylib`__wt_session_get_btree_ckpt(session=0x0000000100881260, uri="file:bison.wt", cfg=0x00007ffeefbff890, flags=0) at session_dhandle.c:319:11
    frame #5: 0x0000000100211ff0 libwiredtiger-10.0.1.dylib`__wt_curfile_open(session=0x0000000100881260, uri="file:bison.wt", owner=0x0000000000000000, cfg=0x00007ffeefbff890, cursorp=0x00007ffeefbff858) at cur_file.c:835:15
    frame #6: 0x0000000100310b81 libwiredtiger-10.0.1.dylib`__session_open_cursor_int(session=0x0000000100881260, uri="file:bison.wt", owner=0x0000000000000000, other=0x0000000000000000, cfg=0x00007ffeefbff890, hash_value=15657415417984460718, cursorp=0x00007ffeefbff858) at session_api.c:487:13
    frame #7: 0x00000001003106c1 libwiredtiger-10.0.1.dylib`__wt_open_cursor(session=0x0000000100881260, uri="file:bison.wt", owner=0x0000000000000000, cfg=0x00007ffeefbff890, cursorp=0x00007ffeefbff858) at session_api.c:561:13
    frame #8: 0x000000010025569d libwiredtiger-10.0.1.dylib`__wt_curtable_open(session=0x0000000100881260, uri="table:bison", owner=0x0000000000000000, cfg=0x00007ffeefbff890, cursorp=0x00007ffeefbff858) at cur_table.c:1000:15
    frame #9: 0x0000000100310792 libwiredtiger-10.0.1.dylib`__session_open_cursor_int(session=0x0000000100881260, uri="table:bison", owner=0x0000000000000000, other=0x0000000000000000, cfg=0x00007ffeefbff890, hash_value=2778475941804087040, cursorp=0x00007ffeefbff858) at session_api.c:453:13
    frame #10: 0x00000001003120b1 libwiredtiger-10.0.1.dylib`__session_open_cursor(wt_session=0x0000000100881260, uri="table:bison", to_dup=0x0000000000000000, config=0x0000000000000000, cursorp=0x00007ffeefbff968) at session_api.c:615:5
    frame #11: 0x0000000100003b2c test_wt`main(argc=1, argv=0x00007ffeefbff9b8) at main.cpp:32:5
    frame #12: 0x00007fff20535f5d libdyld.dylib`start + 1
    frame #13: 0x00007fff20535f5d libdyld.dylib`start + 1

*/


/**
 * mongodb 应该是把oid又转成int64的值存在mongodb里
 *
 * 为什么wt_connection_impl 的双向链表是wt_data_handle, 而wt_session_impl 的是wt_data_handle_cache
 *
 * 插入的数据会根据id排序， 重新get数据的时候会按顺序get
 */

 /**
  * 广东轻工职业技术学院  510/26731 广州市
  * 广东交通职业技术学院  376/47350  250/50750 广州市
  * 广东水利电力职业技术学院 407/44669 广州市
  * 广东南华工商职业学院  469/35423	广东广州市
  * 私立华联学院 250/50750 广东广州市
  * 广州民航职业技术学院 449/39038 广东广州市
  * 广州番禺职业技术学院 489/31296 435/41164
  * 广东农工商职业技术学院 446/39484
  * 广东食品药品职业学院  416/43658
  * 广东职业技术学院  442/40150 广东佛山市
  * 广东建设职业技术学院 450/38868
  * 广东女子职业技术学院 397/45624
  * 广东机电职业技术学院 443/39980
  * 广东工贸职业技术学院 	447/39350
  * 广东亚视演艺职业学院  250/50750 广东东莞市
  * 广东省外语艺术职业学院  464/36343
  * 江门职业技术学院  439/40592
  * 茂名职业技术学院  250/50750
  * 广州涉外经济职业技术学院  250/50750
  * 惠州经济职业技术学院 	250/50750
  * 广东理工学院 458/37477 广东肇庆市
  * 广东工商职业技术学院 446/39484
  * 广州华立科技职业学院 250/50750
  * 广州铁路职业技术学院 	426/42433
  * 广州科技贸易职业学院 	471/35052
  * 中山职业技术学院  350/48818  公办
  * 广州珠江职业技术学院 386/46588
  * 广东文理职业学院 250/50750 广东湛江市
  * 广州城建职业学院 393/46009 民办
  * 广州华夏职业学院 389/46344	民办
  * 广东创新科技职业学院 去年没有招 广东东莞市  民办
  * 惠州卫生职业技术学院 	459/37299
  * 广东茂名健康职业学院 	391/46152 广东茂名市 公办
  * 广东酒店管理职业技术学院 不招海南
  *
  *
  *
  */