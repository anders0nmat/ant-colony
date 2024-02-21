# Usage
# >> make <platform>
# or
# >> make <platform>-release
#
# Supported platforms: linux, mac


CXX_COMPILER := clang++
CXX_VERSION := c++17
CXX_WARNINGS := -Wall -Wpedantic -Werror

LOCATION_LIBRARIES := lib/
LOCATION_INCLUDES := include/
LOCATION_CPP := src/*.cpp src/colonies/*.cpp
LOCATION_OUTPUT := ./main

MAC_FLAGS := -lglfw3-mac -framework Cocoa -framework OpenGL -framework IOKit
LINUX_FLAGS := -lglfw3-linux -lGL -lX11

DEBUG = -O1 --debug
RELEASE = -O3

# Used for execution, do not touch
FLAGS =
OPTS =


.PHONY: linux
linux: linux-debug

.PHONY: linux-debug
linux-debug: FLAGS = $(LINUX_FLAGS)
linux-debug: debug

.PHONY: linux-release
linux-release: FLAGS = $(LINUX_FLAGS)
linux-release: release

.PHONY: mac
mac: mac-debug

.PHONY: mac-debug
mac-debug: FLAGS = $(MAC_FLAGS)
mac-debug: debug

.PHONY: mac-release
mac-release: FLAGS = $(MAC_FLAGS)
mac-release: release



.PHONY: debug
debug: OPTS = $(DEBUG)
debug: executable

.PHONY: release
release: OPTS = $(RELEASE)
release: executable


.PHONY: executable
executable:
	$(CXX_COMPILER) $(LOCATION_CPP) -o $(LOCATION_OUTPUT) -std=$(CXX_VERSION) $(CXX_WARNINGS) -L $(LOCATION_LIBRARIES) -I $(LOCATION_INCLUDES) $(OPTS) $(FLAGS)




#all:
#	clang++ -Wall -Wpedantic -Werror --debug -O1 -I include/ -L lib/ src/*.cpp src/colonies/*.cpp -lglfw3-linux -lGL -lX11 -o ./main

#mac:
#	clang++ -std=c++17 -Wall -Wpedantic -Werror --debug -O1 -I include/ -L lib/ src/*.cpp src/colonies/*.cpp -lglfw3-mac -framework Cocoa -framework OpenGL -framework IOKit -o ./main

#release:
#	clang++ -Wall -Wpedantic -Werror -O3 -I include/ -L lib/ src/*.cpp src/colonies/*.cpp -lglfw3-linux -lGL -lX11 -o ./main

