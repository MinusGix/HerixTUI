-- For use in Header where 32-bit = 4 bytes and 64-bit = 8 bytes
local pointer_function = function (struct, entry)
    if fh_get_enum_entry_value_be(fh_find_entry(struct, "Class")) == "32-bit" then
        return 4
    else
        return 8
    end
end
-- For use in siblings to Header, grabbing the $INIT and getting the header and getting it's bit size
local sibling_pointer_function = function (struct, entry)
    local parent = fh_get_structure_parent(struct)
    local header = fh_get_entry__struct(fh_find_entry(parent, "header"))
    if fh_get_enum_entry_value_be(fh_find_entry(header, "Class")) == "32-bit" then
        return 4
    else
        return 8
    end
end

-- This is for entries in the header, since the endian is chosen in there, it can't be inherited easily
local header_endian = function (structure, entry)
    local endian_value = fh_get_enum_entry_value(fh_find_entry(structure, "Endian"))
    if endian_value == "Little-Endian" then
        return "Little"
    else
        return "Big"
    end
end

local sibling_endian = function (structure, entry)
    local header_struct = fh_get_entry__struct(fh_find_entry(structure, "header"))
    local endian_value = fh_get_enum_entry_value(
        fh_find_entry(header_struct, "Endian")
    )

    if endian_value == "Little-Endian" then
        return "Little"
    else
        return "Big"
    end
end

-- Yeah, this isn't perfect looking when displayed, but eh, it works good enough for now
-- TODO: make this display nicer
local FlagBuilder = function (num, flags)
    local ret = ""
    for index=1, #flags do
        if num & flags[index][1] then
            ret = ret .. flags[index][2]
        end
    end
    return ret
end

local InverseFlagBuilder = function (num, flags)
    local ret = ""
    for index=1, #flags do
        if (num & flags[index][1]) == 0 then
            ret = ret .. flags[index][2]
        end
    end
    return ret
end

local BoolFlagBuilder = function (num, flags)
    local ret = ""
    for index=1, #flags do
        if (num & flags[index][1]) then
            ret = ret .. flags[index][2]
        else
            ret = ret .. flags[index][3]
        end
    end
    return ret
end


local ProgramHeaderSegmentTypes = {
    [0] = "PT_NULL", -- entry unused
    [1] = "PT_LOAD", -- Loadable segment
    [2] = "PT_DYNAMIC", -- Dynamic linking info
    [3] = "PT_INTERP", -- Interpreter info
    [4] = "PT_NOTE", -- Auxiliary info
    [5] = "PT_SHLIB", -- reserved
    [6] = "PT_PHDR", -- segment containing program header table itself
    [7] = "PT_TLS", -- Thread-local storage
    [8] = "PT_NUM", -- Number of defined type
    [0x60000000] = "PT_LOOS", -- Start of OS-Specific
    [0x6474e550] = "PT_GNU_EH_FRAME", -- GCC .eh_frame_hdr
    [0x6474e551] = "PT_GNU_RELRO", -- Read-only after relocation
    [0x6ffffffa] = "PT_LOSUNW/PT_SUNWBSS", -- Sun specific
    [0x6ffffffb] = "PT_SUNWSTACK", -- Stack segment
    [0x6FFFFFFF] = "PT_HIOS/PT_HISUNW", -- End of os-specific
    [0x70000000] = "PT_LOPROC", -- Start of processor specific
    [0x7FFFFFFF] = "PT_HIPROC" -- End of processor specific
}

local ProgramHeaderFlagText = function (structure, entry)
    local flag = fh_get_bytes_entry_value(entry)
    return BoolFlagBuilder(flag, {
        {1, "Exec", "No-Exec"},
        {1 << 1, ",Write", ",No-Write"},
        {1 << 2, ",No-Read", ",Read"}
    })
end

