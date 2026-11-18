// =====================================================================
//  Settings  -  Singleton + file handling                  [PSOOP CO4]
//  The user's preferences: theme, speed, control unit, hash method,
//  input mode and window size. Every screen reads them and the app
//  writes them back to settings.cfg whenever one changes, so they are
//  restored on the next start.
//  Why a singleton: there must be exactly one set of preferences. Two
//  copies would disagree (one screen dark, another light) and would
//  overwrite each other's file. The constructor is private, copying is
//  deleted, and instance() hands out the single object.
// =====================================================================
#pragma once
#include <string>

namespace vas::core {

struct SettingsValues {
    bool lightTheme = false;
    float speed = 1.0f;
    bool microprogrammed = false;
    int hashMethod = 0;            // 0 linear, 1 quadratic, 2 chaining
    int inputMode = 0;             // 0 English, 1 C++, 2 assembly
    int windowW = 0, windowH = 0;  // 0 = choose from the screen size

    bool operator==(const SettingsValues& o) const;
    bool operator!=(const SettingsValues& o) const { return !(*this == o); }
};

class Settings {
public:
    static Settings& instance();
    static constexpr const char* kFile = "settings.cfg";

    bool load(const std::string& path = kFile);         // false if there is no file yet
    bool save(const std::string& path = kFile) const;

    SettingsValues values;

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

private:
    Settings() = default;
};

} // namespace vas::core
