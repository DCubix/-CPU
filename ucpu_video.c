#include "ucpu_video.h"

#include <string.h>

uColor uCPU_Palette[] = {
	{ 255, 228, 194 },
	{ 220, 164, 86 },
	{ 169, 96, 76 },
	{ 66, 41, 54 }
};

uGfx* ugfx_new() {
	uGfx* gfx = (uGfx*) malloc(sizeof(uGfx));
	gfx->vram = umem_new(UCPU_VIDEO_WIDTH * UCPU_VIDEO_HEIGHT);
	
	SDL_Init(SDL_INIT_VIDEO);
		
	gfx->window = SDL_CreateWindow(
			"μCPU",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			UCPU_VIDEO_WIDTH * UCPU_WINDOW_UPSCALE, UCPU_VIDEO_HEIGHT * UCPU_WINDOW_UPSCALE,
			SDL_WINDOW_SHOWN
	);
	if (!gfx->window) {
		LOG("Window");
		LOG(SDL_GetError());
		return NULL;
	}
	
	gfx->renderer = SDL_CreateRenderer(gfx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!gfx->renderer) {
		LOG("Renderer");
		LOG(SDL_GetError());
		return NULL;
	}
	
	gfx->screen_texture = SDL_CreateTexture(
			gfx->renderer,
			SDL_PIXELFORMAT_RGB24,
			SDL_TEXTUREACCESS_STREAMING,
			UCPU_VIDEO_WIDTH, UCPU_VIDEO_HEIGHT
	);
	
	return gfx;
}
void ugfx_free(uGfx* gfx) {
	SDL_DestroyRenderer(gfx->renderer);
	SDL_DestroyWindow(gfx->window);
	SDL_DestroyTexture(gfx->screen_texture);
	umem_free(gfx->vram);
}

void ugfx_set(uGfx* gfx, u16 x, u16 y, u8 color) {
	if (color >= uCPUColor_Ignore) return;
	if (x < 0 || x >= UCPU_VIDEO_WIDTH || y < 0 || y >= UCPU_VIDEO_HEIGHT) return;
	umem_write(gfx->vram, x + y * UCPU_VIDEO_WIDTH, color);
}

void ugfx_clear(uGfx* gfx, u8 color) {
	memset(gfx->vram->data, color, gfx->vram->size * sizeof(u16));
}

void ugfx_flip(uGfx* gfx) {
	// Update buffer texture
	u8* pixels;
	int pitch;
	SDL_LockTexture(gfx->screen_texture, NULL, &pixels, &pitch);
	for (int y = 0; y < UCPU_VIDEO_HEIGHT; y++) {
		for (int x = 0; x < UCPU_VIDEO_WIDTH; x++) {
			int index = x + y * UCPU_VIDEO_WIDTH;
			u8 cid = umem_read(gfx->vram, index);
			uColor col = uCPU_Palette[cid];
			int pidx = index * 3;
			pixels[pidx + 0] = col.r;
			pixels[pidx + 1] = col.g;
			pixels[pidx + 2] = col.b;
		}
	}
	SDL_UnlockTexture(gfx->screen_texture);
	
	// Render
	SDL_RenderClear(gfx->renderer);
	
	SDL_Rect dst = { 0, 0, UCPU_VIDEO_WIDTH * UCPU_WINDOW_UPSCALE, UCPU_VIDEO_HEIGHT * UCPU_WINDOW_UPSCALE };
	SDL_RenderCopy(gfx->renderer, gfx->screen_texture, NULL, &dst);
	
	SDL_RenderPresent(gfx->renderer);
}

void ugfx_save_screen(uGfx* gfx, const char* file) {
	SDL_Surface *surface;
    Uint32 rmask, gmask, bmask, amask;
	
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
	
	surface = SDL_CreateRGBSurface(
			0,
			UCPU_VIDEO_WIDTH * UCPU_WINDOW_UPSCALE, UCPU_VIDEO_HEIGHT * UCPU_WINDOW_UPSCALE,
			24,
			rmask, gmask, bmask, amask
	);
	
	SDL_LockSurface(surface);
	SDL_RenderReadPixels(
			gfx->renderer, 
			NULL,
			surface->format->format,
			surface->pixels,
			surface->pitch
	);
	SDL_UnlockSurface(surface);
	
	SDL_SaveBMP(surface, file);
	
	SDL_FreeSurface(surface);
}