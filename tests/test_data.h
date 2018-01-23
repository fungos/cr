// clang-format off
#pragma once

#define CASE_TEST(id) case test_id::id: return test_##id(ctx, operation, data);
#define DEFINE_TEST(id) int test_##id(cr_plugin *ctx, cr_op operation, test_data *data)

namespace test_basic {

    #define CR_TEST_LIST_BEGIN() namespace test_id { enum e {
    #define CR_TEST(x)              x,
    #define CR_TEST_LIST_END()   };}
    #include "test_basic.x"
    #undef CR_TEST_LIST_BEGIN
    #undef CR_TEST
    #undef CR_TEST_LIST_END

    struct test_data {
        test_id::e test = test_id::return_version;
        int static_local_state = 0;
        int static_global_state = 0;
        int *heap_data_ptr = nullptr;
        int heap_data_size = 0;
    };
}
