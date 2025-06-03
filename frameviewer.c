#include "frameviewer.h"
#include <dirent.h>
#include <sys/stat.h>
#include <windows.h>

WINDOW *option_window, *viewer_window;
complete_framedata *comp_fdata;
uint32_t selected_skill_index;
enum skill_search_method default_search_method;
char *assets_path;
bool skill_already_selected, char_already_selected;

int main(int args, char **argv) {
    comp_fdata = NULL;
    get_gsdata_path(dirname(argv[0]));
    create_windows();
    start_mainloop();
    teardown();
}

bool is_valid_search_method(char c) {
    return (c == 'h' || c == 's');
}

enum skill_search_method get_search_method(char c) {
    switch(c) {
        case 's': return SKILL_NAME;
        default:
        case 'h': return HEX_INDEX;
    }
}

void get_gsdata_path(char *our_dir) {
    char path[PATH_MAX + 1];
    char *new_assets_path = malloc(PATH_MAX);
    strncpy(path, our_dir, PATH_MAX);
    strcat(path, "/");
    strcat(path, CONFIG_PATH);
    // check if exists
    FILE *config = fopen(path, "r+");
    bool f_exist = true;

    if (!config) {
        f_exist = false;
        // Assume file does not exist
        config = fopen(path, "w");
        if (!config) {
            printf("Can't create file %s, aborting...\n", path);
            exit(errno);
        } else {
            printf("Successfully created config file\n");
        }
    }
    if (!f_exist || fscanf(config, "%260s\n", new_assets_path) == EOF) {
        printf("Missing or empty config file, enter path to Phantom Dust \"Assets\": \n");
        scanf("%260s\n", new_assets_path);
        fprintf(config, "%s\n", new_assets_path);
    }

    char method = ' ';
    if (!f_exist || fscanf(config, "%c\n", &method) == EOF || !is_valid_search_method(method)) {
        bool bad_option = false;
        do {
            printf("Missing or empty skill search method (%c), enter one to continue (h for hex skill index, s for skill name): \n", method);
            char method;
            scanf("%c\n", &method);
            if (!is_valid_search_method(method)) {
                printf("Unreconized option\n");
                bad_option = true;
            }
        } while (bad_option);
        fprintf(config, "%c\n", method);
    }

    assets_path = new_assets_path;
    default_search_method = get_search_method(method);
    fclose(config);
}

void create_windows() {
    WINDOW *m = initscr();
    if (!m) {
        printf("Could not initialize ncurses, your terminal may not support it\n");
        teardown();
        exit(1);
    }
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, false);
    nodelay(stdscr, true);
    touchwin(stdscr);
    int maxx = getmaxx(stdscr);
    int maxy = getmaxy(stdscr);
    if (maxx <= 30 || maxy <= 30)
	{
	    printf("Console window not large enough, aborting...\n");
        teardown();
	    exit(1);
	}
    option_window = derwin(stdscr, 10, maxx, 0, 0);
    viewer_window = derwin(stdscr, maxy - 10, maxx, 10, 0);
    if (has_colors() == TRUE)
    {
        start_color();
        /* Define color pairs */
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_CYAN, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    }

    wrefresh(option_window);
    wrefresh(viewer_window);
}

enum {
    SELECTING_CHAR,
    SELECTING_SKILL,
    VIEWING_FRAMEDATA,
    COMPARING_CHARS
} state;

void start_mainloop() {
    state = SELECTING_CHAR;
    while (mainloop());
    teardown();
}

void create_selection(WINDOW *window, int opt_per_row, int row_spacing, char **options, int opt_count, int selected_option) {
    for (int i = 0; i < opt_count; i ++) {
        // getchar();
        if (i == selected_option) {
            wcolor_set(window, 4, NULL);
            mvwprintw(window, i % opt_per_row, (i / opt_per_row) * row_spacing, options[i]);
            wcolor_set(window, 0, NULL);
        } else {
            mvwprintw(window, i % opt_per_row, (i / opt_per_row) * row_spacing, options[i]);
        }
    }
    wrefresh(window);
}

