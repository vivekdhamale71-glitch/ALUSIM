// =====================================================================
//  Design patterns used around the CPU                     [PSOOP CO4]
//   * Debugger  - a FRIEND class of CPU          (Debugger.h)
//   * Profiler  - OBSERVER of the CPU            (Profiler.h)
//   * Clock     - SINGLETON cycle counter        (Clock.h)
//  Elsewhere: Factory (InstructionFactory, core/Instruction.cpp),
//  Strategy (control units, hash methods, sorters), Command / undo
//  (Step Back history, app/Sim.cpp), static member
//  (Instruction::executedCount).
// =====================================================================
#pragma once
#include "PSOOP_2310261L/CO4_LogicBuilding/Debugger.h"
#include "PSOOP_2310261L/CO4_LogicBuilding/Profiler.h"
#include "PSOOP_2310261L/CO4_LogicBuilding/Clock.h"
