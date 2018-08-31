#include <gtest/gtest.h>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#define CR_HOST
#include "../cr.h"
#include "test_data.h"

// simulates new version of binary
void touch(const char *filename) {

    fs::path p = filename;
    auto ftime = fs::last_write_time(p);
    fs::last_write_time(p, ftime + std::chrono::seconds(1));
}

TEST(crTest, basic_flow) {
#if _WIN32
    const char *bin = CR_DEPLOY_PATH "/test_basic.dll";
#else
    const char *bin = CR_DEPLOY_PATH "/libtest_basic.so";
#endif

    using namespace test_basic;
    cr_plugin ctx;
    test_data data;
    ctx.userdata = &data;
    EXPECT_EQ(true, cr_plugin_load(ctx, bin));

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
    data.test = test_id::crash_update;
    EXPECT_EQ(-1, cr_plugin_update(ctx));
    EXPECT_EQ((unsigned int)2, ctx.version);
    EXPECT_EQ(CR_SEGFAULT, ctx.failure);

    // next update should do a rollback, check version decrements
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
    touch(bin);

    // should be a new version
    data.test = test_id::return_version;
    EXPECT_EQ(2, cr_plugin_update(ctx));

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
    touch(bin);

    // makes it crash during load
    data.test = test_id::crash_load;
    EXPECT_EQ(-2, cr_plugin_update(ctx));
    EXPECT_EQ((unsigned int)3, ctx.version);
    EXPECT_EQ(CR_SEGFAULT, ctx.failure);

    // load crashed, so it should restore version 2
    data.test = test_id::return_version;
    EXPECT_EQ(2, cr_plugin_update(ctx));

    // recompile code
    touch(bin);

    // force crash on unload
    data.test = test_id::crash_unload;
    EXPECT_EQ(-2, cr_plugin_update(ctx));
    EXPECT_EQ((unsigned int)3, ctx.version);
    EXPECT_EQ(CR_SEGFAULT, ctx.failure);

    // unload crashed, should still restore version 2
    data.test = test_id::return_version;
    EXPECT_EQ(2, cr_plugin_update(ctx));

    // modify states
    data.test = test_id::static_local_state_int;
    EXPECT_EQ(saved_local_static + 3, cr_plugin_update(ctx));

    data.test = test_id::static_global_state_int;
    EXPECT_EQ(saved_global_static + 3, cr_plugin_update(ctx));

    cr_plugin_close(ctx);
    EXPECT_EQ(ctx.p, nullptr);
    EXPECT_EQ(0u, ctx.version);
}
