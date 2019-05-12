all: server

CXX := g++
CXXFLAGS := -std=c++17 -Iinclude -g
LDFLAGS := -lg3logger -lsqlite3 -lconfig++

server: main.o log.o Db.o Config.o Server.o GameLogic.o client
	$(CXX) main.o log.o Db.o Config.o Server.o GameLogic.o -o serverMain $(CXXFLAGS) $(LDFLAGS)

countline:
	@echo "Total code lines: `ls include/*.h *.cpp | xargs cat | wc -l`"

client: client.cpp
	clang++ client.cpp -o client -std=c++17
