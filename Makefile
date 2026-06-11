CC = gcc
CFLAGS = -g 
LDFLAGS = -lcurl -lncurses -lmenu
RU_API_DIR = api_rutracker
GUI_DIR = gui
BUILD_DIR = build
TARGET = torrent_manager

OBJS = $(BUILD_DIR)/api.o $(BUILD_DIR)/env.o $(BUILD_DIR)/gui.o $(BUILD_DIR)/main.o 


$(TARGET) : $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/main.o : main.c $(RU_API_DIR)/api.h $(RU_API_DIR)/env.h
	$(CC) $(CFLAGS) -c $< -o $@	

$(BUILD_DIR)/%.o : $(API_DIR)/%.c $(RU_API_DIR)/api.h $(RU_API_DIR)/env.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR_DIR)/%.o : $(GUI_DIR)/%.c gui/gui.h
	$(CC) $(CFLAGS) -c $< -o $@


