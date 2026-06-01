GCC ?= gcc
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

.PHONY: all gcc run run-field1 run-seminar2 run-seminar3 server client clean

all: gcc

gcc: $(APP)

$(APP): $(SRC)
	$(MKDIR)
	$(GCC) $(GCCFLAGS) "$(SRC)" $(LDFLAGS) -o "$(APP)"

run: gcc
	"$(APP)"

run-field1: gcc
	"$(APP)" field1_commands.txt --config seminar_config.txt

run-seminar2: gcc
	"$(APP)" seminar2_commands.txt --config seminar_config.txt

run-seminar3: gcc
	"$(APP)" seminar3_commands.txt --config seminar_config.txt

server: gcc
	"$(APP)" --server --config seminar_config.txt

client: gcc
	"$(APP)" --client field1_commands.txt --config seminar_config.txt --shutdown

clean:
	$(RM) "$(APP)" 2>$(NULL)
