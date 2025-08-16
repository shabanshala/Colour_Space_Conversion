
// Copyright 2023 Mihai SIMA (mihai.sima@ieee.org). All rights reserved.
// Color Space Conversion (CSC) in fixed-point arithmetic


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h> 
#include <string.h> 
#include "CSC_global.h" 

uint8_t **R;
uint8_t **G;
uint8_t **B;
uint8_t **Y;
uint8_t **Cb;
uint8_t **Cr;
uint8_t **Cb_temp;
uint8_t **Cr_temp;

// Helper function to dynamically allocate a 2D array
uint8_t** allocate_2d_array(int rows, int cols) {
    uint8_t** array = (uint8_t**)malloc(rows * sizeof(uint8_t*));
    if (array == NULL) {
        fprintf(stderr, "Memory allocation failed for rows (size: %d).\n", rows);
        return NULL;
    }
    for (int i = 0; i < rows; i++) {
        array[i] = (uint8_t*)malloc(cols * sizeof(uint8_t));
        if (array[i] == NULL) {
            fprintf(stderr, "Memory allocation failed for columns (size: %d) at row %d.\n", cols, i);
            // Clean up already allocated memory
            for (int j = 0; j < i; j++) {
                free(array[j]);
            }
            free(array);
            return NULL;
        }
    }
    return array;
}

// Helper function to deallocate a 2D array
void free_2d_array(uint8_t** array, int rows) {
    if (array == NULL) return;
    for (int i = 0; i < rows; i++) {
        if (array[i] != NULL) {
            free(array[i]);
        }
    }
    free(array);
}

const char *input_files_echo[5][3] = {
    {"./image_echo_R_64_48_03.data",
    "./image_echo_G_64_48_03.data",
    "./image_echo_B_64_48_03.data"},
    {
    "./image_echo_R_64_64_03.data",
    "./image_echo_G_64_64_03.data",
    "./image_echo_B_64_64_03.data"
    },

    {"./image_echo_R_640_480_02.data",
    "./image_echo_G_640_480_02.data",
    "./image_echo_B_640_480_02.data"},

    {"./image_echo_R_768_1024_03.data",
    "./image_echo_G_768_1024_03.data",
    "./image_echo_B_768_1024_03.data"},

    {"./image_echo_R_1024_1024_03.data",
    "./image_echo_G_1024_1024_03.data",
    "./image_echo_B_1024_1024_03.data"}
};

const char *output_files_YCC[5][3] = {
 {
    "./image_output_Y_64_48_03.data",
    "./image_output_Cb_64_48_03.data",
    "./image_output_Cr_64_48_03.data"
 },
 {
    "./image_output_Y_64_64_03.data",
    "./image_output_Cb_64_64_03.data",
    "./image_output_Cr_64_64_03.data",
 },
 {
    "./image_output_Y_640_480_02.data",
    "./image_output_Cb_640_480_02.data",
    "./image_output_Cr_640_480_02.data"
 },
 {
    "./image_output_Y_768_1024_03.data",
    "./image_output_Cb_768_1024_03.data",
    "./image_output_Cr_768_1024_03.data"
 },
 {
    "./image_output_Y_1024_1024_03.data",
    "./image_output_Cb_1024_1024_03.data",
    "./image_output_Cr_1024_1024_03.data"
 }
};

const char *output_files_RGB[5] = {
 "./image_output_RGB_64_48_03.data",
 "./image_output_RGB_64_64_03.data",
 "./image_output_RGB_640_480_02.data",
 "./image_output_RGB_768_1024_03.data",
 "./image_output_RGB_1024_1024_03.data"
};

int image_sizes[5][2] = {
    {64, 48},
    {64, 64},
    {640, 480},
    {768, 1024},
    {1024, 1024}
};

const char *input_files_RGB[5] = {
    "./image_input_RGB_64_48_03.data",
    "./image_input_RGB_64_64_03.data",
    "./image_input_RGB_640_480_02.data",
    "./image_input_RGB_768_1024_03.data",
    "./image_input_RGB_1024_1024_03.data"
};

