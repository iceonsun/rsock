
#include "CSockApp.h"
#include "csock.h"

int csock_main(int argc, char **argv) {
    uv_default_loop();
    ISockApp *app = new CSockApp();

    int nret = app->Parse(argc, reinterpret_cast<const char *const *>(argv));
    if (!nret) {
        if (!(nret = app->Init())) {
            app->Start();
        }
    }
    app->Close();

    delete app;
    return nret;
}