int run_selection(WINDOW *window, int opt_per_row, int row_spacing, char **options, int opt_count, int selected_option) {
    int last_key;
    noecho();
    create_selection(window, opt_per_row, row_spacing, options, opt_count, selected_option);
    while ((last_key = getch()) != ' ') {
        if (last_key == ERR) continue;
        if (last_key == 'w' && selected_option > 0) selected_option --;
        if (last_key == 's' && selected_option < opt_count - 1) selected_option ++;
        if (last_key == 'a' && (selected_option / opt_per_row) > 0) selected_option -= opt_per_row;
        if (last_key == 'd' && (selected_option) < ((opt_count - opt_per_row))) selected_option += opt_per_row;
        create_selection(window, opt_per_row, row_spacing, options, opt_count, selected_option);
    }
    return selected_option;
}

// returns true if you must retry 
bool create_hex_index_search(void) {
    echo();
    mvwprintw(option_window, 0, 0, "Enter skill index (hex):\n");
    wrefresh(option_window);
    char outstr[5];
    move(1,0);
    wgetnstr(option_window, outstr, 4);
    uint32_t n_skills;
    long raw_index = strtol(outstr, NULL, 0x10);
    if (errno) {
        wcolor_set(viewer_window, 2, NULL);
        mvwprintw(viewer_window, 1, 0, "Error: %s", strerror(errno));
        wcolor_set(viewer_window, 0, NULL);
        wrefresh(viewer_window);
        return true;
    } else if (raw_index < 0 || raw_index > UINT32_MAX) {
        wcolor_set(viewer_window, 2, NULL);
        mvwprintw(viewer_window, 1, 0, "Error: value %ld out of range for an unsigned int", raw_index);
        wcolor_set(viewer_window, 0, NULL);
        wrefresh(viewer_window);
        return true;
    } else if (raw_index > (n_skills = get_num_skills(comp_fdata->gsdata))) {
        wcolor_set(viewer_window, 2, NULL);
        mvwprintw(viewer_window, 1, 0, "Error: skills only go up to %d", n_skills);
        wcolor_set(viewer_window, 0, NULL);
        wrefresh(viewer_window);
        return true;
    }
    selected_skill_index = (uint32_t) raw_index;
    return false;
}

bool create_skill_name_search(void) {
    echo();
    mvwprintw(option_window, 0, 0, "Enter full or part skill name:\n");
    wrefresh(option_window);
    char outstr[41];
    move(1,0);
    wgetnstr(option_window, outstr, 40);
    int entered_len = strlen(outstr);
    uint16_t total_skills_matched = 0;
    uint16_t displayed_skills_matched = 0;
    const uint16_t max_displayed_skills = 40;
    uint32_t matching_indices[max_displayed_skills];
    uint32_t num_skills = get_num_skills(comp_fdata->gsdata);
    char *options[max_displayed_skills];
    for (uint32_t i = 0; i < num_skills; i ++) {
        char namebuf[30];
        if (get_text_for_skill(comp_fdata->gsdata, i, namebuf, NULL)) {
            int diff = strncasecmp(namebuf, outstr, entered_len);
            if (!diff) {
                if (total_skills_matched < max_displayed_skills) {
                    size_t option_len = 30 + strlen(namebuf);
                    options[total_skills_matched] = malloc(option_len + 1);
                    short id = *(short *)(comp_fdata->gsdata + 0x1A + i * 0x90);
                    snprintf(options[total_skills_matched], option_len, "0x%X: %s (ID: %d)", i, namebuf, id);
                    matching_indices[total_skills_matched] = i;
                    displayed_skills_matched ++;
                }
                total_skills_matched ++;
            }
        }
    }
    if (!displayed_skills_matched) return true;
    if (displayed_skills_matched == 1) {
        selected_skill_index = matching_indices[0];
    } else {
        int last_key;
        int opt_per_row = 10;
        int row_spacing = 40;
        int selected_option = 0;
        wclear(option_window);
        // wprintw(option_window, "skills found: %d/%d", displayed_skills_matched, total_skills_matched);
        // wrefresh(option_window);
        selected_option = run_selection(option_window, opt_per_row, row_spacing, options, displayed_skills_matched, selected_option);
        selected_skill_index = matching_indices[selected_option];
    }
    while (displayed_skills_matched) free (options[--displayed_skills_matched]);
    return false;
}

