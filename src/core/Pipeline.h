// =====================================================================
//  Instruction pipelining                                 [COA CO4]
//  Classic in-order 5-stage pipeline  IF  ID  EX  MEM  WB  computed
//  from the dynamic instruction trace of a real run:
//   * data hazards (RAW)  - stall until write-back, or with forwarding
//                           only the load-use case stalls one cycle
//   * control hazards     - a taken branch is resolved in EX, the two
//                           wrongly fetched instructions are flushed
//   * structural hazard   - optional single memory port: an IF cannot
//                           happen in the same cycle as a MEM access
//  The non-pipelined machine needs 5 cycles per instruction, so the
//  view can show cycles, CPI and speed-up side by side.
// =====================================================================
#pragma once
#include <string>
#include <vector>

#include "core/Instruction.h"

namespace vas::core {

enum PipeStage { S_IF, S_ID, S_EX, S_MEM, S_WB, STAGE_COUNT };
const char* stageName(int s);

struct Forward {
    int from;          // producer row
    int to;            // consumer row
    int fromStage;     // S_EX (ALU result) or S_MEM (load data)
    int cycle;         // cycle in which the value is forwarded into EX
    std::string reg;
};

struct PipeRow {
    std::string text;
    int instrIndex = -1;
    int start[STAGE_COUNT] = {};   // first cycle in each stage
    int end[STAGE_COUNT] = {};     // last cycle in each stage (start..end)
    bool flushed = false;          // wrong-path instruction squashed by a taken branch
    int flushCycle = -1;
    bool branchTaken = false;
    std::string hazard;            // explanation of the stall, if any
    int stallCycles = 0;
};

struct PipelineConfig {
    bool forwarding = true;
    bool singleMemoryPort = false;
    int maxRows = 40;
};

struct PipelineResult {
    std::vector<PipeRow> rows;
    std::vector<Forward> forwards;
    int instructions = 0;          // committed instructions (not flushed)
    int pipelinedCycles = 0;
    int nonPipelinedCycles = 0;
    int stalls = 0, flushes = 0, forwardsUsed = 0, structural = 0;
    double cpi() const { return instructions ? double(pipelinedCycles) / instructions : 0.0; }
    double speedup() const { return pipelinedCycles ? double(nonPipelinedCycles) / pipelinedCycles : 0.0; }
};

class PipelineSimulator {
public:
    static PipelineResult run(const Program& prog, const std::vector<ExecRecord>& trace, const PipelineConfig& cfg);
};

} // namespace vas::core
