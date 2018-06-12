//
// Created by System Administrator on 6/10/18.
//

#ifndef RSOCK_HANDLERUTIL_H
#define RSOCK_HANDLERUTIL_H


#include "Singleton.h"
#include "../../util/Handler.h"

struct uv_loop_s;

class HandlerUtil : public Singleton<HandlerUtil>, public ICloseable {
public:
    static Handler::SPHandler ObtainHandler(const Handler::Callback &cb = nullptr, uv_loop_t *loop = nullptr);

    explicit HandlerUtil(struct uv_loop_s *loop);

    int Close() override;

private:
    struct uv_loop_s *mLoop = nullptr;
};


#endif //RSOCK_HANDLERUTIL_H
