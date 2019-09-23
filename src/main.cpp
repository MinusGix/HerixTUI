#include "./main.hpp"

using namespace HerixLib;

// Logs to show when the application exits.
static std::vector<std::string> exit_logs;
void logAtExit (std::string val);
void printExitLogs ();

std::filesystem::path findConfigurationFile (cxxopts::ParseResult& result);
void setupCurses ();
void shutdownCurses ();

void Window::createWindow () {
    win = newwin(height, width, y, x);
    // TODO: what does this do?
    box(win, 0, 0);
}

void Window::update () {
    wresize(win, height, width);
    mvwin(win, y, x);
}

void Window::moveOrigin () {
    wmove(win, 0, 0);
}

void Window::move (int to_x, int to_y) {
    wmove(win, to_y, to_x);
}

void Window::print (std::string str, int x_pos, bool allow_wrapping) {
    assert(width >= 0);
    assert(x_pos <= width);
    assert(x_pos >= 0);
    if (allow_wrapping || static_cast<size_t>(x_pos) + str.length() <= static_cast<size_t>(width)) {
        print(str);
    } else {
        print(str.substr(0, static_cast<size_t>(width - x_pos)));
    }
}
void Window::print (std::string str) {
    print(str.c_str());
}

void Window::print (const char* str) {
    wprintw(win, str);
}

ViewWindow::~ViewWindow () {
    for (SubView& sv : sub_views) {
        sv.clearOnRender();
        sv.clearOnResize();
    }
}
int ViewWindow::getHexX () const {
    return x + getLeftWidth();
}
int ViewWindow::getHexY () const {
    return y;
}
int ViewWindow::getHexWidth () const {
    return width - getRightWidth() - getLeftWidth();
}
int ViewWindow::getHexHeight () const {
    return height;
}
int ViewWindow::getHexByteWidth () const {
    return (width - getLeftWidth()) / 4;
}

int ViewWindow::getRightWidth () const {
    int ret = 0;
    for (const SubView& sv : sub_views) {
        if (sv.getLoc() == ViewLocation::Right && sv.getVisible() && sv.getWidth() != -1) {
            ret += sv.getWidth();
        }
    }
    return ret;
}
// TODO: make getHexX use this
int ViewWindow::getLeftWidth () const {
    int ret = 0;
    for (const SubView& sv : sub_views) {
        if (sv.getLoc() == ViewLocation::Left && sv.getVisible() && sv.getWidth() != -1) {
            ret += sv.getWidth();
        }
    }
    return ret;
}

void ViewWindow::enableColor (MColors color) {
    wattron(win, COLOR_PAIR(static_cast<short>(color)));
}
void ViewWindow::disableColor (MColors color) {
    wattroff(win, COLOR_PAIR(static_cast<short>(color)));
}
// TODO: implement top and bottom SubViews^
// TODO: add function that clears window
// TODO: add function that clears subview

SubView::SubView (ViewLocation t_loc, ViewWindow& t_view) : view(t_view), loc(t_loc) {}
void SubView::setupLua (sol::state& lua) {
    sol::usertype<SubView> subview_type = lua.new_usertype<SubView>(
        "SubView",
        sol::constructors<SubView(ViewLocation, ViewWindow&)>(),
        "setHeight", &SubView::setHeight,
        "getHeight", &SubView::getHeight,
        "setWidth", &SubView::setWidth,
        "getWidth", &SubView::getWidth,
        "setX", &SubView::setX,
        "getX", &SubView::getX,
        "setY", &SubView::setY,
        "getY", &SubView::getY,
        "setLoc", &SubView::setLoc,
        "getLoc", &SubView::getLoc,
        "setVisible", &SubView::setVisible,
        "getVisible", &SubView::getVisible,
        "onRender", &SubView::onRender,
        "clearOnRender", &SubView::clearOnRender,
        "onResize", &SubView::onResize,
        "clearOnResize", &SubView::clearOnResize,
        "print", &SubView::print,
        "printStandout", &SubView::printStandout,
        "move", &SubView::move
    );
}
void SubView::setHeight (int val) {
    height = val;
}
int SubView::getHeight () const {
    return height;
}
void SubView::setWidth (int val) {
    width = val;
}

int SubView::getWidth () const {
    return width;
}
void SubView::setX (int val) {
    x = val;
}
int SubView::getX () const {
    return x;
}
void SubView::setY (int val) {
    y = val;
}
int SubView::getY () const {
    return y;
}
void SubView::setLoc (ViewLocation val) {
    loc = val;
}
ViewLocation SubView::getLoc () const {
    return loc;
}
void SubView::setVisible (bool val) {
    visible = val;
}
bool SubView::getVisible () const {
    return visible;
}

void SubView::onRender (sol::protected_function cb) {
    on_render = cb;
}
void SubView::clearOnRender () {
    on_render = sol::lua_nil;
}
void SubView::onResize (sol::protected_function cb) {
    on_resize = cb;
}
void SubView::clearOnResize () {
    on_resize = sol::lua_nil;
}
void SubView::print (std::string text) {
    view.print(text.c_str());
}
void SubView::printStandout (std::string text) {
    // TODO: this should probably preserve the flags, just in case
    wattron(view.win, A_STANDOUT);
    print(text);
    wattroff(view.win, A_STANDOUT);
}
void SubView::move (int to_x, int to_y) {
    int move_x = getX() + to_x;
    if (loc == ViewLocation::Right) {
        move_x += view.getHexX() + view.getHexWidth();
    } else if (loc == ViewLocation::Left) {}
    int move_y = view.getHexY() + getY() + to_y;

    view.move(move_x, move_y);
}


