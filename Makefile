CC = gcc
CFLAGS = -g
LDFLAGS = -lcurl -lncurses -lmenu

RU_API_DIR = api_rutracker
GUI_DIR = gui
BUILD_DIR = build
UTILS_DIR = utils
TARGET = torrent_manager

OBJS = \
	$(BUILD_DIR)/api.o \
	$(BUILD_DIR)/env.o \
	$(BUILD_DIR)/gui.o \
	$(BUILD_DIR)/utils.o \
	$(BUILD_DIR)/main.o 



$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/main.o: main.c $(RU_API_DIR)/api.h $(RU_API_DIR)/env.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/api.o: $(RU_API_DIR)/api.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/env.o: $(RU_API_DIR)/env.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gui.o: $(GUI_DIR)/gui.c $(GUI_DIR)/gui.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/utils.o: $(RU_API_DIR)/utils.c $(RU_API_DIR)/utils.h
	$(CC) $(CFLAGS) -c $< -o $@	