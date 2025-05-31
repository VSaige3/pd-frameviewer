#include "scraper.h"
float buf[0x100];

int main(int args, char **argv) {
    if (args <= 1) {
        printf("too few arguments, usage:\nscraper.exe <path1> <path2> ...\n");
        return -1;
    }
    for (int i = 1; i < args; i ++) {
        char *fname = argv[i];
        FILE *f = fopen(fname, "rb");
        int err;
        if (err = ferror(f)) {
            printf("Failed to read file \"%s\", error %d\n", fname, err);
            return err;
        }
        uint16_t anim_count = get_anim_count(f);
        printf("File \"%s\" has 0x%X animations\n", fname, anim_count);
        if (anim_count >= 0x100) {
            printf("Too many animations (%X), check file for corruption\n", anim_count);
            fclose(f);
        } else {
            read_anim_lens(f, buf);
            fclose(f);
            char *bname = basename(fname);
            strtok(bname, ".");
            char *our_dir = dirname(argv[0]);
            char outflpath[_MAX_PATH];
            strcpy(outflpath, our_dir);
            strcat(outflpath, "/");
            strcat(outflpath, bname);
            strcat(outflpath, "_fdata");
            FILE *outf = fopen(outflpath, "wb");
            if (err = ferror(outf)) {
                printf("Failed to write output file: %s\n", strerror(err));
            } else {
                fwrite(buf, sizeof(float), anim_count, outf);
                fclose(outf);
            }
        }

    }
}

int get_id_and_size(FILE *file, uint32_t *id, uint32_t *size) {
    fread(id, 4, 1, file);
    fread(size, 4, 1, file);
    return feof(file);
}

int get_id_size_and_duration(FILE *file, uint32_t *id, uint32_t *size, float *duration) {
    fread(id, 4, 1, file);
    fread(size, 4, 1, file);
    fread(duration, 4, 1, file);
    printf("duration: %f\n", *duration);
    return feof(file);
}

int goto_next_chunk(FILE *file, uint32_t size, long *offset) {
    long n_offset = *offset + size; 
    *offset = n_offset;
    return fseek(file, n_offset, SEEK_SET);
}

uint16_t get_anim_count(FILE *handle) {
    if (handle == 0) return 0;
    uint16_t count = 0;
    fseek(handle, 0, SEEK_SET);
    uint32_t id, size;
    long c_offset = 0;
    // skip past our first 2
    get_id_and_size(handle, &id, &size);
    goto_next_chunk(handle, size, &c_offset);

    get_id_and_size(handle, &id, &size);
    goto_next_chunk(handle, size, &c_offset);
    // now everything should be 0x05 or 0x00
    while (feof(handle) != -1) {
        get_id_and_size(handle, &id, &size);
        if (id == 0) count ++;
        else if (id != 0x05) break;
        goto_next_chunk(handle, size, &c_offset);
    }
    return count;
}

uint16_t read_anim_lens(FILE *file, float *buf) {
    if (file == 0) return 0;
    uint16_t count = 0;
    fseek(file, 0, SEEK_SET);
    uint32_t id, size;
    long c_offset = 0;
    // skip past our first 2
    get_id_and_size(file, &id, &size);
    goto_next_chunk(file, size, &c_offset);

    get_id_and_size(file, &id, &size);
    goto_next_chunk(file, size, &c_offset);
    // now everything should be 0x05 or 0x00
    while (feof(file) != -1) {
        get_id_and_size(file, &id, &size);
        if (id == 5) {
            fread(&buf[count], sizeof(float), 1, file);
        }
        else if (id == 0) {
            count ++;
        } else break;
        goto_next_chunk(file, size, &c_offset);
    }
    return count;
}