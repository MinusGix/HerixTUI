#include "./mutil.hpp"

#include <cassert>

// TODO: make this cross-platform
std::optional<std::filesystem::path> getConfigPath () {
    std::filesystem::path file = "herixtui.lua";

    char* config = std::getenv("XDG_CONFIG_HOME");
    if (config == nullptr) {
        config = std::getenv("HOME");
        if (config == nullptr) {
            return std::nullopt;
        } else {
            return std::filesystem::path(std::string(config)) / ".config/" / file;
        }
    }

    return std::filesystem::path(std::string(config)) / file;
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

char hexChr (Byte v) {
    assert(v <= 15);
	return static_cast<char>(v + (v > 9 ? 55 : 48));
}
Byte hexChrToNumber (char c) {
    assert((c >= 97 && c <= 102) || (c >= 65 && c <= 70) || (c >= 48 && c <= 57));
    if (c >= 97) { // a-f
        return static_cast<Byte>(c-87); // c-97+10
    } else if (c >= 65) { // A-F
        return static_cast<Byte>(c-55); // c-65+10
    } else if (c >= 48) { // 0-9
        return static_cast<Byte>(c-48);
    } else {
        assert(false);
        return 0;
    }
}
std::string byteToStringPadded (Byte byte) {
    std::string res = "";
	res += hexChr((byte / 16) % 16);
	res += hexChr(byte % 16);
	return res;
}
// Without padding before byte
std::string byteToString (Byte byte) {
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

Byte clearHighestHalfByte (Byte val) {
    return val & 0b00001111;
}
Byte clearLowestHalfByte (Byte val) {
    return val & 0b11110000;
}
Byte setHighestHalfByte (Byte val, Byte num) {
    assert(num <= 0x0F);
    return clearHighestHalfByte(val) | (num * 16);
}
Byte setLowestHalfByte (Byte val, Byte num) {
    assert(num <= 0x0F);
    return clearLowestHalfByte(val) | num;
}

bool isDisplayableCharacter (int c) {
    return c >= 32 && c <= 126;
}
bool isDisplayableCharacter (char c) {
    return isDisplayableCharacter(static_cast<int>(c));
}
