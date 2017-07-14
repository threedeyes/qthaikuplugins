CC=g++
SIGNATURE="application/x-vnd.QtSystrayManager"

all: qsystray

qsystray: qsystray.o
		mkdir -p ../../plugins/tools/
		$(CC) -g qsystray.o -ltracker -lroot -lbe -oqsystray
		rc qsystray.rdef
		xres -o ./qsystray qsystray.rsrc
		mimeset -f ./qsystray

qsystray.o:
		$(CC) -g -oqsystray.o -DAPP_SIGNATURE="\"$(SIGNATURE)\"" -c qsystray.cpp

clean:
		rm -f ./qsystray.o
		rm -f ./qsystray.rsrc
		rm -f ./qsystray
