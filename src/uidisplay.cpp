#include "./uidisplay.hpp"

// Note: these two functions should be ignored after initialization!
HerixLib::ChunkSize UIDisplay::getMaxChunkMemory () {
    return lua.get_or("max_chunk_memory", 1024*10UL);
}
HerixLib::ChunkSize UIDisplay::getMaxChunkSize () {
    return lua.get_or("max_chunk_size", 1024UL);
}


UIDisplay::UIDisplay (std::filesystem::path t_filename, std::filesystem::path t_config_file, std::filesystem::path t_plugins_directory, bool t_debug) {
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

    hex = HerixLib::Herix(t_filename, getMaxChunkMemory(), getMaxChunkSize());

    setupBar();
    setupView();

    setupLuaValues();
    loadPlugins ();

    state = UIState::Hex;
}

UIDisplay::~UIDisplay () {
    delwin(bar.win);
    delwin(view.win);
}

void UIDisplay::debugLog (std::string val) {
    if (debug) {
        logAtExit(val);
    }
}

bool UIDisplay::getEditingPosition () const {
    return editing_position;
}
void UIDisplay::setEditingPosition (bool val) {
    editing_position = val;
}

void UIDisplay::listenForWrite (sol::protected_function cb) {
    on_write = cb;
}
void UIDisplay::runWriteListeners (std::vector<HerixLib::Byte> data, HerixLib::FilePosition file_pos) {
    if (hasWriteListeners()) {
        auto v = on_write(data, data.size(), file_pos);
        if (!v.valid()) {
            logAtExit("Error in write!");
            sol::error err = v;
            throw err;
        }
    }
}
bool UIDisplay::hasWriteListeners () const {
    return static_cast<bool>(on_write);
}

size_t UIDisplay::listenForSave (sol::protected_function cb) {
    on_save.push_back(cb);
    return on_save.size() - 1;
}

