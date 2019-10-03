if file_highlighter_config == nil then
    file_highlighter_config = {}
end

if file_highlighter_config.highlight_method == nil then
    file_highlighter_config.highlight_method = {
        HighlightType.Color_RED_BLACK,
        HighlightType.Color_BLACK_RED,
        HighlightType.Color_BLUE_BLACK,
        HighlightType.Color_BLACK_BLUE,
        HighlightType.Color_YELLOW_BLACK,
        HighlightType.Color_BLACK_YELLOW
    }
end

fh_data = {
    -- Key of the current format in fh_data.formats
    -- nil means it's not set
    -- 0 means it can't be set to anything
    chosen_format = nil,
    formats = {},
    cached_pos = {},
    method_index = 1,
    highlight_method = file_highlighter_config.highlight_method
}

function fh_has_chosen_format ()
    return fh_data.chosen_format ~= nil and fh_data.chosen_format ~= 0
end

function fh_get_chosen_format ()
    return fh_data.formats[fh_data.chosen_format]
end

function fh_next_highlight_method ()
    local ret = fh_data.highlight_method[fh_data.method_index]
    fh_data.method_index = fh_data.method_index + 1

    if fh_data.method_index > #fh_data.highlight_method then
        fh_data.method_index = 1
    end

    return ret
end

-- Clears everything
--  Needs to reparse format
--  Needs to recache things.
function fh_flush_cache ()
    fh_data.cached_pos = {}
    -- Reparse, this should overwrite any previous values.
    if fh_data.chosen_format ~= nil and fh_data.chosen_format ~= 0 then
        fh_parse_format(fh_get_chosen_format())
    end
end

-- Initializing and registering format

function fh_register_format (data)
    local name = data.name
    if name == nil then
        error("File Highlighter: Format did not have name for structure.")
    end

    if fh_data.formats[name] ~= nil then
        logAtExit("File format {" .. tostring(name) .. "} was overwritten by a format with")
    end

    fh_initialize_format(data)

    fh_data.formats[name] = data
end

function fh_initialize_format (format)
    fh_initialize_structures(format)
end
function fh_initialize_structures (format)
    for index = 1, #format.structures do
        fh_initialize_structure(format, format.structures[index])
    end
end
function fh_initialize_structure (format, struct)
    if struct.name == nil then
        error("File Highlighter: {" .. tostring(format.name) .. "} had unnamed structure at index " .. tostring(struct_index) .. ".")
    end

    if struct.entries == nil then
        error("File Highlighter: {" .. tostring(format.name) .. "} had no entries for struct " .. tostring(struct.name) .. ".\n" ..
            "If you wish to have no entries for some reason, use an empty table.")
    end

    for index = 1, #struct.entries do
        fh_initialize_entry(format, struct, struct.entries[index])
    end
end
function fh_initialize_entry (format, struct, entry, conf)
    conf = fh_or(conf, {})
    conf.require_name = fh_or(conf.require_name, true)
    conf.array_element = fh_or(conf.array_element, false)
    conf.auto_type = fh_or(conf.auto_type, "bytes")

    if conf.require_name and entry.name == nil then
        error("File Highlighter: {" .. tostring(format.name) .. "} structure named " .. tostring(struct.name) .. "\n" ..
            "Has entry with no name at " .. tostring(entry_index) .. ".")
    end

    if entry.type == nil then
        entry.type = conf.auto_type
    end

    if entry.type == "struct" then
        fh_initialize_entry__struct(format, struct, entry, conf)
    elseif entry.type == "array" then
        fh_initialize_entry__array(format, struct, entry, conf)
    elseif entry.type == "enum" then
        fh_initialize_entry__enum(format, struct, entry, conf)
    elseif entry.type == "string-null" then
        fh_initialize_entry__string_null(format, struct, entry, conf)
    elseif entry.type == "string" then
        fh_initialize_entry__string(format, struct, entry, conf)
    elseif entry.type == "bytes" or entry.type == "padding" then
        fh_initialize_entry__data(format, struct, entry, conf)
    else
        error("File Highlighter: {" .. tostring(format.name) .. "} structure named " .. tostring(struct.name) .. "\n" ..
            "Has entry with unknown type of '" .. tostring(entry.type) .. "' named " .. tostring(entry.name) .. ".\n" ..
            "This should've been picked up by an earlier check. Report this.")
    end
