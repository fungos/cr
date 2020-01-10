#include <cstdio>
#include <chrono>
#include <thread>

#define CR_HOST CR_UNSAFE // try to best manage static states
#include "../cr.h"

const char *plugin = CR_DEPLOY_PATH "/" CR_PLUGIN("basic_guest");

int main(int argc, char *argv[]) {
    cr_plugin ctx;
    // the host application should initalize a plugin with a context, a plugin
    // filename without extension and the full path to the plugin
    cr_plugin_open(ctx, plugin);

    // call the plugin update function with the plugin context to execute it
    // at any frequency matters to you
    while (true) {
        cr_plugin_update(ctx);
        fflush(stdout);
        fflush(stderr);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // at the end do not forget to cleanup the plugin context, as it needs to
    // allocate some memory to track internal and plugin states
    cr_plugin_close(ctx);
    return 0;
}