void SubView::runRender () {
    if (on_render) {
        if (!on_render.is<sol::nil_t>()) {
            auto v = on_render();
            if (!v.valid()) {
                logAtExit("Error in rendering!");
                sol::error err = v;
                throw err;
            }
        }
    }
}
void SubView::runResize () {
    if (on_resize) {
        if (!on_resize.is<sol::nil_t>()) {
            auto v = on_resize();
            if (!v.valid()) {
                logAtExit("Error in resizing!");
                sol::error err = v;
                throw err;
            }
        }
    }
}


class UIDisplay {
    private:
    // Default configuration file used to laod the base plugins.
    inline static const std::string DEFAULT_CONFIG =
    "plugins = {"
        "PLUGIN_DIR .. \"/AsciiView.lua\","
        "PLUGIN_DIR .. \"/Offsets.lua\","
        "PLUGIN_DIR .. \"/BaseHighlighter.lua\","
        "PLUGIN_DIR .. \"/FileHighlighter.lua\","
        "PLUGIN_DIR .. \"/FileHighlighter_ELF.lua\","
        "PLUGIN_DIR .. \"/HexWrite.lua\""
    "}";

    public:
    // Should be first, so it is destructed before anything that uses lua-things
    sol::state lua;

    UIState state = UIState::Default;
    HexViewState hex_view_state = HexViewState::Default;

    // If we should exit
    bool should_exit = false;

    // Last pressed key
    int key = 0;

    // The config path. Changing this after the start of the program is useless since it doesn't auto-reload
    std::filesystem::path config_path;
    std::filesystem::path plugins_directory;

    bool debug = false;

    // Rows down. This is the row that would be at the top of the screen.
    FilePosition row_pos = 0;
    // In bytes
    FilePosition sel_pos = 0;
    // Where in the current byte (sel_pos) we are at. Can be 0|1 so that's why it uses bool.
    bool editing_position = false;

    // Current id, used for key_handlers
    unsigned int key_handler_id = 0;
    // pair<id, handler>
    std::vector<std::pair<unsigned int, sol::protected_function>> key_handlers;

    Window bar;
    UIBarAsking bar_asking = UIBarAsking::NONE;
    std::string bar_message = "";

    ViewWindow view;

    Herix hex;

    sol::protected_function on_write;
    // Callbacks which are called when we save.
    // These are assured to be called _before_ we save, so that any special edits can happen
    std::vector<sol::protected_function> on_save;

    // TODO: add on_post_save listeners


    std::optional<size_t> cached_file_size = std::nullopt;
    bool should_edit_move_forward = true;

    // Note: these two functions should be ignored after initialization!
    ChunkSize getMaxChunkMemory () {
        return lua.get_or("max_chunk_memory", 1024*10UL);
    }
    ChunkSize getMaxChunkSize () {
        return lua.get_or("max_chunk_size", 1024UL);
    }


    UIDisplay (std::filesystem::path t_filename, std::filesystem::path t_config_file, std::filesystem::path t_plugins_directory, bool t_debug) {
        debug = t_debug;
        plugins_directory = t_plugins_directory;
        config_path = t_config_file;

        debugLog("Debug mode is on");

        lua.open_libraries(
            sol::lib::base, sol::lib::bit32, sol::lib::count, sol::lib::debug, sol::lib::ffi,
            sol::lib::io, sol::lib::jit, sol::lib::math, sol::lib::os, sol::lib::package,
            sol::lib::string, sol::lib::table, sol::lib::utf8
        );

        setupSimpleLua();

        if (config_path == "") {
            logAtExit("Loading Default Config..");
            lua.script(UIDisplay::DEFAULT_CONFIG);
        } else {
            auto result = lua.script_file(config_path);
            if (!result.valid()) {
                // TODO: these checks might be useless?
                if (result.status() == sol::call_status::file) {
                    if (std::filesystem::exists(config_path)) {
                        // TODO: display the error
                        exit_logs.push_back("File exists but there was a file error.\n");
                    }
                }
            }
        }

        hex = Herix(t_filename, getMaxChunkMemory(), getMaxChunkSize());

        setupBar();
        setupView();

        setupLuaValues();
        loadPlugins ();

        state = UIState::Hex;
    }

    ~UIDisplay () {
        delwin(bar.win);
        delwin(view.win);
    }

    void debugLog (std::string val) {
        if (debug) {
            logAtExit(val);
        }
    }

    bool getEditingPosition () const {
        return editing_position;
    }
    void setEditingPosition (bool val) {
        editing_position = val;
    }

    void onWrite (sol::protected_function cb) {
        on_write = cb;
    }
    void runWrite (std::vector<Byte> data, FilePosition file_pos) {
        if (hasOnWrite()) {
            auto v = on_write(data, data.size(), file_pos);
            if (!v.valid()) {
                logAtExit("Error in write!");
                sol::error err = v;
                throw err;
            }
        }
    }
    bool hasOnWrite () const {
        return static_cast<bool>(on_write);
    }

