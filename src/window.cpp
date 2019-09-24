#include "./window.hpp"

#include <cassert>

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