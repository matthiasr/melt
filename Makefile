OBJDIR=objects

CPP=g++
CPPFILES=Melt.cpp StyleUtils.cpp
CPPFLAGS=-o $(OBJDIR)/Melt -O3 -fstrict-aliasing -Wall
LDFLAGS=-lbe -lstdc++

all:
	$(CPP) $(CPPFLAGS) $(CPPFILES) $(LDFLAGS)
