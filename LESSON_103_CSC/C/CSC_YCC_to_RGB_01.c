
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "CSC_global.h"
#include <arm_neon.h>

// private data
// New global variable for the switch case
// static int YCC_to_RGB_ROUTINE = 2; // Assuming 2 is the brute-force integer routine
// static int CHROMINANCE_UPSAMPLING_MODE = 1; // Default to the first case, as it's a simple duplicate
// Fixed point arithmetic constants. Assuming these are defined in CSC_global.h
// #define D1 298
// #define D2 409
// #define D3 208
// #define D4 100
// #define D5 516
// #define K 8

// private prototypes
// =======
static uint8_t saturation_float( float argument);
static void CSC_YCC_to_RGB_brute_force_float( int row, int col);
static void CSC_YCC_to_RGB_vector(int row, int col);

// =======
static uint8_t saturation_int( int argument);
static void CSC_YCC_to_RGB_brute_force_int( int row, int col);

// =======
static void chrominance_upsample(
    uint8_t C_pixel_00, uint8_t C_pixel_01,
    uint8_t C_pixel_10, uint8_t C_pixel_11,
    uint8_t *top, uint8_t *left, uint8_t *middle);
// =======
static void chrominance_array_upsample(int input_row, int input_col);



// private definitions
// =======
static uint8_t saturation_float( float argument) {
  if( argument > 255.0) { // saturation
    return( (uint8_t)255);
  }
  else if( argument < 0.0) {
    return( (uint8_t)0);
  }
  else {
    return( (uint8_t)argument);
  }
} // END of saturation_float()

// =======
static void CSC_YCC_to_RGB_brute_force_float( int row, int col) {
//
  float R_pixel_00, R_pixel_01, R_pixel_10, R_pixel_11;
  float G_pixel_00, G_pixel_01, G_pixel_10, G_pixel_11;
  float B_pixel_00, B_pixel_01, B_pixel_10, B_pixel_11;

  R_pixel_00 =   1.164*(Y[row+0][col+0] - 16.0)
               + 1.596*(Cr_temp[row+0][col+0] - 128.0);
  R[row+0][col+0] = saturation_float( R_pixel_00);
//
  R_pixel_01 =   1.164*(Y[row+0][col+1] - 16.0)
               + 1.596*(Cr_temp[row+0][col+1] - 128.0);
  R[row+0][col+1] = saturation_float( R_pixel_01);
//
  R_pixel_10 =   1.164*(Y[row+1][col+0] - 16.0)
               + 1.596*(Cr_temp[row+1][col+0] - 128.0);
  R[row+1][col+0] = saturation_float( R_pixel_10);
//
  R_pixel_11 =   1.164*(Y[row+1][col+1] - 16.0)
               + 1.596*(Cr_temp[row+1][col+1] - 128.0);
  R[row+1][col+1] = saturation_float( R_pixel_11);

  G_pixel_00 =   1.164*(Y[row+0][col+0] - 16.0)
               - 0.813*(Cr_temp[row+0][col+0] - 128.0)
               - 0.391*(Cb_temp[row+0][col+0] - 128.0);
  G[row+0][col+0] = saturation_float( G_pixel_00);
//
  G_pixel_01 =   1.164*(Y[row+0][col+1] - 16.0)
               - 0.813*(Cr_temp[row+0][col+1] - 128.0)
               - 0.391*(Cb_temp[row+0][col+1] - 128.0);
  G[row+0][col+1] = saturation_float( G_pixel_01);
//
  G_pixel_10 =   1.164*(Y[row+1][col+0] - 16.0)
               - 0.813*(Cr_temp[row+1][col+0] - 128.0)
               - 0.391*(Cb_temp[row+1][col+0] - 128.0);
  G[row+1][col+0] = saturation_float( G_pixel_10);
//
  G_pixel_11 =   1.164*(Y[row+1][col+1] - 16.0)
               - 0.813*(Cr_temp[row+1][col+1] - 128.0)
               - 0.391*(Cb_temp[row+1][col+1] - 128.0);
  G[row+1][col+1] = saturation_float( G_pixel_11);

  B_pixel_00 =   1.164*(Y[row+0][col+0] - 16.0)
               + 2.018*(Cb_temp[row+0][col+0] - 128.0);
  B[row+0][col+0] = saturation_float( B_pixel_00);
//
  B_pixel_01 =   1.164*(Y[row+0][col+1] - 16.0)
               + 2.018*(Cb_temp[row+0][col+1] - 128.0);
  B[row+0][col+1] = saturation_float( B_pixel_01);
//
  B_pixel_10 =   1.164*(Y[row+1][col+0] - 16.0)
               + 2.018*(Cb_temp[row+1][col+0] - 128.0);
  B[row+1][col+0] = saturation_float( B_pixel_10);
//
  B_pixel_11 =   1.164*(Y[row+1][col+1] - 16.0)
               + 2.018*(Cb_temp[row+1][col+1] - 128.0);
  B[row+1][col+1] = saturation_float( B_pixel_11);
} // END of CSC_YCC_to_RGB_brute_force_float()