    size_t onSave (sol::protected_function cb) {
        on_save.push_back(cb);
        return on_save.size() - 1;
    }

    void runSaveListeners () {
        for (auto& cb : on_save) {
            auto v = cb();
            if (!v.valid()) {
                logAtExit("Error in save listener!");
                sol::error err = v;
                throw err;
            }
        }
    }
    // TODO: add function to remove save listener
    // TODO: add function to get save listener count

    bool getShouldExit () const {
        return should_exit;
    }

    void setShouldExit(bool val) {
        should_exit = val;
    }

    int getLastPressedKey () {
        return key;
    }

    std::string getConfigPath () {
        return config_path.string();
    }

    unsigned int getNextKeyHandlerID () {
        return key_handler_id;
    }

    bool getShouldEditMoveForward () const {
        return should_edit_move_forward;
    }

    void setShouldEditMoveForward (bool val) {
        should_edit_move_forward = val;
    }

    unsigned int registerKeyHandler (sol::protected_function handler) {
        key_handlers.push_back(std::make_pair(key_handler_id, handler));
        return key_handler_id++;
    }

    void removeKeyHandler (int id) {
        // TODO: implement this
    }

    bool setBarMessage (std::string value) {
        if (bar_asking != UIBarAsking::NONE) {
            return false;
        }
        bar_message = value;
        return true;
    }
    void clearBarMessage () {
        bar_message = "";
    }
    std::string getBarMessage () {
        return bar_message;
    }

    // TODO: make this invalidate the cache if the file is saved.
    size_t getFileSize () {
        if (cached_file_size.has_value()) {
            return cached_file_size.value();
        } else {
            cached_file_size = hex.getFileSize();
            return cached_file_size.value();
        }
    }

    size_t createSubView (ViewLocation loc) {
        view.sub_views.push_back(SubView(loc, view));
        return view.sub_views.size() - 1;
    }
    SubView& getSubView (size_t id) {
        return view.sub_views.at(id);
    }

    FilePosition getSelectedRow () {
        // TODO: the rounding down might be unneeded?
        return (sel_pos - (sel_pos % static_cast<FilePosition>(view.getHexByteWidth()))) // Rounds down towards multiple of hexByteWidth
            / static_cast<FilePosition>(view.getHexByteWidth());
    }
    int getViewHeight () const {
        return view.height;
    }
    int getViewWidth () const {
        return view.width;
    }
    int getViewX () const {
        return view.x;
    }
    int getViewY () const {
        return view.y;
    }
    bool lua_hasByte (FilePosition pos) {
        return hex.read(pos).has_value();
    }
    Byte lua_readByte (FilePosition pos) {
        return hex.read(pos).value();
    }
    std::vector<Byte> lua_readBytes (FilePosition pos, size_t length) {
        return hex.readMultipleCutoff(pos, length);
    }
    FilePosition getRowOffset () const {
        return row_pos * static_cast<FilePosition>(view.getHexByteWidth());
    }
    FilePosition getRowPosition () const {
        return row_pos;
    }
    void setRowPosition (FilePosition pos) {
        row_pos = pos;
    }
    FilePosition getSelectedPosition () const {
        return sel_pos;
    }
    void setSelectedPosition (FilePosition pos) {
        sel_pos = pos;
    }
    HexViewState getHexViewState () const {
        return hex_view_state;
    }
    void setHexViewState (HexViewState val) {
        hex_view_state = val;
    }

    // TODO: figure out why these two functions don't work if you just use attr[on|of]
    void enableAttr (attr_t attr) {
        wattron(view.win, attr);
    }
    void disableAttr (attr_t attr) {
        wattroff(view.win, attr);
    }

