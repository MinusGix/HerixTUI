function inform_of_error (val)
    logAtExit(val)
    if getShouldQuickExit() == true then
        error(val)
    end
end

if highlighter_config == nil then
    highlighter_config = {}
end

HighlightType = {
    None=0,
    Underlined=1,
    Standout=2,
    Dim=3,
}


temp_mcolors_elements = 0
for a,x in ipairs(MColors) do
    temp_mcolors_elements = temp_mcolors_elements + 1
end

-- Add colors

highlight_type_color_start = 0
highlight_type_color_end = 0
for a,x in pairs(HighlightType) do
    highlight_type_color_start = highlight_type_color_start + 1
end
-- Colors are stored as Color_[FOREGROUND]_[BACKGROUND]
for k,v in pairs(MColors) do
    HighlightType["Color_" .. k] = highlight_type_color_start + v
    if v > highlight_type_color_end then
        highlight_type_color_end = v
    end
end
-- One past end
highlight_type_color_end = highlight_type_color_end + 1



highlighter = {
    init=false,
    base_highlight_type=HighlightType.None,
}

-- Returns styling for position
function highlight_get (position)
    return highlighter.base_highlight_type
end

-- Called by HexWrite when it needs to update.
-- Readposition would be the position it's starting to read at
-- This allows us to check if we need to parse more data
function highlight_update (read_position, size, flush_cache)

end

function attributeToggle (attr, to)
    if to == true then
        enableAttribute(attr)
    else
        disableAttribute(attr)
    end
end

function colorToggle (color, to)
    if to == true then
        enableColor(color)
    else
        disableColor(color)
    end
end

function toggle_highlight_attributes (attr, to)
    if attr == HighlightType.Underlined then
        attributeToggle(DrawingAttributes.Underlined, to)
    elseif attr == HighlightType.Standout then
        attributeToggle(DrawingAttributes.Standout, to)
    elseif attr == HighlightType.Dim then
        attributeToggle(DrawingAttributes.Dim, to)
    elseif attr >= highlight_type_color_start and attr < highlight_type_color_end then -- it's a color
        colorToggle(attr - highlight_type_color_start, to)
    else
        -- For custom implementations
        toggle_highlight_attributes_custom(attr, to)
    end
end

function toggle_highlight_attributes_custom (attr, to)

end