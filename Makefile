.PHONY: build_debug clean

output_folder = build
output = $(output_folder)/program

source_files = src/main.cpp src/mutil.cpp src/window.cpp src/subview.cpp src/Herix/src/herix.cpp src/Herix/src/editstorage.cpp src/Herix/src/types.cpp


build_debug:
	mkdir -p $(output_folder)
	clang++ -std=c++17 -DDEBUG $(source_files) -o $(output) -lncurses -llua -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-exit-time-destructors -Wno-global-constructors

#g++ -std=c++17 $(source_files) -o $(output) -DDEBUG -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wno-unused

clean:
	rm $(output)