bool mainloop() {
    int last_key;
    int opt_per_row = 5;
    int row_spacing = 30;
    switch (state) {
        case SELECTING_CHAR:
            if (char_already_selected) {
                state = SELECTING_SKILL;
                return true;
            }
            char *options[0x40];
            char path[0x40];
            int opt_count = 0;
            // read directory
            struct dirent *dir;
            DIR *d;
            snprintf(path, 0x40, "%s/", FDATA_PATH);
            d = opendir(path);
            size_t fdata_len = strlen(path);
            struct stat file_stat;
            if (d) {
                while (dir = readdir(d)) {
                    strcpy(path + fdata_len, dir->d_name);
                    stat(path, &file_stat);
                    if (S_ISREG(file_stat.st_mode)) {
                        if (opt_count < 0x40) {
                            options[opt_count] = malloc(dir->d_namlen + 1);
                            strncpy(options[opt_count], dir->d_name, dir->d_namlen);
                            options[opt_count][dir->d_namlen] = 0;
                            opt_count ++;
                        }
                    }
                }
                closedir(d);
            }
            int selected_option = 0;
            wclear(option_window);
            selected_option = run_selection(option_window, opt_per_row, row_spacing, options, opt_count, selected_option);
            // use this option to load stuff
            snprintf(path, 0x40, "%s/%s", FDATA_PATH, options[selected_option]);
            comp_fdata = load_fdata_for_player(path);
            if (!comp_fdata) return false;
            state = SELECTING_SKILL;
            return true;
            break;
        case SELECTING_SKILL:
            if (skill_already_selected) {
                state = VIEWING_FRAMEDATA;
                return true;
            }
            wclear(option_window);
            switch(default_search_method) {
                case HEX_INDEX:
                    if (create_hex_index_search()) return true;
                    break;
                case SKILL_NAME:
                    if (create_skill_name_search()) return true;
                    break;
            }
            state = VIEWING_FRAMEDATA;
            return true;
            break;
        case VIEWING_FRAMEDATA:
        case COMPARING_CHARS:
            if (state == VIEWING_FRAMEDATA) {
                create_formatted_skill_diplay(viewer_window, selected_skill_index, comp_fdata);
                display_framedata(viewer_window, 5, 0, selected_skill_index, comp_fdata);
            } else {
                create_and_display_char_deltas(viewer_window, selected_skill_index);
            }
            wrefresh(viewer_window);
            const int num_options = 5;
            char *myoptions[num_options];
            
            myoptions[0] = "Select new skill";
            myoptions[1] = "Select new character";
            myoptions[2] = "Select new skill and character";
            myoptions[3] = "Compare across characters";
            myoptions[4] = "Quit";
            selected_option = 0;
            wclear(option_window);
            selected_option = run_selection(option_window, opt_per_row, row_spacing, myoptions, num_options, selected_option);
            // create_selection(option_window, num_options, 0, myoptions, num_options, selected_option);
            // while ((last_key = getch()) != ' ') {
            //     if (last_key == ERR) continue;
            //     if ((last_key == 'w' || last_key == 'a') && selected_option > 0) selected_option --;
            //     if ((last_key == 's' || last_key == 'd') && selected_option < num_options) selected_option ++;
            //     create_selection(option_window, num_options, 0, myoptions, num_options, selected_option);
            // }
            wclear(viewer_window);
            switch (selected_option) {
                case 0:
                    state = SELECTING_SKILL;
                    skill_already_selected = false;
                    return true;
                case 1:
                    state = SELECTING_CHAR;
                    skill_already_selected = true;
                    return true;
                case 2:
                    state = SELECTING_CHAR;
                    char_already_selected = false;
                    skill_already_selected = false;
                    return true;
                case 3:
                    state = COMPARING_CHARS;
                    return true;
                case 4:
                    return false;
            }
            break;
    }
    return false;
}

