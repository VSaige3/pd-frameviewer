#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

int main(int args, char **argv);
int get_id_and_size(FILE *file, uint32_t *id, uint32_t *size);
int goto_next_chunk(FILE *file, uint32_t size, long *offset);
uint16_t get_anim_count(FILE *handle);
uint16_t read_anim_lens(FILE *file, float *buf);