#ifndef FILE_SEEN_MUTIL
#define FILE_SEEN_MUTIL

#include <optional>
#include <filesystem>
#include <string>
#include "./Herix/src/herix.hpp"

enum class MColors {
    DEFAULT=0,
    BLACK_BLACK=1,
    BLACK_RED=2,
    BLACK_GREEN=3,
    BLACK_YELLOW=4,
    BLACK_BLUE=5,
    BLACK_MAGENTA=6,
    BLACK_CYAN=7,
    BLACK_WHITE=8,
    RED_BLACK=9,
    RED_RED=10,
    RED_GREEN=11,
    RED_YELLOW=12,
    RED_BLUE=13,
    RED_MAGENTA=14,
    RED_CYAN=15,
    RED_WHITE=16,
    GREEN_BLACK=17,
    GREEN_RED=18,
    GREEN_GREEN=19,
    GREEN_YELLOW=20,
    GREEN_BLUE=21,
    GREEN_MAGENTA=22,
    GREEN_CYAN=23,
    GREEN_WHITE=24,
    YELLOW_BLACK=25,
    YELLOW_RED=26,
    YELLOW_GREEN=27,
    YELLOW_YELLOW=28,
    YELLOW_BLUE=29,
    YELLOW_MAGENTA=30,
    YELLOW_CYAN=31,
    YELLOW_WHITE=32,
    BLUE_BLACK=33,
    BLUE_RED=34,
    BLUE_GREEN=35,
    BLUE_YELLOW=36,
    BLUE_BLUE=37,
    BLUE_MAGENTA=38,
    BLUE_CYAN=39,
    BLUE_WHITE=40,
    MAGENTA_BLACK=41,
    MAGENTA_RED=42,
    MAGENTA_GREEN=43,
    MAGENTA_YELLOW=44,
    MAGENTA_BLUE=45,
    MAGENTA_MAGENTA=46,
    MAGENTA_CYAN=47,
    MAGENTA_WHITE=48,
    CYAN_BLACK=49,
    CYAN_RED=50,
    CYAN_GREEN=51,
    CYAN_YELLOW=52,
    CYAN_BLUE=53,
    CYAN_MAGENTA=54,
    CYAN_CYAN=55,
    CYAN_WHITE=56,
    WHITE_BLACK=57,
    WHITE_RED=58,
    WHITE_GREEN=59,
    WHITE_YELLOW=60,
    WHITE_BLUE=61,
    WHITE_MAGENTA=62,
    WHITE_CYAN=63,
    WHITE_WHITE=64,
};

enum class ViewLocation {
    NONE,
    Right,
    Left,
    Top,
    Bottom,
};


enum class UIBarAsking {
    NONE,
    Message,
    ShouldExit,
    ShouldSave,
};
enum class UIState {
    Default,
    DefaultHelp,
    Hex,
    HexHelp,
};
enum class HexViewState {
    Default,
    Editing,
};
// Handler actions: The continued execution of handlers
// Functional actions: Normal actions
// Special actions: RESIZE
// Drawing actions: Drawing to the screen

// This is used as flags, so an int for KeyHandling might not (likely not) be a legitimate enum value
// So they should stay as ints
enum class KeyHandled {
    FullStop = 0, // Don't do anything else. Stop handling events right after this.
    Handler = 1, // Allow continued execution of handlers
    Functional = 2, // Only allow functional actions
    Special = 4, // Only allow special actions
    Drawing = 8, // Only allow drawing actions
    All = Handler | Functional | Special | Drawing, // Continue doing everything.
};

struct KeyHandleFlags {
    bool handler = true;
    bool functional = true;
    bool special = true;
    bool drawing = true;
};


std::string getFilename(int argc, char** argv);
std::optional<std::filesystem::path> getConfigPath ();
std::optional<std::filesystem::path> getPluginsPath (int argc, char** argv);
bool isStringWhitespace (const std::string& str);
std::string byteToString (HerixLib::Byte byte);
std::string byteToStringPadded (HerixLib::Byte byte);
char hexChr (HerixLib::Byte v);
HerixLib::Byte hexChrToNumber (char c);
bool isHexadecimalCharacter (int c);
bool isHexadecimalCharacter (char c);
HerixLib::Byte clearHighestHalfByte (HerixLib::Byte val);
HerixLib::Byte clearLowestHalfByte (HerixLib::Byte val);
HerixLib::Byte setHighestHalfByte (HerixLib::Byte val, HerixLib::Byte num);
HerixLib::Byte setLowestHalfByte (HerixLib::Byte val, HerixLib::Byte num);

bool isDisplayableCharacter (int c);
bool isDisplayableCharacter (char c);

#endif