// =======
static uint8_t saturation_int( int argument) {
  if( argument > 255) { // saturation
    return( (uint8_t)255);
  }
  else if( argument < 0) {
    return( (uint8_t)0);
  }
  else {
    return( (uint8_t)argument);
  }
} // END of saturation_int()

// static uint8_t saturation_int(int argument) {
//   if (argument > 65280) { // 255 << 8
//     return (uint8_t)255;
//   } else if (argument < 0) {
//     return (uint8_t)0;
//   } else {
//     return (uint8_t)(argument >> 8); // Right shift instead of division by 256
//   }
// }

// =======
static void CSC_YCC_to_RGB_brute_force_int( int row, int col) {
//
  int R_pixel_00, R_pixel_01, R_pixel_10, R_pixel_11;
  int G_pixel_00, G_pixel_01, G_pixel_10, G_pixel_11;
  int B_pixel_00, B_pixel_01, B_pixel_10, B_pixel_11;

  int  Y_pixel_00,  Y_pixel_01,  Y_pixel_10,  Y_pixel_11;
  int Cb_pixel_00, Cb_pixel_01, Cb_pixel_10, Cb_pixel_11;
  int Cr_pixel_00, Cr_pixel_01, Cr_pixel_10, Cr_pixel_11;

  Y_pixel_00 = (int)Y[row+0][col+0];
  Y_pixel_01 = (int)Y[row+0][col+1];
  Y_pixel_10 = (int)Y[row+1][col+0];
  Y_pixel_11 = (int)Y[row+1][col+1];

  Cb_pixel_00 = (int)Cb_temp[row+0][col+0];
  Cb_pixel_01 = (int)Cb_temp[row+0][col+1];
  Cb_pixel_10 = (int)Cb_temp[row+1][col+0];
  Cb_pixel_11 = (int)Cb_temp[row+1][col+1];

  Cr_pixel_00 = (int)Cr_temp[row+0][col+0];
  Cr_pixel_01 = (int)Cr_temp[row+0][col+1];
  Cr_pixel_10 = (int)Cr_temp[row+1][col+0];
  Cr_pixel_11 = (int)Cr_temp[row+1][col+1];

  Y_pixel_00 = Y_pixel_00 - 16;
  Y_pixel_01 = Y_pixel_01 - 16;
  Y_pixel_10 = Y_pixel_10 - 16;
  Y_pixel_11 = Y_pixel_11 - 16;

  Cb_pixel_00 = Cb_pixel_00 - 128;
  Cb_pixel_01 = Cb_pixel_01 - 128;
  Cb_pixel_10 = Cb_pixel_10 - 128;
  Cb_pixel_11 = Cb_pixel_11 - 128;

  Cr_pixel_00 = Cr_pixel_00 - 128;
  Cr_pixel_01 = Cr_pixel_01 - 128;
  Cr_pixel_10 = Cr_pixel_10 - 128;
  Cr_pixel_11 = Cr_pixel_11 - 128;

  R_pixel_00 = D1 * Y_pixel_00 + D2 * Cr_pixel_00;
  R_pixel_00 += (1 << (K-1)); // rounding
  R_pixel_00 = R_pixel_00 >> K;

  R_pixel_01 = D1 * Y_pixel_01 + D2 * Cr_pixel_01;
  R_pixel_01 += (1 << (K-1)); // rounding
  R_pixel_01 = R_pixel_01 >> K;

  R_pixel_10 = D1 * Y_pixel_10 + D2 * Cr_pixel_10;
  R_pixel_10 += (1 << (K-1)); // rounding
  R_pixel_10 = R_pixel_10 >> K;

  R_pixel_11 = D1 * Y_pixel_11 + D2 * Cr_pixel_11;
  R_pixel_11 += (1 << (K-1)); // rounding
  R_pixel_11 = R_pixel_11 >> K;


  //Handle overflow with saturating arithmetic
  // R[row+0][col+0] = saturation_int(R_pixel_00);
  // R[row+0][col+1] = saturation_int(R_pixel_01);
  // R[row+1][col+0] = saturation_int(R_pixel_10);
  // R[row+1][col+1] = saturation_int(R_pixel_11);
  R[row+0][col+0] = (uint8_t)R_pixel_00;
  R[row+0][col+1] = (uint8_t)R_pixel_01;
  R[row+1][col+0] = (uint8_t)R_pixel_10;
  R[row+1][col+1] = (uint8_t)R_pixel_11;

  G_pixel_00 = D1 * Y_pixel_00 - D3 * Cr_pixel_00
                             - D4 * Cb_pixel_00;
  G_pixel_00 += (1 << (K-1)); // rounding
  G_pixel_00 = G_pixel_00 >> K;

  G_pixel_01 = D1 * Y_pixel_01 - D3 * Cr_pixel_01
                             - D4 * Cb_pixel_01;
  G_pixel_01 += (1 << (K-1)); // rounding
  G_pixel_01 = G_pixel_01 >> K;

  G_pixel_10 = D1 * Y_pixel_10 - D3 * Cr_pixel_10
                             - D4 * Cb_pixel_10;
  G_pixel_10 += (1 << (K-1)); // rounding
  G_pixel_10 = G_pixel_10 >> K;

  G_pixel_11 = D1 * Y_pixel_11 - D3 * Cr_pixel_11
                             - D4 * Cb_pixel_11;
  G_pixel_11 += (1 << (K-1)); // rounding
  G_pixel_11 = G_pixel_11 >> K;

  // G[row+0][col+0] = saturation_int(G_pixel_00);
  // G[row+0][col+1] = saturation_int(G_pixel_01);
  // G[row+1][col+0] = saturation_int(G_pixel_10);
  // G[row+1][col+1] = saturation_int(G_pixel_11);
  G[row+0][col+0] = (uint8_t)G_pixel_00;
  G[row+0][col+1] = (uint8_t)G_pixel_01;
  G[row+1][col+0] = (uint8_t)G_pixel_10;
  G[row+1][col+1] = (uint8_t)G_pixel_11;

  B_pixel_00 = D1 * Y_pixel_00 + D5 * Cb_pixel_00;
  B_pixel_00 += (1 << (K-1)); // rounding
  B_pixel_00 = B_pixel_00 >> K;

  B_pixel_01 = D1 * Y_pixel_01 + D5 * Cb_pixel_01;
  B_pixel_01 += (1 << (K-1)); // rounding
  B_pixel_01 = B_pixel_01 >> K;

  B_pixel_10 = D1 * Y_pixel_10 + D5 * Cb_pixel_10;
  B_pixel_10 += (1 << (K-1)); // rounding
  B_pixel_10 = B_pixel_10 >> K;

  B_pixel_11 = D1 * Y_pixel_11 + D5 * Cb_pixel_11;
  B_pixel_11 += (1 << (K-1)); // rounding
  B_pixel_11 = B_pixel_11 >> K;

  // B[row+0][col+0] = saturation_int(B_pixel_00);
  // B[row+0][col+1] = saturation_int(B_pixel_01);
  // B[row+1][col+0] = saturation_int(B_pixel_10);
  // B[row+1][col+1] = saturation_int(B_pixel_11);
  B[row+0][col+0] = (uint8_t)B_pixel_00;
  B[row+0][col+1] = (uint8_t)B_pixel_01;
  B[row+1][col+0] = (uint8_t)B_pixel_10;
  B[row+1][col+1] = (uint8_t)B_pixel_11;

} // END of CSC_YCC_to_RGB_brute_force_int()

