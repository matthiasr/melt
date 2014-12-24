CC= $(CXX)
CPPFLAGS+= -O3 -fstrict-aliasing -g
LDLIBS+= -lbe -ltracker

all: Melt

Melt: Melt.o StyleUtils.o

clean:
	rm -f *.o Melt

.PHONY: clean
