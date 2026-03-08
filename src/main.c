#include "pico/stdlib.h"
#include "tusb.h"
#include "ansi.h"

// ========================================
// FORWARD DECLARATIONS
// ========================================

void init_serial_string(void);
extern char serial_str[17];

// ========================================
// HARDWARE TEST FUNCTIONS
// ========================================

// Draw a 'T' in the center and border-detecting boxes using direct draw
void draw_orientation_and_borders(void) {
    LCD_1IN47_Clear(WHITE);

    // If rotated by 90 degrees (HORIZONTAL), swap width and height
    uint16_t w = LCD_1IN47.WIDTH;
    uint16_t h = LCD_1IN47.HEIGHT;

    // Outer border
    for (uint16_t x = 0; x <= w - 1; ++x) {
        LCD_1IN47_DisplayPoint(x, 0, BLACK);
        LCD_1IN47_DisplayPoint(x, h - 1, BLACK);
    }
    for (uint16_t y = 0; y <= h - 1; ++y) {
        LCD_1IN47_DisplayPoint(0, y, BLACK);
        LCD_1IN47_DisplayPoint(w - 1, y, BLACK);
    }
    // Inner border (10px inset)
    for (uint16_t x = 10; x <= w - 11; ++x) {
        LCD_1IN47_DisplayPoint(x, 10, GRAY);
        LCD_1IN47_DisplayPoint(x, h - 11, GRAY);
    }
    for (uint16_t y = 10; y <= h - 11; ++y) {
        LCD_1IN47_DisplayPoint(10, y, GRAY);
        LCD_1IN47_DisplayPoint(w - 11, y, GRAY);
    }

    // Draw a large 'T' in the center using two lines
    uint16_t cx = w / 2;
    uint16_t cy = h / 2;
    uint16_t T_width = w / 4;
    uint16_t T_height = h / 3;
    
    // Horizontal bar of T
    for (uint16_t x = cx - T_width / 2; x <= cx + T_width / 2 && x <= w - 1; ++x) {
        if (x < w) {
            LCD_1IN47_DisplayPoint(x, cy - T_height / 2, RED);
        }
    }
    
    // Vertical bar of T
    for (uint16_t y = cy - T_height / 2; y <= cy + T_height / 2 && y <= h - 1; ++y) {
        if (y < h) {
            LCD_1IN47_DisplayPoint(cx, y, RED);
        }
    }

    // Optionally, draw crosshairs for orientation
    for (uint16_t y = 0; y <= h - 1; ++y) {
        if (y % 4 < 2 && cx < w) {
            LCD_1IN47_DisplayPoint(cx, y, BLUE); // Dotted vertical
        }
    }
    for (uint16_t x = 0; x <= w - 1; ++x) {
        if (x % 4 < 2 && cy < h) {
            LCD_1IN47_DisplayPoint(x, cy, BLUE); // Dotted horizontal
        }
    }
}

// Show color dots at key coordinates (no rotation)
void show_dot_pattern(void) {
    LCD_1IN47_Clear(BLACK);
    
    // Corners
    LCD_1IN47_DisplayPoint(0, 0, RED);  // Red: Top-left
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH - 1, 0, GREEN);  // Green: Top-right
    LCD_1IN47_DisplayPoint(0, LCD_1IN47.HEIGHT - 1, BLUE);  // Blue: Bottom-left
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH - 1, LCD_1IN47.HEIGHT - 1, YELLOW);  // Yellow: Bottom-right
    
    // Edges (center of each edge)
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH / 2, 0, MAGENTA);  // Magenta: Top-center
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH / 2, LCD_1IN47.HEIGHT - 1, CYAN);  // Cyan: Bottom-center
    LCD_1IN47_DisplayPoint(0, LCD_1IN47.HEIGHT / 2, WHITE);  // White: Left-center
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH - 1, LCD_1IN47.HEIGHT / 2, GREEN);  // Green: Right-center
    
    // Center
    LCD_1IN47_DisplayPoint(LCD_1IN47.WIDTH / 2, LCD_1IN47.HEIGHT / 2, YELLOW);  // Yellow: Center
}

// Hardware test animation using Paint rectangles
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
}

void lcd_clear_with_pwm(UWORD color) {
    LCD_1IN47_Clear(color);
    DEV_SET_PWM(50);
    sleep_ms(200);
    DEV_SET_PWM(100);
    sleep_ms(200);
}

// ========================================
// MAIN FUNCTION
// ========================================

int main(void) {
    if (DEV_Module_Init() != 0) {
        return -1;
    }
    
    // LCD Init
    DEV_SET_PWM(0);

    LCD_1IN47_Init(VERTICAL);

    // Initialize ANSI terminal system
    ansi_init();

    if (BlackImage == NULL) {
        printf("Failed to allocate image buffer...\r\n");
        return -1;
    }

    LCD_1IN47_Clear(WHITE);

    for (int i = 0; i <= 10; ++i) {
        DEV_SET_PWM(i * 10);
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
    
    for (const char *p = startup_msg; *p; ++p) {
        process_char(*p);
    }
    for (const char *p = serial_str; *p; ++p) {
        process_char(*p);
    }
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
