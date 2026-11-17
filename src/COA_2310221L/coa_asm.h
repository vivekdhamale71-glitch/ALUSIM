// =====================================================================
//  coa_asm.h - the COA assembly functions, as C++ sees them
//  The bodies are NASM x86-64 in the CO folders and are linked into the
//  simulator; the C++ core calls them for every ALU operation, branch,
//  division, arithmetic trace, decoded instruction and pipeline stage.
// =====================================================================
#pragma once
#include <cstdint>

extern "C" {

// CO1  CO1_Architecture/alu.asm
std::uint64_t vas_alu(int op, std::uint64_t a, std::uint64_t b, std::uint64_t* rflags);

// CO2  CO2_AssemblyISA/isa.asm
int vas_condition(int cc, std::uint64_t rflags);
int vas_idiv(std::uint64_t rdx, std::uint64_t rax, std::int64_t divisor, std::int64_t out[2]);
std::int64_t vas_cqo(std::int64_t rax);

// CO3  CO3_ComputerArithmetic/arithmetic.asm
struct ArithStepRec {
    std::uint64_t A, Q, extra, kind;
};
enum ArithStepKind { STEP_INIT, STEP_SUB, STEP_ADD, STEP_NOP, STEP_SHR, STEP_SHL, STEP_RESTORE, STEP_SETQ };
int vas_booth(std::int64_t m, std::int64_t q, int n, ArithStepRec* out);
int vas_add_shift(std::uint64_t m, std::uint64_t q, int n, ArithStepRec* out);
int vas_restoring_div(std::uint64_t q, std::uint64_t m, int n, ArithStepRec* out);
std::uint64_t vas_bin_to_bcd(std::uint64_t v);
std::uint64_t vas_bcd_to_bin(std::uint64_t bcd);

// CO4  CO4_ControlUnitPipeline/control_unit.asm
std::uint32_t vas_decode(int form);
int vas_decode_rows(void);
int vas_operand_ready(int mem, int wb, int forwarding, int load);
int vas_schedule(const int prev[5], int fetch, int ready, int out[5]);

}
