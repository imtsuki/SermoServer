all: server

CXX := g++
CXXFLAGS := -std=c++17 -Iinclude -g
LDFLAGS := -lg3logger -lsqlite3 -lconfig++ -lpthread

SRCS := $(wildcard src/*.cpp)
OBJS := $(patsubst %.cpp, build/%.o, $(SRCS))

server: $(OBJS)
	$(CXX) $(OBJS) -o build/server $(CXXFLAGS) $(LDFLAGS)

build/src/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

countline:
	@echo "Total code lines: `ls include/*.h src/*.cpp | xargs cat | wc -l`"

clean:
	rm -rf build
