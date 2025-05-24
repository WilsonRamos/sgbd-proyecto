# Makefile simple para debugging
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude

all:
	mkdir -p build bin
	$(CXX) $(CXXFLAGS) -c src/timer.cpp -o build/timer.o
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o build/main.o
	$(CXX) build/timer.o build/main.o -o bin/sgbd

run: all
	./bin/sgbd

clean:
	rm -rf build bin

debug:
	@echo "Verificando archivos..."
	@ls -la include/
	@ls -la src/
	@echo "Compilando con verbose..."
	mkdir -p build bin
	$(CXX) $(CXXFLAGS) -v -c src/timer.cpp -o build/timer.o
	$(CXX) $(CXXFLAGS) -v -c src/main.cpp -o build/main.o
	$(CXX) -v build/timer.o build/main.o -o bin/sgbd

.PHONY: all run clean debug