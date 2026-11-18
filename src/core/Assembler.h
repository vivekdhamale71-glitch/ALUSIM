// =====================================================================
//  Two-pass assembler                                 [COA CO2]
//  Pass 1: data symbols get addresses, labels go into a HashTable
//          (method chosen by the user, probes recorded)   [PL CO5]
//  Pass 2: each line is validated against the sorted opcode table with
//          binary search [PL CO4], operands are parsed (register names
//          through a HashMap) and the InstructionFactory builds the
//          polymorphic instruction objects.                  [PSOOP]
// =====================================================================
#pragma once
#include <memory>
#include <string>

#include "engine/ds/HashTable.h"
#include "core/Instruction.h"

namespace vas::core {

class Assembler {
public:
    // throws SyntaxError / InvalidOpcodeError with the offending line
    static std::unique_ptr<Program> assemble(const std::string& source,
                                             ds::HashMethod labelMethod = ds::HashMethod::Chaining);
    static int registerIndex(const std::string& name);   // -1 if not a register
};

} // namespace vas::core
