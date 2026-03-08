#include "ansi.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ========================================
// GLOBAL VARIABLES DEFINITIONS
// ========================================

// Screen dimensions (calculated at runtime)
int SCREEN_COLS = 0;
int SCREEN_ROWS = 0;

// Screen buffer and cursor state
cell_t **screen = NULL;
uint16_t cursor_row = 0, cursor_col = 0;

// ANSI escape sequence processing
static char esc_buf[ESC_BUF_SIZE];
static uint8_t esc_len = 0;
ansi_color_state_t ansi_color = {.fg = WHITE, .bg = BLACK};

// Paint library image buffer
UDOUBLE Imagesize;
UWORD *BlackImage;

// Standard 8 colors + bright 8 colors (16 total)
static const uint16_t ansi_palette[16] = {
    BLACK,      // 0: Black
    RED,        // 1: Red
    GREEN,      // 2: Green
    YELLOW,     // 3: Yellow
    BLUE,       // 4: Blue
    MAGENTA,    // 5: Magenta
    CYAN,       // 6: Cyan
    WHITE,      // 7: White
    GRAY,       // 8: Bright Black (Gray)
    0xF800,     // 9: Bright Red
    0x07E0,     // 10: Bright Green
    0xFFE0,     // 11: Bright Yellow
    0x001F,     // 12: Bright Blue
    0xF81F,     // 13: Bright Magenta
    0x07FF,     // 14: Bright Cyan
    0xFFFF      // 15: Bright White
};

// ========================================
// ANSI/TERMINAL FUNCTIONS
// ========================================

void clear_screen(void) {
    for (int r = 0; r < SCREEN_ROWS; ++r) {
        for (int c = 0; c < SCREEN_COLS; ++c) {
            screen[r][c].ch = ' ';
            screen[r][c].fg = WHITE;
            screen[r][c].bg = BLACK;
        }
    }
    cursor_row = cursor_col = 0;
    
    // Clear the image buffer too
    Paint_Clear(BLACK);
}

void move_cursor(uint16_t row, uint16_t col) {
    if (row < SCREEN_ROWS) {
        cursor_row = row;
    }
    if (col < SCREEN_COLS) {
        cursor_col = col;
    }
}

// Parse SGR (Select Graphic Rendition) color codes
static void handle_ansi_sgr(const char *params) {
    int p[8] = {0};
    int n = 0;
    const char *s = params;
    while (*s && n < 8) {
        p[n++] = atoi(s);
        s = strchr(s, ';');
        if (!s) {
            break;
        }
        ++s;
    }
    if (n == 0) {
        n = 1;
        p[0] = 0; // Default SGR 0
    }
    for (int i = 0; i < n; ++i) {
        if (p[i] == 0) { // Reset
            ansi_color.fg = WHITE;
            ansi_color.bg = BLACK;
        } else if (p[i] >= 30 && p[i] <= 37) {
            ansi_color.fg = ansi_palette[p[i] - 30];
        } else if (p[i] == 39) {
            ansi_color.fg = WHITE;
        } else if (p[i] >= 40 && p[i] <= 47) {
            ansi_color.bg = ansi_palette[p[i] - 40];
        } else if (p[i] == 49) {
            ansi_color.bg = BLACK;
        } else if (p[i] >= 90 && p[i] <= 97) {
            ansi_color.fg = ansi_palette[p[i] - 90 + 8];
        } else if (p[i] >= 100 && p[i] <= 107) {
            ansi_color.bg = ansi_palette[p[i] - 100 + 8];
        }
        // (Optional: add bold, underline, etc. here)
    }
}

void handle_ansi(const char *seq) {
    if (seq[0] == '[') {
        // SGR: CSI ... m
        char last = seq[strlen(seq) - 1];
        if (last == 'm') {
            char paramstr[ESC_BUF_SIZE] = {0};
            strncpy(paramstr, seq + 1, strlen(seq) - 2); // skip '[' and 'm'
            handle_ansi_sgr(paramstr);
        } else if (seq[1] == '2' && seq[2] == 'J') {
            clear_screen();
        } else if (seq[1] == 'H') {
            move_cursor(0, 0);
        } else if (last == 'b' && seq[1] == '?') {
            // Custom: CSI ?PwmValue b  (e.g. ESC[?80b sets brightness to 80%)
            int pwm = 100;
            sscanf(seq + 2, "%d", &pwm);
            if (pwm < 0) {
                pwm = 0;
            }
            if (pwm > 100) {
                pwm = 100;
            }
            DEV_SET_PWM((uint8_t)pwm);
        } else {
            int row = 0, col = 0;
            if (sscanf(seq + 1, "%d;%dH", &row, &col) == 2) {
                move_cursor(row ? row - 1 : 0, col ? col - 1 : 0);
            }
        }
    }
}

