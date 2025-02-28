#include <barrier.h>
#include <defs.h>
#include <mram.h>
#include <stdio.h>

#include "dpu-jpeg.h"
#include "jpeg-host.h"

__host dpu_inputs_t input;
__host dpu_output_t output;

JpegInfo jpegInfo;
JpegInfoDpu jpegInfoDpu;

BARRIER_INIT(init_barrier, NR_TASKLETS);
BARRIER_INIT(idct_barrier, NR_TASKLETS);
BARRIER_INIT(crop_barrier, NR_TASKLETS);
BARRIER_INIT(prep0_barrier, NR_TASKLETS);
BARRIER_INIT(prep1_barrier, NR_TASKLETS);
BARRIER_INIT(prep2_barrier, NR_TASKLETS);

#define DEBUG 0

#if DEBUG
static void print_jpeg_decompressor() {
  printf("\n********** DQT **********\n");
  for (int i = 0; i < 4; i++) {
    if (jpegInfo.quant_tables[i].exists) {
      printf("Table ID: %d", i);
      for (int j = 0; j < 64; j++) {
        if (j % 8 == 0) {
          printf("\n");
        }
        printf("%d ", jpegInfo.quant_tables[i].table[j]);
      }
      printf("\n\n");
    }
  }

  printf("********** DRI **********\n");
  printf("Restart Interval: %d\n", jpegInfo.restart_interval);

  printf("\n********** SOF **********\n");
  printf("Width: %d\n", jpegInfo.image_width);
  printf("Height: %d\n", jpegInfo.image_height);
  printf("Number of color components: %d\n\n", jpegInfo.num_color_components);
  for (int i = 0; i < jpegInfo.num_color_components; i++) {
    printf("Component ID: %d\n", jpegInfo.color_components[i].component_id);
    printf("H-samp factor: %d\n", jpegInfo.color_components[i].h_samp_factor);
    printf("V-samp factor: %d\n", jpegInfo.color_components[i].v_samp_factor);
    printf("Quantization table ID: %d\n\n", jpegInfo.color_components[i].quant_table_id);
  }

  printf("\n********** DHT **********\n");
  for (int i = 0; i < MAX_HUFFMAN_TABLES; i++) {
    if (jpegInfo.dc_huffman_tables[i].exists) {
      printf("DC Table ID: %d\n", i);
      for (int j = 0; j < 16; j++) {
        printf("%d: ", j + 1);
        for (int k = jpegInfo.dc_huffman_tables[i].valoffset[j]; k < jpegInfo.dc_huffman_tables[i].valoffset[j + 1];
             k++) {
          printf("%d ", jpegInfo.dc_huffman_tables[i].huffval[k]);
        }
        printf("\n");
      }
      printf("\n");
    }
  }
  for (int i = 0; i < MAX_HUFFMAN_TABLES; i++) {
    if (jpegInfo.ac_huffman_tables[i].exists) {
      printf("AC Table ID: %d\n", i);
      for (int j = 0; j < 16; j++) {
        printf("%d: ", j + 1);
        for (int k = jpegInfo.ac_huffman_tables[i].valoffset[j]; k < jpegInfo.ac_huffman_tables[i].valoffset[j + 1];
             k++) {
          printf("%d ", jpegInfo.ac_huffman_tables[i].huffval[k]);
        }
        printf("\n");
      }
      printf("\n");
    }
  }

  printf("\n********** SOS **********\n");
  for (int i = 0; i < jpegInfo.num_color_components; i++) {
    printf("Component ID: %d\n", jpegInfo.color_components[i].component_id);
    printf("DC table ID: %d\n", jpegInfo.color_components[i].dc_huffman_table_id);
    printf("AC table ID: %d\n\n", jpegInfo.color_components[i].ac_huffman_table_id);
  }
  printf("Start of selection: %d\n", jpegInfo.ss);
  printf("End of selection: %d\n", jpegInfo.se);
  printf("Successive approximation high: %d\n", jpegInfo.Ah);
  printf("Successive approximation low: %d\n\n", jpegInfo.Al);

  printf("\n********** BMP **********\n");
  printf("MCU width: %d\n", jpegInfo.mcu_width);
  printf("MCU height: %d\n", jpegInfo.mcu_height);
  printf("BMP padding: %d\n", jpegInfo.padding);
}
#endif

