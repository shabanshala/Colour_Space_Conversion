// Copyright 2023 Mihai SIMA (mihai.sima@ieee.org).  All rights reserved.
// Color Space Conversion (CSC) in fixed-point arithmetic
// RGB to YCC conversion

//#include <stdio.h>
#include <stdint.h>
#include "CSC_global.h"
#include <arm_neon.h>

// private data
extern uint8_t **R;
extern uint8_t **G;
extern uint8_t **B;
extern uint8_t **Y;
extern uint8_t **Cb;
extern uint8_t **Cr;
extern uint8_t **Cb_temp;
extern uint8_t **Cr_temp;
// private prototypes
// =======
static void csc_rgb_to_ycc_brute_force_float( int row, int col);
static void csc_rgb_to_ycc_unrolled_int( int row, int col);
static void csc_rgb_to_ycc_vector( int row, int col);
static void rgb_to_ycc_fixed_point_rounded(int row, int col);

// =======
static void csc_rgb_to_ycc_brute_force_int( int row, int col);
uint8_t saturation_int( int argument);

// =======
static uint8_t chrominance_downsample(
    uint8_t C_pixel_1, uint8_t C_pixel_2,
    uint8_t C_pixel_3, uint8_t C_pixel_4);

// private definitions

static inline uint8_t sat8(int x) {
    if (x < 0)   return 0;
    if (x > 255) return 255;
    return (uint8_t)x;
}

//convert 2 x 2 RGB to YCC using fixed-point arithmetic and rounding
static void rgb_to_ycc_fixed_point_rounded(int row, int col) {
  //temp holders for four Cb/Cr samples
    uint8_t Cb_pixel_00, Cb_pixel_01, Cb_pixel_10, Cb_pixel_11;
    uint8_t Cr_pixel_00, Cr_pixel_01, Cr_pixel_10, Cr_pixel_11;

    //pointer maps for selecting per-pixel slot
    uint8_t* Cb_ptrs[2][2] = {
        { &Cb_pixel_00, &Cb_pixel_01 },
        { &Cb_pixel_10, &Cb_pixel_11 }
    };
    uint8_t* Cr_ptrs[2][2] = {
        { &Cr_pixel_00, &Cr_pixel_01 },
        { &Cr_pixel_10, &Cr_pixel_11 }
    };

    //2 x 2 tile using offsets
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            int rr = row + r;
            int cc = col + c;

            //load 8-bit RGB and widen to int 
            int Rv = (int)R[rr][cc];
            int Gv = (int)G[rr][cc];
            int Bv = (int)B[rr][cc];

            //compute Y
            int Y_tmp  = (16 << K) + C11*Rv + C12*Gv + C13*Bv;
            Y_tmp     += (1 << (K - 1));  //rounding (add half)
            int Y_val  = Y_tmp >> K;      //scale back to 8-bit
            Y[rr][cc] = (uint8_t)Y_val;    

            //comput Cb
            int Cb_tmp = (128 << K) - C21*Rv - C22*Gv + C23*Bv;
            Cb_tmp    += (1 << (K - 1));   //rounding (add half)
            int Cb_val = Cb_tmp >> K;      //scale back to 8-bit

            //compute Cr
            int Cr_tmp = (128 << K) + C31*Rv - C32*Gv - C33*Bv;
            Cr_tmp    += (1 << (K - 1));   //rounding (add half)
            int Cr_val = Cr_tmp >> K;      //scale back to 8-bit

            uint8_t *Cb_slot = Cb_ptrs[r][c];
            uint8_t *Cr_slot = Cr_ptrs[r][c];

            //clamp 
            *Cb_slot = sat8(Cb_val);
            *Cr_slot = sat8(Cr_val);
        }
    }

    //downsampling
    Cb[row >> 1][col >> 1] = chrominance_downsample(Cb_pixel_00, Cb_pixel_01, Cb_pixel_10, Cb_pixel_11);
    Cr[row >> 1][col >> 1] = chrominance_downsample(Cr_pixel_00, Cr_pixel_01, Cr_pixel_10, Cr_pixel_11);
} //END of rgb_to_ycc_fixed_point_rounded()

