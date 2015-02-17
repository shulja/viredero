CC=gcc
CFLAGS=-I. -Werror
LDFLAGS=-lX11 -lXdamage
OBJ = x-viredero.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

x-viredero: $(OBJ)
	gcc -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(OBJ) x-viredero
