#ifndef CSC_GLOBAL_H
#define CSC_GLOBAL_H

#include <stdint.h>

// Image dimensions
#define IMAGE_ROW_SIZE 768 // Rows (Height)
#define IMAGE_COL_SIZE 1024 // Columns (Width)

// Color Space Conversion Routine Selection
// 0: No conversion
// 1: Floating-point (brute force)
// 2: Fixed-point integer (brute force)
#define RGB_to_YCC_ROUTINE 4
#define YCC_to_RGB_ROUTINE 1

// Chrominance Downsampling Mode
// 0: No downsampling (or return 0)
// 1: Discard three samples, keep one (C_pixel_00)
// 2: Average of four samples
#define CHROMINANCE_DOWNSAMPLING_MODE 2
// Chrominance Upsampling Mode
// 0: No upsampling (or return 0)
// 1: Replicate one sample into four
// 2: Linear interpolation
#define CHROMINANCE_UPSAMPLING_MODE 2

#define K 8

// Fixed-point coefficients for RGB to YCC conversion (scaled by 2^K_RGB_YCC)
#define C11 66  // round(0.257 * 2^8)
#define C12 129 // round(0.504 * 2^8)
#define C13 25  // round(0.098 * 2^8)
#define C21 38  // round(0.148 * 2^8)
#define C22 74  // round(0.291 * 2^8)
#define C23 112 // round(0.439 * 2^8)
#define C31 112 // round(0.439 * 2^8)
#define C32 94  // round(0.368 * 2^8)
#define C33 18  // round(0.071 * 2^8)

// Fixed-point coefficients for YCC to RGB conversion (scaled by 2^K_YCC_RGB)
#define D1 74   // round(1.164 * 2^6)
#define D2 102  // round(1.596 * 2^6)
#define D3 52   // round(0.813 * 2^6)
#define D4 25   // round(0.391 * 2^6)
#define D5 129  // round(2.018 * 2^6)


// Global image data arrays - Declared as extern
// These are *declarations*, indicating that the variables are defined elsewhere.
// extern uint8_t R[IMAGE_ROW_SIZE][IMAGE_COL_SIZE];
// extern uint8_t G[IMAGE_ROW_SIZE][IMAGE_COL_SIZE];
// extern uint8_t B[IMAGE_ROW_SIZE][IMAGE_COL_SIZE];
// extern uint8_t Y[IMAGE_ROW_SIZE][IMAGE_COL_SIZE];
// extern uint8_t Cb[IMAGE_ROW_SIZE / 2][IMAGE_COL_SIZE / 2];
// extern uint8_t Cr[IMAGE_ROW_SIZE / 2][IMAGE_COL_SIZE / 2];
// extern uint8_t Cb_temp[IMAGE_ROW_SIZE][IMAGE_COL_SIZE];
// extern uint8_t Cr_temp[IMAGE_ROW_SIZE][IMAGE_COL_SIZE];

extern uint8_t **R;
extern uint8_t **G;
extern uint8_t **B;
extern uint8_t **Y;
extern uint8_t **Cb;
extern uint8_t **Cr;
extern uint8_t **Cb_temp;
extern uint8_t **Cr_temp;


// Function prototypes for CSC routines (defined in CSC_RGB_to_YCC_01.c and CSC_YCC_to_RGB_01.c)
// void CSC_RGB_to_YCC(void);
// void CSC_YCC_to_RGB(void);

void CSC_RGB_to_YCC(int input_col, int input_row);
void CSC_YCC_to_RGB(int input_col, int input_row);

#endif // CSC_GLOBAL_H

