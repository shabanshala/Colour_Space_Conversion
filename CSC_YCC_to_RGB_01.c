
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "CSC_global.h"
#include <arm_neon.h>


// private prototypes
// =======
static uint8_t saturation_float( float argument);
static void CSC_YCC_to_RGB_brute_force_float( int row, int col);
static void CSC_YCC_to_RGB_vector(int row, int col);
static int IMG_H = 0;
static int IMG_W = 0;

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

void YCC_to_RGB_fixed_point_rounded_saturated(
    uint8_t *_img_buffer_in_y,
    uint8_t *_img_buffer_in_cb,
    uint8_t *_img_buffer_in_cr,
    uint8_t *_img_buffer_out_rgb,
    int _length);

static void YCC_to_RGB_fixed_point_rounded_saturated_rowcol(int row, int col);



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


static void YCC_to_RGB_fixed_point_rounded_saturated_rowcol(int row, int col) {
    uint8_t y4[4], cb4[4], cr4[4];
    int idx = 0;
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            int rr = row + r, cc = col + c;
            y4[idx]  = Y[rr][cc];
            cb4[idx] = Cb_temp[rr][cc];
            cr4[idx] = Cr_temp[rr][cc];
            idx++;
        }
    }

    uint8_t rgb4[12];
    YCC_to_RGB_fixed_point_rounded_saturated(y4, cb4, cr4, rgb4, 4);

    idx = 0;
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            int rr = row + r, cc = col + c;
            R[rr][cc] = rgb4[3*idx + 0];
            G[rr][cc] = rgb4[3*idx + 1];
            B[rr][cc] = rgb4[3*idx + 2];
            idx++;
        }
    }
}

