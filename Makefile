CC = gcc
CFLAGS = -g -Wall -Wextra -Wconversion -Wsign-conversion 
LDFLAGS = -lcurl -lncurses -lmenu -lpthread
API_DIR = api
GUI_DIR = gui
BUILD_DIR = build
UTILS_DIR = utils
TARGET = torrent_manager

ifdef DEBUG
CFLAGS += -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined
endif

INSTALL_BIN_DIR = $(HOME)/.local/bin
INSTALL_CONFIG_DIR = $(HOME)/.config/$(TARGET)

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


install: $(TARGET)
	mkdir -p $(INSTALL_BIN_DIR)
	mkdir -p $(INSTALL_CONFIG_DIR)
	cp $(TARGET) $(INSTALL_BIN_DIR)/$(TARGET)
	cp .env $(INSTALL_CONFIG_DIR)/.env

clean:
	rm -rf $(BUILD_DIR) $(TARGET)