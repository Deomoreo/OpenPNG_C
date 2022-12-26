#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "zlib.h"
#include "LinkedListElements.h"

#define PNG_SIGNATURE "\x89PNG\r\n\x1a\n"
#define CHUNK_TYPE_SIZE 4
#define CHUNK_DATA_BUFFER_SIZE 4096

// Use a smaller data type for the chunk length field
typedef struct {
    unsigned char type[CHUNK_TYPE_SIZE];
    unsigned long length; 
    unsigned char data[CHUNK_DATA_BUFFER_SIZE];
    unsigned int crc;
} chunk_t;

// Initialize variables to store image data
typedef struct
{
    uint32_t height;
    uint32_t width;
    uint8_t bit_depth;
    uint8_t color_type;
    uint8_t compression_method;
    uint8_t filter_method;
    uint8_t interlace_method;
} ihdr_info;

typedef struct
{
    struct list_node node;
    chunk_t *chunk;
} chunk_list_t;

static chunk_list_t *chunk_list_new(chunk_t *chunk)
{
    chunk_list_t *item = malloc(sizeof(chunk_list_t));
    if (!item) return NULL;
    item->chunk = chunk;
    return item;
}

static unsigned int data_to_int(unsigned char* data_int, size_t data_length)
{
    if (data_length > 4) return 0;

    unsigned int value = 0;
    for (size_t i = 0; i < data_length; i++)
    {
        value = (value << 8) | data_int[i];
    }
    return value;
}

unsigned int calculate_crc(chunk_t *chunk) 
{
  return crc32(crc32(0, chunk->type, CHUNK_TYPE_SIZE), chunk->data, chunk->length);
}

static void print_chunk(chunk_t* chunk)
{
    printf("Chunk name: %s\n", chunk->type);
    printf("Chunk data_chunk size: %lu\n", chunk->length);
    printf("Chunk CRC: %u\n", chunk->crc);
}

static chunk_t* read_chunk(FILE **f)
{   
    chunk_t* chunk = (chunk_t*)malloc(sizeof(chunk_t));
    if (!chunk) return NULL;
    // Read the chunk length and type directly into the struct
    unsigned char* four_data = (unsigned char *)malloc(4);
    if (!four_data)
    {
        free(chunk);
        return NULL;
    }
    fread(four_data, 4, 1, *f);
    chunk->length = data_to_int(four_data, 4);
    fread(four_data, 4, 1, *f);
    memcpy(chunk->type, four_data, 4);


    unsigned char *chunk_data_file = (unsigned char *)malloc(chunk->length);
    if (!chunk_data_file)
    {
        free(chunk);
        free(four_data);
        return NULL;
    }
    fread(chunk_data_file,chunk->length,1, *f);
    memcpy(chunk->data, chunk_data_file, chunk->length);
    
    // Calculate the actual CRC of the chunk
    fread(four_data, 4, 1, *f);
    unsigned int expected_crc = data_to_int(four_data, 4);
    unsigned int actual_crc = calculate_crc(chunk);
    // Compare the actual and expected CRC values
    if (expected_crc != actual_crc)
    {
        fprintf(stderr, "Error: chunk checksum failed\n");
        free(chunk);
        free(four_data);
        free(chunk_data_file);
        return NULL;
    }
    chunk->crc = expected_crc;
    free(four_data);
    free(chunk_data_file);
    print_chunk(chunk);
    return chunk;
}

static ihdr_info *parse_ihdr(chunk_t *ihdr) 
{
    unsigned char *four_byte_data = (unsigned char *)malloc(4);
    if (!four_byte_data)
    {
        return NULL;
    }
    ihdr_info *infos = (ihdr_info *)malloc(sizeof(ihdr_info));
    if (!infos)
    {
        return NULL;
    }

    int count = 0;

    //Parsing data
    for (int i = 0; i < 4; i++)
    {
        four_byte_data[i] = ihdr->data[count++];
    }
    infos->width = data_to_int(four_byte_data, 4);
    for (int i = 0; i < 4; i++)
    {
        four_byte_data[i] = ihdr->data[count++];
    }
    infos->height = data_to_int(four_byte_data, 4);

    infos->bit_depth = ihdr->data[count++];
    infos->color_type = ihdr->data[count++];
    infos->compression_method = ihdr->data[count++];
    infos->interlace_method = ihdr->data[count++];
    infos->filter_method = ihdr->data[count++];
    
    if (infos->interlace_method || infos->filter_method || infos->compression_method != 0)
    {
        printf("Adam7 interlace not supported [%u]\n", infos->interlace_method);
        printf("Invalid filter method [%u]\n", infos->filter_method);
        printf("Invalid compression method [%u]\n", infos->compression_method);
        free(infos);
        free(four_byte_data);
        return NULL;
    }
    if (infos->color_type != 6)
    {
        printf("Only true color alpha is supported [%u]", infos->color_type);
        free(infos);
        free(four_byte_data);
        return NULL;
    }
    if (infos->bit_depth != 8)
    {
        printf("Only bit depth of 8 is supported [%u]", infos->bit_depth);
        free(infos);
        free(four_byte_data);
        return NULL;
    }
    if (infos->width == 0 || infos->height == 0)
    {
        printf("Invalid image size [%u, %u]", infos->width, infos->height);
        free(infos);
        free(four_byte_data);
        return NULL;
    }

    free(four_byte_data);
    return infos;
}

