include vars.mk

.PHONY: debug all build reset clean cr rc $(OFILES)

#~ RECIPES
debug:
	@echo "No debug script in ./Makefile"

test:
	@echo "No test script in ./Makefile"

all: $(OFILES) $(LIBS) $(EXECS)

build: rc
	$(MAKE) all
	$(MAKE) clean
	cp -r ./$(BINDIR)/* $(BUILDDIR) ||:
	$(IMPLIBCLEANUP)
	cp -r ./$(ASSETDIR)/* $(BUILDDIR) ||:
	rm -r ./$(BINDIR)/* ||:

reset:
	rm -r ./$(BINDIR)/* 2>/dev/null ||:
	rm -r ./$(BUILDDIR)/* 2>/dev/null ||:

clean:
	@echo "cleaning up library and executable nodes..."
	@rm -f $(foreach node,$(ENODES) $(LNODES),$(node)/*.$(DEPEXT) $(node)/*.$(PPFEXT) $(node)/*.$(ASMEXT) $(node)/*.$(OBJEXT))
	
	@for node in $(ONODES) ; do \
		if [ $$($(MAKE) -qC $$node clean 2>/dev/null 1>&2 ||: && echo $$?) -eq 1 ]; then \
			echo "cleaning up $$node..."; \
			$(MAKE) -C $$node clean ; \
		fi \
	done

cr: rc
rc: reset clean

#make the files in the bin folder
$(EXECS) $(LIBS): %: $(BINDIR)/%

#implibs are created at the same time as their corresponding dynamic library
$(call format_lib,%).$(SLFEXT): $(call format_lib,%)

#^ executables linking
$(foreach exe,$(EXECS),$(BINDIR)/$(exe)): $$(call libexec-to-obj,$$@,%$(DOTEXE))
	@echo "building $@ dependencies : $(EDEP)"
	$(if $(EDEP),$(MAKE) $(EDEP))
	@echo "linking $^ along $(EDEP) into $@"
	$(CC) -o $@ $^ $(call link-deps,$(EDEP)) $(call NFLAGS, $(LINKING_STEP_FLAG_ID)) $(LFLAGS)

#^ libraries linking
$(foreach lib,$(LIBS),$(BINDIR)/$(lib)): $$(call libexec-to-obj,$$@,$(call format_lib,%))
	@echo "building $@ dependencies : $(LDEP)"
	$(if $(LDEP),$(MAKE) $(LDEP),@echo no dep to build)
	@echo "linking $^ along $(LDEP) into $@"
	$(CC) -shared -o $@ $^ $(call link-deps,$(LDEP)) $(call NFLAGS, $(LINKING_STEP_FLAG_ID)) $(LFLAGS) $(CREATEIMPLIB)
 

#^ object files assembling
%.$(OBJEXT): %.$(ASMEXT)
	@echo "Assembling $< into $@"
	$(CC) -c -o $@ $< $(call NFLAGS, $(ASSEMBLING_STEP_FLAG_ID)) $(ASMFLAGS)

#assembly files compiling
%.$(ASMEXT): %.$(PPFEXT)
	@echo "Compiling $< into $@"
	$(CC) -S -o $@ $< $(call NFLAGS, $(COMPILING_STEP_FLAG_ID)) $(CMPFLAGS)

#preprocessed file preprocessing :)
%.$(PPFEXT): %.$(DEPEXT) $$(call getppfdep,$$@) # getppfdep usefull when dep file already exists to determine wether to remake it or not
	@echo "deps : $^ $(shell cat $<)"
	@echo "Pre-processing $(shell cat $<) into $@"
	$(CC) -E -o $@ $(firstword $(shell cat $<)) $(call NFLAGS, $(PREPROCESSING_STEP_FLAG_ID)) $(PPFLAGS) -D$(shell echo '$(notdir $(patsubst %/,%,$(dir $*)))_EXPORT' | tr '[:lower:]' '[:upper:]')

#dependency file creation
%.$(DEPEXT): %.$(SRCEXT)
	@echo "Fetching dependencies for $<"
	$(CC) -MM $< $(call NFLAGS, $(DEPENDENCY_STEP_FLAG_ID)) $(PPFLAGS) | sed -z 's/ \\\n//g' | cut -f 2 -d ':' | cut -c2- > $@

#catch-all rule for source and header files
%.$(SRCEXT) %.$(HDREXT): ;

#Ofiles :
$(OFILES): %: $$(shell $(MAKE) -C$$(dir $$@) -qp 2> /dev/null | grep -w "__DEPS:=" | cut -c8-)
	@if [ $$($(MAKE) -C$(dir $@) $$(basename $@) -n 2>/dev/null 1>&2 ||: && echo $$?) -eq 0 ]; then \
		echo "building $@ externally" ; \
		$(MAKE) -C$(dir $@) $$(basename $@); \
	else \
		echo "Error : no rule found for $$(basename $@) in $(dir $@)" ; \
		false ; \
	fi



#~ OTHERS

#node creation
__nl_%:
	@./$(SCRIPTDIR)/createnode.sh -L ./$(SRCDIR)/$(patsubst __nl_%,%,$@)

__ne_%:
	@./$(SCRIPTDIR)/createnode.sh -E ./$(SRCDIR)/$(patsubst __ne_%,%,$@)

__no_%:
	@./$(SCRIPTDIR)/createnode.sh -O ./$(SRCDIR)/$(patsubst __no_%,%,$@)

#*checks if all required scripts are present
ifeq (,$(wildcard $(SCRIPTDIR)/getnodes.sh))
$(error Required shell script not found in script directory ($(SCRIPTDIR)) : getnodes.sh)
endif
ifeq (,$(wildcard $(SCRIPTDIR)/createnode.sh))
$(error Required shell script not found in script directory ($(SCRIPTDIR)) : createnode.sh)
endif
