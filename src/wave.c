#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include "wave.h"

/* specifically to convert sawtooth to <whatever> */

int main(int argc, char** argv) {
    int exitcode = 0;
    if (argc < 2) {
        if (!wave_fh(stdin, stdout)) {
            exitcode = 1;
        }
    } else {
        for (int i = 1; i < argc; i += 1) {
            char* output_filename = gen_output_filename(argv[i]);
            if (!wave_file(argv[i], output_filename)) {
                exitcode = 1;
            }
            free(output_filename);
        }
    }
}

int wave_file(char* filename, char* output_filename) {
    FILE* fpin;
    FILE* fpout;
    if (NULL == (fpin = fopen(filename, "rb"))) {
        perror(filename);
        return 0;
    }
    if (NULL == (fpout = fopen(output_filename, "wb"))) {
        fclose(fpin);
        perror(filename);
        return 0;
    }
    int result = wave_fh(fpin, fpout);
    fclose(fpin);
    fclose(fpout);
    return result;
}

int wave_fh(FILE* fpin, FILE* fpout) {
    chunk_t riff_type_chunk;
    if (!read_chunk(fpin, fpout, &riff_type_chunk, /* header_only */ 1)) {
        return 0;
    }
    if (strcmp(riff_type_chunk.id, "RIFF")) {
        fprintf(stderr, "unsupported file: chunk id is %s\n", riff_type_chunk.id);
        return 0;
    }
    char riff_type[5];
    if (4 > read(fileno(fpin), riff_type, 4)) {
        perror("wave_fh: read riff_type");
        return 0;
    }
    if (4 > write(fileno(fpout), riff_type, 4)) {
        perror("wave_fh: write riff_type");
        return 0;
    }
    riff_type[4] = '\0';
    if (strcmp(riff_type, "WAVE")) {
        fprintf(stderr, "unsupported RIFF type: %s\n", riff_type);
        return 0;
    }

    chunk_t raw_chunk;
    chunk_t raw_fmt_chunk;
    chunk_t fmt_chunk;
    while (read_chunk(fpin, fpout, &raw_chunk, /* header_only */ 0) > 0) {
        if (feof(fpin)) {
            break;
        }
        if (!strcmp(raw_chunk.id, "fmt ")) {
            raw_fmt_chunk = raw_chunk;
            fmt_chunk     = raw_chunk;
            if (!wave_fmt_chunk(&fmt_chunk, &raw_fmt_chunk)) {
                return 0;
            }
        } else if (!strcmp(raw_chunk.id, "data")) {
            chunk_t raw_out_chunk = raw_chunk;
            if (!wave_data_chunk(&raw_chunk, &raw_out_chunk, &fmt_chunk)) {
                return 0;
            }
            if (raw_out_chunk.datasize > write(fileno(fpout),
                                               raw_out_chunk.data,
                                               raw_out_chunk.datasize)) {
                perror("wave_fh: write data chunk\n");
                return 0;
            }
            free(raw_out_chunk.data);
        }
        free(raw_chunk.data);
    }
    return 1;
}

#define SAMPLE_T uint32_t

#define POP 4
#define POP2 8
#define CLIP 0.99

