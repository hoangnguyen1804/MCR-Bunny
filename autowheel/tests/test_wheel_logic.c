#include "../wheel_logic.h"

#include <assert.h>
#include <stdio.h>

enum { MIDDLE = 1, X1 = 2, X2 = 4 };

static WheelState armed_and_released(unsigned interval_ms)
{
    WheelState state = {0};
    assert(wheel_arm(&state, 42, interval_ms));
    assert(state.armed && !state.scrolling && state.waiting_release);
    assert(state.target == 42);
    assert(!wheel_tick(&state, 0, true, true, 0));
    assert(!state.waiting_release);
    return state;
}

static void test_activation_only_arms(void)
{
    WheelState state = armed_and_released(1);
    assert(!wheel_tick(&state, 1000000, true, true, 0));
    assert(state.armed && !state.scrolling && !state.emitted);
}

static void test_activation_requires_release(void)
{
    WheelState state = {0};
    assert(wheel_arm(&state, 42, 1));
    assert(!wheel_tick(&state, 0, true, true, MIDDLE | X1));
    assert(!wheel_tick(&state, 1000000, true, true, MIDDLE));
    assert(!wheel_tick(&state, 1000001, true, true, 0));
    assert(wheel_tick(&state, 1000002, true, true, MIDDLE));
    assert(state.scrolling);
}

static void test_each_mouse_button_toggles(void)
{
    const uint32_t buttons[] = {MIDDLE, X1, X2};
    for (unsigned i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i) {
        WheelState state = armed_and_released(1);
        assert(wheel_tick(&state, 0, true, true, buttons[i]));
        assert(state.scrolling);
        assert(wheel_tick(&state, 1000, true, true, 0));
        assert(state.scrolling);
        assert(!wheel_tick(&state, 2000, true, true, buttons[i]));
        assert(!state.scrolling);
        assert(!wheel_tick(&state, 3000, true, true, 0));
    }
}

static void test_holding_does_not_retoggle(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, MIDDLE));
    for (uint64_t t = 1000; t <= 10000; t += 1000) {
        assert(wheel_tick(&state, t, true, true, MIDDLE));
        assert(state.scrolling);
    }
    assert(wheel_tick(&state, 11000, true, true, 0));
    assert(!wheel_tick(&state, 12000, true, true, MIDDLE));
    for (uint64_t t = 13000; t <= 20000; t += 1000) {
        assert(!wheel_tick(&state, t, true, true, MIDDLE));
        assert(!state.scrolling);
    }
}

static void test_releasing_keeps_scrolling(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, X1));
    assert(wheel_tick(&state, 1000, true, true, 0));
    assert(wheel_tick(&state, 2000, true, true, 0));
    assert(state.scrolling);
}

static void test_distinct_button_while_held_toggles(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, MIDDLE));
    assert(!wheel_tick(&state, 1000, true, true, MIDDLE | X1));
    assert(!state.scrolling);
    assert(!wheel_tick(&state, 2000, true, true, X1));
    assert(wheel_tick(&state, 3000, true, true, X1 | X2));
    assert(state.scrolling);
}

static void test_simultaneous_presses_toggle_once(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, MIDDLE | X1 | X2));
    assert(state.scrolling);
    assert(wheel_tick(&state, 1000, true, true, 0));
    assert(!wheel_tick(&state, 2000, true, true, MIDDLE | X1));
    assert(!state.scrolling);
}

static void test_one_millisecond_pacing(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 100, true, true, MIDDLE));
    for (uint64_t t = 100; t < 1100; ++t)
        assert(!wheel_tick(&state, t, true, true, MIDDLE));
    assert(wheel_tick(&state, 1100, true, true, MIDDLE));
    assert(!wheel_tick(&state, 1100, true, true, MIDDLE));
    assert(!wheel_tick(&state, 2099, true, true, MIDDLE));
    assert(wheel_tick(&state, 2100, true, true, 0));
}

static void test_maximum_interval_and_toggle_stop(void)
{
    WheelState state = armed_and_released(1000);
    assert(wheel_tick(&state, 0, true, true, X2));
    assert(!wheel_tick(&state, 1, true, true, 0));
    assert(!wheel_tick(&state, 999999, true, true, 0));
    assert(wheel_tick(&state, 1000000, true, true, 0));
    assert(!wheel_tick(&state, 1000001, true, true, X2));
    assert(!state.scrolling); /* Stop is not gated by the pulse interval. */
}

