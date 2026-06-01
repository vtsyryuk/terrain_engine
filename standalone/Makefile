GCC ?= gcc
CROSS_GCC ?= x86_64-w64-mingw32-g++
GCCFLAGS ?= -x c++ -std=c++17 -Wall -Wextra -pedantic
LDFLAGS ?= -lstdc++

SRC := gauss_with_clusters.cpp
BUILD_DIR := build-windows
APP := $(BUILD_DIR)/gauss_with_clusters

ifeq ($(OS),Windows_NT)
APP := $(BUILD_DIR)/gauss_with_clusters.exe
MKDIR := if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
RM := del /Q
NULL := nul
else
MKDIR := mkdir -p "$(BUILD_DIR)"
RM := rm -f
NULL := /dev/null
endif

.PHONY: all gcc windows-cross run run-seminar1 run-seminar2 run-seminar3 server client clients-parallel clean

all: gcc

gcc: $(APP)

windows-cross:
	mkdir -p "$(BUILD_DIR)"
	$(CROSS_GCC) -std=c++17 -Wall -Wextra -pedantic "$(SRC)" -static -static-libgcc -static-libstdc++ -o "$(BUILD_DIR)/gauss_with_clusters.exe"

$(APP): $(SRC)
	$(MKDIR)
	$(GCC) $(GCCFLAGS) "$(SRC)" $(LDFLAGS) -o "$(APP)"

run: gcc
	"$(APP)"

run-seminar1: gcc
	"$(APP)" seminar1_commands.txt --config seminar_config.txt

run-seminar2: gcc
	"$(APP)" seminar2_commands.txt --config seminar_config.txt

run-seminar3: gcc
	"$(APP)" seminar3_commands.txt --config seminar_config.txt

server: gcc
	"$(APP)" --server --config seminar_config.txt

client: gcc
	"$(APP)" --client seminar1_commands.txt --config seminar_config.txt --shutdown

clients-parallel: gcc
ifeq ($(OS),Windows_NT)
	run_parallel_clients.bat
else
	@echo "clients-parallel uses Windows Named Pipes and can run only on Windows."
	@echo "On Windows run: mingw32-make clients-parallel"
endif

clean:
	$(RM) "$(APP)" 2>$(NULL)
