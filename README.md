# HerixTUI

A terminal interface over Hex Editor library written in C++. (See: https://github.com/MinusGix/Herix)  

Currently the code is only tested on Linux, but it would be good to support Windows and MacOS.

## Configuration
It first looks at the `$XDG_CONFIG_HOME` environment variable, and then `$HOME` for `herixtui.lua` which is the name of the configuration file.
A custom one can be passed in with the `-c` parameter.

## Features:  
### Lua Plugin System
A lua plugin system (using the `sol2` library) which allows one to write sidewindows for the main Hex-View. The Ascii-View on the right, and the Offsets on the left are both written in lua as examples.
### File Format Highlighting
A supplied plugin for highlighting File Formats with a simple method is included. Currently only supports partial implementation of ELF.
The File-Highlighter reads the file, and displays the names of the fields of the file, and if they have a special meaning it will display that. Since the highlighting code (for example the ELF highlighter) can use Lua, it can do more detailed methods of file highlighting which are required for the wide varied formats out there.
### Ascii Sidebar
An essential in a Hex Editor.

## To-Be-Implemented Features:  
### Commands to Interpret Data
Commands which you can use to transform the data at the cursor (or perhaps elsewhere). For allowing one to read data at position as a certain sized integer (a common feature), but also allow more complex methods of reading and modifying data.
### Various Improvements In Base Library
There are issues with the library (HerixLib) that make certain features hard/impossible to do currently, and improvements from that can leak over to the UI for it.