#ifndef FILE_SEEN_MUTIL
#define FILE_SEEN_MUTIL

#include <optional>
#include <filesystem>
#include <string>
#include "./Herix/src/herix.hpp"

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
