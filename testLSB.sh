#!/bin/bash
set -e

IMAGE="tests/images/png/benchmark/lenna.png"

COUNT_CSV="LSB_count_sweep.csv"
PAYLOAD_CSV="LSB_payload_sweep.csv"

TEMP_SECRET="secret.bin"
TEMP_DECODED="secret_decoded.bin"
TEMP_IMAGE="out.png"

# quality sweep
echo "Running LSB count sweep"

echo "lsb_count,capacity,psnr,ssim,result" > "$COUNT_CSV"

for LSB_COUNT in 1 2 3 4 5 6 7 8
do
    CAPACITY=$(./NIR \
        -A LSB \
        -i "$IMAGE" \
        --capacity \
        --lsb-count "$LSB_COUNT")

    head -c "$CAPACITY" /dev/urandom > "$TEMP_SECRET"

    echo "LSB count = $LSB_COUNT"

    ./NIR -E \
        -A LSB \
        -i "$IMAGE" \
        -s "$TEMP_SECRET" \
        -o "$TEMP_IMAGE" \
        --lsb-count "$LSB_COUNT"

    ./NIR -D \
        -A LSB \
        -i "$TEMP_IMAGE" \
        -o "$TEMP_DECODED" \
        --lsb-count "$LSB_COUNT"

    if diff "$TEMP_SECRET" "$TEMP_DECODED" > /dev/null
    then
        RESULT="OK"
    else
        RESULT="FAIL"
    fi

    PSNR=$(./NIR -C -A PSNR -i "$IMAGE" -i "$TEMP_IMAGE")
    SSIM=$(./NIR -C -A SSIM -i "$IMAGE" -i "$TEMP_IMAGE")

    echo "$LSB_COUNT,$CAPACITY,$PSNR,$SSIM,$RESULT" \
        >> "$COUNT_CSV"
done

# payload sweep

LSB_COUNT=1

CAPACITY=$(./NIR \
    -A LSB \
    -i "$IMAGE" \
    --capacity \
    --lsb-count "$LSB_COUNT")

echo "Running payload sweep (lsb-count=$LSB_COUNT)"

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

    ./NIR -E \
        -A LSB \
        -i "$IMAGE" \
        -s "$TEMP_SECRET" \
        -o "$TEMP_IMAGE" \
        --lsb-count "$LSB_COUNT"

    ./NIR -D \
        -A LSB \
        -i "$TEMP_IMAGE" \
        -o "$TEMP_DECODED" \
        --lsb-count "$LSB_COUNT"

    if diff "$TEMP_SECRET" "$TEMP_DECODED" > /dev/null
    then
        RESULT="OK"
    else
        RESULT="FAIL"
    fi

    PSNR=$(./NIR -C -A PSNR -i "$IMAGE" -i "$TEMP_IMAGE")
    SSIM=$(./NIR -C -A SSIM -i "$IMAGE" -i "$TEMP_IMAGE")

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