int wave_data_chunk(chunk_t* raw_chunkp, chunk_t* raw_out_chunkp, chunk_t* fmt_chunkp) {
    uint32_t* raw_out_data = malloc(raw_out_chunkp->datasize);
    if (raw_out_data == NULL) {
        perror("malloc raw_out_data");
        exit(1);
    }
    int32_t* out_data = malloc(raw_out_chunkp->datasize);
    if (out_data == NULL) {
        perror("malloc out_data");
        exit(1);
    }
    int32_t* in_data = malloc(raw_out_chunkp->datasize);
    if (in_data == NULL) {
        perror("malloc in_data");
        exit(1);
    }

    uint32_t* raw_in_data  = (uint32_t*)(raw_chunkp->data);
    fmt_data_t* fmt_datap = (fmt_data_t*)(fmt_chunkp->data);

    raw_out_chunkp->data = (char*)raw_out_data;

    int sigbytes = (fmt_datap->sigbits + 7) / 8; /* bytes per sample */
    uint32_t nsamples = raw_chunkp->datasize / sigbytes;

    for (size_t i = 0; i < nsamples; i += 1) {
        in_data[i] = (int32_t)(bytes_to_uint_32(raw_in_data[i]));
    }

    char graph[34];
    for (size_t i = 0; i < nsamples; i += 1) {
        float f;
        if (in_data[i] < (-INT32_MAX / POP) || in_data[i] > (INT32_MAX / POP)) {
            f = 0;
        } else if (in_data[i] >= (-INT32_MAX / POP2) && in_data[i] <= INT32_MAX / POP2) {
            f = (float)in_data[i] / (INT32_MAX / POP2);
            f = f < -1.0f ? -1.0f : f > 1.0f ? 1.0f : f;
            f = f * (float)M_PI_2;
            f = sin(f);
        } else if (in_data[i] > INT32_MAX / POP2) {
            f = (float)in_data[i] / (INT32_MAX / POP);
            f = f * 2.0f;
            f = f - 0.5f;
            f = f * (float)M_PI;
            f = 0.5f + 0.5f * sin(f);
        } else if (in_data[i] < (-INT32_MAX / POP2)) {
            f = (float)in_data[i] / (INT32_MAX / POP);
            f = f * 2.0f;
            f = f + 0.5f;
            f = f * (float)M_PI;
            f = -0.5f + 0.5f * sin(f);
        }
        out_data[i] = (int32_t)(f * CLIP * INT32_MAX);
    }

    for (size_t i = 0; i < nsamples; i += 1) {
        raw_out_data[i] = uint_32_to_bytes((uint32_t)(out_data[i]));
    }

    free(in_data);
    free(out_data);
    return 1;
}

int read_chunk(FILE* fpin, FILE* fpout, chunk_t* raw_chunkp, int header_only) {
    ssize_t bytes;

    /* read chunk id */
    if (4 > (bytes = read(fileno(fpin), raw_chunkp->id, 4))) {
        if (bytes == 0) {
            return 0;
        }
        perror("read_chunk: read id");
        return 0;
    }
    if (4 > write(fileno(fpout), raw_chunkp->id, 4)) {
        perror("read_chunk: write id");
        return 0;
    }
    raw_chunkp->id[4] = '\0';

    /* read chunk data size */
    if (4 > read(fileno(fpin), &(raw_chunkp->raw_datasize), 4)) {
        perror("read_chunk: read raw_datasize");
        return 0;
    }
    if (4 > write(fileno(fpout), &(raw_chunkp->raw_datasize), 4)) {
        perror("read_chunk: write raw_datasize");
        return 0;
    }
    raw_chunkp->datasize = bytes_to_uint_32(raw_chunkp->raw_datasize);

    if (header_only) {
        /* caller will read data */
        raw_chunkp->data = NULL;
        return 1;
    }
    if (NULL == (raw_chunkp->data = malloc(raw_chunkp->datasize))) {
        perror("read_chunk: malloc");
        return 0;
    }
    if (raw_chunkp->datasize > read(fileno(fpin), raw_chunkp->data, raw_chunkp->datasize)) {
        perror("read_chunk: read");
        return 0;
    }
    if (strcmp(raw_chunkp->id, "data")) { /* not "data" */
        if (raw_chunkp->datasize > write(fileno(fpout), raw_chunkp->data, raw_chunkp->datasize)) {
            perror("read_chunk: write");
            return 0;
        }
    } /* otherwise, caller will write data */
    return 1;
}