void scroll_up(void) {
    for (int row = 1; row < SCREEN_ROWS; ++row) {
        memcpy(screen[row - 1], screen[row], sizeof(cell_t) * SCREEN_COLS);
    }
    for (int c = 0; c < SCREEN_COLS; ++c) {
        screen[SCREEN_ROWS - 1][c].ch = ' ';
        screen[SCREEN_ROWS - 1][c].fg = WHITE;
        screen[SCREEN_ROWS - 1][c].bg = BLACK;
    }
}

void put_char(char ch) {
    if (ch == '\r') {
        cursor_col = 0;
    } else if (ch == '\n') {
        cursor_row++;
        cursor_col = 0;
        if (cursor_row >= SCREEN_ROWS) {
            scroll_up();
            cursor_row = SCREEN_ROWS - 1;
        }
    } else if (ch >= 32 && ch <= 126) {  // Printable character triggers wrap
        screen[cursor_row][cursor_col].ch = ch;
        screen[cursor_row][cursor_col].fg = ansi_color.fg;
        screen[cursor_row][cursor_col].bg = ansi_color.bg;
        if (cursor_col == SCREEN_COLS - 1) {
            cursor_col = 0;
            cursor_row++;
            if (cursor_row >= SCREEN_ROWS) {
                scroll_up();
                cursor_row = SCREEN_ROWS - 1;
            }
        } else {
            cursor_col++;
        }
    } else {
        // Non-printable, just store as-is and advance (optional: treat as space)
        screen[cursor_row][cursor_col].ch = ' ';
        screen[cursor_row][cursor_col].fg = ansi_color.fg;
        screen[cursor_row][cursor_col].bg = ansi_color.bg;
        if (cursor_col == SCREEN_COLS - 1) {
            cursor_col = 0;
            cursor_row++;
            if (cursor_row >= SCREEN_ROWS) {
                scroll_up();
                cursor_row = SCREEN_ROWS - 1;
            }
        } else {
            cursor_col++;
        }
    }
}

void process_char(char ch) {
    static bool in_esc = false;
    static bool in_csi = false;
    if (in_esc) {
        if (!in_csi && (ch == '[')) {
            in_csi = true;
            esc_buf[0] = '[';
            esc_len = 1;
            return;
        }
        if (in_csi) {
            if (esc_len < ESC_BUF_SIZE - 1) {
                esc_buf[esc_len++] = ch;
                esc_buf[esc_len] = 0;
                // End of CSI sequence: final byte in 0x40-0x7E
                if ((ch >= '@' && ch <= '~') || esc_len == ESC_BUF_SIZE - 1) {
                    handle_ansi(esc_buf);
                    in_esc = false;
                    in_csi = false;
                    esc_len = 0;
                }
            } else {
                in_esc = false;
                in_csi = false;
                esc_len = 0;
            }
        } else {
            // Not a CSI sequence, just ESC + one char (unsupported)
            in_esc = false;
            esc_len = 0;
        }
    } else if (ch == 0x1B) {
        in_esc = true;
        in_csi = false;
        esc_len = 0;
    } else {
        put_char(ch);
    }
}

// Render the screen buffer to the LCD using Paint_DrawChar
void render_screen_to_lcd(void) {
    for (int r = 0; r < SCREEN_ROWS; ++r) {
        for (int c = 0; c < SCREEN_COLS; ++c) {
            char ch = screen[r][c].ch;
            uint16_t fg = screen[r][c].fg;
            uint16_t bg = screen[r][c].bg;
            int x = c * (CHAR_WIDTH + CHAR_SPACING);
            int y = r * CHAR_HEIGHT;
            // Only draw printable chars
            if (ch >= 32 && ch <= 126) {
                Paint_DrawChar(x, y, ch, &Font16, fg, bg);
            } else {
                Paint_DrawChar(x, y, ' ', &Font16, fg, bg);
            }
        }
    }
}

void ansi_init(void) {
    // Set screen size based on LCD dimensions and font size
    SCREEN_COLS = LCD_1IN47.WIDTH / CHAR_WIDTH;
    SCREEN_ROWS = LCD_1IN47.HEIGHT / CHAR_HEIGHT;

    // Allocate screen buffer
    screen = (cell_t **)malloc(SCREEN_ROWS * sizeof(cell_t *));
    for (int r = 0; r < SCREEN_ROWS; ++r) {
        screen[r] = (cell_t *)malloc(SCREEN_COLS * sizeof(cell_t));
    }

    Imagesize = LCD_1IN47.HEIGHT * LCD_1IN47.WIDTH * 2;
    if ((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
        // Handle allocation failure
        return;
    }

    // Initialize Paint drawing context
    Paint_NewImage((UBYTE *)BlackImage, LCD_1IN47.WIDTH, LCD_1IN47.HEIGHT, 0, BLACK);
    Paint_SetScale(65);  // Set color depth (16/32/65 depending on your LCD/driver)
    
    // Initialize screen
    clear_screen();
}