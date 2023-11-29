#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need GLFW (https://www.glfw.org)
#

#CXX = g++
#CXX = clang++

EXE = bunny-ui
SOURCES = main.cpp utils.cpp
IMGUI_SOURCES = imgui.cpp imgui_draw.cpp imgui_widgets.cpp imgui_tables.cpp imgui_demo.cpp
IMGUI_SOURCES += imgui_impl_glfw.cpp imgui_impl_opengl3.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
IMGUI_OBJS = $(patsubst %.cpp,imgui/%.o,$(IMGUI_SOURCES))
UNAME_S := $(shell uname -s)

CXXFLAGS = -Iimgui -DIMGUI_IMPL_OPENGL_LOADER_CUSTOM="<OpenGL/gl3.h>" -DGL_SILENCE_DEPRECATION -DENABLE_VAO
CXXFLAGS += -Wall -Wformat -std=c++17

ifeq ($(DEBUG), 1)
	CXXFLAGS += -g
endif


##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS = -lGLEW
	LIBS += -lGL `pkg-config --static --libs glfw3`

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -L/opt/local/lib
	LIBS += -lglfw

	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lglfw3 -lgdi32 -lopengl32 -limm32 -lglew32

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

imgui/%.o:imgui/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

run: all
	./$(EXE)

$(EXE): $(OBJS) $(IMGUI_OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

main.o: utils.h materials.h
utils.o: utils.h

clean:
	rm -f $(EXE) $(OBJS) imgui.ini

cleanall: clean
	rm -f $(EXE) $(OBJS) $(IMGUI_OBJS)
