OBJDIR=objects

CPP=g++
CPPFILES=Melt.cpp StyleUnits.cpp
CPPFLAGS=-o $(OBJDIR)/Melt -O3 -fstrict_aliasing
LDFLAGS=-lbe -lstdc++

all:
	$(CPP) $(CPPFLAGS) $(CPPFILES) (LDFLAGS)
