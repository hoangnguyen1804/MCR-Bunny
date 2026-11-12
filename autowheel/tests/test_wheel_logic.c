#include "../wheel_logic.h"

#include <assert.h>
#include <stdio.h>

static WheelState armed_and_released(unsigned interval_ms)
{
    WheelState state = {0};
    assert(wheel_arm(&state, 42, interval_ms));
    assert(state.armed && state.waiting_release && state.target == 42);
    assert(!wheel_tick(&state, 0, true, true, false));
    assert(!state.waiting_release);
    return state;
}

static void test_activation_requires_release(void)
{
    WheelState state = {0};
    assert(wheel_arm(&state, 42, 1));
    assert(!wheel_tick(&state, 0, true, true, true));
    assert(!wheel_tick(&state, 1000000, true, true, true));
    assert(!wheel_tick(&state, 1000001, true, true, false));
    assert(wheel_tick(&state, 1000002, true, true, true));
}

static void test_one_millisecond_pacing(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 100, true, true, true));
    for (uint64_t t = 100; t < 1100; ++t) {
        assert(!wheel_tick(&state, t, true, true, true));
    }
    assert(wheel_tick(&state, 1100, true, true, true));
    assert(!wheel_tick(&state, 1100, true, true, true));
    assert(!wheel_tick(&state, 2099, true, true, true));
    assert(wheel_tick(&state, 2100, true, true, true));
}

static void test_maximum_interval(void)
{
    WheelState state = armed_and_released(1000);
    assert(wheel_tick(&state, 0, true, true, true));
    assert(!wheel_tick(&state, 999999, true, true, true));
    assert(wheel_tick(&state, 1000000, true, true, true));
}

static void test_no_catchup_burst(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, true));
    assert(wheel_tick(&state, 10000000, true, true, true));
    for (unsigned i = 0; i < 100; ++i) {
        assert(!wheel_tick(&state, 10000000, true, true, true));
    }
    assert(!wheel_tick(&state, 10000999, true, true, true));
    assert(wheel_tick(&state, 10001000, true, true, true));
}

static void test_focus_loss_disarms(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, true));
    assert(!wheel_tick(&state, 1000, false, true, true));
    assert(!state.armed && state.target == 0);
    assert(!wheel_tick(&state, 2000, true, true, true));
    assert(!wheel_tick(&state, 3000, true, true, false));
    assert(!wheel_tick(&state, 4000, true, true, true));
}

static void test_cursor_return_requires_release(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, true));
    assert(!wheel_tick(&state, 1000, true, false, true));
    assert(state.armed && state.waiting_release);
    assert(!wheel_tick(&state, 2000, true, true, true));
    assert(!wheel_tick(&state, 3000, true, true, true));
    assert(!wheel_tick(&state, 4000, true, true, false));
    assert(wheel_tick(&state, 4001, true, true, true));
}

static void test_release_outside_does_not_resume(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, true));
    assert(!wheel_tick(&state, 1000, true, false, false));
    assert(!wheel_tick(&state, 2000, true, false, true));
    assert(!wheel_tick(&state, 3000, true, true, true));
    assert(!wheel_tick(&state, 4000, true, true, false));
    assert(wheel_tick(&state, 4001, true, true, true));
}

static void test_release_and_repress_are_immediate(void)
{
    WheelState state = armed_and_released(1000);
    assert(wheel_tick(&state, 0, true, true, true));
    assert(!wheel_tick(&state, 1, true, true, false));
    assert(wheel_tick(&state, 2, true, true, true));
    assert(!wheel_tick(&state, 3, true, true, true));
}

static void test_stop_and_rearm(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, true));
    wheel_stop(&state);
    assert(!state.armed && !state.emitted && state.target == 0);
    assert(!wheel_tick(&state, 1000, true, true, true));
    assert(wheel_arm(&state, 99, 10));
    assert(state.target == 99 && state.interval_ms == 10);
    assert(!wheel_tick(&state, 2000, true, true, true));
    assert(!wheel_tick(&state, 2001, true, true, false));
    assert(wheel_tick(&state, 2002, true, true, true));
}

static void test_invalid_settings_disarm(void)
{
    WheelState state = armed_and_released(1);
    assert(!wheel_arm(&state, 0, 1));
    assert(!state.armed);
    assert(!wheel_arm(&state, 42, 0));
    assert(!state.armed);
    assert(!wheel_arm(&state, 42, 1001));
    assert(!state.armed);
    assert(!wheel_arm(&state, 42, (unsigned)-1));
    assert(!state.armed);
    assert(!wheel_tick(&state, 1000, true, true, true));
}

static void test_clock_reset_does_not_underflow(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 10000, true, true, true));
    assert(!wheel_tick(&state, 0, true, true, true));
    assert(!wheel_tick(&state, 999, true, true, true));
    assert(wheel_tick(&state, 1000, true, true, true));
}

static void test_null_state(void)
{
    assert(!wheel_arm(0, 42, 1));
    assert(!wheel_tick(0, 0, true, true, true));
    wheel_stop(0);
}

int main(void)
{
    test_activation_requires_release();
    test_one_millisecond_pacing();
    test_maximum_interval();
    test_no_catchup_burst();
    test_focus_loss_disarms();
    test_cursor_return_requires_release();
    test_release_outside_does_not_resume();
    test_release_and_repress_are_immediate();
    test_stop_and_rearm();
    test_invalid_settings_disarm();
    test_clock_reset_does_not_underflow();
    test_null_state();
    puts("PASS: 12 wheel state-machine test cases");
    return 0;
}