end
function fh_initialize_entry__struct (format, struct, entry, conf)

end
function fh_initialize_entry__array (format, struct, entry, conf)
    conf_element = fh_copy_table(conf)
    conf.require_name = false
    conf.array_element = true

    if entry.elements == nil and entry.size_limit == nil then
        error("File Highlighter: {" .. tostring(format.name) .. "} structure named " .. tostring(struct.name) .. "\n" ..
            "Has array entry with no elements/size_limit field named " .. tostring(entry.name) .. ".")
    end

    if entry.array == nil then
        error("File Highlighter: {" .. tostring(format.name) .. "} structure named " .. tostring(struct.name) .. "\n" ..
            "Has array entry with no array field named " .. tostring(entry.array) .. ".")
    end

    fh_initialize_entry(format, struct, entry.array, conf)
end
function fh_initialize_entry__enum (format, struct, entry, conf)
    if entry.enum == nil then
        error("File Highlighter: {" .. tostring(format.name) .. "} structure named " .. tostring(struct.name) .. "\n" ..
            "Has enum entry with no enum field named " .. tostring(entry.name) .. ".")
    end

    fh_initialize_entry__data(format, struct, entry, conf)
end
function fh_initialize_entry__string (format, struct, entry, conf)
    fh_initialize_entry__data(format, struct, entry, conf)
end
function fh_initialize_entry__string_null (format, struct, entry, conf)
    entry["size"] = 0
    fh_initialize_entry__data(format, struct, entry, conf)
end
function fh_initialize_entry__data (format, struct, entry, conf)
    if entry.size == nil then
        error("File Highlighter: {" .. tostring(format.name) .. "} structure named " .. tostring(struct.name) .. "\n" ..
            "Has data entry with no size named " .. tostring(entry.name) .. ".")
    end
end

-- Parsing data, once it's been decided this is the format we're using

function fh_parse_format (format, conf)
    conf = fh_or(conf, {})
    conf.root_structure = fh_or(conf.root_structure, "$INIT")

    root_struct = fh_find_structure(format, conf.root_structure)
    if root_struct == nil then
        error("Could not find " .. tostring(conf.root_structure) .. " struct whilst parsing format {" .. tostring(format.name) .. "}")
    end

    fh_parse_structure(format, root_struct, "Unknown", 0)
end
-- offset parameter is the offset we should start at if there isn't a custom offset.
function fh_parse_structure (format, structure, endian, offset, conf)
    structure["$size"] = 0
    structure["$contig_size"] = 0
    structure["$endian"] = fh_callif(fh_or(structure.endian, endian), structure)

    if type(structure.pre) == "function" then
        structure.pre(structure, endian, offset, conf)
    end

    for index=1, #structure.entries do
        local entry = structure.entries[index]
        fh_parse_entry(format, structure, entry, structure["$endian"], offset, conf)

        local entry_size
        if entry.type == "struct" then
            entry_size = entry["$structure"]["$size"]
        else
            entry_size = entry["$size"]
        end

        structure["$size"] = structure["$size"] + entry_size
        -- Checks if the offset is the same as the offset we gave it
        --  if we are still contiguous
        if entry["$offset"] == offset then
            structure["$contig_size"] = structure["$contig_size"] + entry_size
        end
        offset = entry["$offset"] + entry_size
    end

    if type(structure.post) == "function" then
        structure.post(structure, endian, offset, conf)
    end
