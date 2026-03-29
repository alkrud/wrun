#ifndef _CRT_SECURE_NO_DEPRECATE
    #define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#define SIMPLE_CALC_IMPLEMENTATION
#include "simple_calc.h"
#include "raylib.h"
#include "utils.h"
#include "list.h"
#include "strmap.h"
#include "bins.h"

#define invalid_chars "\\/:*?\"<>|"
#define invalid_chars_len (array_len(invalid_chars) - 1)
#define STR_ARENA_SIZE 1000
char STR_ARENA[STR_ARENA_SIZE];

typedef struct {
    String buffer;
    int cursor;
} TextBox;

typedef struct {
    Bins *bins;
    TextBox input;
    StrMap aliases;
    Font font;
    int font_size;
    int height;
    int width;
} App;

typedef struct {
    KeyboardKey key;
    float cooldown; // current cooldown
    float init_cooldown; // how much it waits before starting to repeat
    float hold_cooldown; // how much it waits after starting to repeat
    float elapsed; // how much time between pressses
    bool pressed;
    void (*norm)(App*); // without control
    void (*ctrl)(App*); // with control
} ButtonHandler;

FORCE_INLINE char* str_to_charptr(String *str) 
{
    if (str->count > STR_ARENA_SIZE) return NULL;

    char *ptr = &STR_ARENA[0];

    for (size_t i = 0; i < str->count; i++) {
        ptr[i] = str->items[i];
    }

    ptr[str->count] = '\0';


    return ptr;
}

void refresh_bins(App *app, char* *selected);

FORCE_INLINE void move_cursor_left(App *app)
{
    if (app->input.cursor > 0) app->input.cursor -= 1;
}

FORCE_INLINE void move_cursor_right(App *app)
{
    if (app->input.cursor < app->input.buffer.count) app->input.cursor += 1;
}

FORCE_INLINE void move_cursor_left_word(App *app)
{
    if (app->input.cursor <= 0 || app->input.buffer.count <= 0) return;

    if (is_space(app->input.buffer.items[app->input.cursor - 1])) {
        for (int i = app->input.cursor - 1; i >= 0 && is_space(app->input.buffer.items[i]); i--) {
            move_cursor_left(app);
        }
    } 
    for (int i = app->input.cursor - 1; i >= 0 && !is_space(app->input.buffer.items[i]); i--) {
        move_cursor_left(app);
    }
}

FORCE_INLINE void move_cursor_right_word(App *app)
{
    if (app->input.cursor >= app->input.buffer.count || app->input.buffer.count <= 0) return;

    if (is_space(app->input.buffer.items[app->input.cursor])) {
        for (int i = app->input.cursor; i < app->input.buffer.count && is_space(app->input.buffer.items[i]); i++) {
            move_cursor_right(app);
        }
    } 
    for (int i = app->input.cursor; i < app->input.buffer.count && !is_space(app->input.buffer.items[i]); i++) {
        move_cursor_right(app);
    }
}

FORCE_INLINE void delete_char(App *app)
{
    if (app->input.buffer.count <= 0 || app->input.cursor <= 0) return;
    list_remove(&app->input.buffer, app->input.cursor - 1);
    move_cursor_left(app);

    refresh_bins(app, NULL);
}

FORCE_INLINE void delete_word(App *app)
{
    if (app->input.buffer.count <= 0 || app->input.cursor <= 0) return;

    if (is_space(app->input.buffer.items[app->input.buffer.count - 1])) {
        while (app->input.buffer.count > 0 && is_space(app->input.buffer.items[app->input.cursor - 1])) {
            list_remove(&app->input.buffer, app->input.cursor - 1);
            move_cursor_left(app);
        }
    } 
    while (app->input.buffer.count > 0 && !is_space(app->input.buffer.items[app->input.cursor - 1])) {
        list_remove(&app->input.buffer, app->input.cursor - 1);
        move_cursor_left(app);
    }

    refresh_bins(app, NULL);
}

FORCE_INLINE void add_bins(char *path, Bins *bins)
{
    if (!DirectoryExists(path)) return;

    FilePathList flist = LoadDirectoryFilesEx(path, ".exe", false);        

    for (size_t j = 0; j < flist.count; j++) {
        char *cur_path = flist.paths[j];
        int start = -1;
        int end = -1;

        for (int k = strlen(cur_path) - 1; k >= 0; k--) {
            if (cur_path[k] == '.' && end == -1) {
                end = k;
            }
            else if ((cur_path[k] == '\\' || cur_path[k] == '/') && start == -1) {
                start = k + 1;
            }
            if (start != -1 && end != -1) {
                break;
            }
        }

        size_t len = end - start;
        if (len <= 0) continue;

        char *bin = malloc(sizeof(char) * (len + 1));
        memcpy(bin, &cur_path[start], len);
        bin[len] = '\0';

        list_push(get_strlist(bins, bin[0]), bin);
    }

    UnloadDirectoryFiles(flist);
}