// =======
static void csc_rgb_to_ycc_brute_force_float( int row, int col) {
//
  uint8_t Cb_pixel_00, Cb_pixel_01;
  uint8_t Cb_pixel_10, Cb_pixel_11;
  uint8_t Cr_pixel_00, Cr_pixel_01;
  uint8_t Cr_pixel_10, Cr_pixel_11;

  Y[row+0][col+0] = (uint8_t)(16.0 + 0.257*R[row+0][col+0]
                                   + 0.504*G[row+0][col+0]
                                   + 0.098*B[row+0][col+0]);
  Y[row+0][col+1] = (uint8_t)(16.0 + 0.257*R[row+0][col+1]
                                   + 0.504*G[row+0][col+1]
                                   + 0.098*B[row+0][col+1]);
  Y[row+1][col+0] = (uint8_t)(16.0 + 0.257*R[row+1][col+0]
                                   + 0.504*G[row+1][col+0]
                                   + 0.098*B[row+1][col+0]);
  Y[row+1][col+1] = (uint8_t)(16.0 + 0.257*R[row+1][col+1]
                                   + 0.504*G[row+1][col+1]
                                   + 0.098*B[row+1][col+1]);

  Cb_pixel_00 = (uint8_t)(128.0 - 0.148*R[row+0][col+0]
                                - 0.291*G[row+0][col+0]
                                + 0.439*B[row+0][col+0]);
  Cb_pixel_01 = (uint8_t)(128.0 - 0.148*R[row+0][col+1]
                                - 0.291*G[row+0][col+1]
                                + 0.439*B[row+0][col+1]);
  Cb_pixel_10 = (uint8_t)(128.0 - 0.148*R[row+1][col+0]
                                - 0.291*G[row+1][col+0]
                                + 0.439*B[row+1][col+0]);
  Cb_pixel_11 = (uint8_t)(128.0 - 0.148*R[row+1][col+1]
                                - 0.291*G[row+1][col+1]
                                + 0.439*B[row+1][col+1]);

  Cr_pixel_00 = (uint8_t)(128.0 + 0.439*R[row+0][col+0]
                                - 0.368*G[row+0][col+0]
                                - 0.071*B[row+0][col+0]);
  Cr_pixel_01 = (uint8_t)(128.0 + 0.439*R[row+0][col+1]
                                - 0.368*G[row+0][col+1]
                                - 0.071*B[row+0][col+1]);
  Cr_pixel_10 = (uint8_t)(128.0 + 0.439*R[row+1][col+0]
                                - 0.368*G[row+1][col+0]
                                - 0.071*B[row+1][col+0]);
  Cr_pixel_11 = (uint8_t)(128.0 + 0.439*R[row+1][col+1]
                                - 0.368*G[row+1][col+1]
                                - 0.071*B[row+1][col+1]);

  Cb[row>>1][col>>1] = chrominance_downsample( Cb_pixel_00,
                                               Cb_pixel_01,
                                               Cb_pixel_10,
                                               Cb_pixel_11);

  Cr[row>>1][col>>1] = chrominance_downsample( Cr_pixel_00,
                                               Cr_pixel_01,
                                               Cr_pixel_10,
                                               Cr_pixel_11);
} // END of CSC_RGB_to_YCC_brute_force_float()