int main(void) {
    int row, col;
    FILE *f_ID_input_RGB;
    FILE *f_ID_echo_R;
    FILE *f_ID_echo_G;
    FILE *f_ID_echo_B;
    FILE *f_ID_output_Y;
    FILE *f_ID_output_Cb;
    FILE *f_ID_output_Cr;
    FILE *f_ID_output_RGB;

    for(int i = 0; i < 5; i++) {
        clock_t start_time, end_time;
        double elapsed_time;

        start_time = clock(); // Start the timer

        int current_row_size = image_sizes[i][0];
        int current_col_size = image_sizes[i][1];
        const char* RGB_filename = input_files_RGB[i];
        

        // Dynamically allocate memory for all arrays
        R = allocate_2d_array(current_row_size, current_col_size);
        G = allocate_2d_array(current_row_size, current_col_size);
        B = allocate_2d_array(current_row_size, current_col_size);
        Y = allocate_2d_array(current_row_size, current_col_size);
        Cb = allocate_2d_array(current_row_size / 2, current_col_size / 2);
        Cr = allocate_2d_array(current_row_size / 2, current_col_size / 2);
        Cb_temp = allocate_2d_array(current_row_size, current_col_size);
        Cr_temp = allocate_2d_array(current_row_size, current_col_size);

        if (!R || !G || !B || !Y || !Cb || !Cr || !Cb_temp || !Cr_temp) {
            fprintf(stderr, "Failed to allocate memory for image data.\n");
           
            free_2d_array(R, current_row_size);
            free_2d_array(G, current_row_size);
            free_2d_array(B, current_row_size);
            free_2d_array(Y, current_row_size);
            free_2d_array(Cb, current_row_size / 2);
            free_2d_array(Cr, current_row_size / 2);
            free_2d_array(Cb_temp, current_row_size);
            free_2d_array(Cr_temp, current_row_size);
            return 1;
        }

        for (row = 0; row < current_row_size; row++) {
            memset(Y[row], 0, current_col_size);
            memset(Cb_temp[row], 0, current_col_size);
            memset(Cr_temp[row], 0, current_col_size);
        }
        for (row = 0; row < (current_row_size >> 1); row++) {
            memset(Cb[row], 0, (current_col_size >> 1));
            memset(Cr[row], 0, (current_col_size >> 1));
        }
       
        f_ID_input_RGB = fopen(RGB_filename, "rb");
        if( f_ID_input_RGB == NULL) {
            printf( "Error: Could not open input file %s\n", RGB_filename);
            free_2d_array(R, current_row_size);
            free_2d_array(G, current_row_size);
            free_2d_array(B, current_row_size);
            free_2d_array(Y, current_row_size);
            free_2d_array(Cb, current_row_size / 2);
            free_2d_array(Cr, current_row_size / 2);
            free_2d_array(Cb_temp, current_row_size);
            free_2d_array(Cr_temp, current_row_size);
            return( 1);
        }

        f_ID_echo_R = fopen( input_files_echo[i][0], "wb");
        if( f_ID_echo_R == NULL) {
            printf( "Error: Could not open echo R file.\n");
            fclose(f_ID_input_RGB);
            free_2d_array(R, current_row_size);
            free_2d_array(G, current_row_size);
            free_2d_array(B, current_row_size);
            free_2d_array(Y, current_row_size);
            free_2d_array(Cb, current_row_size / 2);
            free_2d_array(Cr, current_row_size / 2);
            free_2d_array(Cb_temp, current_row_size);
            free_2d_array(Cr_temp, current_row_size);
            return( 1);
        }

        f_ID_echo_G = fopen( input_files_echo[i][1], "wb");
        if( f_ID_echo_G == NULL) {
            printf( "Error: Could not open echo G file.\n");
            fclose(f_ID_echo_R);
            fclose(f_ID_input_RGB);
            free_2d_array(R, current_row_size);
            free_2d_array(G, current_row_size);
            free_2d_array(B, current_row_size);
            free_2d_array(Y, current_row_size);
            free_2d_array(Cb, current_row_size / 2);
            free_2d_array(Cr, current_row_size / 2);
            free_2d_array(Cb_temp, current_row_size);
            free_2d_array(Cr_temp, current_row_size);
            return( 1);
        }

        f_ID_echo_B = fopen( input_files_echo[i][2], "wb");
        if( f_ID_echo_B == NULL) {
            printf( "Error: Could not open echo B file.\n");
            fclose(f_ID_echo_G);
            fclose(f_ID_echo_R);
            fclose(f_ID_input_RGB);
            free_2d_array(R, current_row_size);
            free_2d_array(G, current_row_size);
            free_2d_array(B, current_row_size);
            free_2d_array(Y, current_row_size);
            free_2d_array(Cb, current_row_size / 2);
            free_2d_array(Cr, current_row_size / 2);
            free_2d_array(Cb_temp, current_row_size);
            free_2d_array(Cr_temp, current_row_size);
            return( 1);
        }

        for (row = 0; row < current_row_size; row++) {
            for (col = 0; col < current_col_size; col++) {
                int r_val = fgetc(f_ID_input_RGB);
                int g_val = fgetc(f_ID_input_RGB);
                int b_val = fgetc(f_ID_input_RGB);

                if (r_val == EOF || g_val == EOF || b_val == EOF) {
                    fprintf(stderr, "Unexpected EOF while reading RGB data at row %d, col %d\n", row, col);
                    exit(1);
                }

                R[row][col] = (uint8_t)r_val;
                G[row][col] = (uint8_t)g_val;
                B[row][col] = (uint8_t)b_val;

                fputc(R[row][col], f_ID_echo_R);
                fputc(G[row][col], f_ID_echo_G);
                fputc(B[row][col], f_ID_echo_B);
            }
        }
       
        fclose( f_ID_echo_B);
        fclose( f_ID_echo_G);
        fclose( f_ID_echo_R);
        fclose( f_ID_input_RGB);
        
        csc_rgb_to_ycc(current_col_size, current_row_size);

        f_ID_output_Y = fopen( output_files_YCC[i][0], "wb");
        if( f_ID_output_Y == NULL) {
            fprintf( stderr, "Error: Could not open output Y file %s\n",
                output_files_YCC[i][0]);
            free_2d_array(R, current_row_size);
            free_2d_array(G, current_row_size);
            free_2d_array(B, current_row_size);
            free_2d_array(Y, current_row_size);
            free_2d_array(Cb, current_row_size / 2);
            free_2d_array(Cr, current_row_size / 2);
            free_2d_array(Cb_temp, current_row_size);
            free_2d_array(Cr_temp, current_row_size);
            return( 1);
        }

        f_ID_output_Cb = fopen( output_files_YCC[i][1], "wb");
        if( f_ID_output_Cb == NULL) {
            fprintf( stderr, "Error: Could not open output Cb file %s\n",
                output_files_YCC[i][1]);
            fclose(f_ID_output_Y);
            free_2d_array(R, current_row_size);
            free_2d_array(G, current_row_size);
            free_2d_array(B, current_row_size);
            free_2d_array(Y, current_row_size);
            free_2d_array(Cb, current_row_size / 2);
            free_2d_array(Cr, current_row_size / 2);
            free_2d_array(Cb_temp, current_row_size);
            free_2d_array(Cr_temp, current_row_size);
            return( 1);
        }

        f_ID_output_Cr = fopen( output_files_YCC[i][2], "wb");
        if( f_ID_output_Cr == NULL) {
            fprintf( stderr, "Error: Could not open output Cr file %s\n",
                output_files_YCC[i][2]);
            fclose(f_ID_output_Cb);
            fclose(f_ID_output_Y);
            free_2d_array(R, current_row_size);
            free_2d_array(G, current_row_size);
            free_2d_array(B, current_row_size);
            free_2d_array(Y, current_row_size);
            free_2d_array(Cb, current_row_size / 2);
            free_2d_array(Cr, current_row_size / 2);
            free_2d_array(Cb_temp, current_row_size);
            free_2d_array(Cr_temp, current_row_size);
            return( 1);
        }

        for( row=0; row < current_row_size; row++) {
            for( col=0; col < current_col_size; col++) {
                fputc( Y[row][col], f_ID_output_Y);
            }
        }

        fclose( f_ID_output_Y);

        for( row=0; row < (current_row_size >> 1); row++) {
            for( col=0; col < (current_col_size >> 1); col++) {
                fputc( Cb[row][col], f_ID_output_Cb);
                fputc( Cr[row][col], f_ID_output_Cr);
            }
        }
        fclose( f_ID_output_Cr);
        fclose( f_ID_output_Cb);

        csc_ycc_to_rgb(current_row_size, current_col_size);

        f_ID_output_RGB = fopen( output_files_RGB[i], "wb");
        if( f_ID_output_RGB == NULL) {
            printf( "Error: Could not open output RGB file.\n");
            free_2d_array(R, current_row_size);
            free_2d_array(G, current_row_size);
            free_2d_array(B, current_row_size);
            free_2d_array(Y, current_row_size);
            free_2d_array(Cb, current_row_size / 2);
            free_2d_array(Cr, current_row_size / 2);
            free_2d_array(Cb_temp, current_row_size);
            free_2d_array(Cr_temp, current_row_size);
            return( 1);
        }

        for( row=0; row < current_row_size; row++) {
            for( col=0; col < current_col_size; col++) {
                fputc( R[row][col], f_ID_output_RGB);
                fputc( G[row][col], f_ID_output_RGB);
                fputc( B[row][col], f_ID_output_RGB);
            }
        }
        fclose( f_ID_output_RGB);
        free_2d_array(R, current_row_size);
        free_2d_array(G, current_row_size);
        free_2d_array(B, current_row_size);
        free_2d_array(Y, current_row_size);
        free_2d_array(Cb, current_row_size / 2);
        free_2d_array(Cr, current_row_size / 2);
        free_2d_array(Cb_temp, current_row_size);
        free_2d_array(Cr_temp, current_row_size);

        end_time = clock(); // Stop the timer
        elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        printf("Elapsed time for image size %dx%d: %.4f seconds\n", current_row_size, current_col_size, elapsed_time);
    }

    return 0;
}