static void CSC_YCC_to_RGB_vector(int row, int col) {

  // We only need Y for conversion back to RGB, Cb and Cr are already in temp arrays
  // 0, 1
  // 2, 3
  int16_t Y_data[4] = {
    (int16_t)Y[row][col],
    (int16_t)Y[row][col + 1],
    (int16_t)Y[row + 1][col],
    (int16_t)Y[row + 1][col + 1]
  };

  // Load single Cb and Cr values for the 2x2 block from subsampled arrays
  // 0, 1
  // 2, 3
  int16_t Cb_val = (int16_t)Cb[row >> 1][col >> 1];
  int16_t Cr_val = (int16_t)Cr[row >> 1][col >> 1];

  // Cb and Cr values to 4-lane NEON vectors,
  // so we can process all 4 Y pixels in parallel with the same Cb/Cr
  int16x4_t Cb_vec = vdup_n_s16(Cb_val);
  int16x4_t Cr_vec = vdup_n_s16(Cr_val);

  // Create Y vector, we already have the 4 pixels loaded
  int16x4_t Y_vec = vld1_s16(Y_data);

  // Prevent overflow, expanding 16 bit vector to 32
  int32x4_t Y32 = vmovl_s16(Y_vec);   
  int32x4_t Cb32 = vmovl_s16(Cb_vec); 
  int32x4_t Cr32 = vmovl_s16(Cr_vec); 

  //Y_pixel = Y_pixel - 16
  int32x4_t Y_pix = vsubq_s32(Y32, vdupq_n_s32(16));
  //Cb = Cb - 128
  int32x4_t Cb_pix = vsubq_s32(Cb32, vdupq_n_s32(128));
  //Cr = Cr - 128
  int32x4_t Cr_pix = vsubq_s32(Cr32, vdupq_n_s32(128));

  // Calculate R_pixel
  // Y = Y * D1
  Y_pix = vmulq_n_s32(Y_pix, D1);
  // R = Y + Cr * D2
  int32x4_t R_pix = vmlaq_n_s32(Y_pix, Cr_pix, D2);

  // Calculate G_pixel
  // G = Y - D3 * Cr - D4 * Cb
  // We already have D1 * Y from before so no need to calculate again
  int32x4_t G_pix = vmlsq_n_s32(Y_pix, Cb_pix, D3); // Sub D3*Cb
  G_pix = vmlsq_n_s32(G_pix, Cr_pix, D4);           // Sub D4*Cr

  // Calculate B_pixel
  // B = Y + D5*Cb
  // Y is already calculated with D1 scaling from before
  int32x4_t B_pix = vmlaq_n_s32(Y_pix, Cb_pix, D5); // Add D5 * Cb

  // Add rounding offset before shifting right by 6 (because constants scaled by 2^6)
  const int shift = 6;
  int32x4_t round = vdupq_n_s32(1 << (shift - 1));
  R_pix = vaddq_s32(R_pix, round);
  G_pix = vaddq_s32(G_pix, round);
  B_pix = vaddq_s32(B_pix, round);

  // Right shift results by 6 bits to scale back to 8-bit range
  R_pix = vshrq_n_s32(R_pix, shift);
  G_pix = vshrq_n_s32(G_pix, shift);
  B_pix = vshrq_n_s32(B_pix, shift);

  // Change 32 bit to 16 bit vectors for conversion
  int16x4_t R16 = vmovn_s32(R_pix);
  int16x4_t G16 = vmovn_s32(G_pix);
  int16x4_t B16 = vmovn_s32(B_pix);

  // Load pixels into corresponding RGB array
  // 0, 1
  // 2, 3

  // Clamp values to 0..255 (unsigned 16-bit)
  uint16x4_t R_u16 = vmin_u16(vmax_u16(vreinterpret_u16_s16(R16), vdup_n_u16(0)), vdup_n_u16(255));
  uint16x4_t G_u16 = vmin_u16(vmax_u16(vreinterpret_u16_s16(G16), vdup_n_u16(0)), vdup_n_u16(255));
  uint16x4_t B_u16 = vmin_u16(vmax_u16(vreinterpret_u16_s16(B16), vdup_n_u16(0)), vdup_n_u16(255));

  // Store the results back into the global R, G, B arrays,
  // maintaining the 2x2 block layout:
  R[row + 0][col + 0] = (uint8_t)vget_lane_u16(R_u16, 0);
  R[row + 0][col + 1] = (uint8_t)vget_lane_u16(R_u16, 1);
  R[row + 1][col + 0] = (uint8_t)vget_lane_u16(R_u16, 2);
  R[row + 1][col + 1] = (uint8_t)vget_lane_u16(R_u16, 3);

  G[row + 0][col + 0] = (uint8_t)vget_lane_u16(G_u16, 0);
  G[row + 0][col + 1] = (uint8_t)vget_lane_u16(G_u16, 1);
  G[row + 1][col + 0] = (uint8_t)vget_lane_u16(G_u16, 2);
  G[row + 1][col + 1] = (uint8_t)vget_lane_u16(G_u16, 3);

  B[row + 0][col + 0] = (uint8_t)vget_lane_u16(B_u16, 0);
  B[row + 0][col + 1] = (uint8_t)vget_lane_u16(B_u16, 1);
  B[row + 1][col + 0] = (uint8_t)vget_lane_u16(B_u16, 2);
  B[row + 1][col + 1] = (uint8_t)vget_lane_u16(B_u16, 3);
}

