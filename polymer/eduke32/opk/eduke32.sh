#!/bin/sh
# Detect whether libvorbis libs are already there then copy them if not
if [ ! -e /usr/lib/libvorbis.so.0 ] || [ ! -e /usr/lib/libvorbisfile.so.3 ]; then
	rw
	cp -f libvorbis.so.0 /usr/lib
	cp -f libvorbisfile.so.3 /usr/lib
	ro
	./eduke32 &
	pid record $!
	wait $!
	pid erase
else
	./eduke32 &
	pid record $!
	wait $!
	pid erase
fi
