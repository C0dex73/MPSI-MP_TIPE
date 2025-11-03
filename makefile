#enables the makefile secondary expansion
.SECONDEXPANSION:

#~ VARIABLES

#^ constants
BIN_DIR:=./bin
DIST_DIR:=./dist
SRC_DIR:=./src
SRC_EXT:=c
HDR_EXT:=h
OBJ_EXT:=o
DEP_FILE:=.dep
C:=gcc
NODE_ARGS_FILE:=$(C).flags

DEBUG_FLAGS:=-g3
BUILD_FLAGS:=-s -O3

#^ System dependent variables
LIB_EXT:=
DOTEXE:=
IMP_LIB_EXT:=
CFLAGS:=
ifeq ($(OS),Windows_NT)
	DOTEXE+=.exe
	LIB_EXT+=dll
	IMP_LIB_EXT+=lib
	CFLAGS+=-mwindows
else
	LIB_EXT+=so
	DOTEXE+=
	IMP_LIB_EXT+=a
endif

#^ Command specific variables

ifeq ($(firstword $(MAKECMDGOALS)),build)
	CFLAGS+=$(BUILD_FLAGS)
else
	CFLAGS+=$(DEBUG_FLAGS)
endif

#^ Processed variables

EXEC_NODES:=$(shell ./scripts/getnodes.sh -E $(SRC_DIR))
LIB_NODES:=$(shell ./scripts/getnodes.sh -L $(SRC_DIR))

EXECS:=$(foreach node,$(EXEC_NODES),$(BIN_DIR)/$(shell basename $(node))$(DOTEXE))
LIBS:=$(foreach node,$(LIB_NODES),$(BIN_DIR)/$(shell basename $(node)).$(LIB_EXT))

DEP_FILES:=$(EXEC_NODES:%/=%/$(DEP_FILE)) $(LIB_NODES:%/=%/$(DEP_FILE))

INCLUDE_DIRS_DIRECTIVE:=$(foreach node,$(LIB_NODES) $(EXEC_NODES),-I$(node))

#~ MAIN RULES
.PHONY: all reset debug eod_%_eod node_%

all: $(DEP_FILES) $(EXECS)

debug: 
	@echo no debug script in debug recipe

clean:
	@echo "deleting $(OBJ_EXT) files from $(BIN_DIR)..."
	@rm -f $(BIN_DIR)/*.$(OBJ_EXT) 2>/dev/null || :
	@echo "Done !"
	

reset:
	@echo "deleting everything from $(BIN_DIR)..."
	@echo "deleting dependencies files..."
	@rm -f $(BIN_DIR)/* !.gitkeep 2>/dev/null || :
	@rm -f $(DEP_FILES) 2>/dev/null || :
	@echo "Done !"

_buildreset:
	@echo "deleting binaries from $(DIST_DIR)..."
	@rm -f $(DIST_DIR)/*$(DOTEXE) 2>/dev/null || :
	@rm -f $(DIST_DIR)/*.$(LIB_EXT) 2>/dev/null || :
	@echo "Done !"

build: _buildreset reset all clean
	@echo "copying output to $(DIST_DIR)..."
	@cp -f $(BIN_DIR)/*$(DOTEXE) $(DIST_DIR) 2>/dev/null || : 
	@cp -f $(BIN_DIR)/*.$(LIB_EXT) $(DIST_DIR) 2>/dev/null || :
	@echo "Done !"
	@echo ""
	@echo "-------- BUILDING COMPLETE --------"


#~ NODES SHENANIGANS

#^ executable nodes
$(EXECS): $(BIN_DIR)/%$(DOTEXE): $$(wildcard $(SRC_DIR)/$$*/*.$(SRC_EXT)) $$(wildcard $(SRC_DIR)/$$*/*.$(HDR_EXT)) $$(shell [ "$$(shell cat $(SRC_DIR)/$$*/$(DEP_FILE) 2> /dev/null)" != "" ] && cat $(SRC_DIR)/$$*/$(DEP_FILE) 2> /dev/null || echo "eod_$(SRC_DIR)/$$*/_eod")
	@echo
	@echo "------------- EXECUTABLE NODE PROCESSING START : $@ -------------"
	@echo "processed dependencies : $(foreach dep,$(filter-out eod_$(SRC_DIR)/$*/_eod,$^),$(patsubst %$(HDR_EXT),,$(patsubst %$(SRC_EXT),,$(dep))))"
	$(C) -o $@ $(wildcard $(SRC_DIR)/$*/*.$(SRC_EXT)) $(foreach file,$(foreach dep,$(filter-out eod_$(SRC_DIR)/$*/_eod,$^),$(patsubst %$(HDR_EXT),,$(patsubst %$(SRC_EXT),,$(dep)))),$(BIN_DIR)/$(file).$(LIB_EXT).$(IMP_LIB_EXT)) @$(SRC_DIR)/$*/$(NODE_ARGS_FILE) $(INCLUDE_DIRS_DIRECTIVE) $(CFLAGS)
	@echo "Done !"

