
RAYLIB_FLAGS := `pkg-config --cflags raylib`
RAYLIB_LIB := `pkg-config --libs raylib`

RELEASE_FLAGS = -std=c++2a $(RAYLIB_FLAGS)

FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
		-Wmost -Wconversion -g $(RAYLIB_FLAGS)

NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers \
		  -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion \
		  -Wno-implicit-int-float-conversion -Werror
INCLUDES = -Ivendor/ -Isrc/
LIBS = -L. -Lvendor/ $(RAYLIB_LIB)

SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp)
H_FILES := $(wildcard src/**/*.h src/**/*.hpp)
OBJ_DIR := ./output
OBJ_FILES := $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

OUTPUT_EXE := tetr.exe

CXX := clang++

.PHONY: all clean

all: post-build

main-build: $(OUTPUT_EXE)

post-build: main-build
	./$(OUTPUT_EXE) 2>&1 $(GAME_LOG)

$(OUTPUT_EXE): $(H_FILES) $(OBJ_FILES)
	$(CXX) $(FLAGS) $(LEAKFLAGS) $(NOFLAGS) $(INCLUDES) $(LIBS) $(OBJ_FILES) -o $(OUTPUT_EXE)

release: FLAGS=$(RELEASE_FLAGS)
release: NOFLAGS=

$(OBJ_DIR)/%.o: %.cpp $(H_FILES) makefile
	$(CXX) $(FLAGS) $(NOFLAGS) $(INCLUDES) -c $< -o $@ -MMD -MF $(@:.o=.d)

%.d: %.cpp
	$(MAKEDEPEND)

clean:
	rm -r $(OBJ_DIR)
