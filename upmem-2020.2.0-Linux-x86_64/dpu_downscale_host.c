#include <stdio.h>
#include <assert.h>
#include <alloc.h>
#include <dpu.h>
#include "./libjpeg-turbo/jpeglib.h"
#include <dpu_log.h>

#ifndef DPU_BINARY
#define DPU_BINARY "./dpu_downscale"
#endif

int image_height = 700; /* Number of rows in image */
int image_width = 465;  /* Number of columns in image */
int num_of_lines_to_skip = 1;

int main(void)
{
  struct dpu_set_t set, dpu;
  FILE *infile;
  if ((infile = fopen("../wall-murals-lale.jpg", "rb")) == NULL)
  {
    fprintf(stderr, "can't open the input file");
  }
  JSAMPLE image_buffer = mem_alloc(sizeof(char) * 3 * image_width * image_height / (num_of_lines_to_skip + 1)); /* Points to large array of R,G,B-order data */

  DPU_ASSERT(dpu_alloc(1, NULL, &set));
  DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));
  DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));

  DPU_FOREACH(set, dpu)
  {
    DPU_ASSERT(dpu_log_read(dpu, stdout));
  }

  DPU_ASSERT(dpu_free(set));

  return 0;
}
