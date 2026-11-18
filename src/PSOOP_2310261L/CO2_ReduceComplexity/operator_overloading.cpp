// Registers and vectors with overloaded operators, so the code reads like the maths.

#include <cmath>
#include <cstdint>
#include <string>

// A 64-bit register. The operators are overloaded so the ALU code reads like the
// hardware equations:  result = a + b,  carry = result < a.
class Register64 {
public:
    Register64(std::uint64_t value = 0) : value_(value) {
    }

    std::uint64_t value() const {
        return value_;
    }

    // the sign bit
    bool negative() const {
        return (value_ >> 63) != 0;
    }

    Register64 operator+(const Register64& other) const {
        return Register64(value_ + other.value_);
    }

    Register64 operator-(const Register64& other) const {
        return Register64(value_ - other.value_);
    }

    Register64 operator&(const Register64& other) const {
        return Register64(value_ & other.value_);
    }

    Register64 operator|(const Register64& other) const {
        return Register64(value_ | other.value_);
    }

    Register64 operator^(const Register64& other) const {
        return Register64(value_ ^ other.value_);
    }

    Register64 operator~() const {
        return Register64(~value_);
    }

    Register64 operator<<(unsigned count) const {
        if (count >= 64) {
            return Register64(0);
        }
        return Register64(value_ << count);
    }

    bool operator==(const Register64& other) const {
        return value_ == other.value_;
    }

    // unsigned comparison: used to find out whether an addition carried
    bool operator<(const Register64& other) const {
        return value_ < other.value_;
    }

private:
    std::uint64_t value_;
};

// the carry flag of an addition, written with the overloaded operators
bool addCarries(const Register64& a, const Register64& b) {
    Register64 sum = a + b;
    return sum < a;
}

// the overflow flag of an addition: both operands have the same sign, the sum a different one
bool addOverflows(const Register64& a, const Register64& b) {
    Register64 sum = a + b;
    return ((a ^ sum) & (b ^ sum)).negative();
}

// A point or direction on the screen.
class Vec2 {
public:
    Vec2(float x = 0, float y = 0) : x_(x), y_(y) {
    }

    float x() const {
        return x_;
    }

    float y() const {
        return y_;
    }

    Vec2 operator+(const Vec2& other) const {
        return Vec2(x_ + other.x_, y_ + other.y_);
    }

    Vec2 operator-(const Vec2& other) const {
        return Vec2(x_ - other.x_, y_ - other.y_);
    }

    // scaling by a number
    Vec2 operator*(float factor) const {
        return Vec2(x_ * factor, y_ * factor);
    }

    float length() const {
        return std::sqrt(x_ * x_ + y_ * y_);
    }

private:
    float x_;
    float y_;
};

// the point a fraction t of the way from a to b: reads like the maths  a + (b - a) * t
Vec2 interpolate(const Vec2& a, const Vec2& b, float t) {
    return a + (b - a) * t;
}
