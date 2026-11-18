// A fixed array of 16 registers with indexed and checked access, plus the 32, 16 and 8-bit views of one register.

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

const int REGISTER_COUNT = 16;

// the names of the 16 registers, in index order
const char* const REGISTER_NAMES[REGISTER_COUNT] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
                                                    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"};

class RegisterFile {
public:
    // all registers start at zero
    RegisterFile() {
        clear();
    }

    // sets every element of the array to zero (one pass over the array: O(16))
    void clear() {
        for (int i = 0; i < REGISTER_COUNT; ++i) {
            values_[i] = 0;
        }
    }

    // fast access: the address of element i is  start + i * 8 bytes, no checking
    std::uint64_t read(int index) const {
        return values_[index];
    }

    void write(int index, std::uint64_t value) {
        values_[index] = value;
    }

    // safe access: an index outside 0..15 is an error, not silent memory corruption
    std::uint64_t checkedRead(int index) const {
        if (index < 0 || index >= REGISTER_COUNT) {
            throw std::out_of_range("register index " + std::to_string(index) + " is outside 0..15");
        }
        return values_[index];
    }

    // finds the index of a register by name (linear search through the array)
    int indexOf(const std::string& name) const {
        for (int i = 0; i < REGISTER_COUNT; ++i) {
            if (name == REGISTER_NAMES[i]) {
                return i;
            }
        }
        return -1;
    }

private:
    std::uint64_t values_[REGISTER_COUNT];
};

// the 8086 -> 80386 -> x86-64 views of one register: the same 64 bits, read in pieces
void printSubRegisters(std::uint64_t rax) {
    std::uint32_t eax = static_cast<std::uint32_t>(rax);          // low 32 bits
    std::uint16_t ax = static_cast<std::uint16_t>(rax);           // low 16 bits
    std::uint8_t ah = static_cast<std::uint8_t>(rax >> 8);        // bits 8..15
    std::uint8_t al = static_cast<std::uint8_t>(rax);             // bits 0..7
    std::cout << std::hex << "  RAX = 0x" << rax << "\n  EAX = 0x" << eax << "\n  AX  = 0x" << ax
              << "\n  AH  = 0x" << static_cast<int>(ah) << "   AL = 0x" << static_cast<int>(al) << std::dec << "\n";
}

// prints all 16 elements, four per line
