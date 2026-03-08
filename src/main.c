#include "pico/stdlib.h"
#include "tusb.h"
#include "LCD_1in47.h"

#include "GUI_Paint.h"
#include "fonts.h"

// Draw a 'T' in the center and border-detecting boxes using direct draw
void draw_orientation_and_borders(void) {
    LCD_1IN47_Clear(WHITE);

    // If rotated by 90 degrees (HORIZONTAL), swap width and height
    uint16_t w = LCD_1IN47.WIDTH;
    uint16_t h = LCD_1IN47.HEIGHT;

    // Outer border
    for (uint16_t x = 0; x <= w-1; ++x) {
        LCD_1IN47_DisplayPoint(x, 0, BLACK);
        LCD_1IN47_DisplayPoint(x, h-1, BLACK);
    }
    for (uint16_t y = 0; y <= h-1; ++y) {
        LCD_1IN47_DisplayPoint(0, y, BLACK);
        LCD_1IN47_DisplayPoint(w-1, y, BLACK);
    }
    // Inner border (10px inset)
    for (uint16_t x = 10; x <= w-11; ++x) {
        LCD_1IN47_DisplayPoint(x, 10, GRAY);
        LCD_1IN47_DisplayPoint(x, h-11, GRAY);
    }
    for (uint16_t y = 10; y <= h-11; ++y) {
        LCD_1IN47_DisplayPoint(10, y, GRAY);
        LCD_1IN47_DisplayPoint(w-11, y, GRAY);
    }

    // Draw a large 'T' in the center using two lines
    uint16_t cx = w/2;
    uint16_t cy = h/2;
    uint16_t T_width = w/4;
    uint16_t T_height = h/3;
    // Horizontal bar of T
    for (uint16_t x = cx - T_width/2; x <= cx + T_width/2 && x <= w-1; ++x) {
        if (x < w)
            LCD_1IN47_DisplayPoint(x, cy - T_height/2, RED);
    }
    // Vertical bar of T
    for (uint16_t y = cy - T_height/2; y <= cy + T_height/2 && y <= h-1; ++y) {
        if (y < h)
            LCD_1IN47_DisplayPoint(cx, y, RED);
    }

    // Optionally, draw crosshairs for orientation
    for (uint16_t y = 0; y <= h-1; ++y) {
        if (y % 4 < 2 && cx < w)
            LCD_1IN47_DisplayPoint(cx, y, BLUE); // Dotted vertical
    }
    for (uint16_t x = 0; x <= w-1; ++x) {
        if (x % 4 < 2 && cy < h)
            LCD_1IN47_DisplayPoint(x, cy, BLUE); // Dotted horizontal
    }
}


#define ESC_BUF_SIZE 32
static char esc_buf[ESC_BUF_SIZE];
static uint8_t esc_len = 0;

// --- ANSI/VT100 Color State ---
#define ANSI_COLOR_DEFAULT 0xFFFF
typedef struct {
    uint16_t fg;
    uint16_t bg;
} ansi_color_state_t;
static ansi_color_state_t ansi_color = { .fg = WHITE, .bg = BLACK };

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


// Font size (should match Font16)
#define CHAR_WIDTH 11
#define CHAR_HEIGHT 16
#define CHAR_SPACING 0 // No space between chars

// Calculate screen size based on LCD dimensions and font size
// These will be set at runtime
int SCREEN_COLS = 0;
int SCREEN_ROWS = 0;
typedef struct {
    char ch;
    uint16_t fg;
    uint16_t bg;
} cell_t;
cell_t **screen = NULL;
uint16_t cursor_row = 0, cursor_col = 0;

// Dynamically allocate image buffer for Paint
UDOUBLE Imagesize;
UWORD *BlackImage;

void clear_screen() {
    for (int r = 0; r < SCREEN_ROWS; ++r)
        for (int c = 0; c < SCREEN_COLS; ++c) {
            screen[r][c].ch = ' ';
            screen[r][c].fg = WHITE;
            screen[r][c].bg = BLACK;
        }
    cursor_row = cursor_col = 0;
    // Clear the image buffer too
    Paint_Clear(BLACK);
}

void move_cursor(uint16_t row, uint16_t col) {
    if (row < SCREEN_ROWS) cursor_row = row;
    if (col < SCREEN_COLS) cursor_col = col;
}

