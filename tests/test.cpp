#include <gtest/gtest.h>

#define CR_HOST
#include "../cr.h"
#include "test_data.h"

#if defined(CR_WINDOWS) || defined(CR_LINUX)
#if defined(_MSC_VER)
// avoid experimental/filesystem deprecation #error on MSVC >= 16.3 since we're
// not using C++17 explicitly.
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#endif
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

void touch(const char *filename) {
    fs::path p = filename;
    auto ftime = fs::last_write_time(p);
    fs::last_write_time(p, ftime + std::chrono::seconds(1));
}
#else
void touch(const char *filename) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    char cmd[1024] = {};
    snprintf(cmd, sizeof(cmd), "touch %s", filename);
    system(cmd);
}
#endif

void delete_old_files(cr_plugin &ctx, unsigned int max_version) {
    auto p = (cr_internal *)ctx.p;
    const auto file = p->fullname;
    for (unsigned int i = 0; i < max_version; i++) {
        cr_del(cr_version_path(file, i, p->temppath));
#if defined(_MSC_VER)
        cr_del(cr_replace_extension(cr_version_path(file, i, p->temppath), ".pdb"));
#endif
    }
}

TEST(crTest, basic_flow) {
    const char *bin = CR_DEPLOY_PATH "/" CR_PLUGIN("test_basic");

    using namespace test_basic;
    cr_plugin ctx;
    test_data data;
    ctx.userdata = &data;
    // version 1
    EXPECT_EQ(true, cr_plugin_open(ctx, bin));

    data.test = test_id::return_version;
    EXPECT_EQ(1, cr_plugin_update(ctx));

    // modify local static variable
    data.test = test_id::static_local_state_int;
    EXPECT_EQ(11, cr_plugin_update(ctx));

    // modify global static variable
    data.test = test_id::static_global_state_int;
    int saved_global_static = cr_plugin_update(ctx);
    EXPECT_EQ(1, saved_global_static);

    // modify local static variable
    data.test = test_id::static_local_state_int;
    int saved_local_static = cr_plugin_update(ctx);
    EXPECT_EQ(12, saved_local_static);

    // simulate a new compilation, causes unload
    // states should be saved as 1 and 12
    // version 2
    touch(bin);

    // should reload and increment version
    data.test = test_id::return_version;
    EXPECT_EQ(2, cr_plugin_update(ctx));

    // check if our global static was saved, it should increment again
    data.test = test_id::static_global_state_int;
    EXPECT_EQ(saved_global_static + 1, cr_plugin_update(ctx));
    EXPECT_EQ(saved_global_static + 2, cr_plugin_update(ctx));

    // check if our local static was saved, it should increment again
    data.test = test_id::static_local_state_int;
    EXPECT_EQ(saved_local_static + 1, cr_plugin_update(ctx));
    EXPECT_EQ(saved_local_static + 2, cr_plugin_update(ctx));

    // alloc some data on the heap
    data.test = test_id::heap_data_alloc;
    EXPECT_EQ(4096 * 1024, cr_plugin_update(ctx));
    // test our allocated data
    EXPECT_EQ(0, cr_plugin_update(ctx));

    // emulate a segmentation fault trying to access nullptr
    // crash version 2, it should rollback to version 1 because the version
    // decrement happens within the crash handler
    // version 1
    data.test = test_id::crash_update;
    EXPECT_EQ(-1, cr_plugin_update(ctx));
    EXPECT_EQ((unsigned int)1, ctx.version);
    EXPECT_EQ(CR_SEGFAULT, ctx.failure);

    // next update should do a rollback, check we're still in version 1 and
    // no further decrement happens
    // version 1
    data.test = test_id::return_version;
    EXPECT_EQ(1, cr_plugin_update(ctx));

    // state should have rollbacked to what it was at the last unload before the crash
    // +1 because it increments each time we query
    data.test = test_id::static_local_state_int;
    EXPECT_EQ(saved_local_static + 1, cr_plugin_update(ctx));

    // state should have rollbacked to what it was at the last unload before the crash
    // +1 because it increments each time we query
    data.test = test_id::static_global_state_int;
    EXPECT_EQ(saved_global_static + 1, cr_plugin_update(ctx));

    // heap shouln't have been touched due to rollback, test it is still good
    data.test = test_id::heap_data_alloc;
    EXPECT_EQ(0, cr_plugin_update(ctx));

    // recompile code
    // a new version 3, hopefuly with the bug fixed!
    touch(bin);

    // should be a new version
    data.test = test_id::return_version;
    EXPECT_EQ(3, cr_plugin_update(ctx));

    // test our allocated data is still good
    data.test = test_id::heap_data_alloc;
    EXPECT_EQ(0, cr_plugin_update(ctx));

    // modify states
    data.test = test_id::static_local_state_int;
    EXPECT_EQ(saved_local_static + 2, cr_plugin_update(ctx));

    data.test = test_id::static_global_state_int;
    EXPECT_EQ(saved_global_static + 2, cr_plugin_update(ctx));

    // free our heap data
    data.test = test_id::heap_data_free;
    EXPECT_EQ(4096 * 1024, cr_plugin_update(ctx));
    // check it is freed
    EXPECT_EQ(0, cr_plugin_update(ctx));

    // recompile code
    // version 4
    touch(bin);

    // makes it crash during load
    // crash handler automatically decrements the version
    data.test = test_id::crash_load;
    EXPECT_EQ(-2, cr_plugin_update(ctx));
    EXPECT_EQ((unsigned int)3, ctx.version);
    EXPECT_EQ(CR_SEGFAULT, ctx.failure);

    // load crashed, so it should be at version 3
    data.test = test_id::return_version;
    EXPECT_EQ(3, cr_plugin_update(ctx));

    // recompile code
    // a new version 5 retry
    touch(bin);

    // force crash on unload
    // but now the bug moved and crashed again, we're back to version 2
    data.test = test_id::crash_unload;
    EXPECT_EQ(-2, cr_plugin_update(ctx));
    EXPECT_EQ((unsigned int)2, ctx.version);
    EXPECT_EQ(CR_SEGFAULT, ctx.failure);

    // unload crashed, next update should do a rollback
    data.test = test_id::return_version;
    EXPECT_EQ(2, cr_plugin_update(ctx));

    // modify states
    data.test = test_id::static_local_state_int;
    EXPECT_EQ(saved_local_static + 3, cr_plugin_update(ctx));

    data.test = test_id::static_global_state_int;
    EXPECT_EQ(saved_global_static + 3, cr_plugin_update(ctx));

    // Cleanup old version
    delete_old_files(ctx, ctx.next_version);

    cr_plugin_close(ctx);
    EXPECT_EQ(ctx.p, nullptr);
    EXPECT_EQ(0u, ctx.version);
}
