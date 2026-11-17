// =====================================================================
//  Control unit - generation of control signals          [COA CO4]
//  Two organisations behind one abstract interface        [PSOOP]
//   * HardwiredControlUnit      - combinational decoder: the opcode
//     selects a row of the decoder table, phases follow FETCH / DECODE
//     / EXECUTE / MEMORY / WRITE-BACK.
//   * MicroprogrammedControlUnit - a control store (ROM) of micro-
//     instructions, a micro-PC and a dispatch ROM (HashMap) that maps
//     the decoded instruction to the start of its micro-routine.
//  Both produce a list of MicroSteps: which buses carry data, which
//  units are active and the full control-signal vector for each cycle.
//  The datapath view, waveform and 3D chip are driven from these.
// =====================================================================
#pragma once
#include <string>
#include <vector>

#include "PL_2310214L/CO5_Hashing/HashMap.h"
#include "COA_2310221L/CO2_AssemblyISA/Instruction.h"

namespace vas::core {

enum class Phase { Fetch, Decode, Execute, Memory, WriteBack };
const char* phaseName(Phase p);

// datapath wires (see DatapathView for the drawing)
enum Bus {
    B_PC_IMEM, B_IMEM_IR, B_PC_INC, B_INC_PC, B_IR_CU, B_IR_REGS, B_IR_IMM,
    B_REGA_ALU, B_REGB_MUX, B_IMM_MUX, B_MUX_ALU, B_ALU_FLAGS, B_ALU_DMEM,
    B_REGB_DMEM, B_DMEM_WB, B_ALU_WB, B_WB_REGS, B_FLAGS_CU, B_IR_PC, B_DMEM_MUX, BUS_COUNT
};

// datapath functional units
enum Unit { U_PC, U_IMEM, U_IR, U_CU, U_REGS, U_IMM, U_MUX, U_ALU, U_FLAGS, U_DMEM, U_WB, UNIT_COUNT };
const char* unitName(int u);

struct MicroStep {
    Phase phase = Phase::Fetch;
    std::string microOp;           // register-transfer notation, e.g. "IR <- IMEM[RIP]"
    std::string detail;            // plain-English explanation
    ControlSignals sig;
    std::vector<int> buses;
    unsigned units = 0;            // bit mask of Unit
    std::string value;             // what travels on the buses
    int microAddr = -1;            // microprogrammed: address in the control store
    int nextAddr = -1;
};

struct MicroInstr {
    int addr;
    std::string label;
    std::string microOp;
    Phase phase;
    ControlSignals sig;
    std::vector<int> buses;
    unsigned units;
    std::string next;              // "next", "FETCH", "DISPATCH", "HALT"
};

class ControlUnit {
public:
    virtual ~ControlUnit() = default;
    virtual std::string name() const = 0;
    virtual std::vector<MicroStep> sequence(const Instruction& ins, const ExecRecord& rec) const = 0;
};

struct DecoderRow {
    std::string form;
    ControlSignals sig;
};

class HardwiredControlUnit : public ControlUnit {
public:
    std::string name() const override { return "Hardwired"; }
    std::vector<MicroStep> sequence(const Instruction& ins, const ExecRecord& rec) const override;
    static const std::vector<DecoderRow>& decoderTable();
    static int decoderRow(const Instruction& ins);
};

class MicroprogrammedControlUnit : public ControlUnit {
public:
    MicroprogrammedControlUnit();
    std::string name() const override { return "Microprogrammed"; }
    std::vector<MicroStep> sequence(const Instruction& ins, const ExecRecord& rec) const override;
    const std::vector<MicroInstr>& controlStore() const { return store_; }
    static std::string routineKey(const Instruction& ins);
    int dispatch(const Instruction& ins) const;

private:
    void add(const std::string& label, const std::string& op, Phase ph, ControlSignals sig, std::vector<int> buses,
             unsigned units, const std::string& next = "next");
    std::vector<MicroInstr> store_;
    ds::HashMap<std::string, int> dispatchRom_;   // routine key -> micro address
};

std::string fmtValue(std::int64_t v);

} // namespace vas::core
