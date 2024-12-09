#include <stdint.h>
#include <stdio.h>

// GPIO Register Addresses
#define SYSCTL_RCGCGPIO        (*((volatile uint32_t *)0x400FE608))
#define GPIO_PORTA_DATA        (*((volatile uint32_t *)0x400043FC))
#define GPIO_PORTA_DIR         (*((volatile uint32_t *)0x40004400))
#define GPIO_PORTA_PUR         (*((volatile uint32_t *)0x40004510))
#define GPIO_PORTA_AFSEL       (*((volatile uint32_t *)0x40004420))
#define GPIO_PORTA_PCTL        (*((volatile uint32_t *)0x4000452C))
#define GPIO_PORTA_DEN         (*((volatile uint32_t *)0x4000451C))
#define GPIO_PORTC_DATA        (*((volatile uint32_t *)0x400063FC))
#define GPIO_PORTC_DIR         (*((volatile uint32_t *)0x40006400))
#define GPIO_PORTC_PUR         (*((volatile uint32_t *)0x40006510))
#define GPIO_PORTC_DEN         (*((volatile uint32_t *)0x4000651C))
#define GPIO_PORTB_DATA        (*((volatile uint32_t *)0x400053FC))
#define GPIO_PORTB_DIR         (*((volatile uint32_t *)0x40005400))
#define GPIO_PORTB_DEN         (*((volatile uint32_t *)0x4000551C))
#define GPIO_PORTE_DATA        (*((volatile uint32_t *)0x400243FC))
#define GPIO_PORTE_DIR         (*((volatile uint32_t *)0x40024400))
#define GPIO_PORTE_DEN         (*((volatile uint32_t *)0x4002451C))

// Updated Pin Definitions
#define RS_PIN                 (1U << 1)  // PE1
#define RW_PIN                 (1U << 3)  // PE3
#define EN_PIN                 (1U << 2)  // PE2
#define LCD_DATA_MASK          0xFF       // Mask for PB0-PB7

// Other definitions
#define INITIAL_STATE            0
#define A_STATE                  1
#define B_STATE                  2
#define DISPLAY_STATE            3
#define ROW_MASK                 (0x3C)    // PA2-PA5 (Rows)
#define COLUMN_MASK_A            (0xC0)    // PA6, PA7 (Columns on Port A)
#define COLUMN_MASK_C            (0xC0)    // PC6, PC7 (Columns on Port C)