    // These are things which don't rely on the config being loaded, or can behave before it is
    void setupSimpleLua () {
        lua.new_enum("ViewLocation",
            "NONE", ViewLocation::NONE,
            "Right", ViewLocation::Right,
            "Left", ViewLocation::Left,
            "Top", ViewLocation::Top,
            "Bottom", ViewLocation::Bottom
        );
        lua.new_enum("UIBarAsking",
            "NONE", UIBarAsking::NONE,
            "ShouldExit", UIBarAsking::ShouldExit
        );
        lua.new_enum("UIState",
            "Default", UIState::Default,
            "DefaultHelp", UIState::DefaultHelp,
            "Hex", UIState::Hex,
            "HexHelp", UIState::HexHelp
        );
        lua.new_enum("HexViewState",
            "Default", HexViewState::Default,
            "Editing", HexViewState::Editing
        );
        lua.new_enum("KeyHandled",
            "FullStop", KeyHandled::FullStop,
            "Handler", KeyHandled::Handler,
            "Functional", KeyHandled::Functional,
            "Special", KeyHandled::Special,
            "Drawing", KeyHandled::Drawing,
            "All", KeyHandled::All
        );
        lua.create_named_table("MColors",
            "DEFAULT", MColors::DEFAULT,
            "BLACK_BLACK", MColors::BLACK_BLACK,
            "BLACK_RED", MColors::BLACK_RED,
            "BLACK_GREEN", MColors::BLACK_GREEN,
            "BLACK_YELLOW", MColors::BLACK_YELLOW,
            "BLACK_BLUE", MColors::BLACK_BLUE,
            "BLACK_MAGENTA", MColors::BLACK_MAGENTA,
            "BLACK_CYAN", MColors::BLACK_CYAN,
            "BLACK_WHITE", MColors::BLACK_WHITE,
            "RED_BLACK", MColors::RED_BLACK,
            "RED_RED", MColors::RED_RED,
            "RED_GREEN", MColors::RED_GREEN,
            "RED_YELLOW", MColors::RED_YELLOW,
            "RED_BLUE", MColors::RED_BLUE,
            "RED_MAGENTA", MColors::RED_MAGENTA,
            "RED_CYAN", MColors::RED_CYAN,
            "RED_WHITE", MColors::RED_WHITE,
            "GREEN_BLACK", MColors::GREEN_BLACK,
            "GREEN_RED", MColors::GREEN_RED,
            "GREEN_GREEN", MColors::GREEN_GREEN,
            "GREEN_YELLOW", MColors::GREEN_YELLOW,
            "GREEN_BLUE", MColors::GREEN_BLUE,
            "GREEN_MAGENTA", MColors::GREEN_MAGENTA,
            "GREEN_CYAN", MColors::GREEN_CYAN,
            "GREEN_WHITE", MColors::GREEN_WHITE,
            "YELLOW_BLACK", MColors::YELLOW_BLACK,
            "YELLOW_RED", MColors::YELLOW_RED,
            "YELLOW_GREEN", MColors::YELLOW_GREEN,
            "YELLOW_YELLOW", MColors::YELLOW_YELLOW,
            "YELLOW_BLUE", MColors::YELLOW_BLUE,
            "YELLOW_MAGENTA", MColors::YELLOW_MAGENTA,
            "YELLOW_CYAN", MColors::YELLOW_CYAN,
            "YELLOW_WHITE", MColors::YELLOW_WHITE,
            "BLUE_BLACK", MColors::BLUE_BLACK,
            "BLUE_RED", MColors::BLUE_RED,
            "BLUE_GREEN", MColors::BLUE_GREEN,
            "BLUE_YELLOW", MColors::BLUE_YELLOW,
            "BLUE_BLUE", MColors::BLUE_BLUE,
            "BLUE_MAGENTA", MColors::BLUE_MAGENTA,
            "BLUE_CYAN", MColors::BLUE_CYAN,
            "BLUE_WHITE", MColors::BLUE_WHITE,
            "MAGENTA_BLACK", MColors::MAGENTA_BLACK,
            "MAGENTA_RED", MColors::MAGENTA_RED,
            "MAGENTA_GREEN", MColors::MAGENTA_GREEN,
            "MAGENTA_YELLOW", MColors::MAGENTA_YELLOW,
            "MAGENTA_BLUE", MColors::MAGENTA_BLUE,
            "MAGENTA_MAGENTA", MColors::MAGENTA_MAGENTA,
            "MAGENTA_CYAN", MColors::MAGENTA_CYAN,
            "MAGENTA_WHITE", MColors::MAGENTA_WHITE,
            "CYAN_BLACK", MColors::CYAN_BLACK,
            "CYAN_RED", MColors::CYAN_RED,
            "CYAN_GREEN", MColors::CYAN_GREEN,
            "CYAN_YELLOW", MColors::CYAN_YELLOW,
            "CYAN_BLUE", MColors::CYAN_BLUE,
            "CYAN_MAGENTA", MColors::CYAN_MAGENTA,
            "CYAN_CYAN", MColors::CYAN_CYAN,
            "CYAN_WHITE", MColors::CYAN_WHITE,
            "WHITE_BLACK", MColors::WHITE_BLACK,
            "WHITE_RED", MColors::WHITE_RED,
            "WHITE_GREEN", MColors::WHITE_GREEN,
            "WHITE_YELLOW", MColors::WHITE_YELLOW,
            "WHITE_BLUE", MColors::WHITE_BLUE,
            "WHITE_MAGENTA", MColors::WHITE_MAGENTA,
            "WHITE_CYAN", MColors::WHITE_CYAN,
            "WHITE_WHITE", MColors::WHITE_WHITE
        );

        lua.create_named_table("DrawingAttributes",
            "Underlined", WA_UNDERLINE,
            "Standout", WA_STANDOUT,
            // Doesn't work on mine, but I include it for completeness
            "Blink", WA_BLINK,
            "Bold", WA_BOLD,
            "Dim", WA_DIM,
            "Invisible", WA_INVIS,
            "Italic", WA_ITALIC,
            "Reverse", WA_REVERSE,
            "Normal", WA_NORMAL
            // TODO: add any more that exist
        );

        lua.create_named_table("Testing",
            "TestA", 4,
            "TestB", 5,
            "TestC", 6,
            "TestD", 7
        );


        // Utility
        lua.set_function("logAtExit", logAtExit);

        lua.set_function("isStringWhitespace", isStringWhitespace);
        lua.set_function("byteToString", byteToString);
        lua.set_function("byteToStringPadded", byteToStringPadded);
        lua.set_function("hexCharacter", hexChr);
        lua.set_function("hexCharacterToNumber", hexChrToNumber);
        lua.set_function("clearHighestHalfByte", clearHighestHalfByte);
        lua.set_function("clearLowestHalfByte", clearLowestHalfByte);
        lua.set_function("setHighestHalfByte", setHighestHalfByte);
        lua.set_function("setLowestHalfByte", setLowestHalfByte);

        // Information
        lua.set_function("getConfigPath", &UIDisplay::getConfigPath, this);

        lua.set_function("registerKeyHandler", &UIDisplay::registerKeyHandler, this);
        lua.set_function("removeKeyHandler", &UIDisplay::removeKeyHandler, this);
        lua.set_function("getNextKeyHandlerID", &UIDisplay::getNextKeyHandlerID, this);

        // Plugin information
        lua["PLUGIN_DIR"] = plugins_directory.string();
    }

