#include "unity.h"
#include "report_descriptor.h"

void setUp() {}
void tearDown() {}

void is_report_id_present_in_descriptor__should_return_false_for_null_descriptor() {
    TEST_ASSERT_FALSE(is_report_id_present_in_descriptor(nullptr,0));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(is_report_id_present_in_descriptor__should_return_false_for_null_descriptor);
    return UNITY_END();
}