# ANSI Display Terminal

A USB-based ANSI/VT100 terminal emulator running on the Raspberry Pi Pico RP2350 with a 1.47" LCD display.

## Overview

This project transforms a Raspberry Pi Pico RP2350 with a Waveshare 1.47" LCD into a fully functional ANSI terminal display. It appears as a USB Serial (CDC) device on your computer and renders terminal output with full color support and ANSI escape sequence processing.

## Hardware Requirements

- **Raspberry Pi Pico RP2350** (or compatible RP2350-based board)
- **Waveshare RP2350-LCD-1.47-B** development board
- USB cable for programming and communication

## Features

### Core Terminal Features
- **USB CDC Serial Interface** - Appears as `/dev/ttyACM0` (Linux) or `COMx` (Windows)
- **ANSI/VT100 Escape Sequence Support**:
  - Text colors (8 standard + 8 bright colors)
  - Background colors
  - Cursor positioning (`ESC[row;colH`)
  - Screen clearing (`ESC[2J`)
  - Home cursor (`ESC[H`)
- **Custom Extensions**:
  - Display brightness control (`ESC[?<0-100>b`)
- **Text Rendering**: 16-pixel font with configurable colors
- **Screen Buffer**: Character grid with per-cell foreground/background colors
- **Scrolling**: Automatic text scrolling when reaching bottom

### Hardware Test Features
- **Startup Animation**: PWM brightness fade-in
- **Color Pattern Tests**: Corner and edge detection patterns
- **Orientation Test**: 'T' shape with crosshairs for orientation verification
- **Checkerboard Animation**: Dynamic color pattern test using Paint library

## Building

### Prerequisites
- **Pico SDK 2.2.0** (automatically managed by VS Code extension)
- **CMake** with Ninja generator
- **ARM GCC toolchain** (included with Pico SDK)
- **VS Code** with Raspberry Pi Pico extension (recommended)

### Build Steps

1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   cd ANSI_Display
   ```

2. **Configure CMake**:
   ```bash
   cmake -S . -B build -G Ninja
   ```

3. **Build the project**:
   ```bash
   ninja -C build
   ```

4. **Flash to device**:
   - Hold BOOTSEL button while connecting USB
   - Copy `build/ANSI_Display.uf2` to the mounted drive, or
   - Use picotool: `picotool load build/ANSI_Display.elf -fx`

## Usage

### Basic Operation
1. Connect the device via USB
2. Open a terminal application (screen, minicom, PuTTY, etc.)
3. Connect to the serial port (typically `/dev/ttyACM0` on Linux)
4. Start typing - text appears on the LCD with full ANSI color support

### Example Terminal Commands
```bash
# Connect on Linux
screen /dev/ttyACM0

# Connect on macOS
screen /dev/cu.usbmodem*

# Test colors
echo -e "\033[31mRed text\033[0m"
echo -e "\033[42;97mWhite on green\033[0m"

# Set brightness to 50%
echo -e "\033[?50b"

# Clear screen and position cursor
echo -e "\033[2J\033[H"
```

### Supported ANSI Codes

| Code | Function | Example |
|------|----------|---------|
| `ESC[<n>m` | Set text color | `ESC[31m` (red) |
| `ESC[<n>m` | Set background color | `ESC[41m` (red bg) |
| `ESC[<r>;<c>H` | Position cursor | `ESC[10;5H` |
| `ESC[H` | Home cursor | `ESC[H` |
| `ESC[2J` | Clear screen | `ESC[2J` |
| `ESC[0m` | Reset colors | `ESC[0m` |
| `ESC[?<n>b` | Set brightness (custom) | `ESC[?80b` (80%) |

### Color Codes
- **Foreground**: 30-37 (standard), 90-97 (bright)
- **Background**: 40-47 (standard), 100-107 (bright)
- **Colors**: Black, Red, Green, Yellow, Blue, Magenta, Cyan, White

## File Structure

```
├── src/
│   ├── main.c              # Main application and terminal logic
│   └── usb_descriptors.c   # USB device configuration
├── lib/                    # Hardware abstraction libraries
│   ├── Config/             # Device configuration
│   ├── Fonts/              # Font definitions
│   ├── GUI/                # Paint/drawing functions
│   └── LCD/                # LCD driver
├── CMakeLists.txt          # Build configuration
├── pico_sdk_import.cmake   # Pico SDK integration
└── README.md               # This file
```

## Configuration

### Display Settings
- **Resolution**: 172x320 pixels (VERTICAL orientation)
- **Font**: 16-pixel height, 11-pixel width
- **Text Area**: ~15 columns × 20 rows
- **Colors**: 16-bit RGB565 format

### Terminal Settings
- **Baud Rate**: Not applicable (USB CDC)
- **Buffer Size**: 256 bytes TX/RX
- **Escape Buffer**: 32 characters

## Development

### Architecture
- **USB CDC**: TinyUSB library provides virtual serial port
- **Character Processing**: State machine parses ANSI escape sequences
- **Screen Buffer**: 2D array of character cells with color attributes
- **Rendering**: Paint library draws characters to image buffer
- **Display**: Buffer transferred to LCD via SPI

### Adding ANSI Codes
1. Extend `handle_ansi()` function in `main.c`
2. Add new escape sequence parsing logic
3. Implement the desired terminal behavior
4. Update this README with the new codes

### Hardware Integration
- **SPI Interface**: LCD communication
- **PWM**: Backlight brightness control
- **USB**: CDC serial communication
- **GPIO**: Additional pins available for expansion

## License

[Add your license information here]

## Contributing

[Add contribution guidelines here]

## Troubleshooting

### Common Issues
1. **Device not recognized**: Check USB cable and drivers
2. **No display**: Verify hardware connections and power
3. **Garbled text**: Check terminal application settings
4. **Build errors**: Ensure Pico SDK is properly installed

### Debug Output
The device uses stdio over USB, so debug prints appear in the terminal alongside normal text output.