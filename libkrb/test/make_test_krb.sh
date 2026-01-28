#!/bin/sh
# Create a minimal valid KRB file for testing

OUTPUT_FILE=${1:-test_simple.krb}

# Create a temporary file
TMP_FILE=$(mktemp)

# Write KRB file header (little-endian)
# Magic: "KRYN" = 0x4B52594E
printf "\x4E\x59\x52\x4B" > "$TMP_FILE"

# Version: 1.0
printf "\x01\x00\x00\x00" >> "$TMP_FILE"

# Flags, reserved: 0
printf "\x00\x00\x00\x00" >> "$TMP_FILE"

# Section counts (all zero for minimal file)
printf "\x00\x00\x00\x00" >> "$TMP_FILE"  # style_count
printf "\x00\x00\x00\x00" >> "$TMP_FILE"  # theme_count
printf "\x00\x00\x00\x00" >> "$TMP_FILE"  # widget_def_count
printf "\x00\x00\x00\x00" >> "$TMP_FILE"  # widget_instance_count
printf "\x00\x00\x00\x00" >> "$TMP_FILE"  # property_count
printf "\x00\x00\x00\x00" >> "$TMP_FILE"  # event_count
printf "\x00\x00\x00\x00" >> "$TMP_FILE"  # script_count

# String table offset (starts at byte 128)
printf "\x80\x00\x00\x00" >> "$TMP_FILE"
printf "\x00\x00\x00\x00" >> "$TMP_FILE"
printf "\x00\x00\x00\x00" >> "$TMP_FILE"
printf "\x00\x00\x00\x00" >> "$TMP_FILE"
printf "\x00\x00\x00\x00" >> "$TMP_FILE"
printf "\x00\x00\x00\x00" >> "$TMP_FILE"

# String table size
STR_SIZE=256
printf "$(printf '%02x' $((STR_SIZE & 0xFF)))\x00\x00\x00" | xxd -r -p >> "$TMP_FILE"

# Other section sizes (all zero)
for i in $(seq 1 6); do
    printf "\x00\x00\x00\x00" >> "$TMP_FILE"
done

# Reserved
for i in $(seq 1 8); do
    printf "\x00\x00\x00\x00" >> "$TMP_FILE"
done

# Pad to offset 128
dd if=/dev/zero bs=1 count=$((128 - $(stat -c%s "$TMP_FILE"))) 2>/dev/null >> "$TMP_FILE"

# String table
printf "\x02\x00\x00\x00" >> "$TMP_FILE"  # count = 2
printf "Text\0" >> "$TMP_FILE"            # String 0 at offset 4
printf "Column\0" >> "$TMP_FILE"          # String 1 at offset 9

# Pad to string table size
dd if=/dev/zero bs=1 count=$((256 - 14)) 2>/dev/null >> "$TMP_FILE"

# Move to output
mv "$TMP_FILE" "$OUTPUT_FILE"

echo "Created test KRB file: $OUTPUT_FILE"
ls -l "$OUTPUT_FILE"
