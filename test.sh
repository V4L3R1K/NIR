./main -E -A DCT -i tests/images/jpg/lena_2048.jpg -s tests/secrets/secret_1m -o xxx2048.jpg --jpeg-quality 100
./main -D -A DCT -i xxx2048.jpg -o secret_decoded
diff ./tests/secrets/secret_1m secret_decoded
./main -C -A PSNR -i tests/images/jpg/lena_2048.jpg -i xxx2048.jpg
./main -C -A SSIM -i tests/images/jpg/lena_2048.jpg -i xxx2048.jpg