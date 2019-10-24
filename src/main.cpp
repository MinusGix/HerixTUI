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
#include "./uidisplay.hpp"

using namespace HerixLib;

std::filesystem::path findConfigurationFile (cxxopts::ParseResult& result);
std::filesystem::path findPluginsDirectory (cxxopts::ParseResult& result, int argc, char** argv);
void setupCurses ();
void shutdownCurses ();

int main (int argc, char** argv) {
    cxxopts::Options options("HerixTUI", "Terminal Hex Editor");
    options.add_options()
        ("h,help", "Shows help")
        ("c,config_file", "Set where the config file is located.", cxxopts::value<std::string>())
        ("locate_config", "Find where the code looks for the config")
        ("p,plugin_dir", "Set where plugins are looked for.", cxxopts::value<std::string>())
        ("locate_plugins", "Find where the code looks for plugins")
        ("w,no_writing", "Disallows writing to the file. Needed for files which are not writable.")
        ("s,start", "The start position in the file, restricts editing to after this.", cxxopts::value<std::string>())
        ("e,end", "The end position in the file, restricts editing to before this.", cxxopts::value<std::string>())
        ("d,debug", "Turn on debug mode.")
        ;

    cxxopts::ParseResult result = options.parse(argc, argv);

    bool debug_mode = false;

    if (result.count("debug")) {
        debug_mode = true;
    }

    // Show help
    if (result.count("help")) {
        std::cout << options.help({}) << std::endl;
        return 0;
    }

    if (result.count("config_file") > 1) {
        std::cout << "Multiple config file locations can not be specified." << std::endl;
        return 1;
    }

    if (result.count("plugin_dir") > 1) {
        std::cout << "Multiple plugin directory locations can not be specified." << std::endl;
        return 1;
    }

    std::filesystem::path config_file = findConfigurationFile(result);

    if (result.count("locate_config") != 0) {
        std::cout << "Config File: '" << config_file << "'\n";
        return 0;
    }



    std::filesystem::path plugin_dir = findPluginsDirectory(result, argc, argv);

    if (result.count("locate_plugins") != 0) {
        std::cout << "Plugins Directory: '" << plugin_dir << "'\n";
        return 0;
    }

    std::filesystem::path filename = getFilename(argc, argv);
    // TODO: allow creation of empty file to edit in
    if (filename == "") {
        std::cout << "No file to open was specified.\n";
        return 0;
    }

    bool allow_writing = true;
    if (result.count("no_writing") != 0) {
        allow_writing = false;
    }

    AbsoluteFilePosition start_position = 0;
    if (result.count("start") > 1) {
        throw std::runtime_error("Start specified more than once.");
    } else if (result.count("start") == 1) {
        std::string temp_start = result["start"].as<std::string>();
        if (temp_start.size() >= 2 && temp_start.at(0) == '0' && temp_start.at(1) == 'x') {
            start_position = hexToNumber<AbsoluteFilePosition>(temp_start.substr(2));
        } else {
            start_position = decToNumber<AbsoluteFilePosition>(temp_start);
        }
    }

    std::optional<size_t> end_position = std::nullopt;
    if (result.count("end") > 1) {
        throw std::runtime_error("End specified more than once.");
    } else if (result.count("end") == 1) {
        std::string temp_end = result["end"].as<std::string>();
        if (temp_end.size() >= 2 && temp_end.at(0) == '0' && temp_end.at(1) == 'x') {
            end_position = std::make_optional(hexToNumber<AbsoluteFilePosition>(temp_end.substr(2)));
        } else {
            end_position = std::make_optional(decToNumber<AbsoluteFilePosition>(temp_end));
        }
    }

    std::cout << "S: " << start_position << "\n";
    std::cout << "E: ";
    if (end_position.has_value()) {
        std::cout << end_position.value();
    } else {
        std:: cout << "No Value";
    }
    std::cout << "\n";

    setupCurses();
    try {
        UIDisplay display = UIDisplay(filename, config_file, plugin_dir, allow_writing, std::make_pair(start_position, end_position), debug_mode);

        refresh();

        // This could be done in the UIDisplay constructor, but I find it more palatable to do it explicitly.
        display.handleInit();

        while (true) {
            display.key = getch();
            display.handleEvent();

            if (display.should_exit) {
                break;
            }
        }

        shutdownCurses();

        printExitLogs();
    } catch (...) {
        shutdownCurses();
        printExitLogs();
        throw;
    }

    return 0;
}

