#!/bin/bash
DFU=(dfu-util "$@")

OUTPUT="$("${DFU[@]}" 2>&1)"
EXIT=$?

if [ $EXIT -ne 0 ]
then
	echo "${DFU[@]}"
	echo "$OUTPUT"
	exit $EXIT
fi