    void setupLuaValues () {
        SubView::setupLua(lua);

        // Subview
        lua.set_function("createSubView", &UIDisplay::createSubView, this);
        lua.set_function("getSubView", &UIDisplay::getSubView, this);

        // Utility
        lua.set_function("moveView", &ViewWindow::move, &view);
        lua.set_function("printView", sol::resolve<void(std::string)>(&ViewWindow::print), &view);

        lua.set_function("setBarMessage", &UIDisplay::setBarMessage, this);
        lua.set_function("clearBarMessage", &UIDisplay::clearBarMessage, this);
        lua.set_function("getBarMessage", &UIDisplay::getBarMessage, this);

        // Configuration
        lua.set_function("getShouldEditMoveForward", &UIDisplay::getShouldEditMoveForward, this);
        lua.set_function("setShouldEditMoveForward", &UIDisplay::setShouldEditMoveForward, this);

        lua.set_function("getShouldExit", &UIDisplay::getShouldExit, this);
        lua.set_function("setShouldExit", &UIDisplay::setShouldExit, this);

        // Information
        lua.set_function("getViewHeight", &UIDisplay::getViewHeight, this);
        lua.set_function("getViewWidth", &UIDisplay::getViewWidth, this);
        lua.set_function("getViewX", &UIDisplay::getViewX, this);
        lua.set_function("getViewY", &UIDisplay::getViewY, this);

        lua.set_function("getFileSize", &UIDisplay::getFileSize, this);

        lua.set_function("getRowOffset", &UIDisplay::getRowOffset, this);

        // Information - Bytes
        lua.set_function("hasByte", &UIDisplay::lua_hasByte, this);
        lua.set_function("readByte", &UIDisplay::lua_readByte, this);
        lua.set_function("readBytes", &UIDisplay::lua_readBytes, this);

        // Information - Row
        lua.set_function("getRowPosition", &UIDisplay::getRowPosition, this);
        lua.set_function("setRowPosition", &UIDisplay::setRowPosition, this);

        // Information - Selected position
        lua.set_function("getSelectedPosition", &UIDisplay::getSelectedPosition, this);
        lua.set_function("setSelectedPosition", &UIDisplay::setSelectedPosition, this);
        lua.set_function("getSelectedRow", &UIDisplay::getSelectedRow, this);

        // Information - Keys
        // - Note: you should register a keyhandler rather than directly use this most of the time
        lua.set_function("getLastPressedKey", &UIDisplay::getLastPressedKey, this);
        lua.set_function("isExitKey", &UIDisplay::isExitKey, this);
        lua.set_function("isYesKey", &UIDisplay::isYesKey, this);
        lua.set_function("isQuestionKey", &UIDisplay::isQuestionKey, this);
        lua.set_function("isUpKey", &UIDisplay::isUpKey, this);
        lua.set_function("isDownKey", &UIDisplay::isDownKey, this);
        lua.set_function("isLeftKey", &UIDisplay::isLeftKey, this);
        lua.set_function("isRightKey", &UIDisplay::isRightKey, this);

        // Information - Hex View
        lua.set_function("getHexViewHeight", &ViewWindow::getHexHeight, &view);
        lua.set_function("getHexViewWidth", &ViewWindow::getHexWidth, &view);
        lua.set_function("getHexViewX", &ViewWindow::getHexX, &view);
        lua.set_function("getHexViewY", &ViewWindow::getHexY, &view);
        lua.set_function("getHexByteWidth", &ViewWindow::getHexByteWidth, &view);
        lua.set_function("getLeftViewsWidth", &ViewWindow::getLeftWidth, &view);
        lua.set_function("getRightViewsWidth", &ViewWindow::getRightWidth, &view);

        lua.set_function("getHexViewState", &UIDisplay::getHexViewState, this);
        lua.set_function("setHexViewState", &UIDisplay::setHexViewState, this);

        lua.set_function("getEditingPosition", &UIDisplay::getEditingPosition, this);
        lua.set_function("setEditingPosition", &UIDisplay::setEditingPosition, this);

        // Writing
        lua.set_function("onWrite", &UIDisplay::onWrite, this);
        lua.set_function("hasOnWrite", &UIDisplay::hasOnWrite, this);
        lua.set_function("runWrite", &UIDisplay::runWrite, this);

        // Saving
        lua.set_function("onSave", &UIDisplay::onSave, this);
        lua.set_function("runSaveListeners", &UIDisplay::runSaveListeners, this);

        // Colors
        lua.set_function("enableColor", &ViewWindow::enableColor, &view);
        lua.set_function("disableColor", &ViewWindow::disableColor, &view);

        // Attributes
        lua.set_function("enableAttribute", &UIDisplay::enableAttr, this);
        lua.set_function("disableAttribute", &UIDisplay::disableAttr, this);
    }