// =======
static void chrominance_upsample(
    uint8_t C_pixel_00, uint8_t C_pixel_01,
    uint8_t C_pixel_10, uint8_t C_pixel_11,
    uint8_t *top, uint8_t *left, uint8_t *middle) {

  int temp_top;
  int temp_left;
  int temp_middle;

  switch (CHROMINANCE_UPSAMPLING_MODE) {
    case 0:
      *top = 0;
      *left = 0;
      *middle = 0;
      break;
    case 1:
      *top = (uint8_t)C_pixel_00;
      *left = (uint8_t)C_pixel_00;
      *middle = (uint8_t)C_pixel_00;
      break;
    case 2:
      temp_top = (int)C_pixel_00 + (int)C_pixel_01;
      temp_top += (1 << 0); // rounding
      *top = (uint8_t)(temp_top >> 1);
//
      temp_left = (int)C_pixel_00 + (int)C_pixel_10;
      temp_left += (1 << 0); // rounding
      *left = (uint8_t)(temp_left >> 1);
//
      temp_middle = (int)C_pixel_00 + (int)C_pixel_01 +
                    (int)C_pixel_10 + (int)C_pixel_11;
      temp_middle += (1 << 1); // rounding
      *middle = (uint8_t)(temp_middle >> 2);
      break;
    default:
      break;
  }
} // END of chrominance_upsample()

