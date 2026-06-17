CC = gcc
CFLAGS = -g -Wall -Wextra -Wconversion -Wsign-conversion -fsanitize=address,undefined
LDFLAGS = -lcurl -lncurses -lmenu -fsanitize=address,undefined

API_DIR = api
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

$(BUILD_DIR)/main.o: main.c $(API_DIR)/api.h $(API_DIR)/env.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/api.o: $(API_DIR)/api.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/env.o: $(API_DIR)/env.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gui.o: $(GUI_DIR)/gui.c $(GUI_DIR)/gui.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/utils.o: $(API_DIR)/utils.c $(API_DIR)/utils.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
