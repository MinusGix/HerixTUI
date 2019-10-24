fh_register_format({
    name = "base_PNG",
    verifier = function ()
        local b = readBytes(0, 4)

        if #b < 4 then
            return false
        end

        return b[1] == 0x89 and b[2] == 0x50 and b[3] == 0x4E and b[4] == 0x47
    end,
    structures = {
        {
            name = "$INIT",
            entries = {
                {
                    name = "header",
                    type = "struct",
                    struct = "Header"
                }
            }
        },

        {
            name = "Header",
            entries = {
                { -- 0x69
                    name = "TransmissionCheck",
                    size = 1
                },
                { -- 0x5E4E47 (PNG)
                    name = "Magic",
                    size = 3
                },
                { -- 0x0D0A
                    name = "DOSLineEnding",
                    size = 2
                },
                { -- 0x1A
                    name = "DOSEOF",
                    size = 1
                },
                { -- 0x0A
                    name = "UNIXLineEnding",
                    size = 1
                },
                {
                    name = "Chunks",
                    type = "array",
                    array = {
                        type = "struct",
                        struct = "Chunk"
                    },
                    size_limit = function (structure, entry)
                        return getFileEnd() - 1 - 3 - 2 - 1 - 1
                    end
                }
            }
        },

        {
            name = "Chunk",
            entries = {
                {
                    name = "Length",
                    size = 4,
                    endian = "Big"
                },
                {
                    name = "Type",
                    type = "string",
                    size = 4,
                    endian = "Big"
                },
                {
                    name = "Data",
                    type = "struct",
                    struct = function (structure, entry)
                        local type = fh_find_entry(structure, "Type")["$text"]
                        local length = fh_get_bytes_entry_value(fh_find_entry(structure, "Length"))
                        if type == "'IHDR'" and length == 13 then -- If it's not 13, then it's an unknown weird IHDR
                            return "ChunkIHDR"
                        else
                            return "ChunkGeneric"
                        end
                    end
                },
                { -- CRC32 over chunk type and chunk data, not length
                    name = "CRC",
                    size = 4,
                    endian = "Big"
                }
            }
        },

        {
            name = "ChunkGeneric",
            entries = {
                {
                    name = "Data",
                    size = function (structure, entry)
                        local len = fh_get_bytes_entry_value(fh_find_entry(fh_get_structure_parent(structure), "Length"))
                        return len
                    end,
                }
            }
        },

        {
            name = "ChunkIHDR",
            entries = {
                {
                    name = "Width",
                    size = 4
                },
                {
                    name = "Height",
                    size = 4
                },
                {
                    name = "Bit-Depth",
                    size = 1
                },
                {
                    name = "ColorType",
                    size = 1
                },
                {
                    name = "CompressionMethod",
                    size = 1
                },
                {
                    name = "FilterMethod",
                    size = 1
                },
                {
                    name = "InterlaceMethod",
                    size = 1
                }
            }
        }
    }
})