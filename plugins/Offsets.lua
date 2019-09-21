-- Configuration
if offset_view_config == nil then
    offset_view_config = {}
end
if offset_view_config["width"] == nil then
    offset_view_config["width"] = 8
end

-- Create view
offset_view_id = createSubView(ViewLocation.Left)

function offset_view_updateDimensions ()
    offset_view = getSubView(offset_view_id)
    offset_view:setHeight(getHexViewHeight())
    -- 8% of screen
    offset_view:setWidth(offset_view_config["width"])
    -- TODO: setX and setY should be handled properly in the main code..
    offset_view:setX(0)
    offset_view:setY(0)
end

offset_view_updateDimensions()


getSubView(offset_view_id):onRender(function ()
    offset_view = getSubView(offset_view_id)
    byte_entries_row = getHexViewHeight()-1
    width = offset_view:getWidth() - 1
    format_string = "%0" .. tostring(width) .. "X"
    base_offset = getRowOffset()
    byte_count = getHexByteWidth()
    -- TODO: check if there's enough space for the number
    for i = 0, byte_entries_row do
        offset_view:move(0, i)
        offset_view:print(string.format(format_string, byte_count * i + base_offset))
    end
end)
getSubView(offset_view_id):onResize(function ()
    offset_view_updateDimensions()
end)