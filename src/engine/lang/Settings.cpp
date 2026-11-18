#include "engine/lang/Settings.h"

#include <fstream>
#include <sstream>

namespace vas::core {

bool SettingsValues::operator==(const SettingsValues& o) const {
    return lightTheme == o.lightTheme && speed == o.speed && microprogrammed == o.microprogrammed &&
           hashMethod == o.hashMethod && inputMode == o.inputMode && windowW == o.windowW && windowH == o.windowH;
}

Settings& Settings::instance() {
    static Settings s;          // the only Settings object, created on first use
    return s;
}

// file format: one "key = value" per line; unknown keys are ignored so an
// old file still loads after a new setting is added
bool Settings::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        std::size_t eq = line.find('=');
        if (line.empty() || line[0] == '#' || eq == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, eq), val = line.substr(eq + 1);
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t"));
            s.erase(s.find_last_not_of(" \t\r") + 1);
        };
        trim(key);
        trim(val);
        std::istringstream v(val);
        if (key == "theme") {
            values.lightTheme = val == "light";
        } else if (key == "speed") {
            v >> values.speed;
        } else if (key == "control_unit") {
            values.microprogrammed = val == "microprogrammed";
        } else if (key == "hash_method") {
            v >> values.hashMethod;
        } else if (key == "input_mode") {
            v >> values.inputMode;
        } else if (key == "window_width") {
            v >> values.windowW;
        } else if (key == "window_height") {
            v >> values.windowH;
        }
    }
    return true;
}

bool Settings::save(const std::string& path) const {
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out << "# Virtual ALU Simulator settings (written automatically)\n"
        << "theme = " << (values.lightTheme ? "light" : "dark") << "\n"
        << "speed = " << values.speed << "\n"
        << "control_unit = " << (values.microprogrammed ? "microprogrammed" : "hardwired") << "\n"
        << "hash_method = " << values.hashMethod << "\n"
        << "input_mode = " << values.inputMode << "\n"
        << "window_width = " << values.windowW << "\n"
        << "window_height = " << values.windowH << "\n";
    return bool(out);
}

} // namespace vas::core
