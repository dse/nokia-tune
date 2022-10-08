#ifndef WAVE_H
#define WAVE_H

typedef struct chunk {
    char id[5];
    uint32_t raw_datasize;
    uint32_t datasize;          /* number of samples * number of channels * bytes per channel */
    char* data;
} chunk_t;

typedef struct fmt_data {
    uint16_t compression;
    uint16_t channels;          /* 1 or 2, usually */
    uint32_t rate;              /*  */
    uint32_t bps;
    uint16_t align;
    uint16_t sigbits;           /*  */
} fmt_data_t;

#define CMPR_UNKNOWN      0
#define CMPR_PCM          1
#define CMPR_MS_ADPCM     2
#define CMPR_A_LAW        6
#define CMPR_MU_LAW       7
#define CMPR_IMA_ADPCM    17
#define CMPR_G723_ADPCM   20
#define CMPR_GSM_610      49
#define CMPR_G721_ADPCM   64
#define CMPR_MPEG         80
#define CMPR_EXPERIMENTAL 0xffff

int wave_file(char*, char*);
int wave_fh(FILE*, FILE*);
int read_chunk(FILE*, FILE*, chunk_t*, int);
inline uint32_t bytes_to_uint_32(uint32_t);
inline uint16_t bytes_to_uint_16(uint16_t);
inline uint32_t uint_32_to_bytes(uint32_t);
inline uint16_t uint_16_to_bytes(uint16_t);
int wave_fmt_chunk(chunk_t*, chunk_t*);
int wave_data_chunk(chunk_t*, chunk_t*, chunk_t*);
char* gen_output_filename(char*);
#endif
