
# Ensure Homebrew's pkgconfig is found (non-interactive shells don't source .bashrc,
# and MacPorts pkg-config is first in PATH). ~/homebrew is Gabe's brew prefix.
export PKG_CONFIG_PATH := $(HOME)/homebrew/lib/pkgconfig:$(PKG_CONFIG_PATH)

RAYLIB_FLAGS := `pkg-config --cflags raylib`
# Link the STATIC raylib archive: the Homebrew dylib does not re-export glfw
# symbols that afterhours's window_manager calls directly (glfwGetVideoModes, etc.)
RAYLIB_LIB := $(shell brew --prefix raylib)/lib/libraylib.a -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreFoundation -framework CoreGraphics

# raylib 5.5's raymath.h ships C++ operator overloads that collide with the
# vendored RaylibOpOverloads.h; disable raylib's so ours win.
RAYMATH_FLAGS := -DRAYMATH_DISABLE_CPP_OPERATORS

RELEASE_FLAGS = -std=c++2a $(RAYLIB_FLAGS) $(RAYMATH_FLAGS)

FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
		-Wconversion -g $(RAYLIB_FLAGS) $(RAYMATH_FLAGS)

NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers \
		  -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion \
		  -Wno-implicit-int-float-conversion -Werror
INCLUDES = -Ivendor/ -Isrc/
LIBS = -L. -Lvendor/ $(RAYLIB_LIB)

SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp)
H_FILES := $(wildcard src/**/*.h src/**/*.hpp)
OBJ_DIR := ./output
OBJ_FILES := $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

DEPENDS := $(patsubst %.cpp,%.d,$(SOURCES))
-include $(DEPENDS)


OUTPUT_EXE := tetr.exe

# CXX := clang++ -Wmost
# NOTE: g++ (Homebrew GCC) cannot parse the macOS 15/26 SDK headers. Use clang++.
CXX := clang++

.PHONY: all clean

all:
	$(CXX) $(FLAGS) $(NOFLAGS) $(INCLUDES) $(LIBS) src/main.cpp -o $(OUTPUT_EXE) && ./$(OUTPUT_EXE)

prof:
	rm -rf recording.trace/
	xctrace record --template 'Game Performance' --output 'recording.trace' --launch $(OUTPUT_EXE)


check:
	python3 scripts/check_correct.py src