void setupCurses () {
    initscr();
    clear();
    raw();
    noecho();
    keypad(stdscr, true);
    curs_set(0);
    start_color();

    // Generated by script, ofc I wouldn't write this by hand.
    // Also the enum declaration (for MColors) and the lua setting was also generated via script.
    init_pair(static_cast<short>(MColors::BLACK_BLACK), COLOR_BLACK, COLOR_BLACK);
    init_pair(static_cast<short>(MColors::BLACK_RED), COLOR_BLACK, COLOR_RED);
    init_pair(static_cast<short>(MColors::BLACK_GREEN), COLOR_BLACK, COLOR_GREEN);
    init_pair(static_cast<short>(MColors::BLACK_YELLOW), COLOR_BLACK, COLOR_YELLOW);
    init_pair(static_cast<short>(MColors::BLACK_BLUE), COLOR_BLACK, COLOR_BLUE);
    init_pair(static_cast<short>(MColors::BLACK_MAGENTA), COLOR_BLACK, COLOR_MAGENTA);
    init_pair(static_cast<short>(MColors::BLACK_CYAN), COLOR_BLACK, COLOR_CYAN);
    init_pair(static_cast<short>(MColors::BLACK_WHITE), COLOR_BLACK, COLOR_WHITE);
    init_pair(static_cast<short>(MColors::RED_BLACK), COLOR_RED, COLOR_BLACK);
    init_pair(static_cast<short>(MColors::RED_RED), COLOR_RED, COLOR_RED);
    init_pair(static_cast<short>(MColors::RED_GREEN), COLOR_RED, COLOR_GREEN);
    init_pair(static_cast<short>(MColors::RED_YELLOW), COLOR_RED, COLOR_YELLOW);
    init_pair(static_cast<short>(MColors::RED_BLUE), COLOR_RED, COLOR_BLUE);
    init_pair(static_cast<short>(MColors::RED_MAGENTA), COLOR_RED, COLOR_MAGENTA);
    init_pair(static_cast<short>(MColors::RED_CYAN), COLOR_RED, COLOR_CYAN);
    init_pair(static_cast<short>(MColors::RED_WHITE), COLOR_RED, COLOR_WHITE);
    init_pair(static_cast<short>(MColors::GREEN_BLACK), COLOR_GREEN, COLOR_BLACK);
    init_pair(static_cast<short>(MColors::GREEN_RED), COLOR_GREEN, COLOR_RED);
    init_pair(static_cast<short>(MColors::GREEN_GREEN), COLOR_GREEN, COLOR_GREEN);
    init_pair(static_cast<short>(MColors::GREEN_YELLOW), COLOR_GREEN, COLOR_YELLOW);
    init_pair(static_cast<short>(MColors::GREEN_BLUE), COLOR_GREEN, COLOR_BLUE);
    init_pair(static_cast<short>(MColors::GREEN_MAGENTA), COLOR_GREEN, COLOR_MAGENTA);
    init_pair(static_cast<short>(MColors::GREEN_CYAN), COLOR_GREEN, COLOR_CYAN);
    init_pair(static_cast<short>(MColors::GREEN_WHITE), COLOR_GREEN, COLOR_WHITE);
    init_pair(static_cast<short>(MColors::YELLOW_BLACK), COLOR_YELLOW, COLOR_BLACK);
    init_pair(static_cast<short>(MColors::YELLOW_RED), COLOR_YELLOW, COLOR_RED);
    init_pair(static_cast<short>(MColors::YELLOW_GREEN), COLOR_YELLOW, COLOR_GREEN);
    init_pair(static_cast<short>(MColors::YELLOW_YELLOW), COLOR_YELLOW, COLOR_YELLOW);
    init_pair(static_cast<short>(MColors::YELLOW_BLUE), COLOR_YELLOW, COLOR_BLUE);
    init_pair(static_cast<short>(MColors::YELLOW_MAGENTA), COLOR_YELLOW, COLOR_MAGENTA);
    init_pair(static_cast<short>(MColors::YELLOW_CYAN), COLOR_YELLOW, COLOR_CYAN);
    init_pair(static_cast<short>(MColors::YELLOW_WHITE), COLOR_YELLOW, COLOR_WHITE);
    init_pair(static_cast<short>(MColors::BLUE_BLACK), COLOR_BLUE, COLOR_BLACK);
    init_pair(static_cast<short>(MColors::BLUE_RED), COLOR_BLUE, COLOR_RED);
    init_pair(static_cast<short>(MColors::BLUE_GREEN), COLOR_BLUE, COLOR_GREEN);
    init_pair(static_cast<short>(MColors::BLUE_YELLOW), COLOR_BLUE, COLOR_YELLOW);
    init_pair(static_cast<short>(MColors::BLUE_BLUE), COLOR_BLUE, COLOR_BLUE);
    init_pair(static_cast<short>(MColors::BLUE_MAGENTA), COLOR_BLUE, COLOR_MAGENTA);
    init_pair(static_cast<short>(MColors::BLUE_CYAN), COLOR_BLUE, COLOR_CYAN);
    init_pair(static_cast<short>(MColors::BLUE_WHITE), COLOR_BLUE, COLOR_WHITE);
    init_pair(static_cast<short>(MColors::MAGENTA_BLACK), COLOR_MAGENTA, COLOR_BLACK);
    init_pair(static_cast<short>(MColors::MAGENTA_RED), COLOR_MAGENTA, COLOR_RED);
    init_pair(static_cast<short>(MColors::MAGENTA_GREEN), COLOR_MAGENTA, COLOR_GREEN);
    init_pair(static_cast<short>(MColors::MAGENTA_YELLOW), COLOR_MAGENTA, COLOR_YELLOW);
    init_pair(static_cast<short>(MColors::MAGENTA_BLUE), COLOR_MAGENTA, COLOR_BLUE);
    init_pair(static_cast<short>(MColors::MAGENTA_MAGENTA), COLOR_MAGENTA, COLOR_MAGENTA);
    init_pair(static_cast<short>(MColors::MAGENTA_CYAN), COLOR_MAGENTA, COLOR_CYAN);
    init_pair(static_cast<short>(MColors::MAGENTA_WHITE), COLOR_MAGENTA, COLOR_WHITE);
    init_pair(static_cast<short>(MColors::CYAN_BLACK), COLOR_CYAN, COLOR_BLACK);
    init_pair(static_cast<short>(MColors::CYAN_RED), COLOR_CYAN, COLOR_RED);
    init_pair(static_cast<short>(MColors::CYAN_GREEN), COLOR_CYAN, COLOR_GREEN);
    init_pair(static_cast<short>(MColors::CYAN_YELLOW), COLOR_CYAN, COLOR_YELLOW);
    init_pair(static_cast<short>(MColors::CYAN_BLUE), COLOR_CYAN, COLOR_BLUE);
    init_pair(static_cast<short>(MColors::CYAN_MAGENTA), COLOR_CYAN, COLOR_MAGENTA);
    init_pair(static_cast<short>(MColors::CYAN_CYAN), COLOR_CYAN, COLOR_CYAN);
    init_pair(static_cast<short>(MColors::CYAN_WHITE), COLOR_CYAN, COLOR_WHITE);
    init_pair(static_cast<short>(MColors::WHITE_BLACK), COLOR_WHITE, COLOR_BLACK);
    init_pair(static_cast<short>(MColors::WHITE_RED), COLOR_WHITE, COLOR_RED);
    init_pair(static_cast<short>(MColors::WHITE_GREEN), COLOR_WHITE, COLOR_GREEN);
    init_pair(static_cast<short>(MColors::WHITE_YELLOW), COLOR_WHITE, COLOR_YELLOW);
    init_pair(static_cast<short>(MColors::WHITE_BLUE), COLOR_WHITE, COLOR_BLUE);
    init_pair(static_cast<short>(MColors::WHITE_MAGENTA), COLOR_WHITE, COLOR_MAGENTA);
    init_pair(static_cast<short>(MColors::WHITE_CYAN), COLOR_WHITE, COLOR_CYAN);
    init_pair(static_cast<short>(MColors::WHITE_WHITE), COLOR_WHITE, COLOR_WHITE);

    logAtExit(std::to_string(COLORS) + " colors supported.");
    logAtExit(std::to_string(COLOR_PAIRS) + " color pairs supported.");
}