void* get_bins(void *_list)
{
    Bins *bins = (Bins*)_list;

    char *Path = getenv("PATH");

	char *cur_path = Path; 

    for (size_t i = 0; Path[i] != '\0'; i++) {
        if (Path[i] != ';') continue;

        Path[i] = '\0';
    
        add_bins(cur_path, bins);

        cur_path = &Path[i + 1];
    }

    add_bins(cur_path, bins);

    return NULL;
}

bool read_key_value(char* *key, char* *value, FILE *f)
{
    int i = 0;
    char buffer[100] = {0};

    while (!feof(f) && i < 100) {
        buffer[i] = getc(f);

        if (is_space(buffer[i]) && buffer[i] != ' ') continue;

        if (buffer[i] == ':') {
            int size1 = i + 1;
            *key = (char*)malloc(sizeof(char) * size1);
            (*key)[size1 - 1] = '\0';
            buffer[i] = '\0';
            // printf("buffer1: %s\n", buffer);
            strcpy(*key, &buffer[0]);
            i++;
            int start = i;
            while (i < 100) {
                buffer[i] = getc(f);

                if (buffer[i] == '\n' || feof(f)) {
                    int size2 = (i - start) + 1;
                    *value = (char*)malloc(sizeof(char) * size2);
                    (*value)[size2 - 1] = '\0';
                    buffer[i] = '\0';
                    // printf("buffer2: %s\n", &buffer[start]);
                    strcpy(*value, &buffer[start]);

                    return true;
                }
                i++;
            }
        }

        i++;
    }

    return false;
}

StrMap import_aliases(void)
{
    const char *home_env = getenv("HOME");
    char aliases_path[100]; 
    sprintf(aliases_path, "%s%s", home_env, "/.aliases.txt");
    StrMap map = strmap_new();

    if (!FileExists(aliases_path)) return map;

    FILE *f = fopen(aliases_path, "r");
    char *key;
    char *value;

    while (read_key_value(&key, &value, f)) { 
        strmap_add(&map, key, value);
    }

    fclose(f);

    return map;
}

void refresh_bins(App *app, char* *selected)
{
    if (app->input.buffer.count <= 0) return;

    CstrList *list;
    if (str_contains(invalid_chars, invalid_chars_len, app->input.buffer.items[0]) && app->input.buffer.count > 1) {
        list = get_strlist(app->bins, app->input.buffer.items[1]);
    }
    else {
        list = get_strlist(app->bins, app->input.buffer.items[0]);
    }

    if (list->count <= 0 || app->input.buffer.count <= 0) return;

    quickSort(list, &app->input.buffer, 0, list->count - 1);
    if (selected != NULL) *selected = list->items[0];
}


void handle_button(App *app, ButtonHandler *btn, float frame_time)
{
    bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

#define EXECUTE()                                                                \
    do {                                                                         \
        if (control) btn->ctrl(app);                                             \
        else btn->norm(app);                                                     \
        btn->elapsed = 0;                                                        \
    } while (0)

    if (IsKeyDown(btn->key)) {
        if (btn->pressed) {
            if (btn->elapsed > btn->cooldown) {
                EXECUTE();
                btn->cooldown = btn->hold_cooldown;
            }
        }
        else {
            EXECUTE();
            btn->cooldown = btn->init_cooldown;
        }
        btn->pressed = true;
    }
    else {
        btn->pressed = false;
    }

    btn->elapsed += frame_time;

#undef EXECUTE
}

