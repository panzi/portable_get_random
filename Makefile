CC=gcc
CFLAGS=-std=c99 -Wall -Werror -pedantic -O3 -DWIN_EXPORT
BUILD_DIR=build
LIB_OBJS=$(BUILD_DIR)/obj/portable_get_random.o

SO_OBJS=$(patsubst $(BUILD_DIR)/obj/%,$(BUILD_DIR)/shared-obj/%,$(LIB_OBJS))

AR=ar
SO_EXT=.so
SO_PREFIX=lib
BIN_EXT=
TARGET=$(shell uname|tr '[:upper:]' '[:lower:]')$(shell getconf LONG_BIT)
RELEASE=OFF
PREFIX=/usr/local
SO_FLAGS=-fPIC
SHARED_BIN_OBJS=

ifeq ($(patsubst %32,32,$(TARGET)),32)
    CFLAGS += -m32
else
ifeq ($(patsubst %64,64,$(TARGET)),64)
    CFLAGS += -m64
endif
endif

ifeq ($(patsubst win%,win,$(TARGET)),win)
    CFLAGS   += -Wno-pedantic-ms-format
    BIN_EXT   = .exe
    SO_EXT    = .dll
    SO_PREFIX =

    REAL_IMPL=$(patsubst dynamic,LoadLibrary,$(IMPL))
else
    REAL_IMPL=$(patsubst dynamic,dlsym,$(IMPL))

ifeq ($(patsubst darwin%,darwin,$(TARGET)),darwin)
    CC     = clang
    SO_EXT = .dylib
endif
endif

ifeq ($(TARGET),win32)
    CC=i686-w64-mingw32-gcc
else
ifeq ($(TARGET),win64)
    CC=x86_64-w64-mingw32-gcc
endif
endif

BUILD_DIR := $(BUILD_DIR)/$(TARGET)

LIB_DIRS=-L$(BUILD_DIR)/lib
INC_DIRS=-Isrc
EXAMPLES=$(BUILD_DIR)/examples/getrandom$(BIN_EXT)
EXAMPLES_SHARED=$(BUILD_DIR)/examples-shared/getrandom$(BIN_EXT)
LIB=$(BUILD_DIR)/lib/libportable-get-random.a
SO=$(BUILD_DIR)/lib/$(SO_PREFIX)portable-get-random$(SO_EXT)
INC=$(BUILD_DIR)/include/portable_get_random.h
LIBS=

ifeq ($(REAL_IMPL),BCryptGenRandom)
    CFLAGS += -DPORTABLE_GET_RANDOM_USE=PORTABLE_GET_RANDOM_IMPL_BCryptGenRandom
    LIBS   += -lbcrypt
else
ifeq ($(REAL_IMPL),dlsym)
    CFLAGS += -DPORTABLE_GET_RANDOM_USE=PORTABLE_GET_RANDOM_IMPL_dlsym
    LIBS   += -ldl
else
ifeq ($(patsubst /dev/%,/dev/,$(REAL_IMPL)),/dev/)
    CFLAGS += -DPORTABLE_GET_RANDOM_USE=PORTABLE_GET_RANDOM_IMPL_dev_random \
              -DPORTABLE_GET_RANDOM_DEV_RANDOM=\"$(REAL_IMPL)\"
else
ifneq ($(REAL_IMPL),)
    CFLAGS += -DPORTABLE_GET_RANDOM_USE=PORTABLE_GET_RANDOM_IMPL_$(REAL_IMPL)
endif
endif
endif
endif

ifeq ($(RELEASE),ON)
    CFLAGS    += -DNDEBUG
    BUILD_DIR := $(BUILD_DIR)/release
else
ifeq ($(RELEASE),OFF)
    CFLAGS    += -g
    BUILD_DIR := $(BUILD_DIR)/debug
else
    $(error illegal value for RELEASE=$(RELEASE))
endif
endif

.PHONY: static shared lib so inc examples examples_shared clean install uninstall

static: lib inc

shared: so inc

lib: $(LIB)

examples: $(EXAMPLES)

examples_shared: $(EXAMPLES_SHARED)

so: $(SO)

inc: $(inc)

install: $(LIB) $(SO) $(INC) $(BIN)
	@mkdir -p $(PREFIX)/lib $(PREFIX)/include
	cp $(LIB) $(SO) $(PREFIX)/lib
	cp $(INC) $(PREFIX)/include

uninstall:
	rm $(PREFIX)/lib/libportable-get-random.a \
	   $(PREFIX)/lib/$(SO_PREFIX)portable-get-random$(SO_EXT) \
	   $(PREFIX)/include/portable_get_random.h

$(BUILD_DIR)/examples/%$(BIN_EXT): $(BUILD_DIR)/obj/examples/%.o $(LIB)
	@mkdir -p $(BUILD_DIR)/examples
	$(CC) $(CFLAGS) -static $< $(LIB_DIRS) -lportable-get-random $(LIBS) -o $@

$(BUILD_DIR)/examples-shared/%$(BIN_EXT): $(BUILD_DIR)/shared-obj/examples/%.o $(SO)
	@mkdir -p $(BUILD_DIR)/examples-shared
	$(CC) $(CFLAGS) $< $(LIB_DIRS) -lportable-get-random $(LIBS) -o $@

$(BUILD_DIR)/obj/%.o: src/%.c
	@mkdir -p $(BUILD_DIR)/obj
	$(CC) $(CFLAGS) $(INC_DIRS) $< $(LIB_DIRS) $(LIBS) -c -o $@

$(BUILD_DIR)/shared-obj/%.o: src/%.c
	@mkdir -p $(BUILD_DIR)/shared-obj
	$(CC) $(CFLAGS) $(SO_FLAGS) $(INC_DIRS) $< $(LIB_DIRS) $(LIBS) -c -o $@

$(BUILD_DIR)/obj/examples/%.o: examples/%.c
	@mkdir -p $(BUILD_DIR)/obj/examples
	$(CC) $(CFLAGS) $(INC_DIRS) $< $(LIB_DIRS) $(LIBS) -c -o $@

$(BUILD_DIR)/shared-obj/examples/%.o: examples/%.c
	@mkdir -p $(BUILD_DIR)/shared-obj/examples
	$(CC) $(CFLAGS) $(SO_FLAGS) $(INC_DIRS) $< $(LIB_DIRS) $(LIBS) -c -o $@

$(LIB): $(LIB_OBJS)
	@mkdir -p $(BUILD_DIR)/lib
	@rm $@ 2>/dev/null || true
	$(AR) rcs $@ $^

$(SO): $(SO_OBJS)
	@mkdir -p $(BUILD_DIR)/lib
	$(CC) $(CFLAGS) -shared $^ $(LIB_DIRS) $(LIBS) -o $@

$(INC): src/portable_get_random.h
	@mkdir -p $(BUILD_DIR)/include
	cp src/portable_get_random.h $(BUILD_DIR)/include/portable_get_random.h

clean:
	rm -vf $(LIB_OBJS) $(SO_OBJS) $(LIB) $(EXAMPLES) $(EXAMPLES_SHARED) || true
