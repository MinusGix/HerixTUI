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

enum class MColors {
    DEFAULT=0,
    BLACK_BLACK=1,
    BLACK_RED=2,
    BLACK_GREEN=3,
    BLACK_YELLOW=4,
    BLACK_BLUE=5,
    BLACK_MAGENTA=6,
    BLACK_CYAN=7,
    BLACK_WHITE=8,
    RED_BLACK=9,
    RED_RED=10,
    RED_GREEN=11,
    RED_YELLOW=12,
    RED_BLUE=13,
    RED_MAGENTA=14,
    RED_CYAN=15,
    RED_WHITE=16,
    GREEN_BLACK=17,
    GREEN_RED=18,
    GREEN_GREEN=19,
    GREEN_YELLOW=20,
    GREEN_BLUE=21,
    GREEN_MAGENTA=22,
    GREEN_CYAN=23,
    GREEN_WHITE=24,
    YELLOW_BLACK=25,
    YELLOW_RED=26,
    YELLOW_GREEN=27,
    YELLOW_YELLOW=28,
    YELLOW_BLUE=29,
    YELLOW_MAGENTA=30,
    YELLOW_CYAN=31,
    YELLOW_WHITE=32,
    BLUE_BLACK=33,
    BLUE_RED=34,
    BLUE_GREEN=35,
    BLUE_YELLOW=36,
    BLUE_BLUE=37,
    BLUE_MAGENTA=38,
    BLUE_CYAN=39,
    BLUE_WHITE=40,
    MAGENTA_BLACK=41,
    MAGENTA_RED=42,
    MAGENTA_GREEN=43,
    MAGENTA_YELLOW=44,
    MAGENTA_BLUE=45,
    MAGENTA_MAGENTA=46,
    MAGENTA_CYAN=47,
    MAGENTA_WHITE=48,
    CYAN_BLACK=49,
    CYAN_RED=50,
    CYAN_GREEN=51,
    CYAN_YELLOW=52,
    CYAN_BLUE=53,
    CYAN_MAGENTA=54,
    CYAN_CYAN=55,
    CYAN_WHITE=56,
    WHITE_BLACK=57,
    WHITE_RED=58,
    WHITE_GREEN=59,
    WHITE_YELLOW=60,
    WHITE_BLUE=61,
    WHITE_MAGENTA=62,
    WHITE_CYAN=63,
    WHITE_WHITE=64,
};



enum class ViewLocation {
    NONE,
    Right,
    Left,
    Top,
    Bottom,
};

struct ViewWindow;
class SubView {
    private:
    int height = -1;
    int width = -1;
    int x = -1;
    int y = -1;

    bool visible = true;

    ViewWindow& view;


    // TODO: find out if there's a way to see if on_render has a value without this optional
    sol::protected_function on_render;
    sol::protected_function on_resize;

    ViewLocation loc = ViewLocation::NONE;

    public:
    SubView (ViewLocation t_loc, ViewWindow& t_view);
    static void setupLua (sol::state& lua);
    void setHeight (int val);
    int getHeight () const;
    void setWidth (int val);
    int getWidth () const;
    void setX (int val);
    int getX () const;
    void setY (int val);
    int getY () const;
    void setLoc (ViewLocation val);
    ViewLocation getLoc () const;
    void setVisible (bool val);
    bool getVisible () const;

    void onRender (sol::protected_function cb);
    void clearOnRender ();

    void onResize (sol::protected_function cb);
    void clearOnResize ();
    void print (std::string text);
    void printStandout (std::string text);
    void move (int to_x, int to_y);

    void runRender ();
    void runResize ();
};

struct ViewWindow : public Window {
    std::vector<SubView> sub_views;

    ~ViewWindow ();

    int getHexX () const;
    int getHexY () const;
    int getHexWidth () const;
    int getHexHeight () const;

    int getHexByteWidth () const;

    int getRightWidth () const;
    int getLeftWidth () const;

    void enableColor (MColors color);
    void disableColor (MColors color);
};

enum class UIBarAsking {
    NONE,
    Message,
    ShouldExit,
    ShouldSave,
};
enum class UIState {
    Default,
    DefaultHelp,
    Hex,
    HexHelp,
};
enum class HexViewState {
    Default,
    Editing,
};
// Handler actions: The continued execution of handlers
// Functional actions: Normal actions
// Special actions: RESIZE
// Drawing actions: Drawing to the screen

// This is used as flags, so an int for KeyHandling might not (likely not) be a legitimate enum value
// So they should stay as ints
enum class KeyHandled {
    FullStop = 0, // Don't do anything else. Stop handling events right after this.
    Handler = 1, // Allow continued execution of handlers
    Functional = 2, // Only allow functional actions
    Special = 4, // Only allow special actions
    Drawing = 8, // Only allow drawing actions
    All = Handler | Functional | Special | Drawing, // Continue doing everything.
};

struct KeyHandleFlags {
    bool handler = true;
    bool functional = true;
    bool special = true;
    bool drawing = true;
};