end
function fh_parse_entry (format, structure, entry, endian, offset, conf)
    entry["$offset"] = fh_callif(fh_or(entry.offset, offset), structure, entry)
    entry["$parent_structure"] = structure
    entry["$endian"] = fh_callif(fh_or(entry.endian, endian), structure, entry)

    if type(entry.pre) == "function" then
        entry.pre(structure, entry, endian, offset, conf)
    end

    if entry.type == "struct" then
        fh_parse_entry__struct(format, structure, entry, entry["$endian"], offset, conf)
    elseif entry.type == "bytes" then
        fh_parse_entry__bytes(format, structure, entry, entry["$endian"], offset, conf)
    elseif entry.type == "string-null" then
        fh_parse_entry__string_null(format, structure, entry, entry["$endian"], offset, conf)
    elseif entry.type == "string" then
        fh_parse_entry__string(format, structure, entry, entry["$endian"], offset, conf)
    elseif entry.type == "padding" then
        fh_parse_entry__padding(format, structure, entry, entry["$endian"], offset, conf)
    elseif entry.type == "enum" then
        fh_parse_entry__enum(format, structure, entry, entry["$endian"], offset, conf)
    elseif entry.type == "array" then
        fh_parse_entry__array(format, structure, entry, entry["$endian"], offset, conf)
    end

    -- Custom display text.
    entry["$text"] = fh_callif(fh_or(entry.text, nil), structure, entry)

    if type(entry.post) == "function" then
        entry.post(structure, entry, endian, offset, conf)
    end
end
function fh_parse_entry__struct (format, structure, entry, endian, offset, conf)
    entry["$structure"] = fh_copy_table(
        fh_find_structure(format, fh_callif(entry.struct, structure, entry))
    )
    entry["$structure"]["$entry"] = entry -- yay, circular!

    fh_parse_structure(format, entry["$structure"], entry["$endian"], entry["$offset"], conf)
end
function fh_parse_entry__array (format, structure, entry, offset, conf)
    entry["$size"] = 0
    entry["$contig_size"] = 0
    entry["$size_limit"] = fh_callif(entry.size_limit, structure, entry)
    entry["$elements"] = fh_callif(entry.elements, structure, entry)
    entry["$data"] = {}

    offset = entry["$offset"]

    -- TODO: add checks

    -- Rather than a for loop we use this, because the condition is a bit complex..
    local index = 0
    while true do
        -- If we go over the size limit, we stop
        if entry["$size_limit"] ~= nil then
            if entry["$size"] >= entry["$size_limit"] then
                entry["$elements"] = index
                break
            end
        end

        index = index + 1

        if entry["$elements"] ~= nil then
            if index > entry["$elements"] then
                break
            end
        end

        local a_entry = fh_copy_table(entry["array"])
        entry["$data"][index] = a_entry
        a_entry["$array"] = entry
        a_entry["$array_index"] = index

        fh_parse_entry(format, structure, a_entry, entry["$endian"], offset, conf)

        local a_size
        if a_entry.type == "struct" then
            a_size = a_entry["$structure"]["$size"]
        else
            a_size = a_entry["$size"]
        end

        entry["$size"] = entry["$size"] + a_size
        offset = a_entry["$offset"] + a_size
        if entry["$offset"] == offset then
            entry["$contig_size"] = entry["$contig_size"] + a_size
        end
    end
end
function fh_parse_entry__enum (format, structure, entry, endian, offset, conf)
    fh_parse_entry___data(format, structure, entry, entry["$endian"], offset, conf)
    entry["$enum"] = fh_callif(entry.enum, structure, entry)
