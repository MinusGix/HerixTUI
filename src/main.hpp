#include <cstdlib>
#include <string>
#include <iostream>
#include <optional>
#include <filesystem>

#include <curses.h>

// ignore errors from these headers since I'm not changing these

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"

#define SOL_ALL_SAFETIES_ON 1
#include "./sol.hpp"
#include "./cxxopts.hpp"

#pragma GCC diagnostic pop

#include "./Herix/src/herix.hpp"

#include "./mutil.hpp"
#include "./window.hpp"