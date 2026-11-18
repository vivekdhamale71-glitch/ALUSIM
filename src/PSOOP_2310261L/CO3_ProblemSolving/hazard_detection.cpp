// Finds read-after-write hazards between instructions and works out the stall cycles of a five-stage pipeline.

#include <string>
#include <vector>

// what the pipeline needs to know about one instruction
struct PipeInstruction {
    std::string text;
    std::vector<int> reads;       // register numbers read
    std::vector<int> writes;      // register numbers written
    bool isLoad;                  // a load gets its value only after the memory stage
    bool isTakenBranch;           // a taken branch makes the two instructions after it wrong
};

// one hazard: the consumer needs a register that the producer has not written yet
struct Hazard {
    int producer;
    int consumer;
    int registerNumber;
    int stallCycles;
};

// the stages of the pipeline
const int STAGE_IF = 0;
const int STAGE_ID = 1;
const int STAGE_EX = 2;
const int STAGE_MEM = 3;
const int STAGE_WB = 4;
const int STAGE_COUNT = 5;

// does the list contain the number?
static bool contains(const std::vector<int>& list, int number) {
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (list[i] == number) {
            return true;
        }
    }
    return false;
}

// the nearest earlier instruction that writes the register, or -1 if none does
static int lastWriter(const std::vector<PipeInstruction>& program, int before, int registerNumber) {
    for (int i = before - 1; i >= 0; --i) {
        if (contains(program[i].writes, registerNumber)) {
            return i;
        }
    }
    return -1;
}

// the cycle in which the consumer's EX stage may start, so that the value it reads is ready:
//   without forwarding the producer must have written the register back (its WB), the consumer executes one cycle later
//   with forwarding an ALU result is passed on after the producer's EX, so the consumer's EX may be in the producer's MEM cycle;
//   a loaded value exists only after the MEM stage, so it is available one cycle later (the producer's WB)
static int earliestExecute(const std::vector<int>& producer, bool producerIsLoad, bool forwarding) {
    if (!forwarding) {
        return producer[STAGE_WB] + 1;
    }
    if (producerIsLoad) {
        return producer[STAGE_WB];
    }
    return producer[STAGE_MEM];
}

// the start cycle of every stage of every instruction, one row per instruction.
// An instruction cannot enter a stage before the instruction in front of it has left it:
//   ID = max(IF + 1, EX of the previous)      EX = max(ID + 1, MEM of the previous, data ready)
//   MEM = max(EX + 1, WB of the previous)     WB = MEM + 1
std::vector<std::vector<int> > scheduleStages(const std::vector<PipeInstruction>& program, bool forwarding) {
    std::vector<std::vector<int> > start(program.size(), std::vector<int>(STAGE_COUNT, 0));
    for (int i = 0; i < static_cast<int>(program.size()); ++i) {
        int fetch = 1;
        int previousEx = 0;
        int previousMem = 0;
        int previousWb = 0;
        if (i > 0) {
            fetch = start[i - 1][STAGE_ID];                  // fetched as soon as the previous one has moved on to ID
            previousEx = start[i - 1][STAGE_EX];
            previousMem = start[i - 1][STAGE_MEM];
            previousWb = start[i - 1][STAGE_WB];
        }
        start[i][STAGE_IF] = fetch;
        int decode = fetch + 1;
        if (decode < previousEx) {
            decode = previousEx;
        }
        start[i][STAGE_ID] = decode;
        int execute = decode + 1;
        if (execute < previousMem) {
            execute = previousMem;
        }
        for (std::size_t r = 0; r < program[i].reads.size(); ++r) {
            int producer = lastWriter(program, i, program[i].reads[r]);
            if (producer >= 0) {
                int ready = earliestExecute(start[producer], program[producer].isLoad, forwarding);
                if (ready > execute) {
                    execute = ready;
                }
            }
        }
        start[i][STAGE_EX] = execute;
        int memory = execute + 1;
        if (memory < previousWb) {
            memory = previousWb;
        }
        start[i][STAGE_MEM] = memory;
        start[i][STAGE_WB] = memory + 1;
    }
    return start;
}

// every hazard: an instruction whose EX had to wait longer than the one cycle after its ID
std::vector<Hazard> findHazards(const std::vector<PipeInstruction>& program, bool forwarding) {
    std::vector<std::vector<int> > start = scheduleStages(program, forwarding);
    std::vector<Hazard> hazards;
    for (int consumer = 0; consumer < static_cast<int>(program.size()); ++consumer) {
        int stalls = start[consumer][STAGE_EX] - start[consumer][STAGE_ID] - 1;
        if (stalls <= 0) {
            continue;
        }
        for (std::size_t r = 0; r < program[consumer].reads.size(); ++r) {
            int producer = lastWriter(program, consumer, program[consumer].reads[r]);
            if (producer >= 0 && earliestExecute(start[producer], program[producer].isLoad, forwarding) >= start[consumer][STAGE_EX]) {
                Hazard hazard;
                hazard.producer = producer;
                hazard.consumer = consumer;
                hazard.registerNumber = program[consumer].reads[r];
                hazard.stallCycles = stalls;
                hazards.push_back(hazard);
                break;
            }
        }
    }
    return hazards;
}

// the total number of cycles: the last instruction's write-back cycle
int totalCycles(const std::vector<std::vector<int> >& start) {
    if (start.empty()) {
        return 0;
    }
    return start.back()[STAGE_WB];
}

// the speed-up over a machine that finishes each instruction (five cycles) before it starts the next
double speedUp(const std::vector<std::vector<int> >& start) {
    if (start.empty()) {
        return 0.0;
    }
    return static_cast<double>(STAGE_COUNT * start.size()) / static_cast<double>(totalCycles(start));
}
