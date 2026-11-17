#include "app/Sim.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "app/Samples.h"

namespace vas::ui {

using namespace core;

Sim::Sim() {
    cu = std::make_unique<HardwiredControlUnit>();
    cpu.addObserver(&profiler);
    for (Phase p : {Phase::Fetch, Phase::Decode, Phase::Execute, Phase::Memory, Phase::WriteBack}) phaseRing.insert(p);
    loadSample(0);
}

void Sim::loadSample(int i) {
    const auto& s = samples();
    sampleIndex = (i % int(s.size()) + int(s.size())) % int(s.size());
    source = s[sampleIndex].code;
    inputMode = s[sampleIndex].mode;
    asmMode = inputMode == MODE_ASM;
}

void Sim::setInputMode(int m) {
    inputMode = m;
    asmMode = m == MODE_ASM;
}

void Sim::setMicro(bool m) {
    if (m == micro && cu) return;
    micro = m;
    if (m) cu = std::make_unique<MicroprogrammedControlUnit>();
    else cu = std::make_unique<HardwiredControlUnit>();
    // re-sequence the instruction on screen with the new control unit
    if (hasCurrent && program) {
        steps = cu->sequence(*program->code[current.index], current);
        stepIdx = std::min(stepIdx, int(steps.size()));
        if (stepIdx >= int(steps.size())) stepIdx = int(steps.size());
    }
}

bool Sim::compile() {
    running = false;
    hasCurrent = false;
    errorLine = -1;
    ++compileGen;
    try {
        if (asmMode) {
            compiled = CompileResult{};
            compiledOk = false;
            program = Assembler::assemble(source, hashMethod);
        } else {
            std::string code = source;
            english = core::EnglishResult{};
            if (inputMode == MODE_ENGLISH) {
                // step 1: plain English -> statements
                english = core::EnglishTranslator::translate(source);
                if (!english.ok) {
                    compiled = CompileResult{};
                    compiledOk = false;
                    errorLine = english.errorLine;
                    program.reset();
                    cpu.load(nullptr);
                    reset();
                    toast("Could not understand the sentence", english.error, true);
                    status = "English error - " + english.error;
                    return false;
                }
                code = english.code;
            }
            compiled = compiler_.compile(code, hashMethod);
            if (!compiled.ok && inputMode == MODE_ENGLISH && compiled.errorLine >= 0) {
                // map the generated-code line back to the English sentence
                int line = 0;
                for (const auto& s : english.sentences) {
                    line += int(s.code.size());
                    if (compiled.errorLine < line) { compiled.errorLine = s.line; break; }
                }
            }
            if (!compiled.ok) {
                compiledOk = false;
                errorLine = compiled.errorLine;
                program.reset();
                cpu.load(nullptr);
                reset();
                toast(compiled.errorKind, compiled.error, true);
                status = "Compile failed - " + compiled.error;
                return false;
            }
            compiledOk = true;
            program = Assembler::assemble(compiled.asmText, hashMethod);
        }
    } catch (const SimulatorError& e) {
        program.reset();
        cpu.load(nullptr);
        reset();
        errorLine = e.line();
        toast(e.kind(), e.what(), true);
        status = std::string("Assembly failed - ") + e.what();
        return false;
    }
    cpu.load(program.get());
    reset();
    std::ostringstream os;
    os << (asmMode ? "Assembled " : "Compiled ") << program->code.size() << " instructions, " << program->data.size()
       << " variable" << (program->data.size() == 1 ? "" : "s") << (compiled.cacheHit ? "  (translation cache hit)" : "")
       << " - press Step or Run";
    status = os.str();
    return true;
}

void Sim::reset() {
    cpu.reset();
    trace.clear();
    history.clear();
    wave.clear();
    hasCurrent = false;
    running = false;
    steps.clear();
    stepIdx = 0;
    stepT = 0;
    for (int i = 0; i < REG_COUNT; ++i) { shownRegs[i] = cpu.reg(i).u(); regFlash[i] = 0; }
    shownFlags = cpu.flags();
    Clock::instance().reset();
    ++runGen;
    pipeValid_ = false;
    if (program) status = "Reset - RIP points to the first instruction";
}

bool Sim::beginNext() {
    if (!program) return false;
    if (cpu.halted()) {
        running = false;
        status = "Program finished (HLT) - press Reset to run again";
        return false;
    }
    HistoryEntry e;
    e.before = cpu.snapshot();
    try {
        e.rec = cpu.step();
    } catch (const SimulatorError& ex) {
        fail(ex);
        return false;
    }
    if (e.rec.index < 0) {
        running = false;
        status = "Program finished";
        return false;
    }
    history.pushBack(e);
    trace.push_back(e.rec);
    current = e.rec;
    steps = cu->sequence(*program->code[current.index], current);
    stepIdx = 0;
    stepT = 0;
    hasCurrent = true;
    status = "Executing  " + current.text;
    return true;
}

void Sim::completeStep(const MicroStep& m) {
    Clock::instance().tick();
    SignalSample s;
    s.sig = m.sig;
    s.phase = m.phase;
    s.label = current.mnemonic;
    s.cycle = Clock::instance().cycle();
    s.boundary = stepIdx == 0;
    wave.enqueue(s);
    if (m.sig.RegWrite || m.phase == Phase::WriteBack) {
        for (int r : current.regsWritten) {
            if (shownRegs[r] != current.regAfter[r]) regFlash[r] = 1;
            shownRegs[r] = current.regAfter[r];
        }
    }
    if (m.sig.FlagWrite) {
        if (shownFlags != current.flagsAfter) flagFlash = 1;
        shownFlags = current.flagsAfter;
    }
    if (m.sig.MemWrite)
        for (const auto& a : current.mem)
            if (a.write) { lastMemWrite = a.addr; memFlash = 1; }
}

void Sim::completeInstruction() {
    for (int r = 0; r < REG_COUNT; ++r) {
        if (shownRegs[r] != current.regAfter[r]) regFlash[r] = 1;
        shownRegs[r] = current.regAfter[r];
    }
    if (shownFlags != current.flagsAfter) flagFlash = 1;
    shownFlags = current.flagsAfter;
    if (cpu.halted()) {
        running = false;
        status = "Program finished (HLT) after " + std::to_string(cpu.steps()) + " instructions";
        ++runGen;
    }
}

void Sim::step() {
    if (!program) {
        toast("Nothing to run", "Compile a statement (or assemble a program) first.", true);
        return;
    }
    running = false;
    if (animating()) {
        while (animating()) { completeStep(steps[stepIdx]); ++stepIdx; }
        completeInstruction();
    }
    beginNext();
}

void Sim::stepBack() {
    running = false;
    if (history.empty()) {
        status = "Already at the first instruction";
        return;
    }
    auto* tail = history.tail();
    cpu.restore(tail->value.before);
    if (!trace.empty()) trace.pop_back();
    history.truncateAfter(tail->prev);
    hasCurrent = false;
    steps.clear();
    for (int i = 0; i < REG_COUNT; ++i) {
        if (shownRegs[i] != cpu.reg(i).u()) regFlash[i] = 1;
        shownRegs[i] = cpu.reg(i).u();
    }
    shownFlags = cpu.flags();
    ++runGen;
    status = "Stepped back - RIP = instruction #" + std::to_string(cpu.pc());
}

void Sim::toggleRun() {
    if (!program) {
        if (!compile()) return;
    }
    if (cpu.halted() && !animating()) reset();
    running = !running;
    if (running && !animating()) beginNext();
    status = running ? "Running..." : "Paused";
}

void Sim::finish() {
    if (!program) return;
    running = false;
    if (animating()) {
        while (animating()) { completeStep(steps[stepIdx]); ++stepIdx; }
        completeInstruction();
    }
    while (!cpu.halted()) {
        HistoryEntry e;
        e.before = cpu.snapshot();
        try {
            e.rec = cpu.step();
        } catch (const SimulatorError& ex) {
            fail(ex);
            break;
        }
        if (e.rec.index < 0) break;
        history.pushBack(e);
        trace.push_back(e.rec);
        current = e.rec;
        hasCurrent = true;
        Clock::instance().tick();
    }
    if (hasCurrent) {
        steps = cu->sequence(*program->code[current.index], current);
        stepIdx = int(steps.size());
    }
    for (int i = 0; i < REG_COUNT; ++i) {
        if (shownRegs[i] != cpu.reg(i).u()) regFlash[i] = 1;
        shownRegs[i] = cpu.reg(i).u();
    }
    shownFlags = cpu.flags();
    ++runGen;
    if (cpu.halted()) status = "Program finished - " + std::to_string(cpu.steps()) + " instructions executed";
}

void Sim::update(float dt) {
    toastTime -= dt;
    for (auto& f : regFlash) f = std::max(0.f, f - dt * 0.9f);
    flagFlash = std::max(0.f, flagFlash - dt * 0.9f);
    memFlash = std::max(0.f, memFlash - dt * 0.9f);

    if (animating()) {
        float dur = std::max(0.02f, 0.62f / speed);
        stepT += dt / dur;
        while (stepT >= 1 && animating()) {
            stepT -= 1;
            completeStep(steps[stepIdx]);
            ++stepIdx;
            if (!animating()) {
                completeInstruction();
                if (running) {
                    if (!beginNext()) running = false;
                } else {
                    stepT = 0;
                }
                break;
            }
        }
    } else if (running) {
        if (!beginNext()) running = false;
    }
}

const MicroStep* Sim::currentStep() const {
    if (!hasCurrent || steps.empty()) return nullptr;
    if (stepIdx < int(steps.size())) return &steps[stepIdx];
    return &steps.back();
}

int Sim::currentLine() const {
    if (!program) return -1;
    if (hasCurrent && current.index >= 0 && current.index < int(program->instrLine.size()))
        return program->instrLine[current.index];
    return cpu.currentLine();
}

const PipelineResult& Sim::pipeline(bool fwd, bool port) {
    if (!pipeValid_ || fwd != pipeFwd_ || port != pipePort_ || trace.size() != pipeTraceSize_ || runGen != pipeRunGen_) {
        PipelineConfig cfg;
        cfg.forwarding = fwd;
        cfg.singleMemoryPort = port;
        cfg.maxRows = 36;
        if (program) pipeCache_ = PipelineSimulator::run(*program, trace, cfg);
        else pipeCache_ = PipelineResult{};
        pipeFwd_ = fwd;
        pipePort_ = port;
        pipeTraceSize_ = trace.size();
        pipeRunGen_ = runGen;
        pipeValid_ = true;
    }
    return pipeCache_;
}

void Sim::toast(const std::string& title, const std::string& body, bool error) {
    toastTitle = title;
    toastBody = body;
    toastError = error;
    toastTime = 5.0f;
}

void Sim::fail(const SimulatorError& e) {
    running = false;
    errorLine = e.line();
    toast(e.kind(), e.what(), true);
    status = "Stopped - " + e.kind();
}

bool Sim::saveSource(const std::string& path) {
    std::ofstream f(path);
    if (!f) return false;
    f << source;
    return bool(f);
}

bool Sim::loadSource(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::stringstream ss;
    ss << f.rdbuf();
    source = ss.str();
    if (source.find("section") != std::string::npos || source.find("_start") != std::string::npos) setInputMode(MODE_ASM);
    else if (source.find(';') != std::string::npos || source.find('{') != std::string::npos) setInputMode(MODE_CPP);
    else setInputMode(MODE_ENGLISH);
    return true;
}

bool Sim::exportTrace(const std::string& path) {
    std::ofstream f(path);
    if (!f) return false;
    f << "Virtual ALU Simulator - execution trace\n";
    f << "Control unit: " << cu->name() << "\n\n";
    if (program) {
        f << "Program:\n";
        for (const auto& l : program->lines) f << "    " << l << "\n";
        f << "\n";
    }
    for (const auto& r : trace) f << Debugger::describe(r) << "\n";
    f << "\nFinal state:\n" << Debugger::dumpRegisters(cpu);
    if (program)
        for (const auto& d : program->data) f << "    " << d.name << " = " << cpu.peek(d.addr) << "\n";
    return bool(f);
}

} // namespace vas::ui
