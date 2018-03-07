//
// Created by System Administrator on 12/27/17.
//

#include "ssock.h"
#include "SSockApp.h"

int ssock_main(int argc, char **argv) {
    uv_default_loop();
    ISockApp *app = new SSockApp();

    int nret = app->Parse(argc, reinterpret_cast<const char *const *>(argv));
    if (!nret) {
        if (!(nret = app->Init())) {
            nret = app->Start();
        }
    }
    app->Close();

    delete app;
    return nret;
}
