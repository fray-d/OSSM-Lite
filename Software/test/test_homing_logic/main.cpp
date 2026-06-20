#include <unity.h>
#include "homing_logic.h"

void setUp(void) {}
void tearDown(void) {}

// ─── isCurrentOverLimit ───

void test_isCurrentOverLimit_above_threshold(void) {
    // reading=5.0, offset=1.0, threshold=3.0 → (5-1)=4 > 3 → true
    TEST_ASSERT_TRUE(homing_logic::isCurrentOverLimit(5.0f, 1.0f, 3.0f));
}

void test_isCurrentOverLimit_below_threshold(void) {
    // reading=3.0, offset=1.0, threshold=3.0 → (3-1)=2 > 3 → false
    TEST_ASSERT_FALSE(homing_logic::isCurrentOverLimit(3.0f, 1.0f, 3.0f));
}

void test_isCurrentOverLimit_exactly_at_threshold(void) {
    // reading=4.5, offset=1.0, threshold=3.5 → (4.5-1)=3.5 > 3.5 → false (not strictly greater)
    TEST_ASSERT_FALSE(homing_logic::isCurrentOverLimit(4.5f, 1.0f, 3.5f));
}

// ─── calculateMeasuredStroke ───

void test_calculateMeasuredStroke_positive_position(void) {
    // position=5000, max=10000 → 5000
    float result = homing_logic::calculateMeasuredStroke(5000, 0, 10000.0f);
    TEST_ASSERT_EQUAL_FLOAT(5000.0f, result);
}

void test_calculateMeasuredStroke_negative_position(void) {
    // position=-5000, max=10000 → abs → 5000
    float result = homing_logic::calculateMeasuredStroke(-5000, 0, 10000.0f);
    TEST_ASSERT_EQUAL_FLOAT(5000.0f, result);
}

void test_calculateMeasuredStroke_positive_position_large_current(void) {
    // position=5000, current=7000, max=10000 → 7000
    float result = homing_logic::calculateMeasuredStroke(5000, 7000, 10000.0f);
    TEST_ASSERT_EQUAL_FLOAT(7000.0f, result);
}

void test_calculateMeasuredStroke_negative_position_large_current(void) {
    // position=-5000, current=7000, max=10000 → abs → 7000
    float result = homing_logic::calculateMeasuredStroke(-5000, 7000, 10000.0f);
    TEST_ASSERT_EQUAL_FLOAT(7000.0f, result);
}

void test_calculateMeasuredStroke_clamped_to_max(void) {
    // position=15000, max=10000 → clamped to 10000
    float result = homing_logic::calculateMeasuredStroke(15000, 0, 10000.0f);
    TEST_ASSERT_EQUAL_FLOAT(10000.0f, result);
}

// ─── calculatePostHomingPosition (Path X positive-going convention) ───

void test_calculatePostHomingPosition_positive_sign(void) {
    // sign=+1 (measure.run, just zeroed at extended hard stop):
    //   goTo = -1*5000 = -5000. Not > 0, so unchanged = -5000.
    //   Drives across full range to retracted side where homing.run detects.
    int32_t result = homing_logic::calculatePostHomingPosition(1, 5000.0f, 0.5f);
    TEST_ASSERT_EQUAL_INT32(-5000, result);
}

void test_calculatePostHomingPosition_negative_sign(void) {
    // sign=-1 (homing.run, just zeroed at retracted hard stop):
    //   goTo = -(-1)*5000 = +5000. > 0, so * 0.5 = 2500.
    //   Parks rod at mid-extension when afterHomingPosition=0.5.
    int32_t result = homing_logic::calculatePostHomingPosition(-1, 5000.0f, 0.5f);
    TEST_ASSERT_EQUAL_INT32(2500, result);
}

// ─── isHomingTimedOut ───

void test_isHomingTimedOut_not_timed_out(void) {
    // 30000ms, timeout=40000 → false
    TEST_ASSERT_FALSE(homing_logic::isHomingTimedOut(30000, 40000));
}

void test_isHomingTimedOut_timed_out(void) {
    // 41000ms, timeout=40000 → true
    TEST_ASSERT_TRUE(homing_logic::isHomingTimedOut(41000, 40000));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_isCurrentOverLimit_above_threshold);
    RUN_TEST(test_isCurrentOverLimit_below_threshold);
    RUN_TEST(test_isCurrentOverLimit_exactly_at_threshold);

    RUN_TEST(test_calculateMeasuredStroke_positive_position);
    RUN_TEST(test_calculateMeasuredStroke_negative_position);
    RUN_TEST(test_calculateMeasuredStroke_positive_position_large_current);
    RUN_TEST(test_calculateMeasuredStroke_negative_position_large_current);
    RUN_TEST(test_calculateMeasuredStroke_clamped_to_max);

    RUN_TEST(test_isHomingTimedOut_not_timed_out);
    RUN_TEST(test_isHomingTimedOut_timed_out);

    return UNITY_END();
}
