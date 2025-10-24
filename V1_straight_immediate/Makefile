CXX=c++
CXXFLAGS=-std=c++17 -O2 -pthread
BIN=run
all: $(BIN)
$(BIN): main.cpp
	$(CXX) $(CXXFLAGS) -o $(BIN) main.cpp
run: all
	./$(BIN)
clean:
	rm -f $(BIN)