bool get_text_for_skill(void *gsdata, uint32_t skill_index, char *name_out, char *desc_out) {
    int text_offset = *(int *)(gsdata + 0x3400C);
    void *str_start = gsdata + 0x34004 + text_offset;
    int *text_dat_start = (int *)(gsdata + 0x34018);
    int num_text_entries = *(int *)(gsdata + 0x34008);

    // get text id for our skill
    short text_id = *(short *)(gsdata + 0x18 + 0x90 * skill_index);
    // find the text in our list
    for (int i = 0; i < num_text_entries; i ++) {
        int curr_id = text_dat_start[0];
        if (curr_id == text_id) {
            if (name_out) strcpy(name_out, &str_start[text_dat_start[1]]);
            if (desc_out) strcpy(desc_out, &str_start[text_dat_start[2]]);
            return true;
        }
        text_dat_start += 3;
    }
    return false;
}

char *str_cap_type(int cap_type) {
    switch (cap_type) {
        case 0: return "Aura";
        case 1: return "Attack";
        case 2: return "Defense";
        case 3: return "Erase";
        case 4: return "Status";
        case 5: return "Special";
        case 6: return "Environment";
        default: return "Unknown";
    }
    return NULL;
}

char *str_school(int school) {
    switch (school) {
        case 0: return "Aura";
        case 1: return "Psycho";
        case 2: return "Optical";
        case 3: return "Nature";
        case 4: return "Ki";
        case 5: return "Faith";
        default: return "Unknown";
    }
    return NULL;
}

void create_formatted_skill_diplay(WINDOW *window, uint32_t skill_index, complete_framedata *fdata) {
    void *curr_skill = (fdata->gsdata + 0x18 + skill_index * 0x90);
    char name[0x20], desc[0x80];
    bool has_text = get_text_for_skill(fdata->gsdata, skill_index, name, desc);
    short cap_type = *(short *)(curr_skill + 0xE);
    short school = *(short *)(curr_skill + 0x12);
    short skill_id = *(short *)(curr_skill + 0x2);
    if (has_text) {
        wclear(window);
        wcolor_set(window, 3, NULL);
        mvwprintw(window, 0, 0, "Name: %s;\t ID: %d;\t Capsule Type: %s;\t School: %s", name, skill_id, str_cap_type(cap_type), str_school(school));
        mvwprintw(window, 1, 0, "Desc: %s", desc);
        wcolor_set(window, 0, NULL);
    } else {
        wclear(window);
        mvwprintw(window, 0, 0, "DATA CORRUPTED OR UNAVAILALBLE");
    }
}

//// UTILITY METHODS
long get_fsize(FILE *f) {
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    return size;
}

void teardown() {
    if (comp_fdata) free_complete_framedata(comp_fdata);
    if (assets_path) free(assets_path);
    endwin();
}

void create_fdata_diagram(WINDOW *window, int y, int x, float total_frames, uint16_t delay, uint16_t active) { // TODO: add more
    wmove(window, y, x);
    wcolor_set(window, COLOR_PAIR_STARTUP, NULL);
    int i = 0;
    for (; i < delay; i ++)
        waddch(window, '=');
    
    wcolor_set(window, COLOR_PAIR_ACTIVE, NULL);
    for (; i < active; i ++)
        waddch(window, '+');
    
    wcolor_set(window, COLOR_PAIR_RECOVERY, NULL);
    for (; i < (uint16_t)total_frames; i ++)
        waddch(window, '=');
    
    wcolor_set(window, 0, NULL);
}

void display_framedata(WINDOW *window, int y, int x, uint32_t skill_index, complete_framedata *fdata) {
    uint16_t g_prefab = get_prefab_for_skill(fdata, skill_index);
    uint16_t g_anim = get_anim_for_prefab(fdata, g_prefab);
    float g_dur = get_duration_for_anim(fdata, g_anim);
    
    uint16_t a_prefab = get_air_prefab_for_skill(fdata, skill_index);
    uint16_t a_anim = get_anim_for_prefab(fdata, a_prefab);
    float a_dur = get_duration_for_anim(fdata, a_anim);

    display_framedata_ex(window, y, x, skill_index, fdata, (int)g_dur, (int)a_dur);
}

