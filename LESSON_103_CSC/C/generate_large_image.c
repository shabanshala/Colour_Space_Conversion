#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define IMAGE_WIDTH 1024
#define IMAGE_HEIGHT 768
#define FILENAME "./image_input_RGB_768_1024_03.data" 

int main() {
    FILE *f_out = fopen(FILENAME, "wb");
    if (f_out == NULL) {
        perror("Error opening output file");
        return 1;
    }

    printf("Generating a %dx%d RGB raw image file: %s\n", IMAGE_WIDTH, IMAGE_HEIGHT, FILENAME);

    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            uint8_t r, g, b;

    
            r = (uint8_t)((float)x / (IMAGE_WIDTH - 1) * 255);
            g = (uint8_t)((float)y / (IMAGE_HEIGHT - 1) * 255);
            b = (uint8_t)(((float)x / (IMAGE_WIDTH - 1) + (float)y / (IMAGE_HEIGHT - 1)) / 2.0 * 255);

            fputc(r, f_out);
            fputc(g, f_out);
            fputc(b, f_out);
        }
    }

    fclose(f_out);
    printf("Image generation complete.\n");
    return 0;
}
