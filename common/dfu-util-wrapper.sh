#!/bin/bash
DFU=(dfu-util "$@")

OUTPUT="$("${DFU[@]}" 2>&1)"
EXIT=$?

if [ $EXIT -ne 0 ]
then
	if echo "$OUTPUT" | grep -Eq "^File downloaded successfully$"
	then
		exit 0
	else
		echo "${DFU[@]}"
		echo "$OUTPUT"
		exit $EXIT
	fi
fi