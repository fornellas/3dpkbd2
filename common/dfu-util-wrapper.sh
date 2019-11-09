#!/bin/bash
LDSCRIPT="$1"
BIN="$2"
DFU_ADDRESS="$(gawk 'BEGIN{FS="[ ,]+"}/^ rom \(rx\) : ORIGIN = 0x[0-9A-Fa-f]+,/{print $7}' < "$LDSCRIPT")"

if [ -z "$DFU_ADDRESS" ]
then
	echo "Could not extract DFU_ADDRESS from $LDSCRIPT"
	exit 1
fi

DFU=(dfu-util --alt 0 --device 0483:df11 --dfuse-address $DFU_ADDRESS:leave --download "$BIN")

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