// =======
static void csc_rgb_to_ycc_brute_force_int( int row, int col) {
//
  int R_pixel_00, R_pixel_01, R_pixel_10, R_pixel_11;
  int G_pixel_00, G_pixel_01, G_pixel_10, G_pixel_11;
  int B_pixel_00, B_pixel_01, B_pixel_10, B_pixel_11;

  int  Y_pixel_00,  Y_pixel_01,  Y_pixel_10,  Y_pixel_11;
  int Cb_pixel_00, Cb_pixel_01, Cb_pixel_10, Cb_pixel_11;
  int Cr_pixel_00, Cr_pixel_01, Cr_pixel_10, Cr_pixel_11;

  R_pixel_00 = (int)R[row+0][col+0];
  R_pixel_01 = (int)R[row+0][col+1];
  R_pixel_10 = (int)R[row+1][col+0];
  R_pixel_11 = (int)R[row+1][col+1];

  G_pixel_00 = (int)G[row+0][col+0];
  G_pixel_01 = (int)G[row+0][col+1];
  G_pixel_10 = (int)G[row+1][col+0];
  G_pixel_11 = (int)G[row+1][col+1];

  B_pixel_00 = (int)B[row+0][col+0];
  B_pixel_01 = (int)B[row+0][col+1];
  B_pixel_10 = (int)B[row+1][col+0];
  B_pixel_11 = (int)B[row+1][col+1];

  Y_pixel_00 = (16 << (K)) + C11 * R_pixel_00
                           + C12 * G_pixel_00
                           + C13 * B_pixel_00;
  Y_pixel_00 += (1 << (K-1)); // rounding
  Y_pixel_00 = Y_pixel_00 >> K;

  Y_pixel_01 = (16 << (K)) + C11 * R_pixel_01
                           + C12 * G_pixel_01
                           + C13 * B_pixel_01;
  Y_pixel_01 += (1 << (K-1)); // rounding
  Y_pixel_01 = Y_pixel_01 >> K;

  Y_pixel_10 = (16 << (K)) + C11 * R_pixel_10
                           + C12 * G_pixel_10
                           + C13 * B_pixel_10;
  Y_pixel_10 += (1 << (K-1)); // rounding
  Y_pixel_10 = Y_pixel_10 >> K;

  Y_pixel_11 = (16 << (K)) + C11 * R_pixel_11
                           + C12 * G_pixel_11
                           + C13 * B_pixel_11;
  Y_pixel_11 += (1 << (K-1)); // rounding
  Y_pixel_11 = Y_pixel_11 >> K;

  Y[row+0][col+0] = (uint8_t)Y_pixel_00;
  Y[row+0][col+1] = (uint8_t)Y_pixel_01;
  Y[row+1][col+0] = (uint8_t)Y_pixel_10;
  Y[row+1][col+1] = (uint8_t)Y_pixel_11;

  Cb_pixel_00 = (128 << (K)) - C21 * R_pixel_00
                             - C22 * G_pixel_00
                             + C23 * B_pixel_00;
  Cb_pixel_00 += (1 << (K-1)); // rounding
  Cb_pixel_00 = Cb_pixel_00 >> K;

  Cb_pixel_01 = (128 << (K)) - C21 * R_pixel_01
                             - C22 * G_pixel_01
                             + C23 * B_pixel_01;
  Cb_pixel_01 += (1 << (K-1)); // rounding
  Cb_pixel_01 = Cb_pixel_01 >> K;

  Cb_pixel_10 = (128 << (K)) - C21 * R_pixel_10
                             - C22 * G_pixel_10
                             + C23 * B_pixel_10;
  Cb_pixel_10 += (1 << (K-1)); // rounding
  Cb_pixel_10 = Cb_pixel_10 >> K;

  Cb_pixel_11 = (128 << (K)) - C21 * R_pixel_11
                             - C22 * G_pixel_11
                             + C23 * B_pixel_11;
  Cb_pixel_11 += (1 << (K-1)); // rounding
  Cb_pixel_11 = Cb_pixel_11 >> K;

  Cr_pixel_00 = (128 << (K)) + C31 * R_pixel_00
                             - C32 * G_pixel_00
                             - C33 * B_pixel_00;
  Cr_pixel_00 += (1 << (K-1)); // rounding
  Cr_pixel_00 = Cr_pixel_00 >> K;

  Cr_pixel_01 = (128 << (K)) + C31 * R_pixel_01
                             - C32 * G_pixel_01
                             - C33 * B_pixel_01;
  Cr_pixel_01 += (1 << (K-1)); // rounding
  Cr_pixel_01 = Cr_pixel_01 >> K;

  Cr_pixel_10 = (128 << (K)) + C31 * R_pixel_10
                             - C32 * G_pixel_10
                             - C33 * B_pixel_10;
  Cr_pixel_10 += (1 << (K-1)); // rounding
  Cr_pixel_10 = Cr_pixel_10 >> K;

  Cr_pixel_11 = (128 << (K)) + C31 * R_pixel_11
                             - C32 * G_pixel_11
                             - C33 * B_pixel_11;
  Cr_pixel_11 += (1 << (K-1)); // rounding
  Cr_pixel_11 = Cr_pixel_11 >> K;

  Cb[row>>1][col>>1] = chrominance_downsample( (uint8_t)Cb_pixel_00,
                                               (uint8_t)Cb_pixel_01,
                                               (uint8_t)Cb_pixel_10,
                                               (uint8_t)Cb_pixel_11);

  Cr[row>>1][col>>1] = chrominance_downsample( (uint8_t)Cr_pixel_00,
                                               (uint8_t)Cr_pixel_01,
                                               (uint8_t)Cr_pixel_10,
                                               (uint8_t)Cr_pixel_11);
} // END of CSC_RGB_to_YCC_brute_force_int()

