# ArcadeHub - Makefile
# Follows the exact structure of epsilon-sample-app-cpp

TOOLCHAIN ?= /usr

CC  = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
CXX = $(TOOLCHAIN)/bin/arm-none-eabi-g++
LD  = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
OC  = $(TOOLCHAIN)/bin/arm-none-eabi-objcopy

EADK_ARCHIVE ?= sdk/eadk.a
EADK_INCLUDE ?= sdk/include

CPPFLAGS = -I$(EADK_INCLUDE) -Isrc
CXXFLAGS = -std=c++17 -O2 -Wall \
            -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard \
            -ffunction-sections -fdata-sections \
            -fno-exceptions -fno-rtti \
            -nostdinc++

LDFLAGS = -Wl,--gc-sections \
          -Wl,-T,sdk/app.ld \
          -L. \
          $(EADK_ARCHIVE) \
          -lm

SRCS = \
  src/main.cpp \
  src/save.cpp \
  src/menu.cpp \
  src/tetris.cpp \
  src/sudoku.cpp \
  src/watersort.cpp \
  src/memory.cpp \
  src/blockblast.cpp \
  src/fruitmerge.cpp \
  src/hangman.cpp \
  src/checkers.cpp \
  src/snake.cpp \
  src/game2048.cpp \
  src/minesweeper.cpp

OBJS = $(addprefix output/,$(SRCS:.cpp=.o))

.PHONY: all clean

all: output/arcade_hub.nwa

output/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

output/arcade_hub.elf: $(OBJS)
	$(LD) $^ $(LDFLAGS) -o $@

output/arcade_hub.nwa: output/arcade_hub.elf
	$(OC) -O binary $< $@

clean:
	rm -rf output
