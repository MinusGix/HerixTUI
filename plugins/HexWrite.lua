-- Requires highlighter currently.

if hex_write_config == nil then
    hex_write_config = {}
end
if hex_write_config["selected_attribute"] == nil then
    hex_write_config["selected_attribute"] = DrawingAttributes.Standout
end
if hex_write_config["selected_editing_attribute"] == nil then
    hex_write_config["selected_editing_attribute"] = DrawingAttributes.Underlined
end

-- Cache the bytes that byteToStringPadded uses
-- I was unsure if I should do this optimization in lua or in the byteToStringPadded.
local bytes = {}
for index=0, 255 do
    bytes[index] = byteToStringPadded(index)
end
function hw_byte_to_string (byte)
    return bytes[byte]
end

local prev_state = getHexViewState()

function base_on_write (data, size, position)
    local byte_entries_col = getHexByteWidth()
    local byte_entries_row = getHexViewHeight()
    local selected_pos = getSelectedPosition()
    local hex_view_state = getHexViewState()
    local hex_view_x = getHexViewX()
    local hex_view_y = getHexViewY()

    -- Unsure if this is that great of an ''optimization''
    local func_on_write
    if hex_view_state == HexViewState.Editing then
        func_on_write = on_write_selected_state_editing
    else
        func_on_write = on_write_selected
    end

    highlight_update(position, size, prev_state == HexViewState.Editing and hex_view_state ~= HexViewState.Editing)
    prev_state = hex_view_state

    fh_highlight_get_load_screen()

    -- This assumes that data is sequential, not sure we can rely on that
    for j, v in ipairs(data) do
        local i = j - 1 -- one-indexed
        if i % byte_entries_col == 0 and i ~= 0 then
            moveView(hex_view_x, hex_view_y + (i // byte_entries_col))
        end

        local v_highlight_attr = highlight_get(position + i)

        toggle_highlight_attributes(v_highlight_attr, true)

        local byte_text = hw_byte_to_string(v)
        if selected_pos == (position + i) then
            func_on_write(byte_text)
        else
            printView(byte_text .. " ")
        end

        toggle_highlight_attributes(v_highlight_attr, false)
    end
end

function on_write_selected_state_editing (byte_text)
    if getEditingPosition() == false then
        enableAttribute(hex_write_config["selected_editing_attribute"])
        printView(string.sub(byte_text, 1, 1))
        disableAttribute(hex_write_config["selected_editing_attribute"])
        printView(string.sub(byte_text, 2, 2) .. " ")
    else
        printView(string.sub(byte_text, 1, 1))
        enableAttribute(hex_write_config["selected_editing_attribute"])
        printView(string.sub(byte_text, 2, 2))
        disableAttribute(hex_write_config["selected_editing_attribute"])
        printView(" ")
    end
end

function on_write_selected (byte_text)
    enableAttribute(hex_write_config["selected_attribute"])
    printView(byte_text)
    disableAttribute(hex_write_config["selected_attribute"])
    printView(" ")
end

onWrite(base_on_write)