static void init_jpeg_info() {
  jpegInfo.valid = 1;

  for (int i = 0; i < 4; i++) {
    jpegInfo.quant_tables[i].exists = 0;

    if (i < 2) {
      jpegInfo.color_components[i].exists = 0;
      jpegInfo.dc_huffman_tables[i].exists = 0;
      jpegInfo.ac_huffman_tables[i].exists = 0;

    } else if (i < 3) {
      jpegInfo.color_components[i].exists = 0;
    }
  }

  jpegInfo.restart_interval = 0;
  jpegInfo.image_height = 0;
  jpegInfo.image_width = 0;
  jpegInfo.num_color_components = 0;
  jpegInfo.ss = 0;
  jpegInfo.se = 0;
  jpegInfo.Ah = 0;
  jpegInfo.Al = 0;

  jpegInfo.mcu_width = 0;
  jpegInfo.mcu_height = 0;
  jpegInfo.padding = 0;

  for (int i = 0; i < NR_TASKLETS; i++) {
    jpegInfoDpu.mcu_end_index[i] = 0;
    jpegInfoDpu.mcu_start_index[i] = 0;
    if (i < NR_TASKLETS - 1) {
      jpegInfoDpu.dc_offset[i][0] = 0;
      jpegInfoDpu.dc_offset[i][1] = 0;
      jpegInfoDpu.dc_offset[i][2] = 0;
    }
  }

  for (int i = 0; i < 3; i++) {
    jpegInfoDpu.sum_rgb[i] = 0;
  }
}

static int read_all_markers(JpegDecompressor *d) {
  int result = 1;

  init_file_reader_index(d);
  init_jpeg_info();

  int not_jpeg = check_start_of_image(d);
  if (not_jpeg) {
    return 1;
  }

  // Continuously read all markers until we reach Huffman coded bitstream
  while (jpegInfo.valid && result) {
    result = read_next_marker(d);
  }

  if (!jpegInfo.valid) {
    return 1;
  }

  jpegInfo.image_data_start = d->file_index + d->cache_index;
  jpegInfo.size_per_tasklet = (jpegInfo.length - jpegInfo.image_data_start + (NR_TASKLETS - 1)) / NR_TASKLETS;

  output.image_width = jpegInfo.image_width;
  output.image_height = jpegInfo.image_height;
  output.padding = jpegInfo.padding;
  output.mcu_width_real = jpegInfo.mcu_width_real;

#if DEBUG
  print_jpeg_decompressor();
#endif

  return 0;
}

static int round_down_to_nearest_multiple(int to_align, int multiple) {
  while (multiple < to_align && (multiple << 1) <= to_align) {
    multiple <<= 1;
  }
  return multiple;
}

static void crop_and_scale(JpegDecompressor *d) {
  uint16_t aligned_width = round_down_to_nearest_multiple(jpegInfo.image_width, input.scale_width);
  uint16_t aligned_height = round_down_to_nearest_multiple(jpegInfo.image_height, input.scale_width);

  uint16_t new_width = aligned_height < aligned_width ? aligned_height : aligned_width;

  int start_x = (jpegInfo.image_width - new_width) >> 1;
  int start_y = (jpegInfo.image_height - new_width) >> 1;
  start_x = ALIGN(start_x, 8);
  start_y = ALIGN(start_y, 8);

  crop(d, start_x, start_y, new_width, new_width);

  int scale_factor = new_width / input.scale_width;
  if (scale_factor != 1) {
    jpeg_scale(d, scale_factor, scale_factor);
  }

  output.image_width = jpegInfo.image_width;
  output.image_height = jpegInfo.image_height;
  output.mcu_width_real = jpegInfo.mcu_width_real;
}

int main() {
  JpegDecompressor decompressor;
  decompressor.length = input.file_length;
  decompressor.tasklet_id = me();
  jpegInfo.length = decompressor.length;

  if (decompressor.tasklet_id == 0) {
    int error = read_all_markers(&decompressor);
    if (error) {
      return error;
    }
  }

  // All tasklets should wait until tasklet 0 has finished reading all JPEG markers
  barrier_wait(&init_barrier);

  init_jpeg_decompressor(&decompressor);

  // Process Huffman coded bitstream, perform inverse DCT, and convert YCbCr to RGB
  decode_bitstream(&decompressor);
  if (!jpegInfo.valid) {
    return 1;
  }

  // All tasklets should wait until tasklet 0 has finished adjusting the DC coefficients
  barrier_wait(&idct_barrier);
  inverse_dct_convert(&decompressor);

  barrier_wait(&crop_barrier);
  /*  if (decompressor.tasklet_id == 0) {
      crop_and_scale(&decompressor);
    }

    barrier_wait(&prep0_barrier);
    if (input.horizontal_flip) {
      horizontal_flip(&decompressor);
    }
  */
  barrier_wait(&prep1_barrier);
  find_sum_rgb(&decompressor);

  barrier_wait(&prep2_barrier);

  for (int color_index = 0; color_index < jpegInfo.num_color_components; color_index++) {
    output.sum_rgb[color_index] = jpegInfoDpu.sum_rgb[color_index];
  }

  return 0;
}
