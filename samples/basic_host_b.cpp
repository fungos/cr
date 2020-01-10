#include <cstdio>
#include <chrono>
#include <thread>

#define CR_HOST CR_UNSAFE // try to best manage static states
#include "../cr.h"

// avoid finishing the name by a number to not clash with the version
// as we copy the file as filenameversion.ext
const char *plugin = CR_DEPLOY_PATH "/" CR_PLUGIN("basic_guest");
const char *plugin2 = CR_DEPLOY_PATH "/" CR_PLUGIN("basic_guest_b");

int main(int argc, char *argv[]) {
    cr_plugin ctx, ctx2;
    // the host application should initalize a plugin with a context, a plugin
    // filename without extension and the full path to the plugin
    cr_plugin_open(ctx, plugin);
    cr_plugin_open(ctx2, plugin2);

    // call the plugin update function with the plugin context to execute it
    // at any frequency matters to you
    while (true) {
        cr_plugin_update(ctx);
        cr_plugin_update(ctx2);
        fflush(stdout);
        fflush(stderr);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // at the end do not forget to cleanup the plugin context, as it needs to
    // allocate some memory to track internal and plugin states
    cr_plugin_close(ctx2);
    cr_plugin_close(ctx);
    return 0;
}
