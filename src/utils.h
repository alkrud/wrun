#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>
#include "list.h"

#include "raylib.h"

#define Vec2(x_, y_) ((Vector2){.x = x_, .y = y_})
#define is_alpha(c) (             \
    ((c) >= 'a' && (c) <= 'z') || \
    ((c) >= 'A' && (c) <= 'Z') || \
    ((c) == '_')                  \
)
#define is_num(c) ((c) >= '0' && (c) <= '9')
#define is_alnum(c) (is_alpha(c) || is_num(c))
#define is_space(c) ((c) == ' ' || (c) == '\r' || (c) == '\t' || (c) == '\n')
#define to_lower(c) ((c) >= 65 && (c) <= 90 ? (c) + 32 : (c))
#define min(i, j) ((i) < (j) ? (i) : (j))
#define max(i, j) ((i) > (j) ? (i) : (j))
#define swap(x, y, t) \
    do {              \
        t temp = (x); \
        (x) = (y);    \
        (y) = temp;   \
    } while (0)

LIST_DEF(String, char);
LIST_DEF(StringList, String);
LIST_DEF(CstrList, char*);

static inline void pause(bool (*resume_checker)(void))
{
    while (!resume_checker()) {
        WaitTime(0.5f);
    }

    return;
}

static inline void create_window(int width, int height, const char *title, int fps)
{
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);
    SetExitKey(KEY_BACKSPACE);
    InitWindow(width, height, title);
    SetWindowTitle("wrun");
    SetTargetFPS(fps);
    int current_monitor = GetCurrentMonitor();
    int monitor_width = GetMonitorWidth(current_monitor);
    int monitor_height = GetMonitorHeight(current_monitor);
    SetWindowPosition((monitor_width / 2) - (width / 2), (monitor_height / 2) - (height / 2));
}

static inline void print_fraction(char *output, double d) // Stole
{
    char buf[
        1 + // sign, '-' or '+'
            (sizeof(d) * CHAR_BIT + 3) / 4 + // mantissa hex digits max
            1 + // decimal point, '.'
            1 + // mantissa-exponent separator, 'p'
            1 + // mantissa sign, '-' or '+'
            (sizeof(d) * CHAR_BIT + 2) / 3 + // exponent decimal digits max
            1 // string terminator, '\0'
    ];
    int n;
    char *pp = NULL, *p = NULL;
    int e, lsbFound, lsbPos;

    // convert d into "+/- 0x h.hhhh p +/- ddd" representation and check for errors
    if ((n = snprintf(buf, sizeof(buf), "%+a", d)) < 0 ||
            (unsigned)n >= sizeof(buf)) {
        lsbPos = 0;
        goto END_OF_FUNC;
    }

    if (strstr(buf, "0x") != buf + 1 ||
        (pp = strchr(buf, 'p')) == NULL) {
        lsbPos = 0;
        goto END_OF_FUNC;
    }

    // extract the base-2 exponent manually, checking for overflows
    e = 0;
    p = pp + 1 + (pp[1] == '-' || pp[1] == '+'); // skip the exponent sign at first
    for (; *p != '\0'; p++)
    {
        if (e > INT_MAX / 10) {
            lsbPos = 0;
            goto END_OF_FUNC;
        }
        e *= 10;
        if (e > INT_MAX - (*p - '0')) {
            lsbPos = 0;
            goto END_OF_FUNC;
        }
        e += *p - '0';
    }
    if (pp[1] == '-') // apply the sign to the exponent
        e = -e;

    // find the position of the least significant non-zero bit
    lsbFound = lsbPos = 0;
    for (p = pp - 1; *p != 'x'; p--)
    {
        if (*p == '.')
            continue;
        if (!lsbFound)
        {
            int hdigit = (*p >= 'a') ? (*p - 'a' + 10) : (*p - '0'); // assuming ASCII chars
            if (hdigit)
            {
                static const int lsbPosInNibble[16] = { 0,4,3,4,  2,4,3,4, 1,4,3,4, 2,4,3,4 };
                lsbFound = 1;
                lsbPos = -lsbPosInNibble[hdigit];
            }
        }
        else
        {
            lsbPos -= 4;
        }
    }
    lsbPos += 4;

    if (!lsbFound) {
        lsbPos = 0; // d is 0 (integer)
        goto END_OF_FUNC;
    }

    // adjust the least significant non-zero bit position
    // by the base-2 exponent (just add them), checking
    // for overflows

    if (lsbPos >= 0 && e >= 0) {
        lsbPos = 0; // lsbPos + e >= 0, d is integer
        goto END_OF_FUNC;
    }

    if (lsbPos < 0 && e < 0) {
        if (lsbPos < INT_MIN - e) {
            lsbPos = -2; // d isn't integer and needs too many fractional digits
            goto END_OF_FUNC;
        }
    }

    if ((lsbPos += e) >= 0) {
        lsbPos = 0; 
        goto END_OF_FUNC;
    }

    if (lsbPos == INT_MIN && -INT_MAX != INT_MIN) {
        lsbPos = -2; // d isn't integer and needs too many fractional digits
        goto END_OF_FUNC;
    }
    
END_OF_FUNC:
    assert(-lsbPos >= 0);
    sprintf(output, "%.*f", -lsbPos < 15 ? -lsbPos : 14, d);
}

bool str_contains(char* str, size_t size, char c);
void str_copy(char *dest, size_t dest_size, char *src, size_t src_size);
int partition(CstrList *arr1, String *arr2, int low, int high);
void quickSort(CstrList *list, String *buffer, int low, int high);
