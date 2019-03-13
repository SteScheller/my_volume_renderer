# OpenGL + GLFW + gl3w + imgui Makefile
#
# Target Platform: Linux

TARGET = mvr
TARGET_LIB = libmvr.so.1.1.1
TARGET_LIB_SONAME = libmvr.so.1
BUILD_DIR = build

SOURCES = src/main.cpp src/mvr.cpp
SOURCES += src/util/util.cpp src/util/texture.cpp src/util/geometry.cpp
SOURCES += src/configraw.cpp src/util/transferfunc.cpp
SOURCES += libs/imgui/imgui_impl_glfw.cpp libs/imgui/imgui_impl_opengl3.cpp
SOURCES += libs/imgui/imgui.cpp libs/imgui/imgui_demo.cpp
SOURCES += libs/imgui/imgui_draw.cpp libs/imgui/imgui_widgets.cpp
SOURCES += libs/imgui/imgui_stl.cpp
SOURCES += libs/gl3w/GL/gl3w.c

OBJS = $(addsuffix .o, $(basename $(SOURCES)))

INCLUDE = -I./src -I./include -I./libs/gl3w -I./libs/imgui -I./libs/nlohmann

CC = cc
CXX = g++
LINKER = ld

CXXFLAGS = $(INCLUDE) -std=c++14 `pkg-config --cflags glfw3`
CXXFLAGS += -Wall -Wextra
DEBUG_CXXFLAGS = -DDEBUG -g
RELEASE_CXXFLAGS = -DRELEASE -O3

CFLAGS = $(INCLUDE) -std=c11 `pkg-config --cflags glfw3`
CFLAGS += -Wall -Wextra
DEBUG_CFLAGS = -DDEBUG -g
RELEASE_CFLAGS = -DRELEASE -O3

LDFLAGS = -lGL `pkg-config --static --libs glfw3`
LDFLAGS += -lboost_system -lboost_filesystem -lboost_regex
LDFLAGS += -lboost_program_options
LDFLAGS += -lfreeimage

.PHONY: clean

default: debug

debug: CADDITIONALFLAGS = $(DEBUG_CFLAGS)
debug: CXXADDITIONALFLAGS = $(DEBUG_CXXFLAGS)
debug: TARGET_DIR = $(BUILD_DIR)/debug
debug: $(BUILD_DIR) $(BUILD_DIR)/debug start $(TARGET)
	@echo Build of standalone executable complete!

release: CADDITIONALFLAGS = $(RELEASE_CFLAGS)
release: CXXADDITIONALFLAGS = $(RELEASE_CXXFLAGS)
release: TARGET_DIR = $(BUILD_DIR)/release
release: $(BUILD_DIR) $(BUILD_DIR)/release start $(TARGET)
	@echo Build of standalone executable complete!

shared: CADDITIONALFLAGS = $(RELEASE_CFLAGS) -fpic
shared: CXXADDITIONALFLAGS = $(RELEASE_CXXFLAGS) -fpic
shared: TARGET_DIR = $(BUILD_DIR)/lib
shared: LDADDITIONALFLAGS = -shared -Wl,-soname,$(TARGET_LIB_SONAME)
shared: $(BUILD_DIR) $(BUILD_DIR)/lib start $(TARGET_LIB)
	@echo Build of shared library complete!

start:
	@echo -------------------------------------------------------------------------------
	@echo Compiling...
	@echo
	@echo CXXFLAGS: $(CXXFLAGS) $(CXXADDITIONALFLAGS)
	@echo CFLAGS: $(CFLAGS) $(CADDITIONALFLAGS)
	@echo TARGET_DIR $(TARGET_DIR)
	@echo

%.o: %.cpp
	@echo $<
	@$(CXX) $(CXXFLAGS) $(CXXADDITIONALFLAGS) -c -o $(TARGET_DIR)/$(@F) $<

%.o: %.c
	@echo $<
	@$(CC) $(CFLAGS)  $(CADDITIONALFLAGS) -c -o $(TARGET_DIR)/$(@F) $<

$(BUILD_DIR):
	@echo Creating build directory...
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%:
	@echo Creating target directory...
	@mkdir -p $@

$(TARGET): $(OBJS)
	@echo -------------------------------------------------------------------------------
	@echo Linking executable...
	@echo
	@echo LDFLAGS: $(LDFLAGS) $(LDADDITIONALFLAGS)
	@echo TARGET_DIR: $(TARGET_DIR)
	@echo TARGET: $(TARGET)
	@echo
	@$(CXX) $(addprefix $(TARGET_DIR)/, $(notdir $^)) $(LDFLAGS) $(LDADDITIONALFLAGS) -o $(TARGET_DIR)/$(TARGET)

$(TARGET_LIB): $(OBJS)
	@echo -------------------------------------------------------------------------------
	@echo Linking shared library...
	@echo
	@echo LDFLAGS: $(LDFLAGS) $(LDADDITIONALFLAGS)
	@echo TARGET_DIR: $(TARGET_DIR)
	@echo TARGET_LIB: $(TARGET_LIB)
	@echo
	@$(CXX) $(addprefix $(TARGET_DIR)/, $(notdir $^)) $(LDFLAGS) $(LDADDITIONALFLAGS) -o $(TARGET_DIR)/$(TARGET_LIB)

clean:
	@echo Cleaning up...
	@rm -rf ./$(BUILD_DIR)/debug
	@rm -rf ./$(BUILD_DIR)/release
	@rm -rf ./$(BUILD_DIR)/lib
	@echo Done!

