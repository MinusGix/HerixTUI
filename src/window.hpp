#ifndef FILE_SEEN_MWINDOW
#define FILE_SEEN_MWINDOW

#include <string>
#include <vector>

#include <curses.h>
#include "./mutil.hpp"
#include "./subview.hpp"

struct Window {
    WINDOW* win = nullptr;
    int height = -1;
    int width = -1;
    int x = -1;
    int y = -1;

    void createWindow ();

    void update ();

    void moveOrigin ();

    void move (int to_x, int to_y);

    void print (std::string str);

    void print (const char* str);

    void print (std::string str, int x_pos, bool allow_wrapping);
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

#endif