static int path_predictor(int a, int b, int c)
{
    int p = a + b - c;
    int pa = p - a;
    int pb = p - b;
    int pc = p - c;
    if (pa * a <= 0 && pa * p <= 0)
        return a;
    else if (pb * b <= 0 && pb * p <= 0)
        return b;
    else
        return c;
}

static int get_reconstructed_value(unsigned char *pixels, unsigned int r, unsigned int c, int stride, int bytes_per_pixels, int filter_type)
{
    if (filter_type == 0) {
        return pixels[r * stride + c];
    } else if (filter_type == 1) {
        return pixels[r * stride + c] + (c >= bytes_per_pixels ? pixels[r * stride + c - bytes_per_pixels] : 0);
    } else if (filter_type == 2) {
        return pixels[r * stride + c] + (r > 0 ? pixels[(r - 1) * stride + c] : 0);
    } else if (filter_type == 3) {
        return pixels[r * stride + c] + ((c >= bytes_per_pixels ? pixels[r * stride + c - bytes_per_pixels] : 0) + (r > 0 ? pixels[(r - 1) * stride + c] : 0)) / 2;
    } else if (filter_type == 4) {
        return pixels[r * stride + c] + path_predictor((c >= bytes_per_pixels ? pixels[r * stride + c - bytes_per_pixels] : 0), (r > 0 ? pixels[(r - 1) * stride + c] : 0), (c >= bytes_per_pixels && r > 0 ? pixels[(r - 1) * stride + c - bytes_per_pixels] : 0));
    } else {
        return pixels[r * stride + c];
    }
}

static void de_filter_generic(unsigned char *pixels, unsigned char *uncompressed_idat, int height, int stride, int bytes_per_pixels) 
{
    int i = 0;
    int filter_type;
    int filt_x, recon_x;
    for (unsigned int r = 0; r < height; r++) {
        filter_type = uncompressed_idat[i++];
        for (unsigned int c = 0; c < stride; c++) {
            filt_x = uncompressed_idat[i++];
            recon_x = filt_x + get_reconstructed_value(pixels, r, c, stride, bytes_per_pixels, filter_type);
            pixels[r * stride + c] = recon_x;
        }
    }
}

static unsigned char *parse_idat(chunk_t *idat, ihdr_info *ihdr_infos) 
{
    int bytes_per_pixels = 4;
    int stride = ihdr_infos->width * bytes_per_pixels;

    // Allocate memory for the uncompressed data and the pixel data once at the beginning of the program
    unsigned long uncompressed_size = ihdr_infos->height * (1 + stride);
    unsigned char *uncompressed_idat = (unsigned char *)malloc(uncompressed_size);
    unsigned char *pixels = (unsigned char * )malloc(ihdr_infos->height * stride);
    idat->length = (unsigned long)idat->length;
    // Use a faster decompression algorithm, such as zlib
    int result = uncompress2(uncompressed_idat, &uncompressed_size, idat->data, &idat->length);
    if (result != 0) {
        printf("Unable to decompress IDAT: error %d\n", result);
        return NULL;
    }
    de_filter_generic(pixels, uncompressed_idat, ihdr_infos->height, stride,bytes_per_pixels);
    return pixels;
}

unsigned char *parse_png(const char *string, int *width, int *height, int *channels)
{
    unsigned char *pixels = NULL;
    // Open the PNG file
    FILE *f;
    errno_t err;
    err = fopen_s(&f,string, "rb");
    if (!f)
    {
        printf("Error opening file");
        return NULL;
    }
    unsigned char *signature_data = (unsigned char *)malloc(8);
    if (!signature_data)
    {
        printf("Could not load memory");
        fclose(f);
        return pixels;
    }
    fread(signature_data, 8, 1, f);
    if (memcmp(signature_data, PNG_SIGNATURE, 8) != 0)
    {
        printf("Invalid PNG signature");
        fclose(f);
        return pixels;
    }
    free(signature_data);
    chunk_list_t *chunk_list = NULL;
    int running = 1;
    while (running)
    {
        chunk_t* chunk = read_chunk(&f);
        if (!chunk)
        {
            break;
        }
        list_append((struct list_node **)&chunk_list, (struct list_node *)chunk_list_new(chunk));
        if (memcmp("IEND", chunk->type, 4) == 0)
        {
            break;
        }

    }

    ihdr_info *ihdr_infos = parse_ihdr(((chunk_list_t *)list_pop((struct list_node **)&chunk_list))->chunk);
    if (!ihdr_infos)
    {
        fclose(f);
        return pixels;
    }

    chunk_list_t *node = (chunk_list_t *)list_pop((struct list_node **)&chunk_list);
    chunk_t *idat = NULL;
    while (node)
    {
        chunk_t *chunk = node->chunk;
        if (memcmp("IDAT", chunk->type, 4) == 0)
        {
            idat = chunk;
            break;
        }
        node = (chunk_list_t *)list_pop((struct list_node **)&chunk_list);
    }
    if (!idat)
    {
        printf("Could not find pixels data");
        fclose(f);
        return pixels;
    }

    //Parse IDAT chunk
    pixels = parse_idat(idat, ihdr_infos);
    if (!pixels)
    {
        fclose(f);
        return pixels;
    }

    *width = ihdr_infos->width;
    *height = ihdr_infos->height;
    *channels = 4;
    free(chunk_list);
    fclose(f);
    return pixels;
}


