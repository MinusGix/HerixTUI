-- For use in anywhere the in the tree.
local header_enum_option = function (enum_name, values)
    local tail_func
    tail_func = function (struct)
        if struct == nil then
            error("Header pointer function failed.")
        elseif struct.name == "$INIT" then
            return tail_func(fh_get_entry__struct(fh_find_entry(struct, "header")))
        elseif struct.name == "Header" then
            local value = fh_get_enum_entry_value(fh_find_entry(struct, enum_name))

            for index=1, #values do
                if values[index][1] == value then
                    return values[index][2]
                end
            end

            logAtExit("header_enum_option failed.")
            return nil
        else
            return tail_func(fh_get_structure_parent(struct))
        end
    end

    return tail_func
end

local header_class_option
header_class_option = function (v1, v2)
    return header_enum_option("Class", {
        {"32-bit", v1},
        {"64-bit", v2}
    })
end
local header_class_pointer
header_class_pointer = header_class_option(4, 8)

local header_endian_option
header_endian_option = function (v1, v2)
    return header_enum_option("Endian", {
        {"Little-Endian", v1},
        {"Big-Endian", v2}
    })
end
local header_endian_decide
header_endian_decide = header_endian_option("Little", "Big")

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
            local b = readBytes(0, 4)

            if #b < 4 then
                return false
            end
            return b[1] == 0x7F and b[2] == 0x45 and b[3] == 0x4c and b[4] == 0x46
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
                    endian = header_endian_decide,
                    array = { -- an entry of what it is made up of.
                        type = "struct",
                        struct = header_class_option("ProgramHeader32", "ProgramHeader64")
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
                    endian = header_endian_decide,
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
                    endian = header_endian_decide,
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
                    endian = header_endian_decide,
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
                        type = "struct",
                        struct = "SymbolTable",
                        offset = function (structure, entry)
                            local index = entry["$array_index"]
                            local ind_struct = entry["$array"]["$user_symbols"][index]

                            return fh_get_bytes_entry_value(fh_find_entry(ind_struct, "FileOffset"))
                        end
                    }
                },

                {
                    name = "dynamicsymboltable",
                    type = "array",
                    endian = header_endian_decide,
                    elements = function (structure, entry)
                        local sections = fh_find_entry(structure, "sectionheadertable")

                        entry["$user_symbols"] = {}
                        for index=1, #sections["$data"] do
                            local s_entry = sections["$data"][index]
                            local s_struct = fh_get_entry__struct(s_entry)
                            local s_type = fh_get_enum_entry_value(fh_find_entry(s_struct, "Type"))
                            if s_type == "SHT_DYNSYM" then
                                table.insert(entry["$user_symbols"], s_struct)
                            end
                        end
                        return #entry["$user_symbols"]
                    end,
                    array = {
                        type = "struct",
                        struct = "SymbolTable",
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
                    endian = header_endian_decide
                },

                {
                    name = "relocationentries",
                    type = "array",
                    elements = function (structure, entry)
                        local sections = fh_find_entry(structure, "sectionheadertable")

                        entry["$user_reloc"] = {}
                        for index=1, #sections["$data"] do
                            local s_entry = sections["$data"][index]
                            local s_struct = fh_get_entry__struct(s_entry)
                            local s_type = fh_get_enum_entry_value(fh_find_entry(s_struct, "Type"))
                            if s_type == "SHT_REL" then
                                table.insert(entry["$user_reloc"], s_struct)
                            end
                        end
                        return #entry["$user_reloc"]
                    end,
                    array = {
                        type = "struct",
                        struct = "RelocationEntries",
                        offset = function (structure, entry)
                            local index = entry["$array_index"]
                            local ind_struct = entry["$array"]["$user_reloc"][index]

                            return fh_get_bytes_entry_value(fh_find_entry(ind_struct, "FileOffset"))
                        end
                    },
                    endian = header_endian_decide
                },

                {
                    name = "relocationappendentries",
                    type = "array",
                    elements = function (structure, entry)
                        local sections = fh_find_entry(structure, "sectionheadertable")

                        entry["$user_reloc"] = {}
                        for index=1, #sections["$data"] do
                            local s_entry = sections["$data"][index]
                            local s_struct = fh_get_entry__struct(s_entry)
                            local s_type = fh_get_enum_entry_value(fh_find_entry(s_struct, "Type"))
                            if s_type == "SHT_RELA" then
                                table.insert(entry["$user_reloc"], s_struct)
                            end
                        end
                        return #entry["$user_reloc"]
                    end,
                    array = {
                        type = "struct",
                        struct = "RelocationAppendEntries",
                        offset = function (structure, entry)
                            local index = entry["$array_index"]
                            local ind_struct = entry["$array"]["$user_reloc"][index]

                            return fh_get_bytes_entry_value(fh_find_entry(ind_struct, "FileOffset"))
                        end
                    },
                    endian = header_endian_decide
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
                    endian = header_endian_decide,
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
                    endian = header_endian_decide,
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
                    endian = header_endian_decide,
                },
                -- e_entry
                {
                    name = "EntryPoint",
                    highlight = HighlightType.Underlined,
                    size = header_class_pointer,
                    endian = header_endian_decide,
                },
                {
                    name = "ProgramHeaderTableOffset",
                    size = header_class_pointer,
                    endian = header_endian_decide,
                },
                {
                    name = "SectionHeaderTableOffset",
                    size = header_class_pointer,
                    endian = header_endian_decide,
                },
                {
                    name = "Flags",
                    size = 4,
                    endian = header_endian_decide,
                },
                {
                    name = "SelfHeaderSize",
                    -- normally 64 bytes for 64 bit and 52 for 32-bit
                    size = 2,
                    endian = header_endian_decide,
                },
                {
                    name = "ProgramHeaderTableEntrySize",
                    size = 2,
                    endian = header_endian_decide,
                },
                {
                    name = "ProgramHeaderTableEntries",
                    size = 2,
                    endian = header_endian_decide,
                },
                {
                    name = "SectionHeaderTableEntrySize",
                    size = 2,
                    endian = header_endian_decide,
                },
                {
                    name = "SectionHeaderTableEntries",
                    size = 2,
                    endian = header_endian_decide,
                },
                {
                    name = "SectionTableNamesEntryIndex",
                    size = 2,
                    endian = header_endian_decide,
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
                    size = header_class_pointer,
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
                    size = header_class_pointer
                },
                {
                    name = "FileOffset",
                    size = header_class_pointer,
                },
                {
                    name = "Size", -- in bytes
                    size = header_class_pointer
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
                    size = header_class_pointer
                },
                {
                    name = "EntrySize", -- Entry size if section holds table
                    size = header_class_pointer
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
                    size = header_class_pointer
                },
                {
                    name = "Info", -- reloc type and symbol index
                    size = header_class_pointer
                }
            }
        },

        {
            name = "RelocationAddend",
            entries = {
                {
                    name = "Offset", -- address
                    size = header_class_pointer
                },
                {
                    name = "Info", -- reloc type and symbol index
                    size = header_class_pointer
                },
                {
                    name = "Addend",
                    size = header_class_pointer
                }
            }
        },

        {
            name = "Dynamic",
            entries = {
                {
                    name = "Tag",
                    size = header_class_pointer
                },
                {
                    -- Union, tag decides what it is
                    -- Value: represents ints with various interpretations
                    -- Pointer: represents program virtual addresses (??)
                    name = "Value/Pointer",
                    size = header_class_pointer
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
            name = "RelocationEntries",
            entries = {
                {
                    name = "Relocations",
                    type = "array",
                    elements = function (structure, entry)
                        local index = structure["$entry"]["$array_index"]
                        local ind_struct = structure["$entry"]["$array"]["$user_reloc"][index]

                        if fh_get_bytes_entry_value(fh_find_entry(ind_struct, "FileOffset")) == 0 then
                            return 0
                        end

                        return fh_get_bytes_entry_value(fh_find_entry(ind_struct, "Size")) //
                            fh_get_bytes_entry_value(fh_find_entry(ind_struct, "EntrySize"))
                    end,
                    array = {
                        type = "struct",
                        struct = "Relocation"
                    }
                }
            }
        },

        {
            name = "RelocationAppendEntries",
            entries = {
                {
                    name = "Relocations",
                    type = "array",
                    elements = function (structure, entry)
                        local index = structure["$entry"]["$array_index"]
                        local ind_struct = structure["$entry"]["$array"]["$user_reloc"][index]

                        if fh_get_bytes_entry_value(fh_find_entry(ind_struct, "FileOffset")) == 0 then
                            return 0
                        end

                        return fh_get_bytes_entry_value(fh_find_entry(ind_struct, "Size")) //
                            fh_get_bytes_entry_value(fh_find_entry(ind_struct, "EntrySize"))
                    end,
                    array = {
                        type = "struct",
                        struct = "Relocation"
                    }
                }
            }
        },

        {
            name = "SymbolTable",
            entries = {
                {
                    name = "Symbols",
                    type = "array",
                    elements = function (structure, entry)
                        local index = structure["$entry"]["$array_index"]
                        local ind_struct = structure["$entry"]["$array"]["$user_symbols"][index]

                        if fh_get_bytes_entry_value(fh_find_entry(ind_struct, "FileOffset")) == 0 then
                            return 0
                        end

                        return fh_get_bytes_entry_value(fh_find_entry(ind_struct, "Size")) //
                            fh_get_bytes_entry_value(fh_find_entry(ind_struct, "EntrySize"))
                    end,
                    array = {
                        type = "struct",
                        struct = header_class_option("SymbolEntry32", "SymbolEntry64")
                    }
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