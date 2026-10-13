#include <stdint.h>
#include "minemu/user_abi.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_LINE_LENGTH 20

static char buffer[MAX_LINE_LENGTH + 1];
static size_t buffer_index = 0;
static bool overflow = false;
static bool command_done = false;
static bool echo = false;
static bool echo_start = false;
static bool line_output = false;

static void write_bytes(const char *str, size_t length) {
    ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, str, length);
}

static void write_string(const char *str) {
    uint32_t length = 0;
    while (str[length] != '\0') {
        ++length;
    }
    write_bytes(str, length);
}

static void line_reset(void) {
    buffer_index = 0;
    overflow = false;
    command_done = false;
    echo_start = false;
    echo = false;
    line_output = false;
}

static void finish_line(bool end) {
    buffer[buffer_index] = '\0';
    size_t length = buffer_index;
    buffer_index = 0;
    if (overflow) {
        if (end) {
            line_reset();
        }
        return;
    }
    if (!command_done) {
        if (length != 0) {
            command_done = true;
            if (length == 4 && buffer[0] == 'e' && buffer[1] == 'c' && buffer[2] == 'h' && buffer[3] == 'o') {
                echo = true;
            } else {
                write_string("command not found: ");
                write_bytes(buffer, length);
                line_output = true;
            }
        }
    } else if (echo && length != 0) {
        if (echo_start) {
            write_string(" ");
        }
        echo_start = true;
        write_bytes(buffer, length);
        line_output = true;
    }
    if (end) {
        if (line_output) {
            write_string("\n");
        }
        line_reset();
    }
}

void minemu_user_main(void) {
    while (1) {
        overflow = false;
        write_string("msh> ");
        while(1) {
            uint8_t c;
            int32_t result = ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (uint32_t)(uintptr_t)&c, 1);
            if (result != 1) {
                continue; 
            }
            if (c == '\n') {
                finish_line(true);
                break; 
            }
            if (c == 0x08 || c == 0x7f) {
                if (buffer_index > 0) {
                    buffer_index--;
                }
                continue;
            }
            if (c == ' ') {
                finish_line(false);
                continue;
            }
            if (buffer_index < MAX_LINE_LENGTH) {
                buffer[buffer_index++] = (char)c;
            } else {
                overflow = true;
            }
            
        }
        
    }
}
