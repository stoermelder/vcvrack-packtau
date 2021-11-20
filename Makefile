RACK_DIR ?= ../..

FLAGS += -Idep/include

# SOURCES += $(wildcard src/*.cpp) $(wildcard src/mb/*.cpp) $(wildcard src/audio/*.cpp)
SOURCES += $(wildcard src/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin is automatically added.
DISTRIBUTABLES += $(wildcard LICENSE*) res

# Dependencies
DEP_LOCAL := dep


include $(RACK_DIR)/plugin.mk
