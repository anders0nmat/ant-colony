
all:
	clang++ -Wall -Wpedantic -Werror --debug -O1 -I include/ -L lib/ src/*.cpp src/colonies/*.cpp -lglfw3-linux -lGL -lX11 -o ./main

mac:
	clang++ -std=c++17 -Wall -Wpedantic -Werror --debug -O1 -I include/ -L lib/ src/*.cpp src/colonies/*.cpp -lglfw3-mac -framework Cocoa -framework OpenGL -framework IOKit -o ./main

release:
	clang++ -Wall -Wpedantic -Werror -O3 -I include/ -L lib/ src/*.cpp src/colonies/*.cpp -lglfw3-linux -lGL -lX11 -o ./main