end
function fh_parse_entry__string (format, structure, entry, endian, offset, conf)
    fh_parse_entry___data(format, structure, entry, endian, offset, conf)

    if entry.text == nil then
        entry.text = function (struct, entry)
            local ret = "'"
            local bytes = readBytes(entry["$offset"], entry["$size"])
            for index=1, #bytes do
                local byte = bytes[index]
                if isDisplayableCharacter(byte) then
                    ret = ret .. string.char(byte0)
                else
                    ret = ret .. "."
                end
            end
            return ret .. "'"
        end
    end
end
function fh_parse_entry__string_null (format, structure, entry, endian, offset, conf)
    -- We ignore the size from this
    -- TODO: Though it may be nice to allow a "Max-Size"
    fh_parse_entry___data(format, structure, entry, endian, offset, conf)

    entry["$size"] = 0
    entry["$has_null"] = false
    -- We read from the file to determine how large the string is, though we don't store it.
    local pos = entry["$offset"]
    while true do
        if not hasByte(pos) then
            entry["$has_null"] = false
            break
        end

        local byte = readByte(pos)
        entry["$size"] = entry["$size"] + 1
        pos = pos + 1
        if byte == 0 then
            entry["$has_null"] = true
            break
        end
    end

    if entry.text == nil then
        entry.text = function (struct, entry)
            local ret = "'"
            local entry_pos = entry["$offset"]
            local bytes = readBytes(entry["$offset"], entry["$size"])
            local len = #bytes
            if len >= 1 then
                len = len - 1 -- ignore null byte
            end

            for index=1,len do
                local byte = bytes[index]
                if isDisplayableCharacter(byte) then
                    ret = ret .. string.char(byte)
                else
                    ret = ret .. "."
                end
            end
            return ret .. "'"
        end
    end
end
function fh_parse_entry__bytes (format, structure, entry, endian, offset, conf)
    fh_parse_entry___data(format, structure, entry, entry["$endian"], offset, conf)
end
function fh_parse_entry__padding (format, structure, entry, endian, offset, conf)
    fh_parse_entry___data(format, structure, entry, entry["$endian"], offset, conf)
end

function fh_parse_entry___data (format, structure, entry, endian, offset, conf)
    entry["$name"] = entry.name
    entry["$size"] = fh_callif(entry.size, structure, entry)
    entry["$highlight"] = fh_callif(entry.highlight, structure, entry)
    if entry["$highlight"] == nil or entry["$highlight"] == HighlightType.Auto then
        entry["$highlight"] = fh_next_highlight_method()
    end
end

-- Getting highlight information

-- Returns nil or entry which it is at.
function fh_get_highlight (position, conf)
    conf = fh_or(conf, {})
    conf.root_structure = fh_or(conf.root_structure, "$INIT")

    if not fh_has_chosen_format() then
        return nil
    end

    format = fh_get_chosen_format()

    root_struct = fh_find_structure(format, conf.root_structure)
    if root_struct == nil then
        error("Could not find " .. tostring(conf.root_structure) .. " struct whilst getting information for format ")
    end

    return fh_get_highlight_structure(format, root_struct, position, conf)
end
function fh_get_highlight_structure (format, structure, position, conf)
    -- We can't check the structure offset, because it's children might be at arbitrary positions.

    for index=1, #structure.entries do
        local ret = fh_get_highlight_entry(format, structure, structure.entries[index], position, conf)
        if ret ~= nil then
            return ret
        end
    end

    return nil
end
function fh_get_highlight_entry (format, structure, entry, position, conf)
    -- Note: entry is not assured to be a direct child of structure, as it might be from an array.
    -- But it is assured to be a child eventually of the structure.

    if entry.type == "bytes" or entry.type == "padding" or entry.type == "enum" or entry.type == "string-null"
        or entry.type == "string" then
        return fh_get_highlight_entry__data(format, structure, entry, position, conf)
    elseif entry.type == "array" then
        return fh_get_highlight_entry__array(format, structure, entry, position, conf)
    elseif entry.type == "struct" then
        return fh_get_highlight_entry__struct(format, structure, entry, position, conf)
    end

    return nil
