# Based on https://github.com/latekvo/tinyscript/blob/main/Makefile
TARGET=plain-sight
SOURCE_DIR=src/
BUILD_DIR=build/
FLAGS=-g -Wall -Wextra
CC=g++
EXT=cpp

OBJECTS=$(subst $(SOURCE_DIR),$(BUILD_DIR),$(subst .$(EXT),.o,$(wildcard $(SOURCE_DIR)*.$(EXT))))

default: $(TARGET) 
.PHONY: clean test 

$(TARGET): $(OBJECTS) 
	$(CC) $(FLAGS) -o $@ $^ 

$(BUILD_DIR)%.o: $(SOURCE_DIR)%.$(EXT)
	mkdir -p $(BUILD_DIR)
	$(CC) $(FLAGS) -c -o $@ $(subst $(BUILD_DIR),$(SOURCE_DIR),$(subst .o,.$(EXT),$@))

clean:
	rm -r $(BUILD_DIR) $(TARGET)