// Keypad configuration
#define ROWS 4
#define COLS 4
char keypad[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// Function prototypes
void LCD_Command(uint8_t cmd);
void LCD_Data(uint8_t data);
void LCD_Clear(void);
void LCD_DisplayString(char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);
char GetKeypadInput(void);
void InitializeCalculator(void);
void ResetCalculator(void);
void HandleInput(char key);
void itoa(uint32_t value, char *buffer);
void DelayMs(uint32_t ms);

// Global variables
uint32_t A = 0, B = 0;
uint8_t current_state = INITIAL_STATE;

// Main function
int main(void) {
    InitializeCalculator();
    LCD_SetCursor(0, 0);
    //LCD_DisplayString("Hello, World!"); // test
    while (1) {
        char key = GetKeypadInput();
        if (key != 0) {
            HandleInput(key);
            key = 0;
        }
        DelayMs(100);
    }
}

// Initialize Calculator to Initial State
void InitializeCalculator(void) {
    SYSCTL_RCGCGPIO |= (1U << 0) | (1U << 2) | (1U << 1) | (1U << 4) | (1U << 5);
    DelayMs(40);

    // Set up keypad rows (PA2-PA5) as outputs
    GPIO_PORTA_DIR |= ROW_MASK;
    GPIO_PORTA_PUR |= ROW_MASK;
    GPIO_PORTA_DEN |= ROW_MASK;

    // Set up keypad columns (PA6, PA7, PC6, PC7) as inputs
    GPIO_PORTA_DIR &= ~COLUMN_MASK_A;
    GPIO_PORTA_PUR |= COLUMN_MASK_A;
    GPIO_PORTA_DEN |= COLUMN_MASK_A;

    GPIO_PORTC_DIR &= ~COLUMN_MASK_C;
    GPIO_PORTC_PUR |= COLUMN_MASK_C;
    GPIO_PORTC_DEN |= COLUMN_MASK_C;

    // LCD setup
    GPIO_PORTB_DIR |= LCD_DATA_MASK; // 0xFF
    GPIO_PORTB_DEN |= LCD_DATA_MASK;

    GPIO_PORTE_DIR |= (RW_PIN | EN_PIN | RS_PIN);
    GPIO_PORTE_DEN |= (RW_PIN | EN_PIN | RS_PIN);

    GPIO_PORTE_DATA &= ~RW_PIN;

    LCD_Command(0x38);  // 8-bit mode, 2 lines, 5x8 font
    DelayMs(20);
    LCD_Command(0x0C); // Display ON, Cursor OFF
    DelayMs(20);
    LCD_Clear();
    DelayMs(20);
    LCD_Command(0x06);  // Entry mode set
    DelayMs(20);

    ResetCalculator();
    DelayMs(20);
}

// Reset Calculator to Initial State
void ResetCalculator(void) {
    A = 0;
    B = 0;
    current_state = A_STATE;
    LCD_Clear();
    LCD_SetCursor(0, 0);
}

// Handle Keypad Input and State Transition
void HandleInput(char key) {
    if (key == 'C') {
        ResetCalculator();
        return;
    }

    if (current_state == A_STATE) {
        if (key >= '0' && key <= '9') {
            A = A * 10 + (key - '0');
            char display[16];
            itoa(A, display);
            LCD_Clear();
            LCD_DisplayString(display);
        } else if (key == '*') {
            current_state = B_STATE;
            LCD_Clear();
        }
    } else if (current_state == B_STATE) {
        if (key >= '0' && key <= '9') {
            B = B * 10 + (key - '0');
            char next_display[16];
            itoa(B, next_display);
            LCD_Clear();
            LCD_DisplayString(next_display);
        } else if (key == '#') {
            current_state = DISPLAY_STATE;
            LCD_Clear();
            char result[16];
            itoa(A*B, result);
            LCD_SetCursor(1, 0);
            LCD_DisplayString(result);
            DelayMs(2000);
            ResetCalculator();
        }
    }
}

void itoa(uint32_t value, char *buffer) {
    int i = 0;
    do {
        buffer[i++] = (value % 10) + '0'; // Convert each digit
        value /= 10;
    } while (value);
    buffer[i] = '\0';

    // Reverse the string
    int j;
    for (j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }
}

// LCD Functions
void LCD_Clear(void) {
    LCD_Command(0x01);
    DelayMs(10);
}

void LCD_DisplayString(char *str) {
    while (*str) {
        LCD_Data(*str++);
    }
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? 0x80 : 0xC0;
    address += col;
    LCD_Command(address);
}

void LCD_Command(uint8_t cmd) {
    GPIO_PORTB_DATA = cmd;            // Write command directly to Port B
    GPIO_PORTE_DATA &= ~RS_PIN;       // RS = 0 for command
    DelayMs(10);
    GPIO_PORTE_DATA |= EN_PIN;        // Pulse Enable pin
    DelayMs(10);
    GPIO_PORTE_DATA &= ~EN_PIN;       // Clear Enable
    DelayMs(10);
}

void LCD_Data(uint8_t data) {
    GPIO_PORTB_DATA = data;           // Write data directly to Port B
    GPIO_PORTE_DATA |= RS_PIN;        // RS = 1 for data
    DelayMs(10);
    GPIO_PORTE_DATA |= EN_PIN;        // Pulse Enable pin
    DelayMs(10);
    GPIO_PORTE_DATA &= ~EN_PIN;       // Clear Enable
    DelayMs(10);
}

// Placeholder for Keypad Scanning
char GetKeypadInput(void) {
    int row, col;

    GPIO_PORTA_DATA |= ROW_MASK;

    for (row = 0; row < ROWS; row++) {
        GPIO_PORTA_DATA |= ROW_MASK;
        GPIO_PORTA_DATA &= ~(1U << (2 + row));
        DelayMs(10);

        if (!(GPIO_PORTA_DATA & (1U << 6))) {
            DelayMs(20);
            if (!(GPIO_PORTA_DATA & (1U << 6))) {
                while (!(GPIO_PORTA_DATA & (1U << 6)));
                return keypad[row][0];
            }
        }
        if (!(GPIO_PORTA_DATA & (1U << 7))) {
            DelayMs(20);
            if (!(GPIO_PORTA_DATA & (1U << 7))) {
                while (!(GPIO_PORTA_DATA & (1U << 7)));
                return keypad[row][1];
            }
        }

        if (!(GPIO_PORTC_DATA & (1U << 6))) {
            DelayMs(20);
            if (!(GPIO_PORTC_DATA & (1U << 6))) {
                while (!(GPIO_PORTC_DATA & (1U << 6)));
                return keypad[row][2];
            }
        }
        if (!(GPIO_PORTC_DATA & (1U << 7))) {
            DelayMs(20);
            if (!(GPIO_PORTC_DATA & (1U << 7))) {
                while (!(GPIO_PORTC_DATA & (1U << 7)));
                return keypad[row][3];
            }
        }
    }

    return 0;
}

void DelayMs(uint32_t ms) {
    volatile uint32_t delay;
    for (delay = 0; delay < (ms * 1000); delay++);
}
