#ifndef WHEEL_LOGIC_H
#define WHEEL_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

/* Initialize to zero before first use. Call only from one thread. */
typedef struct WheelState {
    bool armed;
    bool scrolling;
    bool waiting_release;
    uint32_t previous_buttons;
    uintptr_t target;
    unsigned interval_ms;
    uint64_t last_emit_us;
    bool emitted;
} WheelState;

/* A failed arm request also clears any previous armed state. */
bool wheel_arm(WheelState *state, uintptr_t target, unsigned interval_ms);
void wheel_stop(WheelState *state);

/*
 * Return true for one wheel pulse. No input is emitted by this module.
 * Home/arm only activates: a rising mouse-button edge toggles scrolling.
 * buttons is a bit mask (middle = 1, X1 = 2, X2 = 4); the caller may filter it.
 * Holding or releasing a button does not toggle again. Simultaneous presses
 * toggle only once. A lost foreground window disarms. Leaving the target stops
 * scrolling and requires release followed by a new press to resume. Delayed
 * ticks never catch up with a burst. Supply a monotonic clock in microseconds.
 */
bool wheel_tick(WheelState *state, uint64_t now_us,
                bool foreground_matches, bool cursor_matches, uint32_t buttons);

#endif
