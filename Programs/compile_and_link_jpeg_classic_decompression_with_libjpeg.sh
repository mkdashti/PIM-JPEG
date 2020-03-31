echo "Compiling jpeg_classic_decompression_with_libjpeg program as classic"
gcc -o classic jpeg_classic_decompression_with_libjpeg.c -L../libjpeg-turbo/build -ljpeg -g