end
function fh_get_highlight_entry__struct (format, structure, entry, position, conf)
    return fh_get_highlight_structure(format, entry["$structure"], position, conf)
end
function fh_get_highlight_entry__array (format, structure, entry, position, conf)
    for index=1, #entry["$data"] do
        local ret = fh_get_highlight_entry(format, structure, entry["$data"][index], position, conf)
        if ret ~= nil then
            return ret
        end
    end
    return nil
end
function fh_get_highlight_entry__data (format, structure, entry, position, conf)
    if entry["$offset"] <= position and position < entry["$offset"]+entry["$size"] then
        return entry
    end
    return nil
end

function fh_highlight_get_load_screen ()
    local row_pos = getRowOffset()
    local height = getHexViewHeight()
    local width = getHexByteWidth()
    local dim = width * height

    for z=0,dim do
        highlight_get(row_pos + z)
    end
end

function fh_get_highlight_load_column ()
    local pos = getSelectedPosition()
    local height = getHexHeight()
    -- This will go beyond the height of the screen...
    for o=0, height do
        highlight_get(pos + o, true)
    end
end
function fh_get_highlight_load_row ()
    local row_pos = getRowOffset()
    local width = getHexByteWidth()

    for o=0, width do
        highlight_get(row_pos + o, true)
    end
end



-- Base functions

function highlight_get (position, effectless)
    effectless = fh_or(effectless, false)

    local ret
    -- nil stands for 'we do not have a value'
    -- 0 stands for 'there is no value'
    if fh_data.cached_pos[position] ~= nil then
        ret = fh_data.cached_pos[position]
    else
        ret = fh_get_highlight(position)
        if ret == nil then
            fh_data.cached_pos[position] = 0
        else
            fh_data.cached_pos[position] = ret
        end
    end

    if ret == nil or ret == 0 then
        return highlighter.base_highlight_type
    end

    if getSelectedPosition() == position and not effectless and getBarMessage() == "" then
        local bar_text = tostring(ret.name)

        if ret["$text"] ~= nil then
            bar_text = bar_text .. " " .. tostring(ret["$text"])
        end

        if ret.type == "enum" then
            bar_text = bar_text .. " - (" .. tostring(fh_get_enum_entry_value(ret)) .. ")"
        end

        if ret["$array"] ~= nil then
            bar_text = "[" .. tostring(ret["$array_index"]) .. "] " .. bar_text
        end

        setBarMessage(bar_text)
    end

    return ret["$highlight"]
end

function highlight_update (read_position, size, flush_cache)
    if fh_data.chosen_format == nil then
        fh_choose_format()
    end

    if flush_cache then
        logAtExit("Flushing cache..")
        fh_flush_cache()
    end

    highlighter.init = true
end

-- Choosing format

-- We just choose the first format that decides it can handle the file.
function fh_choose_format ()
    -- TODO: make formats an array, so that a format can deliberately put itself at the end.
    for k,v in pairs(fh_data.formats) do
        if v.verifier() then
            fh_data.chosen_format = k
            logAtExit("Chose format {" .. tostring(k) .. "}")
            local format = fh_get_chosen_format()
            if type(format.on_chosen) == "function" then
                format.on_chosen()
            end
            fh_parse_format(format)
            return
        end
    end
    logAtExit("Could not find format for this file.")
    fh_data.chosen_format = 0
end









-- Tree searching utility

function fh_find_structure (format, struct_name)
    return format.structures[fh_find_structure_index(format, struct_name)]
end
function fh_find_structure_index (format, struct_name)
    for index=1, #format.structures do
        if format.structures[index].name == struct_name then
            return index
        end
    end
    return nil
end
function fh_has_structure (format, struct_name)
    return fh_find_structure_index(format, struct_name) ~= nil
end
-- Assumes entry is of type struct
function fh_get_structure_parent (structure)
    return fh_get_entry_parent(structure["$entry"])
