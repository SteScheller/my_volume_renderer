# OpenGL + GLFW + gl3w + imgui Makefile
#
# Target Platform: Linux

EXE = mvr
BUILD_DIR = build

SOURCES = src/main.cpp src/util.cpp src/configraw.cpp src/transferfunc.cpp
SOURCES += libs/imgui/imgui_impl_glfw.cpp libs/imgui/imgui_impl_opengl3.cpp
SOURCES += libs/imgui/imgui.cpp libs/imgui/imgui_demo.cpp 
SOURCES += libs/imgui/imgui_draw.cpp
SOURCES += libs/gl3w/GL/gl3w.c

OBJS = $(addsuffix .o, $(basename $(SOURCES)))

INCLUDE = -I./src -I./include -I./libs/gl3w -I./libs/imgui -I./libs/nlohmann

CC = cc
CXX = g++
CXXFLAGS = $(INCLUDE) `pkg-config --cflags glfw3`
CXXFLAGS += -Wall -Wextra
CFLAGS = $(CXXFLAGS)

LIBS = -lGL `pkg-config --static --libs glfw3` 
LIBS += -lboost_system -lboost_filesystem -lboost_regex -lboost_program_options

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $(BUILD_DIR)/$(@F) $<

%.o:%.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/$(@F) $<

debug: CXXFLAGS += -DDEBUG -g
debug: CCFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -DRELEASE -O3
release: CCFLAGS += -DRELEASE -O3
release: all

all: directory $(EXE)
	@echo Build complete!

directory:
	mkdir -p $(BUILD_DIR)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LIBS) -o $(BUILD_DIR)/$@ $(addprefix $(BUILD_DIR)/, $(notdir $^))

.PHONY: clean
clean:
	rm $(BUILD_DIR)/*.o $(BUILD_DIR)/$(EXE)

