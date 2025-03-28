#include "cr.h"
#include "test_data.h"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <thread>

using namespace test_basic;

#define CR_TEST_LIST_BEGIN() switch (data->test) {
#define CR_TEST(x) CASE_TEST(x);
#define CR_TEST_LIST_END() }

static uint32_t CR_STATE global_int = 0;

DEFINE_TEST(return_version) {
    return ctx->version;
}

DEFINE_TEST(static_local_state_int) {
    static int CR_STATE local_int = -10;
    if (local_int < 0) {
        local_int = 10;
    }
    return ++local_int;
}

DEFINE_TEST(static_global_state_int) {
    return ++global_int;
}

DEFINE_TEST(heap_data_alloc) {
    const int amount = 4096 * 1024;
    if (!data->heap_data_ptr) {
        data->heap_data_ptr = (int *)malloc(amount * sizeof(int));
        data->heap_data_size = amount;
        for (int i = 0; i < amount; ++i) {
            data->heap_data_ptr[i] = amount - i;
        }
        return amount;
    } else {
        for (int i = 0; i < amount; ++i) {
            if (data->heap_data_ptr[i] != amount - i) {
                return i + 1;
            }
        }
    }

    return 0;
}

DEFINE_TEST(heap_data_free) {
    if (data->heap_data_ptr) {
        int r = data->heap_data_size;
        free(data->heap_data_ptr);
        data->heap_data_ptr = nullptr;
        data->heap_data_size = 0;
        return r;
    }

    return 0;
}

DEFINE_TEST(crash_update) {
    if (operation == CR_STEP) {
        int *addr = nullptr;
        (void)++*addr;
    }
    return 0;
}

DEFINE_TEST(crash_load) {
    if (operation == CR_LOAD) {
        int *addr = nullptr;
        (void)++*addr;
    }
    return 0;
}

DEFINE_TEST(crash_unload) {
    if (operation == CR_UNLOAD) {
        int *addr = nullptr;
        (void)++*addr;
    }
    return 0;
}

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    test_data *data = (test_data *)ctx->userdata;
    // clang-format off
    #include "test_basic.x"
    return -1;
}