void display_framedata_ex(WINDOW *window, int y, int x, uint32_t skill_index, complete_framedata *fdata, int g_dur, int a_dur) {
    enum use_restrict use_res = get_use_restrict(fdata->gsdata, skill_index);
    void *curr_skill = (fdata->gsdata + 0x18 + skill_index * 0x90);

    if (use_res != AIR_ONLY) {
        uint16_t g_prefab = get_prefab_for_skill(fdata, skill_index);
        uint16_t g_anim = get_anim_for_prefab(fdata, g_prefab);
        int g_total_frames = g_dur;
        uint16_t g_delay = get_delay_for_prefab(fdata, g_prefab);
        uint16_t g_active = get_melee_end_frame_for_prefab(fdata, g_prefab);
        mvwprintw(window, y, x, "GROUNDED: Prefab %X, animation %X; Length: %d, Startup: %d, Recovery: %d, Active Until: %d",
            g_prefab, g_anim, g_total_frames, g_delay, g_total_frames - g_delay, g_active);
        create_fdata_diagram(window, y + 1, x, g_total_frames, g_delay, g_active);
        y += 3;
    }
    if (use_res != GROUND_ONLY) {
        uint16_t a_prefab = get_air_prefab_for_skill(fdata, skill_index);
        uint16_t a_anim = get_anim_for_prefab(fdata, a_prefab);
        int a_total_frames = a_dur;
        uint16_t a_delay = get_delay_for_prefab(fdata, a_prefab);
        uint16_t a_active = get_melee_end_frame_for_prefab(fdata, a_prefab);
        mvwprintw(window, y, x, "AIR: Prefab %X, animation %X; Length: %d, Startup: %d, Recovery: %d, Active Until: %d", 
            a_prefab, a_anim, a_total_frames, a_delay, a_total_frames - a_delay, a_active);
        create_fdata_diagram(window, y + 1, x, a_total_frames, a_delay, a_active);
    }
}

void create_and_display_char_deltas(WINDOW *window, uint32_t skill_index) {
    const int max_datas = 7;
    const int max_str_len = getmaxx(viewer_window) - 5;
    char names[max_datas][max_str_len + 1];
    int lens[max_datas][2];
    memset(lens, 0, sizeof(lens));
    int num_diff = 0;

    uint16_t g_anim = get_anim_for_prefab(comp_fdata, get_prefab_for_skill(comp_fdata, skill_index));
    bool has_air = get_use_restrict(comp_fdata->gsdata, skill_index);
    uint16_t a_anim = has_air ? get_anim_for_prefab(comp_fdata, get_air_prefab_for_skill(comp_fdata, skill_index)) : 0;

    char path[PATH_MAX];
    snprintf(path, 0x40, "%s/", FDATA_PATH);
    DIR *d = opendir(path);
    struct dirent *dir;
    size_t fdata_len = strlen(path);
    struct stat file_stat;
    if (d) {
        while (dir = readdir(d)) {
            strcpy(path + fdata_len, dir->d_name);
            stat(path, &file_stat);
            if (S_ISREG(file_stat.st_mode)) {
                if (num_diff < max_datas) {
                    load_durations_for_player(path, comp_fdata);
                    bool same = false;
                    int g_len = (int)get_duration_for_anim(comp_fdata, g_anim);
                    int a_len = (int)get_duration_for_anim(comp_fdata, a_anim);
                    for (int i = 0; i < num_diff; i ++) {
                        if (lens[i][0] == g_len && (!has_air || lens[i][1] == a_len)) {
                            size_t old_len = strlen(names[i]);
                            size_t new_len = dir->d_namlen + strlen(names[i]) + 2;
                            same = true;
                            if (new_len > max_str_len) {
                                if (old_len + 4 > max_str_len) memcpy(&names[i][old_len - 4], ",...", 5);
                                else strcat(names[i], ",...");
                            } else {
                                strcat(names[i], ", ");
                                strcat(names[i], dir->d_name);
                            }
                            break;
                        }
                    }
                    if (!same) {
                        strncpy(names[num_diff], dir->d_name, max_str_len);
                        lens[num_diff][0] = g_len;
                        lens[num_diff][1] = a_len;
                        num_diff ++;
                    }
                }
            }
        }
        closedir(d);
        // for now just print
        create_formatted_skill_diplay(viewer_window, skill_index, comp_fdata);
        for (int i = 0; i < num_diff; i ++) {
            int y = 7 * i + 4;
            if (num_diff == 1)
                mvwprintw(viewer_window, y, 0, "All:");
            else
                mvwprintw(viewer_window, y, 0, "%s:", names[i]);
            display_framedata_ex(viewer_window, y + 1, 0, skill_index, comp_fdata, lens[i][0], lens[i][1]);
        }
        if (num_diff == max_datas) {
            mvwprintw(viewer_window, 7 * max_datas + 4, 0, "== Too Many To Display ==");
        }
        wrefresh(viewer_window);
        getch();
    }
}

