./main -E -A DCT -i tests/images/jpg/lena_2048.jpg -s tests/secret -o xxx2048.jpg
./main -D -A DCT -i xxx2048.jpg -o secret_decoded
diff tests/secret secret_decoded
./main -C -A PSNR -i tests/images/jpg/lena_2048.jpg -i xxx2048.jpg
./main -C -A SSIM -i tests/images/jpg/lena_2048.jpg -i xxx2048.jpg