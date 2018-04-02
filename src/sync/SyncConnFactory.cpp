#include "SyncConnFactory.h"
#include "TcpStreamSyncConn.h"

#include "UdpSyncConn.h"

#ifndef _WIN32

#include "../os/unix/conn/UnixDgramSyncConn.h"
#include "LoopSreamSyncConn.h"

#endif // !_WIN32

// speed are almost same
ISyncConn *SyncConnFactory::CreateSysSyncConn(struct uv_loop_s *loop, const ISyncConn::Callback cb, void *obj) {
#ifndef _WIN32
    return new UnixDgramSyncConn(loop, cb, obj);
#else
    //return new UdpSyncConn(loop, cb, obj);    // extremely low performance on Winddows
    return new TcpStreamSyncConn(loop, cb, obj);
//    return new LoopSreamSyncConn(loop, cb, obj);
#endif // _WIN32
}
