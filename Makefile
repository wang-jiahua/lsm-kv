
LINK.o = $(LINK.cc)
CXXFLAGS = -std=c++17 -Wall

all: correctness persistence

correctness: skiplist.o index.o disk.o kvstore.o correctness.o

persistence: skiplist.o index.o disk.o kvstore.o persistence.o

clean:
	-rm -rf data/ correctness persistence *.o
