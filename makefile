
RAYLIB_FLAGS := `pkg-config --cflags raylib`
RAYLIB_LIB := `pkg-config --libs raylib`

RELEASE_FLAGS = -std=c++2a $(RAYLIB_FLAGS)

FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
		-Wconversion -g $(RAYLIB_FLAGS)

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
CXX := g++

.PHONY: all clean

all:
	$(CXX) $(FLAGS) $(INCLUDES) $(LIBS) src/main.cpp -o $(OUTPUT_EXE) && ./$(OUTPUT_EXE)

prof:
	rm -rf recording.trace/
	xctrace record --template 'Game Performance' --output 'recording.trace' --launch $(OUTPUT_EXE)