int wave_fmt_chunk(chunk_t* chunkp, chunk_t* raw_chunkp) {
    fmt_data_t* raw_datap = (fmt_data_t*)raw_chunkp->data;
    fmt_data_t* datap     = (fmt_data_t*)chunkp->data;
    datap->compression = bytes_to_uint_16(raw_datap->compression); /* 65534 */
    datap->channels    = bytes_to_uint_16(raw_datap->channels);    /* 1 */
    datap->rate        = bytes_to_uint_32(raw_datap->rate);        /* 48000 */
    datap->bps         = bytes_to_uint_32(raw_datap->bps);         /* 192000 */
    datap->align       = bytes_to_uint_16(raw_datap->align);       /* 4 */
    datap->sigbits     = bytes_to_uint_16(raw_datap->sigbits);     /* 32 */
    int supported = 1;
    if (datap->compression != 65534) {
        fprintf(stderr, "ERROR: unsupported compression type %d\n", datap->compression);
        supported = 0;
    }
    if (datap->channels != 1) {
        fprintf(stderr, "ERROR: unsupported channel count %d\n", datap->channels);
        supported = 0;
    }
    if (datap->align != 4) {
        fprintf(stderr, "ERROR: unsupported bytes per sample slice %d\n", datap->align);
        supported = 0;
    }
    if (datap->sigbits != 32) {
        fprintf(stderr, "ERROR: unsupported bits per sample %d\n", datap->sigbits);
        supported = 0;
    }
    if (!supported) {
        exit(1);
    }
    fprintf(stderr, "compression type %d\n", datap->compression);
    fprintf(stderr, "channel count %d\n", datap->channels);
    fprintf(stderr, "sample rate %d\n", datap->rate);
    fprintf(stderr, "average bytes per second %d\n", datap->bps);
    fprintf(stderr, "bytes per sample slice %d\n", datap->align);
    fprintf(stderr, "significant bits per sample %d\n", datap->sigbits);
    return 1;
}

char* gen_output_filename(char* filename) {
    char suffix[] = ".out";
    char len = strlen(filename);
    char* output_filename = malloc(len + sizeof(suffix));
    if (output_filename == NULL) {
        return NULL;
    }
    strcpy(output_filename, filename);
    char* ext = NULL;
    char* basename = output_filename;
    int i;
    for (i = len - 1; i >= 0; i -= 1) {
        if (output_filename[i] == '/' || output_filename[i] == '\\') {
            basename = output_filename + i + 1;
            break;
        }
        if (output_filename[i] == '.' && ext == NULL) {
            ext = output_filename + i;
        }
    }
    if (ext == basename) {
        ext = NULL;
    }
    if (ext == NULL) {
        strcpy(basename + strlen(basename), suffix);
        return output_filename;
    }
    char* newext = ext + sizeof(suffix) - 1;
    memmove(ext + strlen(suffix), ext, strlen(ext) + 1);
    memcpy(ext, suffix, strlen(suffix));
    return output_filename;
}

/* convert little-endian byte streams to integers, agnostic of platform endianness */
extern inline uint32_t bytes_to_uint_32(uint32_t i) {
    uint8_t* data = (uint8_t*)(&i);
    uint32_t result = (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    return result;
}
extern inline uint16_t bytes_to_uint_16(uint16_t i) {
    uint8_t* data = (uint8_t*)(&i);
    return (data[0] << 0) | (data[1] << 8);
}
/* convert integers to little-endian byte streams to integers, agnostic of platform endianness */
extern inline uint32_t uint_32_to_bytes(uint32_t i) {
    uint8_t* data = (uint8_t*)(&i);
    data[0] = (i & 0x000000ff) >> 0;
    data[1] = (i & 0x0000ff00) >> 8;
    data[2] = (i & 0x00ff0000) >> 16;
    data[3] = (i & 0xff000000) >> 24;
    return i;
}
extern inline uint16_t uint_16_to_bytes(uint16_t i) {
    uint8_t* data = (uint8_t*)(&i);
    data[0] = (i & 0x00ff) >> 0;
    data[1] = (i & 0xff00) >> 8;
    return i;
}
