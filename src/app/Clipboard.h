// =====================================================================
//  System clipboard (paste)
//  GLUT has no clipboard API, so the text is fetched from the OS:
//    Linux    wl-paste / xclip / xsel when installed, otherwise a direct
//             X11 request for the CLIPBOARD selection (works under XWayland)
//    Windows  GetClipboardData(CF_UNICODETEXT)
//  Returns "" if the clipboard is empty, not text, or the owner does not
//  answer within about half a second.
// =====================================================================
#pragma once
#include <string>

namespace vas::ui {

std::string clipboardText();

// cleans pasted text for the code editor: CR/LF -> LF, tab -> 4 spaces,
// drops other control and non-ASCII characters, caps the length
std::string sanitizePaste(const std::string& raw, std::size_t maxLen = 100000);

} // namespace vas::ui
