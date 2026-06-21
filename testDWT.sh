#!/bin/bash
set -e

IMAGE="tests/images/jpg/lena_2048.jpg"

PAYLOAD_CSV="DWT_payload_sweep.csv"

TEMP_SECRET="secret.bin"
TEMP_DECODED="secret_decoded.bin"
TEMP_IMAGE="out.jpg"

# payload sweep

CAPACITY=$(NIR -A DWT -i "$IMAGE" --capacity)

echo "Running payload sweep"

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
        -A DWT \
        -i "$IMAGE" \
        -s "$TEMP_SECRET" \
        -o "$TEMP_IMAGE" 

    NIR -D \
        -A DWT \
        -i "$TEMP_IMAGE" \
        -o "$TEMP_DECODED"

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
echo "$PAYLOAD_CSV"