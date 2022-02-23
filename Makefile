
LINK.o = $(LINK.cc)
CXXFLAGS = -std=c++17 -Wall

all: correctness persistence

correctness: skiplist.o kvstore.o correctness.o

persistence: skiplist.o kvstore.o persistence.o

clean:
	-rm -f correctness persistence *.o
