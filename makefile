EXEC:=app

#COMPILER
C:=gcc
CFLAGS:=
LFLAGS:=-I./src/ -lGL -lglfw -lm
DFLAGS:=-g -Wall
BFLAGS:=-s

#ARCHITECTURE
BIN_DIR:=bin
OBJ_EXT:=o
BUILD_DIR:=build
SRC_DIR:=src
SRC_EXT:=c
LIB_DIR:=lib
SHADER_DIR:=$(SRC_DIR)
SHADER_EXT:=glsl


#PROCESSED VARS
SRC_FILES:=$(wildcard ./$(SRC_DIR)/*.$(SRC_EXT))
SHADER_FILES:=$(wildcard ./$(SHADER_DIR)/*.$(SHADER_EXT))
OBJ_FILES:=$(foreach file,$(filter-out $(wildcard ./$(LIB_DIR)/*.$(OBJ_EXT)), $(wildcard ./$(LIB_DIR)/*)),$(file:./$(LIB_DIR)/%=./$(BIN_DIR)/%.$(OBJ_EXT))) $(foreach file,$(SRC_FILES),$(file:./$(SRC_DIR)/%.$(SRC_EXT)=./$(BIN_DIR)/%.$(SRC_EXT).$(OBJ_EXT)))

#RULES
.PHONY: all reset clean
.PRECIOUS: $(foreach file,$(filter-out $(wildcard ./$(LIB_DIR)/*.$(OBJ_EXT)), $(wildcard ./$(LIB_DIR)/*)),$(file:%=%.$(OBJ_EXT)))

all: $(EXEC)_d
	./$(BIN_DIR)/$(EXEC)

reset:
	rm -rf ./$(BIN_DIR)/

clean:
	rm ./$(BIN_DIR)/*.$(OBJ_EXT)

$(EXEC)_d: bin_dir $(OBJ_FILES)
	$(C) $(LFLAGS) $(DFLAGS) -o ./$(BIN_DIR)/$(EXEC) $(filter-out bin_dir, $^)

./$(BIN_DIR)/%.$(OBJ_EXT): ./$(SRC_DIR)/%
	$(C) $(CFLAGS) $(DFLAGS) -c -o $@ $<

./$(BIN_DIR)/%: ./$(LIB_DIR)/%
	cp $< $@

./$(LIB_DIR)/%.$(OBJ_EXT): ./$(LIB_DIR)/%
	$(C) $(CFLAGS) $(DFLAGS) -c -o $@ $<

bin_dir:
	mkdir -p ./$(BIN_DIR)/

shaders.o: $(SHADER_FILES)
	ld -r -b binary -o ./$(BUILD_DIR)/shaders.o $(SHADER_FILES)

#DEBUG
debug:
	echo $(foreach file,$(filter-out $(wildcard ./$(LIB_DIR)/*.$(OBJ_EXT)), $(wildcard ./$(LIB_DIR)/*)),$(file:%=%.$(OBJ_EXT)))