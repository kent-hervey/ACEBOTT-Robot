#include <stdint.h>
#include "unity.h"
#include "acebott_hw.h"

void setUp(void) {}
void tearDown(void) {}

void test_distance_conversion_known_value(void)
{
    int64_t duration_us = 1000; /* 1000 us -> ~17.15 cm */
    float expected = 17.15f;
    TEST_ASSERT_FLOAT_WITHIN(0.05f, expected, distance_cm_from_us(duration_us));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_distance_conversion_known_value);
    return UNITY_END();
}