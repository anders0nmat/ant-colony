
all:
	clang++ -Wall -Wpedantic -Werror --debug -O1 -I include/ -L lib/ src/*.cpp -lglfw3 -lGL -lX11 -o ./main

release:
	clang++ -Wall -Wpedantic -Werror -O3 -I include/ -L lib/ src/*.cpp -lglfw3 -lGL -lX11 -o ./main