void YCC_to_RGB_fixed_point_rounded_saturated(uint8_t *_img_buffer_in_y, uint8_t *_img_buffer_in_cb, uint8_t *_img_buffer_in_cr, uint8_t *_img_buffer_out_rgb, int _length) {
    #define Y_TO_RGB_COEFF      298  
    #define CR_TO_R_COEFF       409  
    #define CB_TO_B_COEFF       517 
    #define CB_TO_G_COEFF       -100 
    #define CR_TO_G_COEFF       -208

    for (int i = 0; i < _length; ++i) {
        
        int32_t y = _img_buffer_in_y[i];
        int32_t cb = _img_buffer_in_cb[i];
        int32_t cr = _img_buffer_in_cr[i];
        
        int32_t y_shifted = y - 16;
        int32_t cb_shifted = cb - 128;
        int32_t cr_shifted = cr - 128;
        
        int32_t temp_r = (Y_TO_RGB_COEFF * y_shifted + CR_TO_R_COEFF * cr_shifted + 128) >> 8;

        int32_t temp_g = (Y_TO_RGB_COEFF * y_shifted + CB_TO_G_COEFF * cb_shifted + CR_TO_G_COEFF * cr_shifted + 128) >> 8;
        
        int32_t temp_b = (Y_TO_RGB_COEFF * y_shifted + CB_TO_B_COEFF * cb_shifted + 128) >> 8;

        _img_buffer_out_rgb[3 * i] = (uint8_t) (temp_r < 0 ? 0 : (temp_r > 255 ? 255 : temp_r));
        _img_buffer_out_rgb[3 * i + 1] = (uint8_t) (temp_g < 0 ? 0 : (temp_g > 255 ? 255 : temp_g));
        _img_buffer_out_rgb[3 * i + 2] = (uint8_t) (temp_b < 0 ? 0 : (temp_b > 255 ? 255 : temp_b));
    }
}


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

  // Somehow this 6 makes the image brighter
  const int shift = 6;

  // Save YCC data from arrays to local variables
  int16_t Y_data[4] = {
      (int16_t)Y[row][col],
      (int16_t)Y[row][col + 1],
      (int16_t)Y[row + 1][col],
      (int16_t)Y[row + 1][col + 1]
  };
  int16_t Cb_data[4] = {
      (int16_t)Cb_temp[row][col],
      (int16_t)Cb_temp[row][col + 1],
      (int16_t)Cb_temp[row + 1][col],
      (int16_t)Cb_temp[row + 1][col + 1]
  };
  int16_t Cr_data[4] = {
      (int16_t)Cr_temp[row][col],
      (int16_t)Cr_temp[row][col + 1],
      (int16_t)Cr_temp[row + 1][col],
      (int16_t)Cr_temp[row + 1][col + 1]
  };

  // Create NEON vectors 
  int16x4_t Y_vec  = vld1_s16(Y_data);  
  int16x4_t Cb_vec = vld1_s16(Cb_data); 
  int16x4_t Cr_vec = vld1_s16(Cr_data);

  // widen to 32-bit for math
  int32x4_t Y32  = vmovl_s16(Y_vec);
  int32x4_t Cb32 = vmovl_s16(Cb_vec);
  int32x4_t Cr32 = vmovl_s16(Cr_vec);

  // subtract offsets (Y-16, Cb-128, Cr-128)
  Y32  = vsubq_s32(Y32,  vdupq_n_s32(16));
  Cb32 = vsubq_s32(Cb32, vdupq_n_s32(128));
  Cr32 = vsubq_s32(Cr32, vdupq_n_s32(128));

  // compute R, G, B using scaled integer constants (D1..D5)
  // R = D1*(Y-16) + D2*(Cr-128)
  int32x4_t R_pix = vmlaq_n_s32(vmulq_n_s32(Y32, D1), Cr32, D2);

  // G = D1*(Y-16) - D3*(Cb-128) - D4*(Cr-128)
  int32x4_t G_pix = vsubq_s32(vmulq_n_s32(Y32, D1), vmulq_n_s32(Cb32, D3));
  G_pix = vsubq_s32(G_pix, vmulq_n_s32(Cr32, D4));

  // B = D1*(Y-16) + D5*(Cb-128)
  int32x4_t B_pix = vmlaq_n_s32(vmulq_n_s32(Y32, D1), Cb32, D5);

  // add rounding and shift down
  int32x4_t round = vdupq_n_s32(1 << (shift - 1));
  R_pix = vaddq_s32(R_pix, round);
  G_pix = vaddq_s32(G_pix, round);
  B_pix = vaddq_s32(B_pix, round);
  
  R_pix = vshrq_n_s32(R_pix, shift);
  G_pix = vshrq_n_s32(G_pix, shift);
  B_pix = vshrq_n_s32(B_pix, shift);

  // Ensure values stay within 0-255
  int32x4_t zero32 = vdupq_n_s32(0);
  int32x4_t max32  = vdupq_n_s32(255);

  // Take max of variable = (pix, 0)
  // Take min of pix = (variable, 255)
  R_pix = vminq_s32(vmaxq_s32(R_pix, zero32), max32);
  G_pix = vminq_s32(vmaxq_s32(G_pix, zero32), max32);
  B_pix = vminq_s32(vmaxq_s32(B_pix, zero32), max32);

  //Get pixels from vector
  int32_t r0 = vgetq_lane_s32(R_pix, 0);
  int32_t r1 = vgetq_lane_s32(R_pix, 1);
  int32_t r2 = vgetq_lane_s32(R_pix, 2);
  int32_t r3 = vgetq_lane_s32(R_pix, 3);

  int32_t g0 = vgetq_lane_s32(G_pix, 0);
  int32_t g1 = vgetq_lane_s32(G_pix, 1);
  int32_t g2 = vgetq_lane_s32(G_pix, 2);
  int32_t g3 = vgetq_lane_s32(G_pix, 3);

  int32_t b0 = vgetq_lane_s32(B_pix, 0);
  int32_t b1 = vgetq_lane_s32(B_pix, 1);
  int32_t b2 = vgetq_lane_s32(B_pix, 2);
  int32_t b3 = vgetq_lane_s32(B_pix, 3);

  // store to correct 2x2 positions
  R[row + 0][col + 0] = (uint8_t)r0;
  R[row + 0][col + 1] = (uint8_t)r1;
  R[row + 1][col + 0] = (uint8_t)r2;
  R[row + 1][col + 1] = (uint8_t)r3;

  G[row + 0][col + 0] = (uint8_t)g0;
  G[row + 0][col + 1] = (uint8_t)g1;
  G[row + 1][col + 0] = (uint8_t)g2;
  G[row + 1][col + 1] = (uint8_t)g3;

  B[row + 0][col + 0] = (uint8_t)b0;
  B[row + 0][col + 1] = (uint8_t)b1;
  B[row + 1][col + 0] = (uint8_t)b2;
  B[row + 1][col + 1] = (uint8_t)b3;

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


static void CSC_YCC_to_RGB_unrolled_int(int row, int col) {
  // Process a 4x4 block of pixels
  // Each call to brute force processes a 2x2 block
  CSC_YCC_to_RGB_brute_force_int(row, col);
  CSC_YCC_to_RGB_brute_force_int(row, col + 2);
  CSC_YCC_to_RGB_brute_force_int(row + 2, col);
  CSC_YCC_to_RGB_brute_force_int(row + 2, col + 2);
} // END of CSC_RGB_to_YCC_unrolled_int()

// =======
void CSC_YCC_to_RGB(int input_row, int input_col) {
  int row, col; // indices for row and column

  IMG_H = input_row;
  IMG_W = input_col;
  
  chrominance_array_upsample(input_row, input_col);

  for (row = 0; row < input_row - 1; row += 2) {
    for (col = 0; col < input_col - 1; col += 2) {
      switch (YCC_to_RGB_ROUTINE) {
        case 0: 
          break;
        case 1: 
          CSC_YCC_to_RGB_brute_force_float(row, col); 
          break;
        case 2: 
          CSC_YCC_to_RGB_brute_force_int(row, col); 
          break;
        case 3: 
          CSC_YCC_to_RGB_unrolled_int(row, col); 
          break;
        case 4: 
          CSC_YCC_to_RGB_vector(row, col); 
          break;
        case 5: 
          YCC_to_RGB_fixed_point_rounded_saturated_rowcol(row, col); 
          break;
          default: 
        break;
      }
    }
  }
} // END of CSC_YCC_to_RGB()