#^ library/binary nodes
$(LIBS): $(BIN_DIR)/%.$(LIB_EXT): $$(wildcard $(SRC_DIR)/$$*/*.$(SRC_EXT)) $$(wildcard $(SRC_DIR)/$$*/*.$(HDR_EXT)) $$(shell [ "$$(shell cat $(SRC_DIR)/$$*/$(DEP_FILE) 2> /dev/null)" != "" ] && cat $(SRC_DIR)/$$*/$(DEP_FILE) 2> /dev/null || echo "eod_$(SRC_DIR)/$$*/_eod")
	@echo
	@echo "------------- LIBRARY/BINARY NODE PROCESSING START : $@ -------------"
	@echo "processed dependencies : $(foreach dep,$(filter-out eod_$(SRC_DIR)/$*/_eod,$^),$(patsubst %$(HDR_EXT),,$(patsubst %$(SRC_EXT),,$(dep))))"
	@if [ -d $(SRC_DIR)/$*/bin/ ]; then \
		cp -f $(SRC_DIR)/$*/bin/$(shell basename $@) $@ ; \
		cp -f $(SRC_DIR)/$*/bin/$(shell basename $@.$(IMP_LIB_EXT)) $@.$(IMP_LIB_EXT) ; \
	else \
		( cd $(SRC_DIR)/$* ; $(C) -c $(foreach file,$(wildcard $(SRC_DIR)/$*/*.$(SRC_EXT)),$(shell basename $(file))) @$(NODE_ARGS_FILE) $(foreach dir,$(INCLUDE_DIRS_DIRECTIVE),$(dir:-I%=-I../../%)) $(CFLAGS)) ; \
		mv $(SRC_DIR)/$*/$(shell basename $(subst .$(LIB_EXT),.$(OBJ_EXT),$@)) $(subst .$(LIB_EXT),.$(OBJ_EXT),$@) ; \
		$(C) -shared -o $@ $(subst .$(LIB_EXT),.$(OBJ_EXT),$@) -Wl,--out-implib,$@.$(IMP_LIB_EXT) $(CFLAGS); \
	fi;
	@echo "Done !"

#^ dependencies files
#checks for includes in all c files of a node, keep only the ones that correspond to other nodes from the project and write them all in a file
# also checks if a node is a binary node (binaries are to be downloaded and put in the bin/ folder of the node) and if so, checks if all the required binaries are in it
$(DEP_FILES): %/$(DEP_FILE): $$(wildcard $$(subst /$(DEP_FILE),,$$@)/*.$(SRC_EXT))
	@echo "building $@..."
	if ! [ -d ./$(dir $@)bin/ ]; then \
		echo $(filter-out $(shell basename $(basename $@)),$(filter $(foreach node,$(EXEC_NODES) $(LIB_NODES),$(shell basename $(node))),$(foreach file,$(shell grep -sh "#include" . $^ | grep "." | sed 's/#include <//' | sed 's/>//' | sed 's/ //g'),$(basename $(shell basename $(file)))))) > $@ ; \
	elif ! [ -f ./$(dir $@)bin/dependencies.lnk ]; then \
			echo "Error, no binary dependencies file nor source code in node $@" ;\
	else \
		for f in $$(tail -n+2 ./$(dir $@)bin/dependencies.lnk) ; do\
			if ! [ -f ./$(dir $@)bin/$$f ]; then \
				echo "Node $@ is dependent of file ./$(dir $@)bin/$$f which is missing, check the url in ./$(dir $@)bin/dependencies.lnk to download it"; \
				for url in "$$(head -n 1 ./$(dir $@)bin/dependencies.lnk)"; do python -m webbrowser -t $$url; done; \
				echo "Missing file, exiting..."; \
				exit 1; \
			fi \
		done \
	fi;
	@echo "Done !"

#^ Node to file
$(foreach node,$(EXEC_NODES),$(shell basename $(node))): %: $(BIN_DIR)/%$(DOTEXE)
$(foreach node,$(LIB_NODES),$(shell basename $(node))): %: $(BIN_DIR)/%.$(LIB_EXT) #$(BIN_DIR)/%.$(IMP_LIB_EXT)

#^ Endpoint node from a dependency perspective
$(foreach node,$(EXEC_NODES) $(LIB_NODES),eod_$(node)_eod): %:
	@echo "$(subst eod_,,$(subst _eod,,$@)) does not have a dependencies (.dep) file, assuming end-of-dep node"

#~ NODE CREATOR


node_% node_e_%:
	@./scripts/create-node.sh -S$(SRC_DIR) $*

node_l_%:
	@./scripts/create-node.sh -L -S$(SRC_DIR) $*

node_b_%:
	@./scripts/create-node.sh -B -S$(SRC_DIR) $*

