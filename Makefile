TARGET = out

CFLAGS = -g -Werror
CC = clang $(CFLAGS)

ODIR = obj
_OBJ = main.o board.o search.o eval.o uci.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

all: main.o board.o search.o eval.o uci.o $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ)

main.o:
	$(CC) -c -o obj/main.o main.c

board.o:
	$(CC) -c -o obj/board.o board.c

uci.o:
	$(CC) -c -o obj/uci.o uci.c

search.o:
	$(CC) -c -o obj/search.o search.c

eval.o:
	$(CC) -c -o obj/eval.o eval.c