int main(void)
{
    SetTraceLogLevel(LOG_NONE); 
    
    App app = {0};
    app.width= 300;
    app.height = 300;
    app.bins = bins_alloc();
    app.input = (TextBox){0};
    app.aliases = import_aliases();
    pthread_t thread;
    pthread_create(&thread, NULL, get_bins, app.bins);
    pthread_detach(thread);
    create_window(app.width, app.height, "", 60);
    // FilePathList flist = LoadDirectoryFilesEx("C:\\Windows\\Fonts", ".ttf", false);        
    // UnloadDirectoryFiles(flist);
    // app.font = LoadFontEx(flist.paths[0], app.font_size, NULL, 0);
    // printf("Font: %s\n", flist.paths[0]);
    app.font_size = 30;
#if defined (__WIN32__)
    app.font = LoadFontEx("C:\\Windows\\Fonts\\cascadia.ttf", app.font_size, NULL, 0);
#else
    app.font = LoadFontEx("/usr/share/fonts/CascadiaMono/static/CascadiaMono-Regular.ttf", app.font_size, NULL, 0);
#endif
    if (!IsFontValid(app.font)) {
        app.font = GetFontDefault();
    }
    ButtonHandler l_arrow = {.key = KEY_LEFT, .init_cooldown = 0.2, .hold_cooldown = 0.05, .norm = move_cursor_left, .ctrl = move_cursor_left_word};
    ButtonHandler r_arrow = {.key = KEY_RIGHT, .init_cooldown = 0.2, .hold_cooldown = 0.05, .norm = move_cursor_right, .ctrl = move_cursor_right_word};
    ButtonHandler backspc = {.key = KEY_BACKSPACE, .init_cooldown = 0.3, .hold_cooldown = 0.06, .norm = delete_char, .ctrl = delete_word};
    char *selected = "";
    char calc_buffer[256];

    while (!WindowShouldClose()) {
        char c;
        if ((c = GetCharPressed()) != 0) {
            list_insert(&app.input.buffer, c, app.input.cursor);
            app.input.cursor += 1;
            refresh_bins(&app, &selected);
        }

        char *value = strmap_get(&app.aliases, str_to_charptr(&app.input.buffer));
        if (value != NULL) {
            selected = value;
        }

        if (app.input.buffer.count > 1 && app.input.buffer.items[0] == '=') {
            double result = sc_calculate(&app.input.buffer.items[1], (int)app.input.buffer.count - 1);
            print_fraction(calc_buffer, result);
            selected = calc_buffer;
        }

        bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

        switch (GetKeyPressed()) {
            case KEY_ENTER: {
                if (app.input.buffer.count <= 0) break;

                static char temp[256];
                if (app.input.buffer.items[0] == '/' && app.input.buffer.count > 1) {
                    sprintf(temp, "%.*s", (int)app.input.buffer.count - 1, &app.input.buffer.items[1]);
                    system(temp);
                }
                else if (app.input.buffer.items[0] == '=') {
                    SetClipboardText(selected);
                }
                else {
#if defined (__WIN32__)
                    sprintf(temp, "start /b %s", selected);
                    system(temp);
#else
                    if (fork() == 0) {
                        execl(selected, "selected", NULL);
                    }
#endif
                }

                list_clear(&app.input.buffer);

                goto PROGRAM_END;
            }
            case KEY_V: {
                if (!control) break;
                const char *clipboard = GetClipboardText();

                for (size_t i = 0; i < strlen(clipboard); i++) {
                    list_insert(&app.input.buffer, clipboard[i], app.input.cursor);
                    app.input.cursor += 1;
                }
                refresh_bins(&app, &selected);
                break;
            }
        }

        float frame_time = GetFrameTime();
        
        handle_button(&app, &l_arrow, frame_time);
        handle_button(&app, &r_arrow, frame_time);
        handle_button(&app, &backspc, frame_time);

        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));
        {
            const float spacing = 0.8f;
            const char *c_buffer = str_to_charptr(&app.input.buffer);
            const Vector2 k = MeasureTextEx(app.font, selected, app.font_size, spacing);
            const char *measure_string = "abcdefghijklmnopqrstuvwxyz0123456789";
            const size_t measure_string_length = strlen(measure_string);
            const float width_per_char = MeasureTextEx(app.font, measure_string, app.font_size, spacing).x / (float)measure_string_length;
            const float text_offset = 20.f;

            DrawRectangle(15, 15, app.width - 30, 45, DARKGRAY);
            DrawRectangle(15, 65, app.width - 30, 35, GRAY);
            DrawRectangle((width_per_char * app.input.cursor) + text_offset, 15, 5, 45, LIGHTGRAY);

            if (app.input.buffer.count <= 0) goto END_DRAWING;

            int start = 70;
            int showed = 6;

            if (k.x > app.width - 35) {
                int last_char = floor((app.width - 35) / width_per_char);
                int mag = ceil(k.x / (app.width - 35.f) + 0.1);
                char temp[100];
                for (int i = 1; i < mag; i++) {
                    DrawRectangle(15, 65 + (i * 33), app.width - 30, 35, GRAY);
                    start += 33;
                    showed--;
                    char *text = &selected[last_char * (i - 1)];
                    sprintf(temp, "%.*s", last_char, text);
                    DrawTextEx(app.font, temp, Vec2(20, 67 + ((i - 1) * 35)), app.font_size, spacing, WHITE);
                }
                char *text = &selected[last_char * (mag - 1)];
                DrawTextEx(app.font, text, Vec2(20, 67 + ((mag - 1) * 35)), app.font_size, spacing, WHITE);
            }
            else {
                DrawTextEx(app.font, selected, Vec2(text_offset, 67), app.font_size, spacing, WHITE);
            }

            CstrList *bin_list = get_strlist(app.bins, selected[0]);
            DrawTextEx(app.font, c_buffer, Vec2(20, 25), app.font_size, spacing, WHITE);
            for (int i = 1; i < (int)bin_list->count && i <= showed; i++) {
                DrawTextEx(app.font, bin_list->items[i - 1], Vec2(20, start + (33 * i)), app.font_size, spacing, WHITE);
            }

        }
END_DRAWING:
        EndDrawing();
    }

PROGRAM_END:
    /* pthread_join(thread, NULL); */
    CloseWindow();
}
