#ifndef FILE_SEEN_UIDISPLAY
#define FILE_SEEN_UIDISPLAY

#include <string>
#include "./mutil.hpp"
#include "./window.hpp"
#include "./subview.hpp"

struct InformationNote {
    std::string name;
    sol::protected_function text_func;

    InformationNote (std::string t_name, sol::protected_function t_text_func) :
        name(t_name), text_func(t_text_func) {}
};

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
        "PLUGIN_DIR .. \"/FileHighlighter_PNG.lua\","
        "PLUGIN_DIR .. \"/FileHighlighter_GIF.lua\","
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
    // If it should be quick to exit on issues.
    bool quick_exit = false;

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

    std::vector<InformationNote> information_notes;
    std::string current_information_text = "";
    size_t information_selected = 0;
    // Used for scrolling in InfoAsking and Info
    size_t information_row_pos = 0;

    sol::protected_function on_write;
    // Callbacks which are called when we save.
    // These are assured to be called _before_ we save, so that any special edits can happen
    std::vector<sol::protected_function> on_save;

    // TODO: add on_post_save listeners

    std::vector<sol::protected_function> on_undo;
    std::vector<sol::protected_function> on_redo;

    std::vector<sol::protected_function> on_init;


    std::optional<size_t> cached_file_end = std::nullopt;
    bool should_edit_move_forward = true;

    // Note: these two functions should be ignored after initialization!
    HerixLib::ChunkSize getMaxChunkMemory ();
    HerixLib::ChunkSize getMaxChunkSize ();


    UIDisplay (std::filesystem::path t_filename, std::filesystem::path t_config_file, std::filesystem::path t_plugins_directory, bool t_allow_writing, std::pair<HerixLib::AbsoluteFilePosition, std::optional<HerixLib::AbsoluteFilePosition>> file_range, bool t_debug);

    ~UIDisplay ();

    void debugLog (std::string val);

    bool getEditingPosition () const;
    void setEditingPosition (bool val);

    void listenForInit (sol::protected_function cb);

    void registerInfo (std::string name, sol::function cb);
    void deregisterInfo (std::string name);

    void listenForWrite (sol::protected_function cb);
    void runWriteListeners (std::vector<HerixLib::Byte> data, HerixLib::FilePosition file_pos);
    bool hasWriteListeners () const;

    size_t listenForSave (sol::protected_function cb);

    void runSaveListeners ();
    // TODO: add function to remove save listener
    // TODO: add function to get save listener count

    bool getShouldExit () const;

    void setShouldQuickExit (bool val);
    bool getShouldQuickExit () const;

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

    size_t getFileEnd ();

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

    void undo (bool dialog=false);
    void redo (bool dialog=false);

    void listenForUndo (sol::protected_function cb);
    void listenForRedo (sol::protected_function cb);

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
    bool isUndoKey (int k) const;
    bool isRedoKey (int k) const;

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

    void handleFunctionalInfoAsking ();

    void handleFunctionalInfo ();

    void handleFunctional ();

    void handleSpecial ();

    void handleDrawing ();
    void drawInfoAsking ();
    void drawInfo ();
};

#endif
