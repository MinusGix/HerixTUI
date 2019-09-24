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