// =======
static uint8_t chrominance_downsample(
    uint8_t C_pixel_00, uint8_t C_pixel_01,
    uint8_t C_pixel_10, uint8_t C_pixel_11) {

  int temp;

  switch (CHROMINANCE_DOWNSAMPLING_MODE) {
    case 0:
      return( 0);
    case 1:
      return( C_pixel_00);
    case 2:
      temp = (int)C_pixel_00 + (int)C_pixel_01 + 
             (int)C_pixel_10 + (int)C_pixel_11;
      temp += (1 << 1); // rounding
      temp = temp >> 2;
      return( (uint8_t)temp);
    default:
      return( 0);
  }
} // END of chrominance_downsample()

static void csc_rgb_to_ycc_unrolled_int(int row, int col) {
  // Process a 4x4 block of pixels by unrolling the loop.
  rgb_to_ycc_fixed_point_rounded(row, col);
  rgb_to_ycc_fixed_point_rounded(row, col + 2);
  rgb_to_ycc_fixed_point_rounded(row + 2, col); 
  rgb_to_ycc_fixed_point_rounded(row + 2, col + 2);
} // END of csc_rgb_to_ycc_unrolled_int()

static void csc_rgb_to_ycc_vector(int row, int col) {

  // Grab RGB values from arrays
  int16_t R_data[4] = {
    (int16_t)R[row][col],
    (int16_t)R[row][col+1],
    (int16_t)R[row+1][col],
    (int16_t)R[row+1][col+1]
  };
  int16_t G_data[4] = {
    (int16_t)G[row][col],
    (int16_t)G[row][col+1],
    (int16_t)G[row+1][col],
    (int16_t)G[row+1][col+1]
  };
  int16_t B_data[4] = {
    (int16_t)B[row][col],
    (int16_t)B[row][col+1],
    (int16_t)B[row+1][col],
    (int16_t)B[row+1][col+1]
  };

  // load data into NEON vectors
  int16x4_t R_vec = vld1_s16(R_data);
  int16x4_t G_vec = vld1_s16(G_data);
  int16x4_t B_vec = vld1_s16(B_data);

  int32x4_t Y_R = vmull_n_s16(R_vec, C11); // C11 * R
  int32x4_t Y_G = vmull_n_s16(G_vec, C12); // C12 * G
  int32x4_t Y_B = vmull_n_s16(B_vec, C13); // C13 * B

  int32x4_t Y_sum = vaddq_s32(Y_R, Y_G);
  Y_sum = vaddq_s32(Y_sum, Y_B);
  Y_sum = vaddq_s32(Y_sum, vdupq_n_s32(16 << K));           
  Y_sum = vaddq_s32(Y_sum, vdupq_n_s32(1 << (K-1)));        
  int32x4_t Y_shifted = vshrq_n_s32(Y_sum, K);              

  int16x4_t Y_pixels = vmovn_s32(Y_shifted); 

  // write Y pixels (lane order matches R_data order)
  Y[row+0][col+0] = (uint8_t)vget_lane_s16(Y_pixels, 0);
  Y[row+0][col+1] = (uint8_t)vget_lane_s16(Y_pixels, 1);
  Y[row+1][col+0] = (uint8_t)vget_lane_s16(Y_pixels, 2);
  Y[row+1][col+1] = (uint8_t)vget_lane_s16(Y_pixels, 3);

  // Cb multiply by constants
  int32x4_t Cb_R = vmull_n_s16(R_vec, C21);
  int32x4_t Cb_G = vmull_n_s16(G_vec, C22);
  int32x4_t Cb_B = vmull_n_s16(B_vec, C23);

  // Add 128 << K to Cb values and subtract R, G 
  // Switch to s32 to avoid overflow
  int32x4_t Cb_sum = vdupq_n_s32(128 << K);
  Cb_sum = vsubq_s32(Cb_sum, Cb_R);
  Cb_sum = vsubq_s32(Cb_sum, Cb_G);
  Cb_sum = vaddq_s32(Cb_sum, Cb_B);
  Cb_sum = vaddq_s32(Cb_sum, vdupq_n_s32(1 << (K-1))); 
  int32x4_t Cb_shifted = vshrq_n_s32(Cb_sum, K);        
  int16x4_t Cb_pixels = vmovn_s32(Cb_shifted);      

  //Cb DOWNSAMPLING
  //Pulling each pixel from Cb_pixels vector
  int cb0 = (int)vget_lane_s16(Cb_pixels, 0);
  int cb1 = (int)vget_lane_s16(Cb_pixels, 1);
  int cb2 = (int)vget_lane_s16(Cb_pixels, 2);
  int cb3 = (int)vget_lane_s16(Cb_pixels, 3);

  //Operation from chrominance_downsample()
  int cb_sum_scalar = cb0 + cb1 + cb2 + cb3;
  cb_sum_scalar += (1 << 1); // rounding for average
  cb_sum_scalar = cb_sum_scalar >> 2;
  Cb[row >> 1][col >> 1] = (uint8_t)cb_sum_scalar;
  //Cb DOWNSAMPLING END

  // Cr multiply by constants
  int32x4_t Cr_R = vmull_n_s16(R_vec, C31);
  int32x4_t Cr_G = vmull_n_s16(G_vec, C32);
  int32x4_t Cr_B = vmull_n_s16(B_vec, C33);

  //Add 128 << K to Cr values and subtract Cb 
  //Switch to s32 to avoid overflow
  int32x4_t Cr_sum = vdupq_n_s32(128 << K);
  Cr_sum = vaddq_s32(Cr_sum, Cr_R);
  Cr_sum = vsubq_s32(Cr_sum, Cr_G);
  Cr_sum = vsubq_s32(Cr_sum, Cr_B);
  Cr_sum = vaddq_s32(Cr_sum, vdupq_n_s32(1 << (K-1))); 
  int32x4_t Cr_shifted = vshrq_n_s32(Cr_sum, K);        
  int16x4_t Cr_pixels = vmovn_s32(Cr_shifted);

  //Cr DOWNSAMPLING
  //Pulling each pixel from Cr_pixels vector
  int cr0 = (int)vget_lane_s16(Cr_pixels, 0);
  int cr1 = (int)vget_lane_s16(Cr_pixels, 1);
  int cr2 = (int)vget_lane_s16(Cr_pixels, 2);
  int cr3 = (int)vget_lane_s16(Cr_pixels, 3);

  //Operation from chrominance_downsample()
  int cr_sum_scalar = cr0 + cr1 + cr2 + cr3;
  cr_sum_scalar += (1 << 1); // rounding for average
  cr_sum_scalar = cr_sum_scalar >> 2;
  Cr[row >> 1][col >> 1] = (uint8_t)cr_sum_scalar;
  //Cr DOWNSAMPLING END

}

// =======
void csc_rgb_to_ycc(int input_col, int input_row) {
  int row, col; // indices for row and column
  int increment;
  if(RGB_to_YCC_ROUTINE == 3) {
    increment = 4; // unrolled version processes 4x4 blocks
  }
  else {
    increment = 2; // other versions process 2x2 blocks
  }

  for(row = 0; row< input_row-1; row += increment) {
    for(col = 0; col< input_col-1; col += increment) {

      switch (RGB_to_YCC_ROUTINE) {
        case 0:
          break;
        case 1:
          csc_rgb_to_ycc_brute_force_float( row, col);
          break;
        case 2:
          csc_rgb_to_ycc_brute_force_int( row, col);
          break;
        case 3:
          csc_rgb_to_ycc_unrolled_int(row, col);
          break;
        case 4:
          csc_rgb_to_ycc_vector(row, col);
          break;
        case 5:
          rgb_to_ycc_fixed_point_rounded(row, col);
        default:
          break;
      }
    }
  }
} // END of CSC_RGB_to_YCC()




