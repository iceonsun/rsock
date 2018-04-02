#ifndef SYNC_CONN_FACTORY_H
#define SYNC_CONN_FACTORY_H

#include "ISyncConn.h"

class SyncConnFactory {
public:
	static ISyncConn* CreateSysSyncConn(struct uv_loop_s *loop, const ISyncConn::Callback cb, void *obj);
};

#endif // !SYNC_CONN_FACTORY_H