void UIDisplay::runSaveListeners () {
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

bool UIDisplay::getShouldExit () const {
    return should_exit;
}

void UIDisplay::setShouldExit(bool val) {
    should_exit = val;
}

int UIDisplay::getLastPressedKey () {
    return key;
}

std::string UIDisplay::getConfigPath () {
    return config_path.string();
}

unsigned int UIDisplay::getNextKeyHandlerID () {
    return key_handler_id;
}

bool UIDisplay::getShouldEditMoveForward () const {
    return should_edit_move_forward;
}

void UIDisplay::setShouldEditMoveForward (bool val) {
    should_edit_move_forward = val;
}

unsigned int UIDisplay::registerKeyHandler (sol::protected_function handler) {
    key_handlers.push_back(std::make_pair(key_handler_id, handler));
    return key_handler_id++;
}

void UIDisplay::removeKeyHandler (int id) {
    // TODO: implement this
}

bool UIDisplay::setBarMessage (std::string value) {
    if (bar_asking != UIBarAsking::NONE) {
        return false;
    }
    bar_message = value;
    return true;
}
void UIDisplay::clearBarMessage () {
    bar_message = "";
}
std::string UIDisplay::getBarMessage () {
    return bar_message;
}

// TODO: make this invalidate the cache if the file is saved.
size_t UIDisplay::getFileSize () {
    if (cached_file_size.has_value()) {
        return cached_file_size.value();
    } else {
        cached_file_size = hex.getFileSize();
        return cached_file_size.value();
    }
}

size_t UIDisplay::createSubView (ViewLocation loc) {
    view.sub_views.push_back(SubView(loc, view));
    return view.sub_views.size() - 1;
}
SubView& UIDisplay::getSubView (size_t id) {
    return view.sub_views.at(id);
}

HerixLib::FilePosition UIDisplay::getSelectedRow () {
    // TODO: the rounding down might be unneeded?
    return (sel_pos - (sel_pos % static_cast<HerixLib::FilePosition>(view.getHexByteWidth()))) // Rounds down towards multiple of hexByteWidth
        / static_cast<HerixLib::FilePosition>(view.getHexByteWidth());
}
int UIDisplay::getViewHeight () const {
    return view.height;
}
int UIDisplay::getViewWidth () const {
    return view.width;
}
int UIDisplay::getViewX () const {
    return view.x;
}
int UIDisplay::getViewY () const {
    return view.y;
}
bool UIDisplay::lua_hasByte (HerixLib::FilePosition pos) {
    return hex.read(pos).has_value();
}
HerixLib::Byte UIDisplay::lua_readByte (HerixLib::FilePosition pos) {
    return hex.read(pos).value();
}
std::vector<HerixLib::Byte> UIDisplay::lua_readBytes (HerixLib::FilePosition pos, size_t length) {
    return hex.readMultipleCutoff(pos, length);
}
HerixLib::FilePosition UIDisplay::getRowOffset () const {
    return row_pos * static_cast<HerixLib::FilePosition>(view.getHexByteWidth());
}
HerixLib::FilePosition UIDisplay::getRowPosition () const {
    return row_pos;
}
void UIDisplay::setRowPosition (HerixLib::FilePosition pos) {
    row_pos = pos;
}
HerixLib::FilePosition UIDisplay::getSelectedPosition () const {
    return sel_pos;
}
void UIDisplay::setSelectedPosition (HerixLib::FilePosition pos) {
    sel_pos = pos;
}
HexViewState UIDisplay::getHexViewState () const {
    return hex_view_state;
}
void UIDisplay::setHexViewState (HexViewState val) {
    hex_view_state = val;
}

// TODO: figure out why these two functions don't work if you just use attr[on|of]
void UIDisplay::enableAttr (attr_t attr) {
    wattron(view.win, attr);
}
void UIDisplay::disableAttr (attr_t attr) {
    wattroff(view.win, attr);
}

// These are things which don't rely on the config being loaded, or can behave before it is
void UIDisplay::setupSimpleLua () {
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

void UIDisplay::setupLuaValues () {
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
    lua.set_function("listenForWrite", &UIDisplay::listenForWrite, this);
    lua.set_function("hasWriteListeners", &UIDisplay::hasWriteListeners, this);
    lua.set_function("runWriteListeners", &UIDisplay::runWriteListeners, this);

    // Saving
    lua.set_function("listenForSave", &UIDisplay::listenForSave, this);
    lua.set_function("runSaveListeners", &UIDisplay::runSaveListeners, this);

    // Colors
    lua.set_function("enableColor", &ViewWindow::enableColor, &view);
    lua.set_function("disableColor", &ViewWindow::disableColor, &view);

    // Attributes
    lua.set_function("enableAttribute", &UIDisplay::enableAttr, this);
    lua.set_function("disableAttribute", &UIDisplay::disableAttr, this);
}

void UIDisplay::loadPlugins () {
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

void UIDisplay::setupBar () {
    // These properties are constant, so they are set here
    bar.height = 2;
    bar.x = 0;

    updateBarProperties();
    bar.createWindow();
    wrefresh(bar.win);
}

void UIDisplay::setupView () {
    view.x = 0;
    view.y = 0;

    updateViewProperties();
    view.createWindow();
    wrefresh(view.win);
}

void UIDisplay::updateBarProperties () {
    int height = getmaxy(stdscr);
    int width = getmaxx(stdscr);
    bar.width = width;
    bar.y = height - 2;
}

void UIDisplay::updateViewProperties () {
    int width = getmaxx(stdscr);

    view.height = bar.y;
    view.width = width;
}

void UIDisplay::drawBar () {
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

void UIDisplay::drawView () {
    //wclear(view.win);
    werase(view.win);

    for (SubView& sv : view.sub_views) {
        sv.runResize();
    }

    // Draw hex-view
    view.move(view.getHexX(), view.getHexY());

    HerixLib::FilePosition file_pos = getRowOffset();
    size_t max_size = static_cast<size_t>(view.getHexByteWidth()) * static_cast<size_t>(view.getHexHeight());
    std::vector<HerixLib::Byte> data = hex.readMultipleCutoff(file_pos, max_size);
    runWriteListeners(data, file_pos);

    for (SubView& sv : view.sub_views) {
        sv.move(0, 0);

        sv.runRender();
    }

    wrefresh(view.win);
}

void UIDisplay::resize () {
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

void UIDisplay::saveFile () {
    runSaveListeners();
    hex.saveHistoryDestructive();
    invalidateCaches();
}

void UIDisplay::invalidateCaches () {
    cached_file_size = std::nullopt;
}


// == KEY HANDLING
// TODO: make sure all key functions are registered with lua

bool UIDisplay::isExitKey (int k) const {
    return k == 'q' || k == 'Q';
}
bool UIDisplay::isYesKey (int k) const {
    return k == 'y' || key == 'Y';
}
bool UIDisplay::isQuestionKey (int k) const {
    return k == '?';
}
bool UIDisplay::isUpArrow (int k) const {
    return k == KEY_UP;
}
bool UIDisplay::isUpKey (int k) const {
    return isUpArrow(k) || k == 'k' || k == 'K';
}
bool UIDisplay::isDownArrow (int k) const {
    return k == KEY_DOWN;
}
bool UIDisplay::isDownKey (int k) const {
    return isDownArrow(k) || k == 'j' || k == 'J';
}
bool UIDisplay::isLeftArrow(int k) const {
    return k == KEY_LEFT;
}
bool UIDisplay::isLeftKey (int k) const {
    return isLeftArrow(k) || k == 'h' || k == 'H';
}
bool UIDisplay::isRightArrow (int k) const {
    return k == KEY_RIGHT;
}
bool UIDisplay::isRightKey (int k) const {
    return isRightArrow(k) || k == 'l' || k == 'L';
}

bool UIDisplay::isEnterKey (int k) const {
    return k == KEY_ENTER || k == '\n';
}

bool UIDisplay::isSaveKey (int k) const {
    std::string key_name = std::string(keyname(k));
    return key_name == "^s" || key_name == "^S"; // || k == 's' || k == 'S';
}

bool UIDisplay::isEndOfFileKey (int k) const {
    return k == 'g' || k == 'G';
}

bool UIDisplay::isPageDownkey (int k) const {
    return k == KEY_NPAGE;
}

bool UIDisplay::isPageUpKey (int k) const {
    return k == KEY_PPAGE;
}

// == EVENT HANDLING

KeyHandleFlags UIDisplay::handleKeyHandlers () {
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

void UIDisplay::handleEvent () {
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

void UIDisplay::handleDownKeyMovement () {
    HerixLib::FilePosition byte_count = static_cast<HerixLib::FilePosition>(view.getHexByteWidth());
    if (sel_pos + byte_count < getFileSize()) {
        sel_pos += byte_count;
    }
}

void UIDisplay::handlePageDownMovement () {
    HerixLib::FilePosition page_size = static_cast<HerixLib::FilePosition>(view.getHexByteWidth()) *
        static_cast<HerixLib::FilePosition>(view.getHexHeight());
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

void UIDisplay::handleUpKeyMovement () {
    HerixLib::FilePosition byte_count = static_cast<HerixLib::FilePosition>(view.getHexByteWidth());
    if (sel_pos >= byte_count) {
        sel_pos -= byte_count;
    }
}

void UIDisplay::handlePageUpMovement () {
    HerixLib::FilePosition page_size = static_cast<HerixLib::FilePosition>(view.getHexByteWidth()) *
        static_cast<HerixLib::FilePosition>(view.getHexHeight());
    if (sel_pos >= page_size) {
        sel_pos -= page_size;
    } else {
        sel_pos = 0;
    }
}

void UIDisplay::handleLeftKeyMovement () {
    if (sel_pos > 0) {
        sel_pos--;
    }
}

void UIDisplay::handleLeftKeyEditingMovement () {
    if (editing_position == true) {
        editing_position = false;
    } else {
        editing_position = true;
        handleLeftKeyMovement();
    }
}

void UIDisplay::handleRightKeyMovement () {
    if (sel_pos+1 < getFileSize()) {
        sel_pos++;
    }
}

void UIDisplay::handleRightKeyEditingMovement () {
    if (editing_position == true) {
        editing_position = false;
        handleRightKeyMovement();
    } else {
        editing_position = true;
    }
}

void UIDisplay::handleJumpEndOfFile () {
    sel_pos = getFileSize() - 1;
}

void UIDisplay::updateRowPosition () {
    HerixLib::FilePosition byte_count = static_cast<HerixLib::FilePosition>(view.getHexByteWidth());
    HerixLib::FilePosition hex_view_rows = static_cast<HerixLib::FilePosition>(view.getHexHeight());
    HerixLib::FilePosition page_size = byte_count * hex_view_rows;

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

bool UIDisplay::hasUnsavedChanges () const {
    return hex.hasUnsavedEdits();
}

void UIDisplay::handleSave () {
    if (hasUnsavedChanges()) {
        bar_asking = UIBarAsking::ShouldSave;
    } else {
        setBarMessage("No changes to save.");
    }
}

void UIDisplay::handleFunctionalDefault () {
    // If we're on default mode then we're not on a file, thus we can just exit immediately
    if (isExitKey(key)) {
        should_exit = true;
        return;
    } else if (isQuestionKey(key)) {
        state = UIState::DefaultHelp;
    }
}

void UIDisplay::handleFunctionalHex () {
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
            HerixLib::Byte hex_num = hexChrToNumber(hex_char);
            std::optional<HerixLib::Byte> opt_value = hex.read(sel_pos);

            if (opt_value.has_value()) {
                HerixLib::Byte value = opt_value.value();

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

void UIDisplay::handleFunctional () {
    if (state == UIState::Default) {
        handleFunctionalDefault();
    } else if (state == UIState::Hex) {
        handleFunctionalHex();
    }
}

void UIDisplay::handleSpecial () {
    if (key == KEY_RESIZE) {
        clear();
        resize();
    }
}

void UIDisplay::handleDrawing () {
    if (state == UIState::Default) {
        drawBar();
    } else if (state == UIState::Hex) {
        drawView();
        drawBar();
    }
}