fh_register_format({
    name = "base_ELF",
    verifier = function ()
        -- FIXME: this is rather simplistic, simply checking for the elf beginning header
        if hasByte(0) and hasByte(1) and hasByte(2) and hasByte(3) then
            local m0 = readByte(0)
            local m1 = readByte(1)
            local m2 = readByte(2)
            local m3 = readByte(3)
            return m0 == 0x7F and m1 == 0x45 and m2 == 0x4c and m3 == 0x46
        end
        return false
    end,
    structures = {
        -- Structures and parameters starting with $ are reserved and may have special behavior.
        -- $INIT is the entry point structure.
        {
            name = "$INIT",
            entries = {
                {
                    name = "header",
                    type = "struct",
                    struct = "Header",
                },
                {
                    name = "programheadertable",
                    type = "array",
                    elements = function (structure, entry)
                        local header_struct = fh_get_entry__struct(fh_find_entry(structure, "header"))
                        return fh_get_bytes_entry_value(
                            fh_find_entry(header_struct, "ProgramHeaderTableEntries")
                        )
                    end,
                    endian = sibling_endian,
                    array = { -- an entry of what it is made up of.
                        type = "struct",
                        struct = function (structure, entry) -- struct would be $INIT, entry would be itself
                            local header_struct = fh_get_entry__struct(fh_find_entry(structure, "header"))
                            local class_value = fh_get_enum_entry_value(
                                fh_find_entry(header_struct, "Class")
                            )

                            if class_value == "32-bit" then
                                return "ProgramHeader32"
                            else
                                return "ProgramHeader64"
                            end
                        end
                    },
                    offset = function (structure, entry)
                        local header_struct = fh_get_entry__struct(fh_find_entry(structure, "header"))
                        local program_offset = fh_get_bytes_entry_value(
                            fh_find_entry(header_struct, "ProgramHeaderTableOffset")
                        )
                        return program_offset
                    end
                },
                {
                    name = "sectionheadertable",
                    type = "array",
                    elements = function (structure, entry)
                        local header_struct = fh_get_entry__struct(fh_find_entry(structure, "header"))
                        return fh_get_bytes_entry_value(
                            fh_find_entry(header_struct, "SectionHeaderTableEntries")
                        )
                    end,
                    endian = sibling_endian,
                    array = {
                        type = "struct",
                        struct = "SectionHeader"
                    },
                    offset = function (structure, entry)
                        local header_struct = fh_get_entry__struct(fh_find_entry(structure, "header"))
                        local program_offset = fh_get_bytes_entry_value(
                            fh_find_entry(header_struct, "SectionHeaderTableOffset")
                        )
                        return program_offset
                    end
                },

                -- These are structures which may or may not exist
                -- Arrays in the loosest sense, they are positioned wherever in the file
                -- It'd be nice to make these kindof "Dynamic" entries be a part of the base
                --  File highlighter but I don't think that's going to happen yet.
                {
                    name = "notes",
                    type = "array",
                    endian = sibling_endian,
                    elements = function (structure, entry)
                        local phdr = fh_find_entry(structure, "programheadertable")

                        entry["$user_notes"] = {}
                        for index=1, #phdr["$data"] do
                            local p_entry = phdr["$data"][index]
                            local p_struct = fh_get_entry__struct(p_entry)
                            local p_type = fh_get_enum_entry_value(fh_find_entry(p_struct, "SegmentType"))
                            if p_type == "PT_NOTE" then
                                table.insert(entry["$user_notes"], p_struct)
                            end
                        end
                        return #entry["$user_notes"]
                    end,
                    array = {
                        type = "struct",
                        struct = "Notes",
                        offset = function (structure, entry)
                            local index = entry["$array_index"]
                            local ind_struct = entry["$array"]["$user_notes"][index]

                            return fh_get_bytes_entry_value(fh_find_entry(ind_struct, "SegmentOffset"))
                        end
                    }
                },

                {
                    name = "symboltable",
                    type = "array",
                    endian = sibling_endian,
                    elements = function (structure, entry)
                        local sections = fh_find_entry(structure, "sectionheadertable")

                        entry["$user_symbols"] = {}
                        for index=1, #sections["$data"] do
                            local s_entry = sections["$data"][index]
                            local s_struct = fh_get_entry__struct(s_entry)
                            local s_type = fh_get_enum_entry_value(fh_find_entry(s_struct, "Type"))
                            if s_type == "SHT_SYMTAB" then
                                table.insert(entry["$user_symbols"], s_struct)
                            end
                        end
                        return #entry["$user_symbols"]
                    end,
                    array = {
                        type = "array",
                        elements = function (structure, entry)
                            local index = entry["$array_index"]
                            local ind_struct = entry["$array"]["$user_symbols"][index]
                            -- Offset is 0, so no elements.
                            if fh_get_bytes_entry_value(fh_find_entry(ind_struct, "FileOffset")) == 0 then
                                return 0
                            end

                            return fh_get_bytes_entry_value(fh_find_entry(ind_struct, "Size")) //
                                fh_get_bytes_entry_value(fh_find_entry(ind_struct, "EntrySize"))
                        end,
                        array = {
                            type = "struct",
                            struct = function (structure, entry)
                                local bitsize = fh_get_enum_entry_value(
                                    fh_find_entry(
                                        fh_get_entry__struct(fh_find_entry(structure, "header")),
                                        "Class"
                                ))

                                if bitsize == "32-bit" then
                                    return "SymbolEntry32"
                                else
                                    return "SymbolEntry64"
                                end
                            end,
                        },
                        offset = function (structure, entry)
                            local index = entry["$array_index"]
                            local ind_struct = entry["$array"]["$user_symbols"][index]

                            return fh_get_bytes_entry_value(fh_find_entry(ind_struct, "FileOffset"))
                        end
                    }
                },

                {
                    name = "stringtables",
                    type = "array",
                    elements = function (structure, entry)
                        local sections = fh_find_entry(structure, "sectionheadertable")

                        entry["$user_strings"] = {}
                        for index=1, #sections["$data"] do
                            local s_entry = sections["$data"][index]
                            local s_struct = fh_get_entry__struct(s_entry)
                            local s_type = fh_get_enum_entry_value(fh_find_entry(s_struct, "Type"))
                            if s_type == "SHT_STRTAB" then
                                table.insert(entry["$user_strings"], s_struct)
                            end
                        end
                        return #entry["$user_strings"]
                    end,
                    array = {
                        type = "struct",
                        struct = "StringTable",
                        offset = function (structure, entry)
                            local index = entry["$array_index"]
                            local ind_struct = entry["$array"]["$user_strings"][index]

                            return fh_get_bytes_entry_value(fh_find_entry(ind_struct, "FileOffset"))
                        end
                    },
                    endian = sibling_endian
                }
            },
        },

        {
            name = "Header",
            entries = {
                -- Start e_ident
                {
                    name = "Magic",
                    -- offset is assumed to be 0 since it's at the start
                    -- default type is 'bytes'
                    size = 4,
                    highlight = HighlightType.Color_GREEN_MAGENTA,
                },
                -- ei_class
                {
                    name = "Class",
                    -- offset is assumed to be Magic.size + 0
                    type = "enum",
                    size = 1,
                    enum = {
                        [0] = "Invalid",
                        [1] = "32-bit",
                        [2] = "64-bit"
                    },
                    highlight = HighlightType.Color_CYAN_BLACK
                },
                -- ei_data
                {
                    name = "Endian",
                    -- Note: either 1 or 2 to signify little or big endianness
                    --  Affects multi-byte fields starting with offset 0x10
                    type = "enum",
                    size = 1,
                    enum = {
                        [0] = "Invalid",
                        [1] = "Little-Endian",
                        [2] = "Big-Endian"
                    },
                    highlight = HighlightType.Color_RED_BLACK
                },
                -- ei_version
                {
                    name = "ELFHeaderVersion",
                    -- Set to 1 for the original and current version of ELF
                    size = 1,
                    highlight = HighlightType.Color_GREEN_BLACK
                },
                -- ei_elfosabi
                {
                    name = "ABI",
                    size = 1,
                    type = 'enum',
                    highlight = HighlightType.Color_YELLOW_RED,
                    enum = {
                        -- Note: is often set to 0 regardless of target paltform
                        [0] = "System V", -- And also NONE
                        [1] = "HP-UX",
                        [2] = "NetBSD",
                        [3] = "Linux", -- defined to ELFOSABI_GNU
                        [4] = "GNU Hurd", -- ? is this corect?
                        -- 5
                        [6] = "Solaris", 
                        [7] = "AIX",
                        [8] = "IRIX",
                        [9] = "FreeBSD",
                        [10] = "Tru64",
                        [11] = "Novell Modesto",
                        [12] = "OpenBSD",
                        [13] = "OpenVMS", -- I didn't see this in elf.h
                        [14] = "NonStop Kernel", -- I didn't see this in elf.h
                        [15] = "AROS", -- I didn't see this in elf.h
                        [16] = "Fenix OS", -- I didn't see this in elf.h
                        [17] = "CloudABI", -- I didn't see this in elf.h
                        [64] = "ARM EABI",
                        [97] = "ARM",
                        [255] = "Standalone" -- (embedded)
                    }
                },
                -- ei_abiversion
                {
                    name = "ABIVersion",
                    size = 1,
                    highlight = HighlightType.Color_CYAN_RED
                    -- ABIVersion may be inside of PADDINGA (thus it would be 8 bytes)
                    -- on some versions of the linux kernel, but we keep it like this
                },
                -- ei_pad
                {
                    name = "PADDINGA",
                    type = 'padding',
                    size = 7,
                    highlight = HighlightType.Dim
                },
                -- end of e_ident

                -- e_type
                {
                    name = "ObjectFileType",
                    type = 'enum',
                    size = 2,
                    highlight = HighlightType.Color_CYAN_BLACK,
                    endian = header_endian,
                    enum = {
                        [0] = "ET_NONE", -- no file type
                        [1] = "ET_REL", -- relocatable file
                        [2] = "ET_EXEC", -- executable file
                        [3] = "ET_DYN", -- shared object file
                        [4] = "ET_CORE", -- core file
                        [5] = "ET_NUM", -- number of defined types (? what is this)
                        [0xffe0] = "ET_LOOS", -- OS-specific range start
                        [0xff00] = "ET_HIOS", -- OS-specific range end
                        [0xff00] = "ET_LOPROC", -- Processor-specific range start
                        [0xffff] = "ET_HIPROC" -- Processor-specific range end
                    }
                },
                -- e_machine
                {
                    name = "ISA",
                    type = 'enum',
                    size = 2,
                    highlight = HighlightType.Color_MAGENTA_BLACK,
                    endian = header_endian,
                    enum = {
                        -- There is a _lot_ of EM_ values defined in elf.h...
                        [0] = "None", -- No machine
                        [1] = "AT&T WE 32100",
                        [2] = "Sparc",
                        [3] = "Intel 80386 (x86)", -- ? I think this is x86 equivalent.
                        [4] = "Motorola m68k",
                        [5] = "Motorola m88k",
                        [6] = "Intel MCU",
                        [7] = "Intel 80860",
                        [8] = "MIPS", -- elf.h says MIPS R3000 big-endian
                        [0x14] = "PowerPC",
                        [0x16] = "S390",
                        [0x28] = "ARM",
                        [0x2A] = "SuperH",
                        [0x32] = "IA-64",
                        [0x3E] = "x86-64",
                        [0xB7] = "AArch64",
                        [0xF3] = "RISC-V"
                    }
                },
                -- e_version
                {
                    name = "ELFVersion",
                    size = 4,
                    highlight = HighlightType.Color_BLACK_WHITE,
                    endian = header_endian,
                },
                -- e_entry
                {
                    name = "EntryPoint",
                    highlight = HighlightType.Underlined,
                    size = pointer_function,
                    endian = header_endian,
                },
                {
                    name = "ProgramHeaderTableOffset",
                    size = pointer_function,
                    endian = header_endian,
                },
                {
                    name = "SectionHeaderTableOffset",
                    size = pointer_function,
                    endian = header_endian,
                },
                {
                    name = "Flags",
                    size = 4,
                    endian = header_endian,
                },
                {
                    name = "SelfHeaderSize",
                    -- normally 64 bytes for 64 bit and 52 for 32-bit
                    size = 2,
                    endian = header_endian,
                },
                {
                    name = "ProgramHeaderTableEntrySize",
                    size = 2,
                    endian = header_endian,
                },
                {
                    name = "ProgramHeaderTableEntries",
                    size = 2,
                    endian = header_endian,
                },
                {
                    name = "SectionHeaderTableEntrySize",
                    size = 2,
                    endian = header_endian,
                },
                {
                    name = "SectionHeaderTableEntries",
                    size = 2,
                    endian = header_endian,
                },
                {
                    name = "SectionTableNamesEntryIndex",
                    size = 2,
                    endian = header_endian,
                }
            }
        },

        {
            name = "ProgramHeader32",
            entries = {
                {
                    name = "SegmentType",
                    size = 4,
                    type = "enum",
                    enum = fh_copy_table(ProgramHeaderSegmentTypes),
                    highlight = HighlightType.Color_GREEN_MAGENTA,
                },
                {
                    name = "SegmentOffset",
                    size = 4,
                },
                {
                    name = "SegmentVirtualAddress",
                    size = 4
                },
                {
                    name = "SegmentPhysicalAddress",
                    size = 4
                },
                {
                    name = "SegmentFileSize",
                    size = 4
                },
                {
                    name = "SegmentMemorySize",
                    size = 4
                },
                {
                    name = "SegmentFlags",
                    size = 4,
                    text = ProgramHeaderFlagText,
                },
                {
                    name = "SegmentAlignment",
                    size = 4
                }
            }
        },

        {
            name = "ProgramHeader64",
            entries = {
                {
                    name = "SegmentType",
                    size = 4,
                    type = "enum",
                    enum = fh_copy_table(ProgramHeaderSegmentTypes)
                },
                {
                    name = "SegmentFlags",
                    highlight = HighlightType.Underlined,
                    size = 4,
                    text = ProgramHeaderFlagText,
                },
                {
                    name = "SegmentOffset",
                    size = 8
                },
                {
                    name = "SegmentVirtualAddress",
                    size = 8
                },
                {
                    name = "SegmentPhysicalAddress",
                    size = 8
                },
                {
                    name = "SegmentFileSize",
                    size = 8
                },
                {
                    name = "SegmentMemorySize",
                    size = 8
                },
                {
                    name = "SegmentAlignment",
                    size = 8
                }
            }
        },

        {
            name = "SectionHeader",
            entries = {
                {
                    name = "NameIndex", -- string table index
                    size = 4,
                    highlight = HighlightType.Color_BLACK_RED
                },
                {
                    name = "Type",
                    size = 4,
                    type = "enum",
                    enum = {
                        [0] = "SHT_NULL", -- entry unused
                        [1] = "SHT_PROGBITS", -- program data
                        [2] = "SHT_SYMTAB", -- symbol table
                        [3] = "SHT_STRTAB", -- string table
                        [4] = "SHT_RELA", -- relocation entries with addends
                        [5] = "SHT_HASH", -- symbol hash table
                        [6] = "SHT_DYNAMIC", -- dynamic linking info
                        [7] = "SHT_NOTE", -- Notes
                        [8] = "SHT_NOBITS", -- program space with no data (bss)
                        [9] = "SHT_REL", -- relocation entries, no addends
                        [10] = "SHT_SHLIB", -- reserved (?)
                        [11] = "SHT_DYNSYM", -- Dynamic linker symbol table
                        [14] = "SHT_INIT_ARRAY", -- Array of constructors
                        [15] = "SHT_FINI_ARRAY", -- Array of destructors
                        [16] = "SHT_PREINIT_ARRAY", -- Array of pre-construtors
                        [17] = "SHT_GROUP", -- Section group
                        [18] = "SHT_SYMTAB_SHNDX", -- Extended section indices
                        [19] = "SHT_NUM", -- Number of defined types
                    }
                },
                {
                    name = "Flags",
                    size = sibling_pointer_function,
                    text = function (struct, entry)
                        local flag = fh_get_bytes_entry_value(entry)
                        return FlagBuilder(flag, {
                            {1, "Writable"},
                            {1 << 1, ",Alloc"},
                            {1 << 2, ",Exec"},
                            {1 << 4, ",Merge"},
                            {1 << 5, ",Strings"},
                            {1 << 6, ",InfoLink"},
                            {1 << 7, ",LinkOrder"},
                            {1 << 8, ",NonStd"},
                            {1 << 9, ",Group"},
                            {1 << 11, ",Compress"}
                        })
                    end
                },
                {
                    name = "VirtualAddress",
                    size = sibling_pointer_function
                },
                {
                    name = "FileOffset",
                    size = sibling_pointer_function,
                },
                {
                    name = "Size", -- in bytes
                    size = sibling_pointer_function
                },
                {
                    name = "Link", -- to another section
                    size = 4
                },
                {
                    name = "Info", -- Additional information
                    size = 4
                },
                {
                    name = "Alignment",
                    size = sibling_pointer_function
                },
                {
                    name = "EntrySize", -- Entry size if section holds table
                    size = sibling_pointer_function
                }
            }
        },

        -- TODO there is almost assuredly some flags for 32/64 symbolentry that aren't here.
        {
            name = "SymbolEntry32",
            entries = {
                {
                    name = "Name", -- string table index
                    size = 4,
                    highlight = HighlightType.Color_YELLOW_GREEN
                },
                {
                    name = "Value",
                    size = 4
                },
                {
                    name = "Size",
                    size = 4
                },
                {
                    name = "Info", -- type and binding
                    size = 1
                },
                {
                    name = "Other", -- Visibility
                    size = 1
                },
                {
                    name = "SectionIndex",
                    size = 2
                }
            },
        },
        {
            name = "SymbolEntry64",
            entries = {
                {
                    name = "Name", -- String table index
                    size = 4,
                    highlight = HighlightType.Color_YELLOW_GREEN
                },
                {
                    name = "Info", -- type and binding
                    size = 1
                },
                {
                    name = "Other", -- visiblity
                    size = 1
                },
                {
                    name = "SectionIndex",
                    size = 2
                },
                {
                    name = "Value",
                    size = 8
                },
                {
                    name = "Size",
                    size = 8
                }
            }
        },

        {
            name = "Relocation",
            entries = {
                {
                    name = "Offset", -- address
                    size = sibling_pointer_function
                },
                {
                    name = "Info", -- reloc type and symbol index
                    size = sibling_pointer_function
                }
            }
        },

        {
            name = "RelocationAddend",
            entries = {
                {
                    name = "Offset", -- address
                    size = sibling_pointer_function
                },
                {
                    name = "Info", -- reloc type and symbol index
                    size = sibling_pointer_function
                },
                {
                    name = "Addend",
                    size = sibling_pointer_function
                }
            }
        },

        {
            name = "Dynamic",
            entries = {
                {
                    name = "Tag",
                    size = sibling_pointer_function
                },
                {
                    -- Union, tag decides what it is
                    -- Value: represents ints with various interpretations
                    -- Pointer: represents program virtual addresses (??)
                    name = "Value/Pointer",
                    size = sibling_pointer_function
                }
            }
        },

        {
            name = "Notes",
            entries = {
                {
                    name = "NameSize",
                    size = 4,
                    highlight = HighlightType.Color_BLACK_GREEN
                },
                {
                    name = "DescSize",
                    size = 4,
                },
                {
                    name = "Type",
                    size = 4,
                    type = "enum",
                    -- TODO: these aren't entirely accurate in all cases...
                    enum = {
                        [1] = "?GNU_ABI_TAG",
                        [2] = "?GNU_HWCAP",
                        [3] = "?GNU_BUILD_ID",
                        [4] = "?GNU_GOLD_VERSION",
                        [5] = "?GNU_PROPERTY_TYPE_0" -- ?I'm insure if this is a valid value based on what it says
                    }
                },
                {
                    name = "Name",
                    size = function (structure, entry)
                        local size = fh_find_entry(structure, "NameSize")
                        return fh_get_bytes_entry_value(size)
                    end,
                    text = function (structure, entry)
                        local bytes = fh_get_bytes_entry_bytes(fh_find_entry(structure, "Name"))
                        local ret = ""
                        for index=1, #bytes do
                            if isDisplayableCharacter(bytes[index]) then
                                ret = ret .. string.char(bytes[index])
                            elseif bytes[index] == 0 then
                                ret = ret .. "\\0"
                            else
                                ret = ret .. "."
                            end
                        end
                        return "'" .. ret .. "'"
                    end
                },
                {
                    name = "Desc",
                    size = function (structure, entry)
                        local size = fh_find_entry(structure, "DescSize")
                        return fh_get_bytes_entry_value(size)
                    end,
                    text = function (structure, entry)
                        local bytes = fh_get_bytes_entry_bytes(fh_find_entry(structure, "Desc"))
                        local ret = ""
                        for index=1, #bytes do
                            if isDisplayableCharacter(bytes[index]) then
                                ret = ret .. string.char(bytes[index])
                            elseif bytes[index] == 0 then
                                ret = ret .. "\\0"
                            else
                                ret = ret .. "."
                            end
                        end
                        return "'" .. ret .. "'"
                    end
                }
            }
        },

        {
            name = "StringTable",
            entries = {
                {
                    name = "BeginningNull",
                    size = 1,
                },
                {
                    name = "Strings",
                    type = "array",
                    array = {
                        type = "struct",
                        struct = "StringTableEntry"
                    },
                    size_limit = function (structure, entry)
                        local index = structure["$entry"]["$array_index"]
                        local ind_struct = structure["$entry"]["$array"]["$user_strings"][index]

                        if fh_get_bytes_entry_value(fh_find_entry(ind_struct, "FileOffset")) == 0 then
                            return 0
                        end

                        return fh_get_bytes_entry_value(fh_find_entry(ind_struct, "Size")) - 1
                    end,
                },
                {
                    name = "EndingNull",
                    size = 1
                }
            }
        },
        {
            name = "StringTableEntry",
            entries = {
                {
                    name = "StringEntry",
                    type = "string-null"
                }
            }
        }
    }
})