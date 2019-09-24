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
