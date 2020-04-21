echo "Compiling dpu_downscale.c as dpu_downscale"
dpu-upmem-dpurte-clang -o dpu_downscale dpu_downscale.c -L./libjpeg-turbo/build -ljpeg -g

