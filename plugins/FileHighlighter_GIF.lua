fh_register_format({
    name = "base_GIF",
    verifier = function ()
        local b = readBytes(0, 6)

        if #b < 6 then
            return false
        end

        return b[1] == string.byte('G') and b[2] == string.byte('I') and b[3] == string.byte('F') and
            b[4] == string.byte('8') and (b[5] == string.byte('7') or b[5] == string.byte('9')) and
            b[6] == string.byte('a')
    end,
    structures = {
        {
            name = "$INIT",
            entries = {
                {
                    name = "header",
                    type = "struct",
                    struct = "Header"
                },
                {
                    name = "globalcolormap",
                    type = "array",
                    elements = function (structure, entry)
                        local header = fh_get_entry__struct(fh_find_entry(structure, "header"))
                        local fields = fh_find_entry(header, "Fields")

                        if not fields["$user_color_map"] then
                            return 0 -- there is no globalcolormap
                        end

                        return 2^(fields["$user_pixel"]+1)
                    end,
                    array = {
                        type = "struct",
                        struct = "GlobalColorMapEntry"
                    }
                }
            }
        },

        {
            name = "Header",
            entries = {
                {
                    name = "Magic",
                    size = 6
                },
                {
                    name = "ScreenWidth",
                    size = 2,
                    type = "int",
                    endian = "Little"
                },
                {
                    name = "ScreenHeight",
                    size = 2,
                    type = "int",
                    endian = "Little"
                },
                {
                    name = "Fields",
                    size = 1,
                    text = function (structure, entry)
                        local ret = ""
                        local value = fh_get_bytes_entry_value(entry)

                        if fh_byte_extract_bit(value, 7) then
                            ret = ret .. "CM:Y"
                            entry["$user_color_map"] = true
                        else
                            ret = ret .. "CM:N"
                            entry["$user_color_map"] = false
                        end

                        local cr = fh_bytes_into_integer_le(
                            {(value >> 4) & 7}
                        ) + 1
                        ret = ret .. ",CR:" .. tostring(cr)
                        entry["$user_color_resolution"] = cr

                        if fh_byte_extract_bit(value, 4) then
                            ret = ret .. ",Sort:Y"
                            entry["$user_sort"] = true
                        else
                            ret = ret .. ",Sort:N"
                            entry["$user_sort"] = false
                        end

                        local pixel = fh_bytes_into_integer_le(
                            {(value >> 4) & 7}
                        ) + 1
                        ret = ret .. ",PixelBits:" .. tostring(pixel)
                        entry["$user_pixel"] = pixel

                        return ret
                    end
                },
                {
                    name = "BackgroundIndex",
                    size = 1
                },
                {
                    name = "AspectRatio",
                    size = 1
                }
            }
        },

        -- Right after header
        {
            name = "GlobalColorMapEntry",
            entries = {
                {
                    name = "Red",
                    size = 1,
                    highlight = HighlightType.Color_BLACK_RED
                },
                {
                    name = "Green",
                    size = 1,
                    highlight = HighlightType.Color_BLACK_GREEN
                },
                {
                    name = "Blue",
                    size = 1,
                    highlight = HighlightType.Color_BLACK_BLUE
                }
            }
        },

        {
            name = "Extension",
            entries = { -- 0x21
                name = "Introducer",
                size = 1
            }
        },

        {
            name = "DataSubBlock",
            entries = {
                {
                    name = "Size",
                    size = 1
                },
                {
                    name = "Data",
                    type = "array",
                    elements = function (structure, entry)
                        return fh_get_bytes_entry_value(fh_find_entry(structure, "Size"))
                    end,
                    array = {
                        type = "bytes",
                        size = 1
                    }
                }
            }
        }
    }
})