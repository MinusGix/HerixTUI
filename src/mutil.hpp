#ifndef FILE_SEEN_MUTIL
#define FILE_SEEN_MUTIL

#include <optional>
#include <filesystem>
#include <string>
#include "./Herix/src/herix.hpp"

std::string getFilename(int argc, char** argv);
std::optional<std::filesystem::path> getConfigPath ();
bool isStringWhitespace (const std::string& str);
std::string byteToString (Byte byte);
std::string byteToStringPadded (Byte byte);
char hexChr (Byte v);
Byte hexChrToNumber (char c);
bool isHexadecimalCharacter (int c);
bool isHexadecimalCharacter (char c);
Byte clearHighestHalfByte (Byte val);
Byte clearLowestHalfByte (Byte val);
Byte setHighestHalfByte (Byte val, Byte num);
Byte setLowestHalfByte (Byte val, Byte num);

bool isDisplayableCharacter (int c);
bool isDisplayableCharacter (char c);

#endif