    void loadPlugins () {
        if (plugins_directory.empty()) {
            // There is no plugins directory. So we ignore the set plugins.
            return;
        }

        // Arrays are just tables with ints for keys
        sol::table plugins;

        // Try to load the variable if it fails, there's no plugins to load
        try {
            plugins = lua["plugins"];
        } catch (...) {
            debugLog("No plugins variable in configuration.");
            return;
        }

        // `plugin` is the key in the table
        // TODO: figure out how this behaves if the plugins start adding values to the table
        for (sol::lua_value plugin : plugins) {
            // Check if it's a valid key
            if (plugins[plugin].valid()) {
                // Make sure type is string
                if (plugins[plugin].get_type() == sol::type::string) {
                    // Get value
                    std::string plugin_filename = plugins[plugin].get<std::string>();
                    debugLog("Loading plugin: '" + plugin_filename + "'");
                    // Load script, ignoring errors (hopefully)
                    lua.script_file(plugin_filename);
                } else {
                    exit_logs.push_back("Couldn't load plugin in plugin list as it's value was not a string.");
                }
            } else {
                exit_logs.push_back("Had issues getting plugin value. This is probably an issue in the hex editor. Please report");
            }
        }
    }

    void setupBar () {
        // These properties are constant, so they are set here
        bar.height = 2;
        bar.x = 0;

        updateBarProperties();
        bar.createWindow();
        wrefresh(bar.win);
    }

    void setupView () {
        view.x = 0;
        view.y = 0;

        updateViewProperties();
        view.createWindow();
        wrefresh(view.win);
    }

    void updateBarProperties () {
        int height = getmaxy(stdscr);
        int width = getmaxx(stdscr);
        bar.width = width;
        bar.y = height - 2;
    }

    void updateViewProperties () {
        int width = getmaxx(stdscr);

        view.height = bar.y;
        view.width = width;
    }

    void drawBar () {
        //wclear(bar.win);
        werase(bar.win);
        bar.moveOrigin();

        wattron(bar.win, A_STANDOUT);

        if (bar_asking == UIBarAsking::ShouldExit) {
            bar.print("Are you sure you want to exit? (y/N)");
        } else if (bar_asking == UIBarAsking::ShouldSave) {
            bar.print("Are you sure you want to save? (y/N)");
        } else if (!bar_message.empty()) {
            bar.print(bar_message, 0, false);
            clearBarMessage();
        }

        wrefresh(bar.win);
    }

    void drawView () {
        //wclear(view.win);
        werase(view.win);

        for (SubView& sv : view.sub_views) {
            sv.runResize();
        }

        // Draw hex-view
        view.move(view.getHexX(), view.getHexY());

        FilePosition file_pos = getRowOffset();
        size_t max_size = static_cast<size_t>(view.getHexByteWidth()) * static_cast<size_t>(view.getHexHeight());
        std::vector<Byte> data = hex.readMultipleCutoff(file_pos, max_size);
        runWrite(data, file_pos);

        for (SubView& sv : view.sub_views) {
            sv.move(0, 0);

            sv.runRender();
        }

        wrefresh(view.win);
    }

    void resize () {
        updateBarProperties();
        updateViewProperties();
        bar.update();
        view.update();

        for (SubView& sv : view.sub_views) {
            sv.runResize();
        }

        wrefresh(bar.win);
        wrefresh(view.win);
        refresh();
    }

    void saveFile () {
        runSaveListeners();
        hex.saveHistoryDestructive();
        invalidateCaches();
    }

    void invalidateCaches () {
        cached_file_size = std::nullopt;
    }


// == KEY HANDLING
    // TODO: make sure all key functions are registered with lua

    bool isExitKey (int k) {
        return k == 'q' || k == 'Q';
    }
    bool isYesKey (int k) {
        return k == 'y' || key == 'Y';
    }
    bool isQuestionKey (int k) {
        return k == '?';
    }
    bool isUpArrow (int k) {
        return k == KEY_UP;
    }
    bool isUpKey (int k) {
        return isUpArrow(k) || k == 'k' || k == 'K';
    }
    bool isDownArrow (int k) {
        return k == KEY_DOWN;
    }
    bool isDownKey (int k) {
        return isDownArrow(k) || k == 'j' || k == 'J';
    }
    bool isLeftArrow(int k) {
        return k == KEY_LEFT;
    }
    bool isLeftKey (int k) {
        return isLeftArrow(k) || k == 'h' || k == 'H';
    }
    bool isRightArrow (int k) {
        return k == KEY_RIGHT;
    }
    bool isRightKey (int k) {
        return isRightArrow(k) || k == 'l' || k == 'L';
    }

    bool isEnterKey (int k) {
        return k == KEY_ENTER || k == '\n';
    }

    bool isSaveKey (int k) {
        std::string key_name = std::string(keyname(k));
        return key_name == "^s" || key_name == "^S"; // || k == 's' || k == 'S';
    }

    bool isEndOfFileKey (int k) {
        return k == 'g' || k == 'G';
    }

