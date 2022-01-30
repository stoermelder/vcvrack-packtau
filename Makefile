RACK_DIR ?= ../..

FLAGS += \
	-Idep/include

SOURCES += $(wildcard src/*.cpp) $(wildcard src/mb/*.cpp) $(wildcard src/audio/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin is automatically added.
DISTRIBUTABLES += $(wildcard LICENSE*) res

# Dependencies
DEP_LOCAL := dep


include $(RACK_DIR)/plugin.mk


win-dist: all
	rm -rf dist
	mkdir -p dist/$(SLUG)
	@# Strip and copy plugin binary
	cp $(TARGET) dist/$(SLUG)/
ifdef ARCH_MAC
	$(STRIP) -S dist/$(SLUG)/$(TARGET)
else
	$(STRIP) -s dist/$(SLUG)/$(TARGET)
endif
	@# Copy distributables
	cp -R $(DISTRIBUTABLES) dist/$(SLUG)/
	@# Create vcvplugin package
	cd dist && tar -c $(SLUG) | zstd -$(ZSTD_COMPRESSION_LEVEL) -o "$(SLUG)"-"$(VERSION)"-$(ARCH_OS_NAME).vcvplugin