// Parse SGR (Select Graphic Rendition) color codes
static void handle_ansi_sgr(const char *params) {
    int p[8] = {0};
    int n = 0;
    const char *s = params;
    while (*s && n < 8) {
        p[n++] = atoi(s);
        s = strchr(s, ';');
        if (!s) break;
        ++s;
    }
    if (n == 0) n = 1, p[0] = 0; // Default SGR 0
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
        char last = seq[strlen(seq)-1];
        if (last == 'm') {
            char paramstr[ESC_BUF_SIZE] = {0};
            strncpy(paramstr, seq+1, strlen(seq)-2); // skip '[' and 'm'
            handle_ansi_sgr(paramstr);
        } else if (seq[1] == '2' && seq[2] == 'J') {
            clear_screen();
        } else if (seq[1] == 'H') {
            move_cursor(0, 0);
        } else if (last == 'b' && seq[1] == '?') {
            // Custom: CSI ?PwmValue b  (e.g. ESC[?80b sets brightness to 80%)
            int pwm = 100;
            sscanf(seq + 2, "%d", &pwm);
            if (pwm < 0) pwm = 0;
            if (pwm > 100) pwm = 100;
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
    } else if (ch >= 32 && ch <= 126) { // Printable character triggers wrap
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


// Forward declaration for serial string
void init_serial_string(void);
extern char serial_str[17];

// Show color dots at key coordinates (no rotation)
void show_dot_pattern(void) {
    LCD_1IN47.WIDTH = LCD_1IN47.WIDTH;
    LCD_1IN47.HEIGHT = LCD_1IN47.HEIGHT;

    LCD_1IN47_Clear(BLACK);

    // Corners
    LCD_1IN47_DisplayPoint(0, 0, RED); // Red: Top-left
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH-1, 0, GREEN); // Green: Top-right
    LCD_1IN47_DisplayPoint(0, LCD_1IN47.HEIGHT-1, BLUE); // Blue: Bottom-left
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH-1, LCD_1IN47.HEIGHT-1, YELLOW); // Yellow: Bottom-right

    // Edges (center of each edge)
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH/2, 0, MAGENTA); // Magenta: Top-center
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH/2, LCD_1IN47.HEIGHT-1, CYAN); // Cyan: Bottom-center
    LCD_1IN47_DisplayPoint(0, LCD_1IN47.HEIGHT/2, WHITE); // White: Left-center
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH-1, LCD_1IN47.HEIGHT/2, GREEN); // Green: Right-center

    // Center
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH/2, LCD_1IN47.HEIGHT/2, YELLOW); // Yellow: Center
}

void hardware_test_animation(void) {
    UWORD colors[] = {RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN, WHITE, BLACK};
    int num_colors = sizeof(colors) / sizeof(colors[0]);
    int block_size = 16;

    // Sequence of 2-color checkerboards

    for (int i = 0; i < num_colors; ++i) {
        for (int j = i + 1; j < num_colors; ++j) {
            for (int y = 0; y < LCD_1IN47.HEIGHT; y += block_size) {
                for (int x = 0; x < LCD_1IN47.WIDTH; x += block_size) {
                    UWORD color = ((x / block_size + y / block_size) % 2) ? colors[i] : colors[j];
                    int x_end = (x + block_size - 1 < LCD_1IN47.WIDTH) ? x + block_size - 1 : LCD_1IN47.WIDTH - 1;
                    int y_end = (y + block_size - 1 < LCD_1IN47.HEIGHT) ? y + block_size - 1 : LCD_1IN47.HEIGHT - 1;
                    Paint_DrawRectangle(x, y, x_end, y_end, color, 1, DOT_PIXEL_1X1);
                }
            }
            LCD_1IN47_Display(BlackImage);
            sleep_ms(100);
        }
    }
    // Restore orientation for terminal
}

void lcd_clear_with_pwm(UWORD color) {
    LCD_1IN47_Clear(color);
    DEV_SET_PWM(50);
    sleep_ms(200);
    DEV_SET_PWM(100);
    sleep_ms(200);
}

int main() {

    if(DEV_Module_Init()!=0){
        return -1;
    }
    /* LCD Init */
    DEV_SET_PWM(0);

    LCD_1IN47_Init(VERTICAL);

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
        printf("Failed to allocate image buffer...\r\n");
        return -1;
    }

    // Initialize Paint drawing context
    Paint_NewImage((UBYTE *)BlackImage, LCD_1IN47.WIDTH, LCD_1IN47.HEIGHT, 0, BLACK);
    Paint_SetScale(65); // Set color depth (16/32/65 depending on your LCD/driver)

    LCD_1IN47_Clear(WHITE);

    for (int i = 0; i <= 10; ++i) {
        DEV_SET_PWM(i *10);
        sleep_ms(100);
    }

    lcd_clear_with_pwm(RED);

    stdio_init_all();

    lcd_clear_with_pwm(BLUE);

    init_serial_string();

    lcd_clear_with_pwm(GREEN);

    tusb_init();

    LCD_1IN47_Clear(GRAY);
    DEV_SET_PWM(50);

    show_dot_pattern();
    sleep_ms(2000);
    draw_orientation_and_borders();
    sleep_ms(2000);
    hardware_test_animation();
    sleep_ms(2000);

    LCD_1IN47_Clear(BLACK);

    // On power-up, clear and write version/serial using ANSI color codes
    clear_screen();
    // Write: bright yellow on blue for header, white on black for serial
    const char *startup_msg =
        "\n\n"
        "\x1b[44;93mANSI USB Terminal\n"
        "\x1b[41;93mFirmware v1.0\n"
        "\x1b[0mSerial: ";
    for (const char *p = startup_msg; *p; ++p) process_char(*p);
    for (const char *p = serial_str; *p; ++p) process_char(*p);
    process_char('\n');
    render_screen_to_lcd();
    LCD_1IN47_Display(BlackImage);


    while (true) {
        tud_task();

        bool updated = false;
        while (tud_cdc_available()) {
            char ch = tud_cdc_read_char();
            process_char(ch);
            updated = true;
        }
        if (updated) {
            render_screen_to_lcd();
            LCD_1IN47_Display(BlackImage);
        }
    }

    // Cleanup (never reached, but good practice)
    for (int r = 0; r < SCREEN_ROWS; ++r) {
        free(screen[r]);
    }
    free(screen);
    screen = NULL;
    free(BlackImage);
    BlackImage = NULL;
}
