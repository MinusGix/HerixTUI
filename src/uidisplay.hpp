#ifndef FILE_SEEN_UIDISPLAY
#define FILE_SEEN_UIDISPLAY

#include <string>
#include "./mutil.hpp"
#include "./window.hpp"
#include "./subview.hpp"

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
    HerixLib::FilePosition row_pos = 0;
    // In bytes
    HerixLib::FilePosition sel_pos = 0;
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

    HerixLib::Herix hex;

    sol::protected_function on_write;
    // Callbacks which are called when we save.
    // These are assured to be called _before_ we save, so that any special edits can happen
    std::vector<sol::protected_function> on_save;

    // TODO: add on_post_save listeners


    std::optional<size_t> cached_file_size = std::nullopt;
    bool should_edit_move_forward = true;

    // Note: these two functions should be ignored after initialization!
    HerixLib::ChunkSize getMaxChunkMemory ();
    HerixLib::ChunkSize getMaxChunkSize ();


    UIDisplay (std::filesystem::path t_filename, std::filesystem::path t_config_file, std::filesystem::path t_plugins_directory, bool t_debug);

    ~UIDisplay ();

    void debugLog (std::string val);

    bool getEditingPosition () const;
    void setEditingPosition (bool val);

    void listenForWrite (sol::protected_function cb);
    void runWriteListeners (std::vector<HerixLib::Byte> data, HerixLib::FilePosition file_pos);
    bool hasWriteListeners () const;

    size_t listenForSave (sol::protected_function cb);

    void runSaveListeners ();
    // TODO: add function to remove save listener
    // TODO: add function to get save listener count

    bool getShouldExit () const;

    void setShouldExit(bool val);

    int getLastPressedKey ();

    std::string getConfigPath ();

    unsigned int getNextKeyHandlerID ();

    bool getShouldEditMoveForward () const;

    void setShouldEditMoveForward (bool val);

    unsigned int registerKeyHandler (sol::protected_function handler);

    void removeKeyHandler (int id);

    bool setBarMessage (std::string value);
    void clearBarMessage ();
    std::string getBarMessage ();

    size_t getFileSize ();

    size_t createSubView (ViewLocation loc);
    SubView& getSubView (size_t id);

    HerixLib::FilePosition getSelectedRow ();
    int getViewHeight () const;
    int getViewWidth () const;
    int getViewX () const;
    int getViewY () const;
    bool lua_hasByte (HerixLib::FilePosition pos);
    HerixLib::Byte lua_readByte (HerixLib::FilePosition pos);
    std::vector<HerixLib::Byte> lua_readBytes (HerixLib::FilePosition pos, size_t length);
    HerixLib::FilePosition getRowOffset () const;
    HerixLib::FilePosition getRowPosition () const;
    void setRowPosition (HerixLib::FilePosition pos);
    HerixLib::FilePosition getSelectedPosition () const;
    void setSelectedPosition (HerixLib::FilePosition pos);
    HexViewState getHexViewState () const;
    void setHexViewState (HexViewState val);

    void enableAttr (attr_t attr);
    void disableAttr (attr_t attr);

    // These are things which don't rely on the config being loaded, or can behave before it is
    void setupSimpleLua ();

    void setupLuaValues ();

    void loadPlugins ();
    void loadPlugin (const std::string& filename);

    void setupBar ();

    void setupView ();

    void updateBarProperties ();

    void updateViewProperties ();
    void drawBar ();

    void drawView ();

    void resize ();

    void saveFile ();

    void invalidateCaches ();

// == KEY HANDLING
    // TODO: make these all const
    bool isExitKey (int k) const;
    bool isYesKey (int k) const;
    bool isQuestionKey (int k) const;
    bool isUpArrow (int k) const;
    bool isUpKey (int k) const;
    bool isDownArrow (int k) const;
    bool isDownKey (int k) const;
    bool isLeftArrow(int k) const;
    bool isLeftKey (int k) const;
    bool isRightArrow (int k) const;
    bool isRightKey (int k) const;
    bool isEnterKey (int k) const;
    bool isSaveKey (int k) const;
    bool isEndOfFileKey (int k) const;
    bool isPageDownkey (int k) const;
    bool isPageUpKey (int k) const;
    bool isEndKey (int k) const;
    bool isHomeKey (int k) const;

// == EVENT HANDLING

    KeyHandleFlags handleKeyHandlers ();

    void handleInit ();
    void handleEvent ();

    void handleDownKeyMovement ();

    void handlePageDownMovement ();

    void handleUpKeyMovement ();

    void handlePageUpMovement ();

    void handleLeftKeyMovement ();

    void handleLeftKeyEditingMovement ();

    void handleRightKeyMovement ();

    void handleRightKeyEditingMovement ();

    void handleJumpEndOfFile ();

    void handleJumpEndOfLine ();
    void handleJumpStartOfLine ();

    void updateRowPosition ();

    bool hasUnsavedChanges () const;

    void handleSave ();

    void handleFunctionalDefault ();

    void handleFunctionalHex ();

    void handleFunctional ();

    void handleSpecial ();

    void handleDrawing ();
};

#endif
