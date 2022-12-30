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

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

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

void DG_DrawFrame() {
	printf("Got frame\n");
	for (int i = 0; i < DOOMGENERIC_RESY; ++i) {
		memcpy(fb + i * framebuffer_info.pitch, DG_ScreenBuffer + i * DOOMGENERIC_RESX, DOOMGENERIC_RESX * 4);
	}
}

// stubbed for now
int DG_GetKey(int *pressed, unsigned char *doomKey) {
	return 0;
}

uint32_t ticks = 0;

void DG_SleepMs(uint32_t ms) {
	printf("Got sleep\n");
	msleep(ms);
	ticks += ms;
}

uint32_t DG_GetTicksMs() {
	return ticks;
}

void DG_SetWindowTitle(const char * title) {
	printf("Got %s\n", title);
}
