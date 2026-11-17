// =====================================================================
//  Clock  -  Singleton                                     [PSOOP CO4]
//  Exactly one clock-cycle counter for the whole program: the
//  constructor is private, copying is deleted, and instance() returns
//  the single object (created on first use).
//  Ticked by app/Sim.cpp on every micro-step, shown in the top bar.
// =====================================================================
#pragma once

namespace vas::core {

class Clock {
public:
    static Clock& instance();
    void tick();
    void reset();
    long cycle() const;

    Clock(const Clock&) = delete;
    Clock& operator=(const Clock&) = delete;

private:
    Clock() = default;
    long cycle_ = 0;
};

} // namespace vas::core
