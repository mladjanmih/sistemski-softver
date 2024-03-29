
IDIR=h
OBJDIR=obj/assembler
SRCDIR=src
ASMDIR=assembler
CC=g++
CFLAGS=-I$(IDIR)
ARCH=-m32 -std=c++11 -static
PROGRAM=asembler


SRC = $(wildcard $(SRCDIR)/*.cpp)
SRC1 = $(wildcard $(ASMDIR)/*.cpp)
OBJ = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRC))
OBJ += $(patsubst $(ASMDIR)/%.cpp,$(OBJDIR)/%.o,$(SRC1))


$(PROGRAM): $(OBJ)
	$(CC) -g -o $@ $^ $(ARCH)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) -g -o $@ -c $< $(CFLAGS) $(ARCH)

$(OBJDIR)/%.o: $(ASMDIR)/%.cpp
	$(CC) -g -o $@ -c $< $(CFLAGS) $(ARCH)

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(PROGRAM)
 
.PHONY: clean