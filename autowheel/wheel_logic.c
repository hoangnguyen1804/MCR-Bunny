#include "wheel_logic.h"

void wheel_stop(WheelState *state)
{
    if (state != 0) {
        *state = (WheelState){0};
    }
}

bool wheel_arm(WheelState *state, uintptr_t target, unsigned interval_ms)
{
    if (state == 0) {
        return false;
    }
    wheel_stop(state);
    if (target == 0 || interval_ms < 1 || interval_ms > 1000) {
        return false;
    }
    state->armed = true;
    state->waiting_release = true;
    state->target = target;
    state->interval_ms = interval_ms;
    return true;
}

bool wheel_tick(WheelState *state, uint64_t now_us,
                bool foreground_matches, bool cursor_matches, bool held)
{
    if (state == 0 || !state->armed) {
        return false;
    }
    if (!foreground_matches) {
        wheel_stop(state);
        return false;
    }
    if (!cursor_matches) {
        state->waiting_release = true;
        state->emitted = false;
        return false;
    }
    if (state->waiting_release) {
        if (!held) {
            state->waiting_release = false;
        }
        return false;
    }
    if (!held) {
        state->emitted = false;
        return false;
    }
    if (state->emitted) {
        /* Prevent unsigned subtraction underflow if the caller's clock resets. */
        if (now_us < state->last_emit_us) {
            state->last_emit_us = now_us;
            return false;
        }
        if (now_us - state->last_emit_us < (uint64_t)state->interval_ms * 1000) {
            return false;
        }
    }
    state->last_emit_us = now_us;
    state->emitted = true;
    return true;
}
