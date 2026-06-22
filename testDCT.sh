#!/bin/bash
set -e

IMAGE="tests/images/jpg/benchmark/baboon.jpg"

QUALITY_CSV="DCT_quality_sweep.csv"
PAYLOAD_CSV="DCT_payload_sweep.csv"

TEMP_SECRET="secret.bin"
TEMP_DECODED="secret_decoded.bin"
TEMP_IMAGE="out.jpg"

# quality sweep
echo "Running quality sweep"
echo "quality,capacity,psnr,ssim,result" > "$QUALITY_CSV"

for QUALITY in 1 10 20 30 40 50 60 70 80 90 100
do
    CAPACITY=$(./NIR -A DCT -i "$IMAGE" --capacity --jpeg-quality "$QUALITY")
    head -c "$CAPACITY" /dev/urandom > "$TEMP_SECRET"
    echo "Quality = $QUALITY"

    ./NIR -E -A DCT -i "$IMAGE" -s "$TEMP_SECRET" -o "$TEMP_IMAGE" --jpeg-quality "$QUALITY"
    ./NIR -D -A DCT -i "$TEMP_IMAGE" -o "$TEMP_DECODED" --jpeg-quality "$QUALITY"

    if diff "$TEMP_SECRET" "$TEMP_DECODED" > /dev/null
    then
        RESULT="OK"
    else
        RESULT="FAIL"
    fi

    PSNR=$(./NIR -C -A PSNR -i "$IMAGE" -i "$TEMP_IMAGE")
    SSIM=$(./NIR -C -A SSIM -i "$IMAGE" -i "$TEMP_IMAGE")

    echo "$QUALITY,$CAPACITY,$PSNR,$SSIM,$RESULT" \
        >> "$QUALITY_CSV"
done

# payload sweep

QUALITY=90
CAPACITY=$(./NIR -A DCT -i "$IMAGE" --capacity --jpeg-quality "$QUALITY")

echo "Running payload sweep (quality=$QUALITY)"

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
        -A DCT \
        -i "$IMAGE" \
        -s "$TEMP_SECRET" \
        -o "$TEMP_IMAGE" \
        --jpeg-quality "$QUALITY"

    ./NIR -D \
        -A DCT \
        -i "$TEMP_IMAGE" \
        -o "$TEMP_DECODED" \
        --jpeg-quality "$QUALITY"

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
echo "$QUALITY_CSV"
echo "$PAYLOAD_CSV"