// =======
static void chrominance_array_upsample(int input_row, int input_col) {
  int row, col;

  uint8_t top;
  uint8_t left;
  uint8_t middle;

  for( row=0; row<((input_row>>1)-1); row+=1) {
    for( col=0; col<((input_col>>1)-1); col+=1) {
      chrominance_upsample( Cb[row+0][col+0], Cb[row+0][col+1],
                            Cb[row+1][col+0], Cb[row+1][col+1],
                            &top, &left, &middle);
      Cb_temp[(row<<1)+0][(col<<1)+0] = Cb[row+0][col+0];
      Cb_temp[(row<<1)+0][(col<<1)+1] = top;
      Cb_temp[(row<<1)+1][(col<<1)+0] = left;
      Cb_temp[(row<<1)+1][(col<<1)+1] = middle;
      //
      chrominance_upsample( Cr[row+0][col+0], Cr[row+0][col+1],
                            Cr[row+1][col+0], Cr[row+1][col+1],
                            &top, &left, &middle);
      Cr_temp[(row<<1)+0][(col<<1)+0] = Cr[row+0][col+0];
      Cr_temp[(row<<1)+0][(col<<1)+1] = top;
      Cr_temp[(row<<1)+1][(col<<1)+0] = left;
      Cr_temp[(row<<1)+1][(col<<1)+1] = middle;
    }
  }

  col = (input_col>>1) - 1;
  for( row=0; row<((input_row>>1)-1); row+=1) {
    chrominance_upsample( Cb[row+0][col], Cb[row+0][col],
                          Cb[row+1][col], Cb[row+1][col],
                          &top, &left, &middle);
    Cb_temp[(row<<1)+0][(col<<1)+0] = Cb[row+0][col];
    Cb_temp[(row<<1)+0][(col<<1)+1] = top;
    Cb_temp[(row<<1)+1][(col<<1)+0] = left;
    Cb_temp[(row<<1)+1][(col<<1)+1] = middle;
    //
    chrominance_upsample( Cr[row+0][col], Cr[row+0][col],
                          Cr[row+1][col], Cr[row+1][col],
                          &top, &left, &middle);
    Cr_temp[(row<<1)+0][(col<<1)+0] = Cr[row+0][col];
    Cr_temp[(row<<1)+0][(col<<1)+1] = top;
    Cr_temp[(row<<1)+1][(col<<1)+0] = left;
    Cr_temp[(row<<1)+1][(col<<1)+1] = middle;
  }

  row = (input_row>>1) - 1;
  for( col=0; col<((input_col>>1)-1); col+=1) {
    chrominance_upsample( Cb[row][col+0], Cb[row][col+1],
                          Cb[row][col+0], Cb[row][col+1],
                          &top, &left, &middle);
    Cb_temp[(row<<1)+0][(col<<1)+0] = Cb[row][col+0];
    Cb_temp[(row<<1)+0][(col<<1)+1] = top;
    Cb_temp[(row<<1)+1][(col<<1)+0] = left;
    Cb_temp[(row<<1)+1][(col<<1)+1] = middle;
    //
    chrominance_upsample( Cr[row][col+0], Cr[row][col+1],
                          Cr[row][col+0], Cr[row][col+1],
                          &top, &left, &middle);
    Cr_temp[(row<<1)+0][(col<<1)+0] = Cr[row][col+0];
    Cr_temp[(row<<1)+0][(col<<1)+1] = top;
    Cr_temp[(row<<1)+1][(col<<1)+0] = left;
    Cr_temp[(row<<1)+1][(col<<1)+1] = middle;
  }

  row = (input_row>>1) - 1;
  col = (input_col>>1) - 1;
  Cb_temp[(row<<1)+0][(col<<1)+0] = Cb[row][col];
  Cb_temp[(row<<1)+0][(col<<1)+1] = Cb[row][col];
  Cb_temp[(row<<1)+1][(col<<1)+0] = Cb[row][col];
  Cb_temp[(row<<1)+1][(col<<1)+1] = Cb[row][col];
  //
  Cr_temp[(row<<1)+0][(col<<1)+0] = Cr[row][col];
  Cr_temp[(row<<1)+0][(col<<1)+1] = Cr[row][col];
  Cr_temp[(row<<1)+1][(col<<1)+0] = Cr[row][col];
  Cr_temp[(row<<1)+1][(col<<1)+1] = Cr[row][col];

} // END of chrominance_array_upsample()

// =======
void CSC_YCC_to_RGB(int input_row, int input_col) {
  int row, col; // indices for row and column
//
  chrominance_array_upsample(input_row, input_col);

  for(row = 0; row < input_row; row+=2) {
    for(col = 0; col < input_col; col += 2) {
      switch (YCC_to_RGB_ROUTINE) {
        case 0:
          break;
        case 1:
          CSC_YCC_to_RGB_brute_force_float( row, col);
          break;
        case 2:
          CSC_YCC_to_RGB_brute_force_int( row, col);
          break;
        case 3:
          CSC_YCC_to_RGB_vector(row, col);
        default:
          break;
      }
    }
  }
} // END of CSC_YCC_to_RGB()

