-- Configuration
if ascii_view_config == nil then
    ascii_view_config = {}
end
if ascii_view_config["highlight_respective_character"] == nil then
    ascii_view_config["highlight_respective_character"] = true
end

ascii_view_id = createSubView(ViewLocation.Right)
ascii_view_focused = false


function ascii_view_updateDimensions ()
    ascii_view = getSubView(ascii_view_id)
    ascii_view:setHeight(getHexViewHeight())
    ascii_view:setWidth(math.floor((getViewWidth() - getLeftViewsWidth()) / 4))
    ascii_view:setX(0)
    ascii_view:setY(0)
end

ascii_view_updateDimensions()


registerKeyHandler(function (key)
    if getHexViewState() == HexViewState.Editing then
        if key == string.byte('\t') then
            logAtExit("Love! And Peace!")
            ascii_view_focused = true
        end
    end
end)

getSubView(ascii_view_id):onRender(function ()
    local ascii_view = getSubView(ascii_view_id)
    local byte_entries_col = getHexByteWidth()
    local byte_entries_row = getHexViewHeight();
    local byte_entries = byte_entries_col*byte_entries_row
    local y_pos = 0
    local sel_pos = getSelectedPosition() - (getRowPosition() * getHexByteWidth())
    local row_offset = getRowOffset()
    local bytes = readBytes(row_offset, byte_entries)

    for i=1, #bytes do
        if (i-1) % byte_entries_col == 0 then
            ascii_view:move(0, y_pos)
            y_pos = y_pos + 1
        end

        local v_highlight_attr = highlight_get(row_offset + i - 1)
        toggle_highlight_attributes(v_highlight_attr, true)

        local byte = bytes[i]
        local is_displayable = byte >= 32 and byte <= 126
        local should_standout = sel_pos == i-1 and ascii_view_config["highlight_respective_character"] == true

        if should_standout then
            enableAttribute(DrawingAttributes.Standout)
        else
            enableColor(MColors.GREEN_BLACK)
        end

        if is_displayable then
            ascii_view:print(string.char(byte))
        else
            ascii_view:print(".")
        end

        if should_standout then
            disableAttribute(DrawingAttributes.Standout)
        else
            disableColor(MColors.GREEN_BLACK)
        end

        toggle_highlight_attributes(v_highlight_attr, false)
    end

end)
getSubView(ascii_view_id):onResize(function ()
    ascii_view_updateDimensions()
end)