void shutdownCurses () {
    endwin();
}

std::filesystem::path findConfigurationFile (cxxopts::ParseResult& result) {
    std::filesystem::path config_file = "";

    // Allows the passing in of a config file by parameter
    if (result.count("config_file") == 1) {
        config_file = result["config_file"].as<std::string>();

        if (isStringWhitespace(config_file)) {
            std::cout << "Config file passed by argument was empty." << std::endl;
            exit(1);
        }
    }

    // If there's no config file as a parameter then we try to get it from the environment variables
    if (config_file == "") {
        std::optional<std::filesystem::path> config_file_env = getConfigPath();

        if (!config_file_env.has_value()) {
            std::cout << "Could not find config file.\n";
        } else {
            config_file = config_file_env.value();
        }
    }

    return config_file;
}

std::filesystem::path findPluginsDirectory (cxxopts::ParseResult& result, int argc, char** argv) {
    std::filesystem::path plugin_dir = "";

    // Allows the passing in of the plugin dir by parameter
    if (result.count("plugin_dir") == 1) {
        plugin_dir = result["plugin_dir"].as<std::string>();

        if (isStringWhitespace(plugin_dir)) {
            std::cout << "Plugin directory passed by argument was empty." << std::endl;
            exit(1);
        }
    }

    // If there's no plugin dir as a parameter we try to divine it.
    if (plugin_dir == "") {
        std::optional<std::filesystem::path> plugin_dir_found = getPluginsPath(argc, argv);

        if (!plugin_dir_found.has_value()) {
            std::cout << "Could not find possible location for plugins to be stored. Please setup a plugins directory\n";
            // We ignore this, allowing you to start up without a plugins directory.
        } else {
            plugin_dir = plugin_dir_found.value();
        }
    }

    return plugin_dir;
}