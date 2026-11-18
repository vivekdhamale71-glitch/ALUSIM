#include "engine/lang/Clock.h"

namespace vas::core {

Clock& Clock::instance() {
    static Clock c;       // built once, the first time instance() runs
    return c;
}

void Clock::tick() { ++cycle_; }
void Clock::reset() { cycle_ = 0; }
long Clock::cycle() const { return cycle_; }

} // namespace vas::core
