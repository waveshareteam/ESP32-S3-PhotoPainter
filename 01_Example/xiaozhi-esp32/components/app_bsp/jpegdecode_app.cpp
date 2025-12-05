#include <stdio.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include "jpegdecode_app.h"
#include "test_decoder.h"

#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

static const uint8_t PALETTE[6][3] = {
    {0, 0, 0},       // Black
    {255, 255, 255}, // White
    {255, 0, 0},     // Red
    {0, 255, 0},     // Green
    {0, 0, 255},     // Blue
    {255, 255, 0}    // Yellow
};

JpegDecodeDither::JpegDecodeDither() {

}

JpegDecodeDither::~JpegDecodeDither() {

}

uint8_t JpegDecodeDither::JpegPort_OnePicture(uint8_t *inbuffer, int inlen, uint8_t **outbuffer, int *outlen) {
    if (inbuffer == NULL) {
        ESP_LOGE(TAG, "jpeg_decode fill inbuffer is NULL");
        return 0;
    }
    if (esp_jpeg_decode_one_picture(inbuffer, inlen, outbuffer, outlen) == JPEG_ERR_OK) {
        return 1;
    }
    return 0;
}

void JpegDecodeDither::JpegPort_BufferFree(uint8_t *buffer) {
    if (buffer != NULL) {
        jpeg_free_align(buffer);
        buffer = NULL;
    }
}

void JpegDecodeDither::JpegPort_DitherRgb888(uint8_t *in_img, uint8_t *out_img, int w, int h) {
    uint8_t *work = (uint8_t *) malloc(w * h * 3);
    if (!work)
        return;
    for (int i = 0; i < w * h * 3; i++)
        work[i] = in_img[i];

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int     idx = (y * w + x) * 3;
            uint8_t r   = work[idx + 0];
            uint8_t g   = work[idx + 1];
            uint8_t b   = work[idx + 2];

            // Find the nearest color
            int     ci = JpegPort_NearestColor(r, g, b);
            uint8_t rr = PALETTE[ci][0];
            uint8_t gg = PALETTE[ci][1];
            uint8_t bb = PALETTE[ci][2];

            // Output result
            out_img[idx + 0] = rr;
            out_img[idx + 1] = gg;
            out_img[idx + 2] = bb;

            // Error
            int err_r = (int) r - rr;
            int err_g = (int) g - gg;
            int err_b = (int) b - bb;

            // Floyd–Steinberg diffusion
            //     *   7
            // 3   5   1
            if (x + 1 < w) {
                int n       = idx + 3;
                work[n + 0] = CLAMP(work[n + 0] + (err_r * 7) / 16, 0, 255);
                work[n + 1] = CLAMP(work[n + 1] + (err_g * 7) / 16, 0, 255);
                work[n + 2] = CLAMP(work[n + 2] + (err_b * 7) / 16, 0, 255);
            }
            if (y + 1 < h) {
                if (x > 0) {
                    int n       = ((y + 1) * w + (x - 1)) * 3;
                    work[n + 0] = CLAMP(work[n + 0] + (err_r * 3) / 16, 0, 255);
                    work[n + 1] = CLAMP(work[n + 1] + (err_g * 3) / 16, 0, 255);
                    work[n + 2] = CLAMP(work[n + 2] + (err_b * 3) / 16, 0, 255);
                }
                int n       = ((y + 1) * w + x) * 3;
                work[n + 0] = CLAMP(work[n + 0] + (err_r * 5) / 16, 0, 255);
                work[n + 1] = CLAMP(work[n + 1] + (err_g * 5) / 16, 0, 255);
                work[n + 2] = CLAMP(work[n + 2] + (err_b * 5) / 16, 0, 255);

                if (x + 1 < w) {
                    int n2       = ((y + 1) * w + (x + 1)) * 3;
                    work[n2 + 0] = CLAMP(work[n2 + 0] + (err_r * 1) / 16, 0, 255);
                    work[n2 + 1] = CLAMP(work[n2 + 1] + (err_g * 1) / 16, 0, 255);
                    work[n2 + 2] = CLAMP(work[n2 + 2] + (err_b * 1) / 16, 0, 255);
                }
            }
        }
    }

    free(work);
}

int JpegDecodeDither::JpegPort_EncodingBmpToSdcard(const char *filename, const uint8_t *inRgb, int width, int height) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen");
        return -1;
    }

    // Each line must be aligned at 4-byte intervals (as required by BMP)
    int row_stride = (width * 3 + 3) & ~3;
    int img_size   = row_stride * height;

    // Construct the file header
    BITMAPFILEHEADER file_header;
    file_header.bfType      = 0x4D42; // 'BM'
    file_header.bfSize      = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + img_size;
    file_header.bfReserved1 = 0;
    file_header.bfReserved2 = 0;
    file_header.bfOffBits   = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Construction of information header
    BITMAPINFOHEADER info_header;
    memset(&info_header, 0, sizeof(info_header));
    info_header.biSize        = sizeof(BITMAPINFOHEADER);
    info_header.biWidth       = width;
    info_header.biHeight      = height; // Positive numbers = Stored in reverse order (from bottom to top)
    info_header.biPlanes      = 1;
    info_header.biBitCount    = 24;
    info_header.biCompression = 0; // BI_RGB
    info_header.biSizeImage   = img_size;

    // Write the file header and information header
    fwrite(&file_header, sizeof(file_header), 1, f);
    fwrite(&info_header, sizeof(info_header), 1, f);

    // Write pixel data (BMP requires BGR order, each row is aligned at 4 bytes, and written in reverse order)
    uint8_t *row_buf = (uint8_t *) malloc(row_stride);
    if (!row_buf) {
        fclose(f);
        return -1;
    }

    for (int y = 0; y < height; y++) {
        int            src_row = height - 1 - y; // 倒序
        const uint8_t *src     = inRgb + src_row * width * 3;

        // 转 RGB888 -> BGR888
        for (int x = 0; x < width; x++) {
            row_buf[x * 3 + 0] = src[x * 3 + 2]; // B
            row_buf[x * 3 + 1] = src[x * 3 + 1]; // G
            row_buf[x * 3 + 2] = src[x * 3 + 0]; // R
        }
        // Fill-aligned bytes
        for (int p = width * 3; p < row_stride; p++) {
            row_buf[p] = 0;
        }
        fwrite(row_buf, 1, row_stride, f);
    }

    free(row_buf);
    fclose(f);
    return 0;
}

// Find the closest color from the palette (RGB888)
int JpegDecodeDither::JpegPort_NearestColor(uint8_t r, uint8_t g, uint8_t b) {
    int best      = 0;
    int best_dist = 999999;

    for (int i = 0; i < 6; i++) {
        int dr   = (int) r - PALETTE[i][0];
        int dg   = (int) g - PALETTE[i][1];
        int db   = (int) b - PALETTE[i][2];
        int dist = dr * dr + dg * dg + db * db;
        if (dist < best_dist) {
            best_dist = dist;
            best      = i;
        }
    }
    return best;
}