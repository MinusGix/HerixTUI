#include "./subview.hpp"

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