static void test_no_catchup_burst(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, MIDDLE));
    assert(wheel_tick(&state, 10000000, true, true, 0));
    for (unsigned i = 0; i < 100; ++i)
        assert(!wheel_tick(&state, 10000000, true, true, 0));
    assert(!wheel_tick(&state, 10000999, true, true, 0));
    assert(wheel_tick(&state, 10001000, true, true, 0));
}

static void test_focus_loss_disarms(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, MIDDLE));
    assert(!wheel_tick(&state, 1000, false, true, 0));
    assert(!state.armed && !state.scrolling && state.target == 0);
    assert(!wheel_tick(&state, 2000, true, true, MIDDLE));
    assert(!wheel_tick(&state, 3000, true, true, 0));
}

static void test_cursor_return_requires_release_and_press(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, MIDDLE));
    assert(!wheel_tick(&state, 1000, true, false, MIDDLE));
    assert(state.armed && !state.scrolling && state.waiting_release);
    assert(!wheel_tick(&state, 2000, true, true, MIDDLE));
    assert(!wheel_tick(&state, 3000, true, true, 0));
    assert(!wheel_tick(&state, 4000, true, true, 0));
    assert(wheel_tick(&state, 4001, true, true, X2));
}

static void test_release_outside_does_not_resume(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, X1));
    assert(!wheel_tick(&state, 1000, true, false, 0));
    assert(!wheel_tick(&state, 2000, true, false, X1));
    assert(!wheel_tick(&state, 3000, true, true, X1));
    assert(!wheel_tick(&state, 4000, true, true, 0));
    assert(!wheel_tick(&state, 5000, true, true, 0));
    assert(wheel_tick(&state, 5001, true, true, X1));
}

static void test_stop_and_rearm(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, MIDDLE));
    wheel_stop(&state);
    assert(!state.armed && !state.scrolling && !state.emitted);
    assert(state.target == 0 && state.previous_buttons == 0);
    assert(!wheel_tick(&state, 1000, true, true, MIDDLE));
    assert(wheel_arm(&state, 99, 10));
    assert(state.target == 99 && state.interval_ms == 10 && !state.scrolling);
    assert(!wheel_tick(&state, 2000, true, true, MIDDLE));
    assert(!wheel_tick(&state, 2001, true, true, 0));
    assert(wheel_tick(&state, 2002, true, true, MIDDLE));
}

static void test_home_rearm_stops_and_waits_release(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, X1));
    assert(wheel_arm(&state, 42, 1));
    assert(state.armed && !state.scrolling && state.waiting_release);
    assert(!wheel_tick(&state, 1000, true, true, X1));
    assert(!wheel_tick(&state, 2000, true, true, 0));
    assert(!wheel_tick(&state, 3000, true, true, 0));
    assert(wheel_tick(&state, 3001, true, true, MIDDLE));
}

static void test_invalid_settings_disarm(void)
{
    const unsigned invalid[] = {0, 1001, (unsigned)-1};
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 0, true, true, X1));
    assert(!wheel_arm(&state, 0, 1));
    assert(!state.armed && !state.scrolling);
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        state = armed_and_released(1);
        assert(wheel_tick(&state, 0, true, true, X1));
        assert(!wheel_arm(&state, 42, invalid[i]));
        assert(!state.armed && !state.scrolling);
    }
    assert(!wheel_tick(&state, 1000, true, true, MIDDLE));
}

static void test_clock_reset_does_not_underflow(void)
{
    WheelState state = armed_and_released(1);
    assert(wheel_tick(&state, 10000, true, true, MIDDLE));
    assert(!wheel_tick(&state, 0, true, true, 0));
    assert(!wheel_tick(&state, 999, true, true, 0));
    assert(wheel_tick(&state, 1000, true, true, 0));
}

static void test_null_state(void)
{
    assert(!wheel_arm(0, 42, 1));
    assert(!wheel_tick(0, 0, true, true, MIDDLE));
    wheel_stop(0);
}

int main(void)
{
    test_activation_only_arms();
    test_activation_requires_release();
    test_each_mouse_button_toggles();
    test_holding_does_not_retoggle();
    test_releasing_keeps_scrolling();
    test_distinct_button_while_held_toggles();
    test_simultaneous_presses_toggle_once();
    test_one_millisecond_pacing();
    test_maximum_interval_and_toggle_stop();
    test_no_catchup_burst();
    test_focus_loss_disarms();
    test_cursor_return_requires_release_and_press();
    test_release_outside_does_not_resume();
    test_stop_and_rearm();
    test_home_rearm_stops_and_waits_release();
    test_invalid_settings_disarm();
    test_clock_reset_does_not_underflow();
    test_null_state();
    puts("PASS: 18 wheel toggle state-machine test cases");
    return 0;
}
