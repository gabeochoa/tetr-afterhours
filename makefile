
#RAYLIB_FLAGS := `../RayLib/debug/lib/pkgconfig/raylib.pc --cflags`
RAYLIB_LIB := F:/Raylib/debug/lib -lraylib
RAYLIB_INCLUDE := F:/Raylib/include/raylib.h

RELEASE_FLAGS = -std=c++20 $(RAYLIB_FLAGS)

FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
		-Wconversion -g $(RAYLIB_FLAGS)

NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers \
		  -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion \
		  -Wno-implicit-int-float-conversion -Werror
INCLUDES = -Ivendor/ -Isrc/ -I$(RAYLIB_INCLUDE)
LIBS = -L. -Lvendor/ -L$(RAYLIB_LIB) -lopengl32 -lgdi32 -lwinmm

SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp)
H_FILES := $(wildcard src/**/*.h src/**/*.hpp)
OBJ_DIR := ./output
OBJ_FILES := $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

DEPENDS := $(patsubst %.cpp,%.d,$(SOURCES))
-include $(DEPENDS)

OUTPUT_EXE := tetr.exe

#CXX := clang++
CXX := g++

.PHONY: all clean

# all:
# 	clang++ $(FLAGS) $(INCLUDES) $(LIBS) src/main.cpp -o $(OUTPUT_EXE) && ./$(OUTPUT_EXE)

all:
	g++ $(FLAGS) $(INCLUDES) $(LIBS) -fmax-errors=10 src/main.cpp -o $(OUTPUT_EXE) && .\$(OUTPUT_EXE) 

prof:
	rm -rf recording.trace/
	xctrace record --template 'Game Performance' --output 'recording.trace' --launch $(OUTPUT_EXE)
