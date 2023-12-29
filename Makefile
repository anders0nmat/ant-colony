
all:
	clang++ -Wall -Wpedantic -Werror --debug -I include/ -L lib/ src/*.cpp -lglfw3 -lGL -lX11 -o ./main
