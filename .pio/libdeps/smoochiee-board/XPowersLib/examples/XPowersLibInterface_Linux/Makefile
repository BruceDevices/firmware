EXAMPLE_DIR = .
SRC_DIR = ../../src
DIR_BIN = ./build

OBJ_C = $(wildcard ${SRC_DIR}/*.cpp ${EXAMPLE_DIR}/*.cpp)
OBJ_O = $(patsubst %.cpp,${DIR_BIN}/%.o,$(notdir ${OBJ_C}))

TARGET = main

DEBUG = -DXPOWERS_NO_ERROR

$(shell mkdir -p $(DIR_BIN))

# Change to the directory where the g++ compilation chain is located
SDK_PATH = /mnt/c/Users/lewis/Desktop/liunx/
CC = $(SDK_PATH)/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++
MSG = -g -O0 -Wall 
CFLAGS += $(MSG) $(DEBUG)

${TARGET}:${OBJ_O}
	$(CC) $(CFLAGS) $(OBJ_O) -o $@ $(LIB)
    
${DIR_BIN}/%.o:$(EXAMPLE_DIR)/%.cpp
	$(CC) $(CFLAGS) -c  $< -o $@ -I $(SRC_DIR) 

${DIR_BIN}/%.o:$(SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c  $< -o $@ $(LIB)

clean :
	rm $(DIR_BIN)/*.* 
	rm $(TARGET) 
