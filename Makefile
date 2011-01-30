include ./Rules.make

APP = saabopentech

all: $(APP)

$(APP): main.c	
	$(CC) -o $(APP) main.c $(CFLAGS)	
	
clean:
	rm -f *.o ; rm $(APP)
