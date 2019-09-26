#include "./mutil.hpp"

#include <cassert>

// TODO: make this cross-platform
std::optional<std::filesystem::path> getConfigPath () {
    std::filesystem::path file = "herixtui.lua";
    std::filesystem::path path;

    char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
    char* home = std::getenv("HOME");

    if (xdg_config_home != nullptr) {
        path = std::filesystem::path(xdg_config_home) / file;
        if (std::filesystem::exists(path)) {
            return std::move(path);
        }
    }

    if (home != nullptr) {
        path = std::filesystem::path(home) / ".config/" / file;
        if (std::filesystem::exists(path)) {
            return std::move(path);
        }

        path = std::filesystem::path(home) / file;
        if (std::filesystem::exists(path)) {
            return std::move(path);
        }
    }

    return std::nullopt;
}
#include <iostream>
std::optional<std::filesystem::path> getPluginsPath(int argc, char** argv) {
    std::filesystem::path dir = "plugins";

    if (argc == 0) {
        return std::nullopt;
    }

    std::filesystem::path executable_name = std::filesystem::path(argv[0]);
    std::filesystem::path directory = executable_name.parent_path();
    std::filesystem::path full_path = directory / dir;

    if (std::filesystem::exists(full_path)) {
        return std::move(full_path);
    }

    char* home_directory = std::getenv("HOME");

    if (home_directory != nullptr) {
        std::filesystem::path user_home = std::filesystem::path(home_directory);

        std::filesystem::path root_location = user_home / ".herixtui/plugins/";
        if (std::filesystem::exists(root_location)) {
            return std::filesystem::path(root_location);
        }
    }

    return std::nullopt;
}

// Returns whether the string is all whitespace. Returns true if string is empty.
bool isStringWhitespace (const std::string& str) {
    for (char val : str) {
        if (val != ' ' && val != '\t' && val != '\v' && val != '\f' && val != '\r') {
            return false;
        }
    }
    return true;
}

char hexChr (HerixLib::Byte v) {
    assert(v <= 15);
	return static_cast<char>(v + (v > 9 ? 55 : 48));
}
HerixLib::Byte hexChrToNumber (char c) {
    assert((c >= 97 && c <= 102) || (c >= 65 && c <= 70) || (c >= 48 && c <= 57));
    if (c >= 97) { // a-f
        return static_cast<HerixLib::Byte>(c-87); // c-97+10
    } else if (c >= 65) { // A-F
        return static_cast<HerixLib::Byte>(c-55); // c-65+10
    } else if (c >= 48) { // 0-9
        return static_cast<HerixLib::Byte>(c-48);
    } else {
        assert(false);
        return 0;
    }
}
std::string byteToStringPadded (HerixLib::Byte byte) {
    std::string res = "";
	res += hexChr((byte / 16) % 16);
	res += hexChr(byte % 16);
	return res;
}
// Without padding before byte
std::string byteToString (HerixLib::Byte byte) {
    std::string res = "";
    char temp = hexChr((byte / 16) % 16);
    if (temp != '0') {
        res += temp;
    }
    res += hexChr(byte % 16);
    return res;
}

std::string getFilename (int argc, char** argv) {
    if (argc > 1) {
        return argv[1];
    } else {
        return "";
    }
}

bool isHexadecimalCharacter (int c) {
    return (c >= static_cast<int>('A') && c <= static_cast<int>('F')) ||
        (c >= static_cast<int>('a') && c <= static_cast<int>('f')) ||
        (c >= static_cast<int>('0') && c <= static_cast<int>('9'));
}
bool isHexadecimalCharacter (char c) {
    return isHexadecimalCharacter(static_cast<int>(c));
}

HerixLib::Byte clearHighestHalfByte (HerixLib::Byte val) {
    return val & 0b00001111;
}
HerixLib::Byte clearLowestHalfByte (HerixLib::Byte val) {
    return val & 0b11110000;
}
HerixLib::Byte setHighestHalfByte (HerixLib::Byte val, HerixLib::Byte num) {
    assert(num <= 0x0F);
    return clearHighestHalfByte(val) | (num * 16);
}
HerixLib::Byte setLowestHalfByte (HerixLib::Byte val, HerixLib::Byte num) {
    assert(num <= 0x0F);
    return clearLowestHalfByte(val) | num;
}

bool isDisplayableCharacter (int c) {
    return c >= 32 && c <= 126;
}
bool isDisplayableCharacter (unsigned char c) {
    return isDisplayableCharacter(static_cast<int>(c));
}

// I wouldn't use these because ncurses, or perhaps just both of the terminals I use, 
// don't seem to work with the extended characters properly
bool isDisplayableCharacterLenient (int c) {
    return (c >= 32 && c <= 126) || (c >= 161 && c <= 255);
}
bool isDisplayableCharacterLenient (unsigned char c) {
    return isDisplayableCharacterLenient(static_cast<int>(c));
}

void printExitLogs () {
    for (const std::string& str : exit_logs) {
        std::cout << str << std::endl;
    }

    if (exit_logs.size() == 0) {
        std::cout << "Exited." << std::endl;
    }
}

void logAtExit (std::string val) {
    exit_logs.push_back(val);
}