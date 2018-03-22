BIN = ldif2csv
OBJ = $(BIN).o
DEST = doiow11:/u/fow0sim/prj/tools/misc/$(BIN)
CC = clang

DEF = -DLONG_OPTS=1
ifndef DEBUG
	DEBUG = 0
endif
ifeq ($(DEBUG), 1)
	CFLAGS = -g
else
	CFLAGS = -O2
endif

LDFLAGS =
LIBS =

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJ) $(LIBS)
$(OBJ): $(BIN).c $(BIN).h
	$(CC) $(CFLAGS) $(DEF) -o $(OBJ) -c $(BIN).c
clean:
	rm -f $(BIN) $(OBJ)
install:
	cp $(BIN) /usr/local/bin
xfer:
	scp $(BIN).c $(BIN).h $(DEST)
