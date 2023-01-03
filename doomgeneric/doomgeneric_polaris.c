#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <termios.h>

#include <time.h>
#include <errno.h>

#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

static unsigned char convertToDoomKey(unsigned char scancode)
{
    unsigned char key = 0;

    switch (scancode)
    {
    case 0x9C:
    case 0x1C:
        key = KEY_ENTER;
        break;
    case 0x01:
        key = KEY_ESCAPE;
        break;
    case 0xCB:
    case 0x4B:
        key = KEY_LEFTARROW;
        break;
    case 0xCD:
    case 0x4D:
        key = KEY_RIGHTARROW;
        break;
    case 0xC8:
    case 0x48:
        key = KEY_UPARROW;
        break;
    case 0xD0:
    case 0x50:
        key = KEY_DOWNARROW;
        break;
    case 0x1D:
        key = KEY_FIRE;
        break;
    case 0x39:
        key = KEY_USE;
        break;
    case 0x2A:
    case 0x36:
        key = KEY_RSHIFT;
        break;
    case 0x15:
        key = 'y';
        break;
    default:
        break;
    }

    return key;
}

static void addKeyToQueue(int pressed, unsigned char keyCode)
{
	//printf("key hex %x decimal %d\n", keyCode, keyCode);

        unsigned char key = convertToDoomKey(keyCode);

        unsigned short keyData = (pressed << 8) | key;

        s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
        s_KeyQueueWriteIndex++;
        s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}


void raw_nanosleep(void *a, void *b) {
	ssize_t ret;
    asm volatile
    (
        "syscall"
        : "=a" (ret)
        : "0"(0x23), "D"(a), "S"(b)
        : "rcx", "r11", "memory"
    );
}

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

	raw_nanosleep(&ts, &ts);


    return res;
}


int framebuffer_fd = -1;
int keyboard_fd = -1;

struct fbdev_info {
	size_t pitch, bpp;
	uint16_t width, height;
};

char *fb = NULL;

struct fbdev_info framebuffer_info = {0};

void DG_Init() {
	printf ("Hello Polaris! %s\n", "WOO");

	framebuffer_fd = open("/dev/fbdev", O_RDWR);

	if (framebuffer_fd == -1) {
		printf ("Failed to open fbdev!\n");
		return;
	}

	ioctl(framebuffer_fd, 0x1, &framebuffer_info);

	if (!framebuffer_info.bpp) {
		printf ("Invalid framebuffer config!\n");
		return;
	}

	keyboard_fd = open("/dev/keyboard", O_RDONLY);

	if (keyboard_fd == -1) {
		printf ("Failed to open keyboard!\n");
		return;
	}

	fb = mmap(0, framebuffer_info.height * framebuffer_info.width * framebuffer_info.bpp / 8, PROT_READ | PROT_WRITE,
				   MAP_SHARED, framebuffer_fd, 0);

	memset(fb, 0x00, framebuffer_info.pitch * framebuffer_info.height);
}

static void handleKeyInput()
{
    if (keyboard_fd < 0)
    {
        return;
    }

    unsigned char scancode = 0;

    if (ioctl(keyboard_fd, 0x1, &scancode) > 0)
    {
        unsigned char keyRelease = (0x80 & scancode);

        scancode = (0x7F & scancode);

        if (0 == keyRelease)
        {
            addKeyToQueue(1, scancode);
        }
        else
        {
            addKeyToQueue(0, scancode);
        }
    }
}

void DG_DrawFrame() {
	if (framebuffer_fd == -1)
		return;

	for (int i = 0; i < DOOMGENERIC_RESY; ++i) {
		memcpy(fb + i * framebuffer_info.pitch, DG_ScreenBuffer + i * DOOMGENERIC_RESX, DOOMGENERIC_RESX * 4);
	}

	handleKeyInput();
}

// stubbed for now
int DG_GetKey(int *pressed, unsigned char *doomKey) {
	if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex)
    {
        //key queue is empty

        return 0;
    }
    else
    {
        unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
        s_KeyQueueReadIndex++;
        s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

        *pressed = keyData >> 8;
        *doomKey = keyData & 0xFF;

        return 1;
    }
}

uint32_t ticks = 0;

void DG_SleepMs(uint32_t ms) {
	msleep(ms);
	ticks += ms;
}

uint32_t DG_GetTicksMs() {
	return ticks;
}

void DG_SetWindowTitle(const char * title) {
	printf("Got %s\n", title);
}
