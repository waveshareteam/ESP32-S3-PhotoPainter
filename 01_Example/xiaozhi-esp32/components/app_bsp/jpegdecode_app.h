#pragma once

#pragma pack(push, 1) // Ensure that the structure is aligned at 1 byte intervals.

typedef struct {
    uint16_t bfType;      // File type must be "BM"
    uint32_t bfSize;      // File size
    uint16_t bfReserved1; // Retain
    uint16_t bfReserved2; // Retain
    uint32_t bfOffBits;   // Pixel data offset
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;          // Size of this structure
    int32_t  biWidth;         // Image width
    int32_t  biHeight;        // Image height (positive value indicates reverse storage)
    uint16_t biPlanes;        // Must be 1
    uint16_t biBitCount;      // Bit depth per pixel, here it is 24
    uint32_t biCompression;   // Compression method, 0 = BI_RGB
    uint32_t biSizeImage;     // Image data size (can be 0)
    int32_t  biXPelsPerMeter; // Horizontal resolution
    int32_t  biYPelsPerMeter; // Vertical resolution
    uint32_t biClrUsed;       // Number of used color indices
    uint32_t biClrImportant;  // Number of important colors
} BITMAPINFOHEADER;


#pragma pack(pop)



class JpegDecodeDither
{
private:
    const char *TAG = "JpegPort";
    int JpegPort_NearestColor(uint8_t r, uint8_t g, uint8_t b);
public:
    JpegDecodeDither();
    ~JpegDecodeDither();

    uint8_t JpegPort_OnePicture(uint8_t *inbuffer, int inlen, uint8_t **outbuffer, int *outlen);
    void JpegPort_BufferFree(uint8_t *buffer);
    void JpegPort_DitherRgb888(uint8_t *in_img, uint8_t *out_img, int w, int h);
    int JpegPort_EncodingBmpToSdcard(const char *filename, const uint8_t *inRgb, int width, int height);
};
