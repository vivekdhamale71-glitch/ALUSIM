// =====================================================================
//  Design patterns used around the CPU                     [PSOOP CO4]
//   * Debugger  - a FRIEND class of CPU          (Debugger.h)
//   * Profiler  - OBSERVER of the CPU            (Profiler.h)
//   * Clock     - SINGLETON cycle counter        (Clock.h)
//   * Settings  - SINGLETON preferences + file   (Settings.h)
//  Elsewhere: Factory (InstructionFactory, core/Instruction.cpp),
//  Strategy (control units, hash methods, sorters), Command / undo
//  (Step Back history, app/Sim.cpp), static member
//  (Instruction::executedCount).
// =====================================================================
#pragma once
#include "engine/lang/Debugger.h"
#include "engine/lang/Profiler.h"
#include "engine/lang/Clock.h"
#include "engine/lang/Settings.h"
