# Compiler configuration
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

# Library configuration
LIBS = -lSDL2 -lm

# Target application binary name
TARGET = HaikuPacman
VERSION = 1.0.2
PACKAGE_DIR := build/package
REVISION = 1

# Shared target architectures
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M), BePC)
    CXX = g++-x86 
    ARCH = x86_gcc2
    INCLUDE = -L/boot/system/lib/x86
    is32bit = _x86
    DEFINES += -DIS_HAIKU_32BIT
else
    CXX = g++
    ARCH = x86_64
    INCLUDE = -L/boot/system/lib
endif

DEFINES := $(DEFINES)

# Source files configuration
SRCS = HaikuPacman.cpp
OBJS = $(SRCS:.cpp=.o)
RSRCS = HaikuPacman.rsrc

# Default compilation rule
all: $(TARGET)

# Link the object files into the final executable binary
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)
	rc -o $(TARGET).rsrc $(TARGET).rdef
	xres -o $(TARGET) $(RSRCS)
	mimeset -f $(TARGET)
	
# Compile C++ source files into binary object files
%.o: %.cpp
	$(CXX) $(DEFINES) $(CXXFLAGS) -c $< -o $@

# Clean rule to wipe temporary object artifacts and target binaries
clean:
	rm -f *.o *.rsrc $(TARGET) *.hpkg
	rm -rf build

# Run rule to compile and instantly launch the app
run: all
	./$(TARGET)

.PHONY: all clean run

release: all
	@[ -n "$(PACKAGE_DIR)" ] || { echo "PACKAGE_DIR is undefined"; exit 1; }
	rm -rf "./$(PACKAGE_DIR)"
	mkdir -p $(PACKAGE_DIR)
	sed -e 's/$$(TARGET)/$(TARGET)/g' -e 's/$$(REVISION)/$(REVISION)/g'  -e 's/$$(is32bit)/$(is32bit)/g' -e 's/$$(VERSION)/$(VERSION)/g' -e 's/$$(ARCH)/$(ARCH)/' -e 's/$$(YEAR)/$(shell date +%Y)/' $(TARGET).tpl > $(PACKAGE_DIR)/.PackageInfo
	mkdir -p $(PACKAGE_DIR)/apps
	mkdir -p $(PACKAGE_DIR)/bin
	mkdir -p $(PACKAGE_DIR)/data/deskbar/menu/Applications
	cp $(TARGET) $(PACKAGE_DIR)/apps/$(TARGET)
	ln -s /boot/system/apps/$(TARGET) $(PACKAGE_DIR)/bin/$(TARGET)
	ln -s /boot/system/apps/$(TARGET) $(PACKAGE_DIR)/data/deskbar/menu/Applications/$(TARGET)
	package create -C $(PACKAGE_DIR) $(TARGET)-$(VERSION)-$(REVISION)-$(ARCH).hpkg	


