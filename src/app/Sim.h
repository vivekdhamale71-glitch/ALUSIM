// =====================================================================
//  Sim - the controller shared by every view
//  Owns the compiler output, the program, the CPU and the selected
//  control unit, and plays instructions back one micro-step at a time.
//   * history  : DoublyLinkedList of snapshots -> Step Back      [PL CO2]
//   * wave     : CircularQueue (overwrite) of control-signal samples
//                -> waveform view                               [PL CO3]
//   * phaseRing: CircularLinkedList FETCH->DECODE->...->WB->FETCH
// =====================================================================
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "COA_2310221L/CO2_AssemblyISA/Assembler.h"
#include "COA_2310221L/CO1_Architecture/CPU.h"
#include "PSOOP_2310261L/CO3_ProblemSolving/Compiler.h"
#include "COA_2310221L/CO4_ControlUnitPipeline/ControlUnit.h"
#include "PSOOP_2310261L/CO1_RealWorldOOP/English.h"
#include "COA_2310221L/CO4_ControlUnitPipeline/Pipeline.h"
#include "PL_2310214L/CO2_LinkedLists/LinkedList.h"
#include "PL_2310214L/CO3_StackQueue/Queue.h"

namespace vas::ui {

struct SignalSample {
    core::ControlSignals sig;
    core::Phase phase = core::Phase::Fetch;
    std::string label;
    long cycle = 0;
    bool boundary = false;   // first micro-step of an instruction
};

struct HistoryEntry {
    core::Snapshot before;
    core::ExecRecord rec;
};

class Sim {
public:
    Sim();

    // ---- source & compilation --------------------------------------------
    std::string source;
    int inputMode = 0;             // 0 English, 1 C++, 2 assembly (see Samples.h)
    bool asmMode = false;          // == (inputMode == 2)
    core::EnglishResult english;   // English -> code translation of the last compile
    void setInputMode(int m);
    int sampleIndex = 0;
    ds::HashMethod hashMethod = ds::HashMethod::Linear;
    core::CompileResult compiled;
    bool compiledOk = false;
    int compileGen = 0;            // increments on every compile (views restart animations)
    int errorLine = -1;
    void loadSample(int i);
    bool compile();

    // ---- execution --------------------------------------------------------
    std::unique_ptr<core::Program> program;
    core::CPU cpu;
    core::Profiler profiler;
    std::unique_ptr<core::ControlUnit> cu;
    bool micro = false;
    void setMicro(bool m);

    std::vector<core::ExecRecord> trace;
    ds::DoublyLinkedList<HistoryEntry> history;
    ds::CircularQueue<SignalSample> wave{56, true};
    ds::CircularLinkedList<core::Phase> phaseRing;

    bool hasCurrent = false;
    core::ExecRecord current;
    std::vector<core::MicroStep> steps;
    int stepIdx = 0;
    float stepT = 0;               // 0..1 progress of the current micro-step
    bool running = false;
    float speed = 1.0f;
    int runGen = 0;                // increments when a run completes / state jumps

    std::uint64_t shownRegs[core::REG_COUNT] = {};
    core::Flags shownFlags;
    float regFlash[core::REG_COUNT] = {};
    float flagFlash = 0;
    std::uint64_t lastMemWrite = 0;
    float memFlash = 0;

    void reset();
    void step();                   // animate the next instruction
    void stepBack();
    void toggleRun();
    void finish();                 // run to the end without animation
    void update(float dt);
    bool loaded() const { return program != nullptr; }
    bool animating() const { return hasCurrent && stepIdx < int(steps.size()); }
    const core::MicroStep* currentStep() const;
    int currentLine() const;       // asm source line of the animated instruction

    // ---- derived data ----------------------------------------------------
    const core::PipelineResult& pipeline(bool forwarding, bool singlePort);

    // ---- messages ----------------------------------------------------------
    std::string status = "Ready - type a statement and press Compile";
    std::string toastTitle, toastBody;
    float toastTime = 0;
    bool toastError = false;
    void toast(const std::string& title, const std::string& body, bool error);

    // ---- files -------------------------------------------------------------
    bool saveSource(const std::string& path);
    bool loadSource(const std::string& path);
    bool exportTrace(const std::string& path);

private:
    bool beginNext();
    void completeStep(const core::MicroStep& m);
    void completeInstruction();
    void fail(const core::SimulatorError& e);

    core::Compiler compiler_;
    core::PipelineResult pipeCache_;
    bool pipeValid_ = false;
    bool pipeFwd_ = true, pipePort_ = false;
    std::size_t pipeTraceSize_ = 0;
    int pipeRunGen_ = -1;
};

} // namespace vas::ui
