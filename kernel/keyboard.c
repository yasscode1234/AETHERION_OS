#include "keyboard.h"
#include "vga.h"
#include "kernel.h"

/* ---- Ports PS/2 ---- */
#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_CMD_PORT     0x64

/* ---- Buffer circulaire pour les scancodes ---- */
#define KB_BUFFER_SIZE 256
static u8 kb_buffer[KB_BUFFER_SIZE];
static u16 kb_head = 0;
static u16 kb_tail = 0;

/* ---- ?tat des modificateurs ---- */
static u8 shift_pressed = 0;
static u8 ctrl_pressed = 0;
static u8 alt_pressed = 0;
static u8 capslock_on = 0;
static u8 extended = 0;

/* ---- Tables de conversion scancode -> ASCII ---- */
static const char scancode_to_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char scancode_to_ascii_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* ---- I/O ports ---- */
static inline u8 inb(u16 port) {
    u8 ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(u16 port, u8 val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void io_wait(void) {
    __asm__ volatile ("jmp 1f\n1: jmp 2f\n2:" : : : "memory");
}

static void ps2_wait_input(void) {
    while (inb(PS2_STATUS_PORT) & 0x02)
        io_wait();
}

static void ps2_wait_output(void) {
    while (!(inb(PS2_STATUS_PORT) & 0x01))
        io_wait();
}

static void ps2_send_cmd(u8 cmd) {
    ps2_wait_input();
    outb(PS2_CMD_PORT, cmd);
}

static void ps2_send_data(u8 data) {
    ps2_wait_input();
    outb(PS2_DATA_PORT, data);
}

static u8 ps2_read_data(void) {
    ps2_wait_output();
    return inb(PS2_DATA_PORT);
}

static void kb_buffer_push(u8 scancode) {
    u16 next = (kb_head + 1) % KB_BUFFER_SIZE;
    if (next != kb_tail) {
        kb_buffer[kb_head] = scancode;
        kb_head = next;
    }
}

static u8 kb_buffer_pop(void) {
    if (kb_head == kb_tail)
        return 0;
    u8 sc = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return sc;
}

void keyboard_init(void) {
    ps2_send_cmd(0xAD);
    ps2_send_cmd(0xA7);

    while (inb(PS2_STATUS_PORT) & 0x01)
        inb(PS2_DATA_PORT);

    ps2_send_cmd(0x20);
    u8 config = ps2_read_data();
    config &= ~0x30;
    config &= ~0x10;
    config &= ~0x20;
    ps2_send_cmd(0x60);
    ps2_send_data(config);

    ps2_send_cmd(0xAA);
    u8 res = ps2_read_data();
    if (res != 0x55) {
        vga_write("KB: Controller test failed\n");
        return;
    }

    ps2_send_cmd(0xAB);
    res = ps2_read_data();
    if (res != 0x00) {
        vga_write("KB: Port 1 test failed\n");
        return;
    }

    ps2_send_cmd(0xAE);

    ps2_send_data(0xFF);
    ps2_read_data();
    u8 self_test = ps2_read_data();
    if (self_test != 0xAA) {
        vga_write("KB: Self test failed\n");
        return;
    }

    ps2_send_data(0xF0);
    ps2_read_data();
    ps2_send_data(0x01);
    ps2_read_data();

    config |= 0x01;
    ps2_send_cmd(0x60);
    ps2_send_data(config);

    vga_write("KB: Keyboard initialized\n");
}

void keyboard_handler(void) {
    u8 scancode = inb(PS2_DATA_PORT);

    if (scancode & 0x80) {
        u8 make_code = scancode & 0x7F;
        switch (make_code) {
            case 0x2A:  /* KEY_LSHIFT */
            case 0x36:  /* KEY_RSHIFT */
                shift_pressed = 0;
                break;
            case 0x1D:  /* KEY_LCTRL */
                ctrl_pressed = 0;
                break;
            case 0x38:  /* KEY_LALT */
                alt_pressed = 0;
                break;
        }
        extended = 0;
        return;
    }

    if (scancode == 0xE0) {
        extended = 1;
        return;
    }

    switch (scancode) {
        case 0x2A:  /* KEY_LSHIFT */
        case 0x36:  /* KEY_RSHIFT */
            shift_pressed = 1;
            extended = 0;
            return;
        case 0x1D:  /* KEY_LCTRL */
            ctrl_pressed = 1;
            extended = 0;
            return;
        case 0x38:  /* KEY_LALT */
            alt_pressed = 1;
            extended = 0;
            return;
        case 0x3A:  /* KEY_CAPSLOCK */
            capslock_on = !capslock_on;
            extended = 0;
            return;
    }

    char c = 0;
    if (scancode < 128) {
        if (shift_pressed ^ capslock_on)
            c = scancode_to_ascii_shift[scancode];
        else
            c = scancode_to_ascii[scancode];
    }

    if (c != 0) {
        kb_buffer_push(c);
    }

    extended = 0;
}

int keyboard_has_key(void) {
    return kb_head != kb_tail;
}

char keyboard_getchar(void) {
    while (!keyboard_has_key())
        __asm__ volatile ("hlt");
    return kb_buffer_pop();
}
