/*
 * X11 window sequence collector for viredero
 * Copyright (c) 2015 Leonid Movshovich
 *
 *
 * viredero is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * viredero is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with viredero; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <X11/Xlibint.h>
#include <X11/extensions/Xdamage.h>

#define PROG "xcoll"
#define SLEEP_TIME_MSEC 50

struct context {
    Display* dpy;
    Window root;
    int damage;
    int (*write)(struct context*, int, int, int, int, char*, int);
    void* writer_private;
};

struct bmp_context {
    int num;
    char* path;
    char* fname;
};
    
static int stdout_writer(struct context* ctx, int x, int y, int width, int height
		, char* data, int size) {
    int fd = (int)((long long)ctx->writer_private);
    int t = htonl(x);
    write(fd, &t, 4);
    t = htonl(y);
    write(fd, &t, 4);
    t = htonl(width);
    write(fd, &t, 4);
    t = htonl(height);
    write(fd, &t, 4);
    write(fd, data, size);
}

static int bmp_writer_init(struct context* ctx, struct bmp_context* bmp_ctx) {
    bmp_ctx->path = "/tmp/imgs/w%0.6d.bmp";
    bmp_ctx->num = 0;
    bmp_ctx->fname = malloc(64);
}
    
struct __attribute__ ((__packed__)) bm_head {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct __attribute__ ((__packed__)) bm_info_head {
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};

static void swap_lines(char* line0, char* line1, int size) {
    int i;
    for (i = 0; i < size; i += 1) {
	line0[i] ^= line1[i];
	line1[i] ^= line0[i];
	line0[i] ^= line1[i];
    }
}

static int bmp_writer(struct context* ctx, int x, int y, int width, int height
	       , char* data, int size) {
    struct bmp_context* bmp_ctx = (struct bmp_context*)ctx->writer_private;
    struct bm_head head;
    struct bm_info_head ihead; 
    FILE *f;
   
    head.bfType = 0x4D42;
    head.bfSize = sizeof(struct bm_head) + sizeof(struct bm_info_head)
	+ width * height * 4;
    head.bfOffBits = sizeof(struct bm_head) + sizeof(struct bm_info_head);
    head.bfReserved1 = 0;
    head.bfReserved2 = 0;
    
    ihead.biSize = sizeof(struct bm_info_head);
    ihead.biWidth = width;
    ihead.biHeight = height;
    ihead.biPlanes = 1;
    ihead.biBitCount = 32;
    ihead.biSizeImage = width * height * 4;
    ihead.biCompression = 0;
    ihead.biXPelsPerMeter = 0;
    ihead.biYPelsPerMeter = 0;
    ihead.biClrUsed = 0;
    ihead.biClrImportant = 0;

    sprintf(bmp_ctx->fname, bmp_ctx->path, bmp_ctx->num);
    f = fopen(bmp_ctx->fname, "wb");
    if(f == NULL)
	return;
    fwrite(&head, sizeof(struct bm_head), 1, f);
    fwrite(&ihead, sizeof(struct bm_info_head), 1, f);
    int i;
    int byte_w = 4 * width;
    for (i = 0; i < height/2; i += 1) {
    	swap_lines(&data[byte_w * i], &data[byte_w * (height - i - 1)], 4 * width);
    }
    fwrite(data, 4*width*height, 1, f);
    fclose(f);
    bmp_ctx->num += 1;
}

static void usage() {
    printf("USAGE: %s <display> (e.g. '%s :0')", PROG, PROG);
}

struct bmp_context bmp_ctx;
static int setup_dpy(const char * display_name, struct context* ctx) {
    Display * dpy = XOpenDisplay(display_name);
    if (!dpy) {
        fprintf(stderr, "%s:  unable to open display '%s'\n"
                , PROG, XDisplayName (display_name));
        usage();
        return 0;
    }

    Window root = DefaultRootWindow(dpy);

    int damage_error;
    if (!XDamageQueryExtension (dpy, &ctx->damage, &damage_error)) {
        fprintf(stderr, "%s: Backend does not have damage extension\n", PROG);
        return 0;
    }
    XDamageCreate (dpy, root, XDamageReportRawRectangles);
    XWindowAttributes attrib;
    XGetWindowAttributes(dpy, root, &attrib);
    if (0 == attrib.width || 0 == attrib.height)
    {
        fprintf(stderr, "%s: Bad root with %d x %d\n"
                , PROG, attrib.width, attrib.height);
        return 0;
    }

    ctx->dpy = dpy;
    ctx->root = root;

    ctx->write = bmp_writer;
    ctx->writer_private = (void*)(&bmp_ctx);
    bmp_writer_init(ctx, &bmp_ctx);
    return 1;
}

int output_damage(struct context* ctx, int x, int y, int width, int height) {

//    fprintf(stderr, "outputing damage: %d %d %d %d\n", x, y, width, height);
    XImage *image = XGetImage(ctx->dpy, ctx->root
			      , x, y, width, height, AllPlanes, ZPixmap);
    if (!image) {
	printf("unabled to get the image\n");
	return 0;
    }
    x = htonl(x);
    ctx->write(ctx, x, y, width, height, image->data
	       , width * height * image->bits_per_pixel / 8);
}

static struct context context;

int main(int argc, char* argv[]) {
    char* disp_name = argv[1];
    int fin = 0;
    XEvent event;
    
    setup_dpy(disp_name, &context);
    while (! fin) {
        if (XPending(context.dpy) > 0) {
            XGrabServer(context.dpy);
            while (XPending(context.dpy) > 0) {
                XNextEvent(context.dpy, &event);
                if (event.type == context.damage + XDamageNotify) {
                    XDamageNotifyEvent *de = (XDamageNotifyEvent *) &event;
                    if (de->drawable == context.root) {
                        output_damage(
                            &context, de->area.x, de->area.y
                            , de->area.width, de->area.height);
                    }
                }
            }
	    XUngrabServer(context.dpy);
	    usleep(SLEEP_TIME_MSEC);
        }
    }
}
  