    bool isPageDownkey (int k) {
        return k == KEY_NPAGE;
    }

    bool isPageUpKey (int k) {
        return k == KEY_PPAGE;
    }

// == EVENT HANDLING

    KeyHandleFlags handleKeyHandlers () {
        KeyHandleFlags key_handle;

        for (std::pair<const unsigned int, sol::protected_function> handler : key_handlers) {
            auto res = handler.second(key);
            if (!res.valid()) {
                // TODO: this should probably be ignored most of the time
                sol::error err = res;
                throw err;
            } else {
                if (res.get_type() == sol::type::number) {
                    int val = res.get<int>();

                    if (val == static_cast<int>(KeyHandled::All)) {
                        continue;
                    }

                    if ((val & static_cast<int>(KeyHandled::Functional) )== 0) {
                        key_handle.functional = false;
                    }

                    if ((val & static_cast<int>(KeyHandled::Special)) == 0) {
                        key_handle.special = false;
                    }

                    if ((val & static_cast<int>(KeyHandled::Drawing)) == 0) {
                        key_handle.drawing = false;
                    }

                    if ((val & static_cast<int>(KeyHandled::Handler)) == 0) {
                        key_handle.handler = false;
                        break;
                    }
                }
                // Otherwise ignore the return value.
                // TODO: log if it returns something that isn't nil and isn't a number
            }
        }

        return key_handle;
    }

    void handleEvent () {
        KeyHandleFlags key_handle = handleKeyHandlers();

        if (key_handle.functional) {
           handleFunctional();
        }

        // Special actions
        if (key_handle.special) {
            handleSpecial();
        }

        // Drawing
        if (key_handle.drawing) {
            handleDrawing();
        }
        flushinp();
    }

    void handleDownKeyMovement () {
        FilePosition byte_count = static_cast<FilePosition>(view.getHexByteWidth());
        if (sel_pos + byte_count < getFileSize()) {
            sel_pos += byte_count;
        }
    }

    void handlePageDownMovement () {
        FilePosition page_size = static_cast<FilePosition>(view.getHexByteWidth()) *
            static_cast<FilePosition>(view.getHexHeight());
        if (sel_pos + page_size < getFileSize()) {
            // This is a bit of a hacky method to make the our cursor be at the top when we page down
            // Go double the page length (since we aren't accessing anything it doesn't matter if we go outside the file)
            logAtExit("Sel_pos: " + std::to_string(sel_pos));
            logAtExit("Page size: " + std::to_string(page_size));
            logAtExit("File Size: " + std::to_string(getFileSize()));
            sel_pos += page_size * 2;
            updateRowPosition();
            sel_pos -= page_size;
            logAtExit("Sel_pos2: " + std::to_string(sel_pos));
        } else {
            handleJumpEndOfFile();
        }
    }

    void handleUpKeyMovement () {
        FilePosition byte_count = static_cast<FilePosition>(view.getHexByteWidth());
        if (sel_pos >= byte_count) {
            sel_pos -= byte_count;
        }
    }

    void handlePageUpMovement () {
        FilePosition page_size = static_cast<FilePosition>(view.getHexByteWidth()) *
            static_cast<FilePosition>(view.getHexHeight());
        if (sel_pos >= page_size) {
            sel_pos -= page_size;
        } else {
            sel_pos = 0;
        }
    }

    void handleLeftKeyMovement () {
        if (sel_pos > 0) {
            sel_pos--;
        }
    }

    void handleLeftKeyEditingMovement () {
        if (editing_position == true) {
            editing_position = false;
        } else {
            editing_position = true;
            handleLeftKeyMovement();
        }
    }

    void handleRightKeyMovement () {
        if (sel_pos+1 < getFileSize()) {
            sel_pos++;
        }
    }

    void handleRightKeyEditingMovement () {
        if (editing_position == true) {
            editing_position = false;
            handleRightKeyMovement();
        } else {
            editing_position = true;
        }
    }

    void handleJumpEndOfFile () {
        sel_pos = getFileSize() - 1;
    }

    void updateRowPosition () {
        FilePosition byte_count = static_cast<FilePosition>(view.getHexByteWidth());
        FilePosition hex_view_rows = static_cast<FilePosition>(view.getHexHeight());
        FilePosition page_size = byte_count * hex_view_rows;

        // TODO: I don't really like having to use row_pos in the checks.
        // It'd be nice to have math that isn't dependant whatsoever on the last state of row_pos
        if (sel_pos == 0) {
            row_pos = 0;
        } else if (sel_pos < row_pos * byte_count) { // if we're currently above the currently displayed row
            // Makes so when going up, the topmost row is the one we just 'scrolled' to
            row_pos = getSelectedRow();
        } else if (sel_pos >= (row_pos * byte_count) + page_size) { // if we're below the display
            row_pos = getSelectedRow() + 1 - hex_view_rows;
        }
    }

    bool hasUnsavedChanges () const {
        return hex.hasUnsavedEdits();
    }

    void handleSave () {
        if (hasUnsavedChanges()) {
            bar_asking = UIBarAsking::ShouldSave;
        } else {
            setBarMessage("No changes to save.");
        }
    }

