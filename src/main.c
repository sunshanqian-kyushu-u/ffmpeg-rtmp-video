#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

/* about fb0 */
#include <linux/fb.h>
#include <sys/mman.h>

#include "getstream.h"

static unsigned char *mem;

int main()
{
    int fp = 0;
    long screensize = 0; 
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    fp = open("/dev/fb0", O_RDWR);
    if (fp < 0)
    {
        printf("Error: Can not open framebuffer device!\r\n");
        goto fail_open_fb;
    }

    if (ioctl(fp, FBIOGET_FSCREENINFO, &finfo))
    {
        printf("Error: Can not read fixed information!\r\n");
        goto fail_get_finfo;
    }

    if (ioctl(fp, FBIOGET_VSCREENINFO, &vinfo))
    {
        printf("Error: Can not read variable information!\r\n");
        goto fail_get_vinfo;
    }

    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    /* map to a memory */
    mem =(char *) mmap (NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fp, 0);
    if ((int) mem == -1)
    {
       printf ("Error: Can not map framebuffer device to a memory!\r\n");
       goto fail_map_mem;
    }

    signal(SIGINT, signal_handler);
    getstream(mem);

fail_map_mem:
    munmap (mem, screensize); /* unmap */

fail_get_vinfo:
fail_get_finfo:
    close(fp);
    
fail_open_fb:
    printf ("Normal exit succeed!\r\n");
    return 0;
}