end
-- Assumes entry is of type struct
function fh_get_entry_parent (entry)
    return entry["$parent_structure"]
end
-- Assumes entry is of type struct
function fh_get_entry__struct (entry)
    return entry["$structure"]
end

-- Only searches top layer
function fh_find_entry_index (structure, name)
    for index = 1, #structure.entries do
        if structure.entries[index].name == name then
            return index
        end
    end
    return nil
end
function fh_find_entry (structure, name)
    return structure.entries[fh_find_entry_index(structure, name)]
end

function fh_get_bytes_entry_bytes (bytes_entry)
    local bytes = readBytes(bytes_entry["$offset"], bytes_entry["$size"])

    if #bytes < bytes_entry["$size"] then
        logAtExit("File Format Highlighter: Entry {" .. tostring(bytes_entry.name) .. "} read bytes, but stopped (likely due to EOF)." ..
            " This may be a bug in the FileFormat handling code.")
    end

    return bytes
end
function fh_get_bytes_entry_value (bytes_entry)
    if bytes_entry["$endian"] == "Unknown" or bytes_entry["$endian"] == "Big" then
        return fh_get_bytes_entry_value_be(bytes_entry)
    else
        return fh_get_bytes_entry_value_le(bytes_entry)
    end
end
function fh_get_bytes_entry_value_le (bytes_entry)
    return fh_bytes_into_integer_le(fh_get_bytes_entry_bytes(bytes_entry))
end
function fh_get_bytes_entry_value_be (bytes_entry)
    return fh_bytes_into_integer_be(fh_get_bytes_entry_bytes(bytes_entry))
end
function fh_get_enum_entry_value (enum_entry)
    if enum_entry["$endian"] == "Unknown" or enum_entry["$endian"] == "Big" then
        return fh_get_enum_entry_value_be(enum_entry)
    else
        return fh_get_enum_entry_value_le(enum_entry)
    end
end
function fh_get_enum_entry_value_be (enum_entry)
    return enum_entry["$enum"][fh_get_bytes_entry_value_be(enum_entry)]
end
function fh_get_enum_entry_value_le (enum_entry)
    return enum_entry["$enum"][fh_get_bytes_entry_value_le(enum_entry)]
end

-- General Utility

function fh_copy_table (tab)
    if tab == nil then
        inform_of_error("Table was nil")
    end

    local ret = {}
    for k,v in pairs(tab) do
        if type(v) == "table" then
            ret[k] = fh_copy_table(v)
        else
            ret[k] = v
        end
    end
    return ret
end

function fh_callif (val, ...)
    if type(val) == "function" then
        return val(...)
    else
        return val
    end
end
function fh_cond (check, v1, v2)
    if check then
        return v1
    end
    return v2
end
function fh_or (v1, v2)
    return fh_cond(v1 == nil, v2, v1)
end
-- If v1 is nil, it returns v2 and logs logMessage
-- Otherwise returns v1
function fh_log_or (v1, v2, logMessage)
    if logMessage == nil then
        inform_of_error("logMessage was nil")
    end

    if v1 == nil then
        logAtExit(logMessage)
        return v2
    end
    return v1
end
-- If v1 is nil, then it logs the message and returns nil
-- Otherwise, returns v1
function fh_nil_log (v1, logMessage)
    return fh_log_or(v1, nil, logMessage)
end


function fh_bytes_into_integer_le (bytes)
    if bytes == nil then
        inform_of_error("Bytes was nil")
    end

    local ret = 0
    for index = #bytes, 1, -1 do
        ret = ret | (bytes[index] * (256^(index-1)))
    end
    return ret
end
function fh_bytes_into_integer_be (bytes)
    if bytes == nil then
        inform_of_error("Bytes was nil")
    end

    local ret = 0
    for index=1,#bytes do
        ret = ret | (bytes[index] * (256^(index-1)))
    end
    return ret
end