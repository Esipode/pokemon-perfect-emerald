#!/usr/bin/env bash
# RAM usage report: buckets EWRAM/IWRAM by symbol and by object file, and prints the
# corrected IWRAM headroom (accounting for the system stack, which the linker's
# --print-memory-usage banner does not). See "Free Space" plan, Stage 1.
#
# Usage: dev_scripts/ram_report.sh [pokeemerald.elf] [pokeemerald.map]
# Env:   EABI_PREFIX (default arm-none-eabi-), NM, READELF, TOP (default 25)

set -euo pipefail

ELF="${1:-pokeemerald.elf}"
MAP="${2:-pokeemerald.map}"
PREFIX="${EABI_PREFIX:-arm-none-eabi-}"
NM="${NM:-${PREFIX}nm}"
READELF="${READELF:-${PREFIX}readelf}"
TOP="${TOP:-25}"

# GBA memory map (fixed by hardware).
EWRAM_START=$((0x02000000)); EWRAM_END=$((0x02040000)); EWRAM_CAP=262144
IWRAM_START=$((0x03000000)); IWRAM_END=$((0x03008000)); IWRAM_CAP=32768

# System stack top, from src/crt0.s (sp_sys: .word IWRAM_END - 0x1c0). Update this
# if crt0.s changes the offset.
SP_SYS=$((IWRAM_END - 0x1c0))

[ -f "$ELF" ] || { echo "error: $ELF not found. Build the ROM first (builds are run by the user, not this script)." >&2; exit 1; }
[ -f "$MAP" ] || { echo "error: $MAP not found." >&2; exit 1; }
command -v "$NM" >/dev/null 2>&1 || { echo "error: $NM not found; set NM= or EABI_PREFIX=" >&2; exit 1; }
command -v "$READELF" >/dev/null 2>&1 || { echo "error: $READELF not found; set READELF= or EABI_PREFIX=" >&2; exit 1; }

echo "== RAM report: $ELF =="
echo

# --- Section ground truth (exact addr/size, not estimated) ---
sec_line() {
    "$READELF" -SW "$ELF" \
        | sed -E 's/^\s*\[[ 0-9]+\]\s*//' \
        | awk -v want="$1" '$1==want{print $3, $5; found=1} END{if(!found) print "0 0"}'
}

read -r EWRAM_ADDR EWRAM_SIZE   <<<"$(sec_line .ewram)"
read -r EWSBSS_ADDR EWSBSS_SIZE <<<"$(sec_line .ewram.sbss)"
read -r IWRAM_ADDR IWRAM_SIZE   <<<"$(sec_line .iwram)"
read -r IWBSS_ADDR IWBSS_SIZE   <<<"$(sec_line .iwram.bss)"

hex2dec() { echo $((16#$1)); }

ewram_used=$(( $(hex2dec "$EWRAM_SIZE") + $(hex2dec "$EWSBSS_SIZE") ))
iwram_used=$(( $(hex2dec "$IWRAM_SIZE") + $(hex2dec "$IWBSS_SIZE") ))
iwram_bss_end=$(( $(hex2dec "$IWBSS_ADDR") + $(hex2dec "$IWBSS_SIZE") ))
corrected_iwram_free=$(( SP_SYS - iwram_bss_end ))

pct() { awk -v u="$1" -v c="$2" 'BEGIN{printf "%.2f%%", (u/c)*100}'; }

printf "%-8s %10s %10s %8s %s\n" "Region" "Used" "Capacity" "Banner%" "Note"
printf "%-8s %10d %10d %8s %s\n" "EWRAM" "$ewram_used" "$EWRAM_CAP" "$(pct "$ewram_used" "$EWRAM_CAP")" "$((EWRAM_CAP - ewram_used)) bytes free"
printf "%-8s %10d %10d %8s %s\n" "IWRAM" "$iwram_used" "$IWRAM_CAP" "$(pct "$iwram_used" "$IWRAM_CAP")" "banner overstates free space, see below"
echo
echo "Corrected IWRAM headroom (accounts for the system stack, src/crt0.s):"
printf "  static .bss ends 0x%08x, sp_sys = 0x%08x -> %d bytes free\n" "$iwram_bss_end" "$SP_SYS" "$corrected_iwram_free"
echo

# --- Per-symbol tables (nm, decimal radix so bucketing is plain arithmetic) ---
symbol_table() {
    local region_start=$1 region_end=$2 label=$3
    echo "-- Top $TOP $label symbols --"
    printf "%12s  %-10s  %s\n" "Bytes" "Address" "Symbol"
    "$NM" --print-size --size-sort --radix=d "$ELF" 2>/dev/null \
        | awk -v lo="$region_start" -v hi="$region_end" \
            '$1 ~ /^[0-9]+$/ && $2 ~ /^[0-9]+$/ && NF>=4 { addr=$1+0; if (addr>=lo && addr<hi) print }' \
        | sort -k2 -n -r \
        | head -n "$TOP" \
        | awk '{ addr=$1+0; size=$2+0; $1=""; $2=""; $3=""; sub(/^ +/,""); printf "%12d  0x%08x  %s\n", size, addr, $0 }'
    echo
}

symbol_table "$EWRAM_START" "$EWRAM_END" "EWRAM"
symbol_table "$IWRAM_START" "$IWRAM_END" "IWRAM"

# --- Per-object-file tables (parsed from the .map memory layout, per §0.5:
# EWRAM_DATA/IWRAM_DATA carry an explicit section attribute, so gc-sections
# can only reclaim RAM at the granularity of a whole object file's section) ---
object_table() {
    local region_start=$1 region_end=$2 label=$3 measured=$4
    echo "-- Top $TOP $label object files --"
    printf "%12s  %s\n" "Bytes" "Object"
    local rows attributed remaining
    rows=$(awk -v lo="$region_start" -v hi="$region_end" '
        function hex2dec(h,    i, c, v, n) {
            sub(/^0[xX]/, "", h)
            v = 0
            n = length(h)
            for (i = 1; i <= n; i++) {
                c = tolower(substr(h, i, 1))
                v = v * 16 + (c ~ /[0-9]/ ? c + 0 : index("abcdef", c) + 9)
            }
            return v
        }
        NF==4 && ($1 ~ /^\./ || $1 == "common_data") && $2 ~ /^0x[0-9a-fA-F]+$/ && $3 ~ /^0x[0-9a-fA-F]+$/ {
            addr = hex2dec($2)
            size = hex2dec($3)
            if (addr >= lo && addr < hi)
                total[$4] += size
        }
        END {
            for (f in total) print total[f], f
        }
    ' "$MAP" | sort -n -r)
    echo "$rows" | head -n "$TOP" | awk '{ printf "%12d  %s\n", $1, $2 }'
    remaining=$(( $(echo "$rows" | wc -l) - TOP ))
    [ "$remaining" -gt 0 ] && echo "  ... $remaining more files"
    attributed=$(echo "$rows" | awk '{s+=$1} END{print s+0}')
    printf "  attributed to files: %d bytes; measured (readelf section size): %d bytes; %d bytes linker fill/alignment padding\n" \
        "$attributed" "$measured" "$((measured - attributed))"
    echo
}

object_table "$EWRAM_START" "$EWRAM_END" "EWRAM" "$ewram_used"
object_table "$IWRAM_START" "$IWRAM_END" "IWRAM" "$iwram_used"
