#ifndef WHEEL_LOGIC_H
#define WHEEL_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

/* Initialize to zero before first use. Call only from one thread. */
typedef struct WheelState {
    bool armed;
    bool waiting_release;
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
 * A lost foreground window disarms. Leaving the target with the cursor pauses
 * and requires release before another hold can emit. Delayed ticks never catch
 * up with a burst of pulses. Supply a monotonic clock in microseconds.
 */
bool wheel_tick(WheelState *state, uint64_t now_us,
                bool foreground_matches, bool cursor_matches, bool held);

#endif
