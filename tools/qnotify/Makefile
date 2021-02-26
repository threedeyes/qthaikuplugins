CC=g++
SIGNATURE="application/x-vnd.qnotify-gate"

all: clean qnotify

qnotify: qnotify.o
		$(CC) -g qnotify.o -oqnotify -lbe
		rc qnotify.rdef
		xres -o ./qnotify qnotify.rsrc
		mimeset -all ./qnotify

qnotify.o:
		$(CC) -g -oqnotify.o -DAPP_SIGNATURE="\"$(SIGNATURE)\"" -c qnotify.cpp

clean:
		rm -f ./qnotify.o
		rm -f ./qnotify.rsrc
		rm -f ./qnotify