/**
 * Loads framedata for a given binary fdata path
 * Must be one that's processed through the scraper
 */
complete_framedata *load_fdata_for_player(char *bin_fdata_path) {
    // time to do this the hard way
    // read all of gsdata into our buffer
    char pathbuf[MAX_PATH];
    void *gsdata_buf = malloc(0x44803);
    if (!gsdata_buf) return NULL;
    strncpy(pathbuf, assets_path, MAX_PATH / 2);
    strncat(pathbuf, GSDATA_REL_PATH, MAX_PATH / 2);
    FILE *gsdat_file = fopen(pathbuf, "rb");
    if (!gsdat_file) {
        printf("Could not open gsdata file at path %s\nreason: \"%s\"\nquitting...\n", pathbuf, strerror(errno));
        free(gsdata_buf);
        teardown();
        exit(-1);
    }
    fread(gsdata_buf, 0x44004, 1, gsdat_file);
    fclose(gsdat_file);

    complete_framedata *fdat = calloc(1, sizeof(complete_framedata));

    if (!load_durations_for_player(bin_fdata_path, fdat)) return 0;

    fdat->gsdata = gsdata_buf;

    return fdat;
}

bool load_durations_for_player(char *bin_fdata_path, complete_framedata *fdat) {
    if (!fdat) return false;

    FILE *fdata_h = fopen(bin_fdata_path, "rb");
    long fsize = get_fsize(fdata_h);
    if (fdat->anim_to_duration) free(fdat->anim_to_duration);
    fdat->anim_to_duration = malloc(fsize);
    fread(fdat->anim_to_duration, fsize, 1, fdata_h);
    fclose(fdata_h);

    return true;
}

void free_complete_framedata(complete_framedata *fdat) {
    free(fdat->anim_to_duration);
    free(fdat->gsdata);
    free(fdat);
}

uint16_t get_prefab_for_skill(complete_framedata *fdat, uint32_t skill_index) {
    return *(short *)((fdat->gsdata + 0x18) + 0x14 + 0x90 * skill_index);
}

uint16_t get_air_prefab_for_skill(complete_framedata *fdat, uint32_t skill_index) {
    return *(short *)((fdat->gsdata + 0x18) + 0x16 + 0x90 * skill_index);
}

float get_duration_for_anim(complete_framedata *fdat, uint16_t anim_index) {
    return fdat->anim_to_duration[anim_index].duration;
}

uint16_t get_anim_for_prefab(complete_framedata *fdat, uint16_t prefab_index) {
    return *(uint16_t *)((fdat->gsdata + 0x1B00C) + 0x4C + 0x72 * prefab_index);
}

uint16_t get_delay_for_prefab(complete_framedata *fdat, uint16_t prefab_index) {
    return *(uint16_t *)((fdat->gsdata + 0x1B00C) + 0x56 + 0x72 * prefab_index);
}

uint16_t get_melee_end_frame_for_prefab(complete_framedata *fdat, uint16_t prefab_index) {
    return *(uint16_t *)((fdat->gsdata + 0x1B00C) + 0x58 + 0x72 * prefab_index);
}

enum use_restrict get_use_restrict(void *gsdata, uint32_t skill_index) {
    return *(short *)((gsdata + 0x18) + 0x4C + 0x90 * skill_index);
}

bool is_melee(complete_framedata *fdat, uint16_t prefab_index) {
    return false;
}

uint32_t get_num_skills(void *gsdata) {
    return *(uint32_t *)(gsdata + 0x8);
}