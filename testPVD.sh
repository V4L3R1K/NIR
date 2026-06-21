#!/bin/bash
set -e

IMAGE="tests/images/png/lena_2048.png"

COUNT_CSV="PVD_count_sweep.csv"
PAYLOAD_CSV="PVD_payload_sweep.csv"

TEMP_SECRET="secret.bin"
TEMP_DECODED="secret_decoded.bin"
TEMP_IMAGE="out.png"

# quality sweep
echo "Running PVD count sweep"

echo "pvd_count,capacity,psnr,ssim,result" > "$COUNT_CSV"

for PVD_COUNT in 1 2 3 4 5 6 7
do
    CAPACITY=$(NIR \
        -A PVD \
        -i "$IMAGE" \
        --capacity \
        --pvd-count "$PVD_COUNT")

    head -c "$CAPACITY" /dev/urandom > "$TEMP_SECRET"

    echo "PVD count = $PVD_COUNT"

    NIR -E \
        -A PVD \
        -i "$IMAGE" \
        -s "$TEMP_SECRET" \
        -o "$TEMP_IMAGE" \
        --pvd-count "$PVD_COUNT"

    NIR -D \
        -A PVD \
        -i "$TEMP_IMAGE" \
        -o "$TEMP_DECODED" \
        --pvd-count "$PVD_COUNT"

    if diff "$TEMP_SECRET" "$TEMP_DECODED" > /dev/null
    then
        RESULT="OK"
    else
        RESULT="FAIL"
    fi

    PSNR=$(NIR -C -A PSNR -i "$IMAGE" -i "$TEMP_IMAGE")
    SSIM=$(NIR -C -A SSIM -i "$IMAGE" -i "$TEMP_IMAGE")

    echo "$PVD_COUNT,$CAPACITY,$PSNR,$SSIM,$RESULT" \
        >> "$COUNT_CSV"
done

# payload sweep

PVD_COUNT=1

CAPACITY=$(NIR \
    -A PVD \
    -i "$IMAGE" \
    --capacity \
    --pvd-count "$PVD_COUNT")

echo "Running payload sweep (pvd-count=$PVD_COUNT)"

echo "payload_bytes,payload_percent,psnr,ssim,result" \
    > "$PAYLOAD_CSV"

for PERCENT in 1 10 20 30 40 50 60 70 80 90 100
do
    PAYLOAD=$((CAPACITY * PERCENT / 100))

    if [ "$PAYLOAD" -eq 0 ]
    then
        PAYLOAD=1
    fi

    echo "Payload $PERCENT% ($PAYLOAD bytes)"

    head -c "$PAYLOAD" /dev/urandom > "$TEMP_SECRET"

    NIR -E \
        -A PVD \
        -i "$IMAGE" \
        -s "$TEMP_SECRET" \
        -o "$TEMP_IMAGE" \
        --pvd-count "$PVD_COUNT"

    NIR -D \
        -A PVD \
        -i "$TEMP_IMAGE" \
        -o "$TEMP_DECODED" \
        --pvd-count "$PVD_COUNT"

    if diff "$TEMP_SECRET" "$TEMP_DECODED" > /dev/null
    then
        RESULT="OK"
    else
        RESULT="FAIL"
    fi

    PSNR=$(NIR -C -A PSNR -i "$IMAGE" -i "$TEMP_IMAGE")
    SSIM=$(NIR -C -A SSIM -i "$IMAGE" -i "$TEMP_IMAGE")

    echo "$PAYLOAD,$PERCENT,$PSNR,$SSIM,$RESULT" \
        >> "$PAYLOAD_CSV"
done

# clear
rm -f \
    "$TEMP_SECRET" \
    "$TEMP_DECODED" \
    "$TEMP_IMAGE"

# done
echo "Done."
echo "$COUNT_CSV"
echo "$PAYLOAD_CSV"