    void handleFunctionalDefault () {
        // If we're on default mode then we're not on a file, thus we can just exit immediately
        if (isExitKey(key)) {
            should_exit = true;
            return;
        } else if (isQuestionKey(key)) {
            state = UIState::DefaultHelp;
        }
    }

    void handleFunctionalHex () {
        if (bar_asking == UIBarAsking::ShouldExit) {
            if (isYesKey(key)) {
                should_exit = true;
                bar_asking = UIBarAsking::NONE;
            } else if (isDisplayableCharacter(key) || isEnterKey(key)) { // consider all other characters to be a no.
                bar_asking = UIBarAsking::NONE;
            }
        } else if (bar_asking == UIBarAsking::ShouldSave) {
            if (isYesKey(key)) {
                saveFile();
                bar_asking = UIBarAsking::NONE;
            } else if (isDisplayableCharacter(key)) {
                bar_asking = UIBarAsking::NONE;
            }
        } else if (hex_view_state == HexViewState::Editing) {
            if (isEnterKey(key) || isExitKey(key)) {
                hex_view_state = HexViewState::Default;
                // Reset back to 0.
                editing_position = false;
            } else if (isQuestionKey(key)) {
                state = UIState::HexHelp;
            } else if (isDownKey(key)) {
                handleDownKeyMovement();
            } else if (isUpKey(key)) {
                handleUpKeyMovement();
            } else if (isRightKey(key)) {
                handleRightKeyEditingMovement();
            } else if (isLeftKey(key)) {
                handleLeftKeyEditingMovement();
            } else if (isPageDownkey(key)) {
                handlePageDownMovement();
            } else if (isPageUpKey(key)) {
                handlePageUpMovement();
            } else if (isHexadecimalCharacter(key)) {
                char hex_char = static_cast<char>(std::toupper(key));
                Byte hex_num = hexChrToNumber(hex_char);
                std::optional<Byte> opt_value = hex.read(sel_pos);

                if (opt_value.has_value()) {
                    Byte value = opt_value.value();

                    if (editing_position == true) {
                        value = setLowestHalfByte(value, hex_num);
                    } else {
                        value = setHighestHalfByte(value, hex_num);
                    }

                    hex.edit(sel_pos, value);
                    if (getShouldEditMoveForward()) {
                        handleRightKeyEditingMovement();
                    }
                }
            } else if (isSaveKey(key)) {
                handleSave();
            } else if (isEndOfFileKey(key)) {
                handleJumpEndOfFile();
            }

            updateRowPosition();
        } else if (hex_view_state == HexViewState::Default) {
            if (isExitKey(key)) {
                bar_asking = UIBarAsking::ShouldExit;
            } else if (isQuestionKey(key)) {
                state = UIState::HexHelp;
            } else if (isDownKey(key)) {
                handleDownKeyMovement();
            } else if (isUpKey(key)) {
                handleUpKeyMovement();
            } else if (isRightKey(key)) {
                handleRightKeyMovement();
            } else if (isLeftKey(key)) {
                handleLeftKeyMovement();
            } else if (isPageDownkey(key)) {
                handlePageDownMovement();
            } else if (isPageUpKey(key)) {
                handlePageUpMovement();
            } else if (isEnterKey(key)) {
                hex_view_state = HexViewState::Editing;
            } else if (isSaveKey(key)) {
                handleSave();
            } else if (isEndOfFileKey(key)) {
                handleJumpEndOfFile();
            }

            updateRowPosition();
        }
    }

    void handleFunctional () {
        if (state == UIState::Default) {
            handleFunctionalDefault();
        } else if (state == UIState::Hex) {
            handleFunctionalHex();
        }
    }

    void handleSpecial () {
        if (key == KEY_RESIZE) {
            clear();
            resize();
        }
    }

    void handleDrawing () {
        if (state == UIState::Default) {
            drawBar();
        } else if (state == UIState::Hex) {
            drawView();
            drawBar();
        }
    }
};

int main (int argc, char** argv) {
    cxxopts::Options options("HerixTUI", "Terminal Hex Editor");
    options.add_options()
        ("h,help", "Shows help")
        ("c,config_file", "Set where the config file is located.", cxxopts::value<std::string>())
        ("locate_config", "Find where the code looks for the config")
        ("p,plugin_dir", "Set where plugins are looked for.", cxxopts::value<std::string>())
        ("locate_plugins", "Find where the code looks for plugins")
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



    std::filesystem::path plugin_dir = "";

    // Allows the passing in of the plugin dir by parameter
    if (result.count("plugin_dir") == 1) {
        plugin_dir = result["plugin_dir"].as<std::string>();

        if (isStringWhitespace(plugin_dir)) {
            std::cout << "Plugin directory passed by argument was empty." << std::endl;
            return 1;
        }
    }

    bool locate_plugins = false;
    if (result.count("locate_plugins") != 0) {
        locate_plugins = true;
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

    if (locate_plugins) {
        std::cout << "Plugins Directory: '" << plugin_dir << "'\n";
        return 0;
    }




    std::filesystem::path filename = getFilename(argc, argv);
    // TODO: allow creation of empty file to edit in
    if (filename == "") {
        std::cout << "No file to open was specified.\n";
        return 0;
    }

    setupCurses();
    try {
        UIDisplay display = UIDisplay(filename, config_file, plugin_dir, debug_mode);

        refresh();

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
