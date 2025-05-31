#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <curses.h>
#include <libgen.h>

#define CONFIG_PATH "config.dat"

#define COLOR_PAIR_ERR 1
#define COLOR_PAIR_SELECTED 2
#define COLOR_PAIR_STARTUP 3
#define COLOR_PAIR_ACTIVE 4
#define COLOR_PAIR_RECOVERY 5

enum use_restrict {
    GROUND_ONLY, AIR_ONLY, BOTH
};

enum skill_search_method {
    HEX_INDEX, SKILL_NAME
};

// typedef struct {
//     uint16_t *prefab_indx;
// } skill_to_prefab_map;

typedef struct {
    float duration;
} anim_to_duration_map;

// typedef struct {
//     struct {
//         uint16_t skeletal;
//         uint16_t delay;
//         int16_t melee_duration;
//     } *entries;
// } prefab_data;

// typedef struct {
//     skill_to_prefab_map *skill_to_prefab;
//     anim_to_duration_map *anim_to_duration;
//     prefab_data *prefab_dat;
// } complete_framedata;


typedef struct {
    anim_to_duration_map *anim_to_duration;
    void *gsdata;
} complete_framedata;

void create_windows();
void start_mainloop();
bool mainloop();
void teardown();
void free_complete_framedata(complete_framedata *fdat);

float get_duration_for_anim(complete_framedata *fdat, uint16_t anim_index);
uint16_t get_prefab_for_skill(complete_framedata *fdat, uint32_t skill_id);
uint16_t get_air_prefab_for_skill(complete_framedata *fdat, uint32_t skill_id);
uint16_t get_anim_for_prefab(complete_framedata *fdat, uint16_t prefab_index);
uint16_t get_delay_for_prefab(complete_framedata *fdat, uint16_t prefab_index);
uint16_t get_melee_end_frame_for_prefab(complete_framedata *fdat, uint16_t prefab_index);
uint32_t get_num_skills(void *gsdata);
complete_framedata *load_fdata_for_player(char *bin_fdata_path);
void create_formatted_skill_diplay(WINDOW *window, uint32_t skill_index, complete_framedata *fdata);
void display_framedata(WINDOW *window, int y, int x, uint32_t skill_index, complete_framedata *fdata);
bool get_text_for_skill(void *gsdata, uint32_t skill_index, char *name_out, char *desc_out);

enum use_restrict get_use_restrict(void *gsdata, uint32_t skill_index);

void get_gsdata_path(char *dir);