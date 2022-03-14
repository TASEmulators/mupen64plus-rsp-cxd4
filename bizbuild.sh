#!/bin/sh
cd "$(dirname "$0")/projects/unix"
make all
if [ -e "../../../../BizHawk.sln" ]; then
	cp -v -t ../../../../Assets/dll mupen64plus-rsp-cxd4-sse2.dll
	if [ -e "../../../../output/dll" ]; then
		cp -v -t ../../../../output/dll mupen64plus-rsp-cxd4-sse2.dll
	fi
fi
