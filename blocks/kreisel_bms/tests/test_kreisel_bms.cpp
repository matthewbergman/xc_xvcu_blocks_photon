#include <gtest/gtest.h>

extern "C" {
#include "../src/kreisel_bms.h"
}

class kreisel_bms_tests : public testing::Test {};

TEST_F(kreisel_bms_tests, test_init) {
    struct kreisel_bms_data_t data;

    kreisel_bms_init(&data);
    data.config.ticks_per_s = 0xA0;

    EXPECT_EQ(data.config.ticks_per_s, 0xA0); 
}
