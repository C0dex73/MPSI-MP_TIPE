include ../../vars.mk

ifeq ($(OS),Windows_NT)
GLFW=glfw3
else
GLFW=glfw
endif

#required variables from main makefile
__FILES:=$(call format_lib,glfw3)
__DEPS:=
__IMPL_LINK:=$(call LINKLIB,$(call format_lib,$(GLFW)))

ifneq ($(OS),Windows_NT)
.PHONY: $(__FILES)
endif

.SECONDARY: $(__FILES)

ifeq ($(OS),Windows_NT)
../../$(BINDIR)/glfw3.dll  ../../$(BINDIR)/glfw3.dll.a:
	touch $@

$(__FILES): ../../$(BINDIR)/glfw3.dll  ../../$(BINDIR)/glfw3.dll.a
	@if [ -f ./glfw3.dll ] && [ -f ./glfw3dll.lib ]; then \
		echo "glfw binaries found." ; \
	elif [ "$(BITWISE_GLFW)" == "True" ]; then \
		echo "glfw binaries absent, downloading them from bitwise..." ; \
		wget https://codex.bitwise.re/glfw3.dll ;\
		wget https://codex.bitwise.re/glfw3dll.lib ;\
	else \
		echo "ERROR : glfw binaries not found, please download them at https://www.glfw.org/download.html and put them in $(pwd)."; \
		exit 1 ;\
	fi
	cp -p ./glfw3.dll ../../$(BINDIR)/glfw3.dll;
	cp -p ./glfw3dll.lib ../../$(BINDIR)/glfw3.dll.a;
	echo "copying binaries to $(BINDIR) ...";

else


$(__FILES):
	if ! [ $$($(CC) /dev/null $(__IMPL_LINK) 2>&1 | grep _start 1>/dev/null ||: echo $$?) -eq 0 ]; then \
		echo "$(__FILES) not installed, please check your package manager"; \
	else \
		echo "$(__FILES) are all checked"; \
	fi


endif
