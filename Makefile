# $Id$

CC	= gcc
CFLAGS	= -O2 -Wall
CURLLIBS= $(shell curl-config --libs)

all:
	$(CC) -pipe -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -g3 -Dmodversion -D_REENTRANT -D_GNU_SOURCE -O6 -fomit-frame-pointer -Wno-missing-prototypes -Wno-missing-declarations -DCRYPTO -fPIC -DDEBUG -c app_voipcid.c -o app_voipcid.o
	$(CC) -shared -Xlinker -x -o app_voipcid.so app_voipcid.o $(CURLLIBS)

install:
	sudo install app_voipcid.so /usr/lib/asterisk/modules/

install_config:
	sudo install voipcid.conf /etc/asterisk

clean:
	rm -f *.o app_voipcid.so

# DO NOT DELETE THIS LINE -- make depend depends on it.
