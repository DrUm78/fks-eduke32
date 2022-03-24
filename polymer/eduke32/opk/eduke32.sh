#!/bin/sh
# Detect whether libvorbis libs are already there then copy them if not
if [ ! -e /usr/lib/libvorbis.so.0 ] || [ ! -e /usr/lib/libvorbisfile.so.3 ]; then
	rw
	cp -f libvorbis.so.0 /usr/lib
	cp -f libvorbisfile.so.3 /usr/lib
	./eduke32 "$1" &
	pid record $!
	wait $!
	pid erase
	ro
else
	./eduke32 "$1" &
	pid record $!
	wait $!
	pid erase
fi
