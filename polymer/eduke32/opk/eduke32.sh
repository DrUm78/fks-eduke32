#!/bin/sh
# Detect whether libvorbis libs are already there then copy them if not
if [ ! -e /usr/lib/libvorbis.so.0 ] || [ ! -e /usr/lib/libvorbisfile.so.3 ]; then
	rw
	cp libvorbis.so.0 /usr/lib
	cp libvorbisfile.so.3 /usr/lib
	ro
fi
# Copy the shareware in /mnt/FunKey/.eduke32 for Instant Play
if [ ! -e /mnt/FunKey/.eduke32/DUKE3D.GRP ]; then
	if [ ! -d /mnt/FunKey/.eduke32 ]; then
		mkdir -p /mnt/FunKey/.eduke32
	fi
	cp DUKE3D.GRP /mnt/FunKey/.eduke32
fi
# Launch EDuke32 and record pid for Instant Play
./eduke32 &
pid record $!
wait $!
pid erase
