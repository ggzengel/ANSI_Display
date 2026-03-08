#ifndef ANSI_H
#define ANSI_H

#include "pico/stdlib.h"
#include "LCD_1in47.h"
#include "GUI_Paint.h"
#include "fonts.h"

// ========================================
// CONSTANTS AND TYPE DEFINITIONS
// ========================================

#define ESC_BUF_SIZE 32
#define ANSI_COLOR_DEFAULT 0xFFFF

// Font size (should match Font16)
#define CHAR_WIDTH 11
#define CHAR_HEIGHT 16
#define CHAR_SPACING 0  // No space between chars

// ANSI/VT100 Color State Type
typedef struct {
    uint16_t fg;
    uint16_t bg;
} ansi_color_state_t;

// Character cell type for screen buffer
typedef struct {
    char ch;
    uint16_t fg;
    uint16_t bg;
} cell_t;

// ========================================
// GLOBAL VARIABLES DECLARATIONS
// ========================================

// Screen dimensions (calculated at runtime)
extern int SCREEN_COLS;
extern int SCREEN_ROWS;

// Screen buffer and cursor state
extern cell_t **screen;
extern uint16_t cursor_row, cursor_col;

// ANSI escape sequence processing
extern ansi_color_state_t ansi_color;

// Paint library image buffer
extern UDOUBLE Imagesize;
extern UWORD *BlackImage;

// ========================================
// FUNCTION DECLARATIONS
// ========================================

// Screen management functions
void clear_screen(void);
void move_cursor(uint16_t row, uint16_t col);
void scroll_up(void);

// ANSI processing functions
void handle_ansi(const char *seq);
void process_char(char ch);

// Character and rendering functions
void put_char(char ch);
void render_screen_to_lcd(void);

// Initialization function
void ansi_init(void);

#endif // ANSI_H