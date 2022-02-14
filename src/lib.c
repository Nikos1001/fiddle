
#include "../piccolo/include.h"
#include "font.h"
#include <SDL.h>
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Window* win;
SDL_Renderer* renderer;
bool end;
int w, h, pw, ph;
float cr, cg, cb;
bool* modified;
bool keyDown[256];
struct tex* target;

struct pixel {
    float r, g, b;
};

struct tex {
    struct pixel* dat;
    int w, h;
};

struct tex scr;

void initTex(struct piccolo_Engine* engine, struct tex* tex, int w, int h) {
    tex->w = w;
    tex->h = h;
    tex->dat = piccolo_reallocate(engine, NULL, 0, w * h * sizeof(struct pixel));
    for(int i = 0; i < w * h; i++) {
        tex->dat[i].r = 0;
        tex->dat[i].g = 0;
        tex->dat[i].b = 0;
    }
}

void freeTex(struct piccolo_Engine* engine, struct tex* tex) {
    piccolo_reallocate(engine, tex->dat, w * h * sizeof(struct pixel), 0);
    tex->dat = NULL;
}

#define FN_SIGNATURE(name) piccolo_Value name (struct piccolo_Engine* engine, int argc, piccolo_Value* argv, piccolo_Value self)

const char* beginParamName[4] = {
    "Width",
    "Height",
    "Pixel width",
    "Pixel height"
};

FN_SIGNATURE(begin) {
    target = NULL;
    if(argc != 5) {
        piccolo_runtimeError(engine, "Expected 5 args.");
        return PICCOLO_NIL_VAL();
    }
    if(!PICCOLO_IS_STRING(argv[0])) {
        piccolo_runtimeError(engine, "Window name must be a string.");
        return PICCOLO_NIL_VAL();
    }
    for(int i = 0; i < 4; i++) {
        if(!PICCOLO_IS_NUM(argv[i + 1])) {
            piccolo_runtimeError(engine, "%s must be a number.", beginParamName[i]);
        }
    }
    const char* winName = ((struct piccolo_ObjString*)PICCOLO_AS_OBJ(argv[0]))->string;
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        piccolo_runtimeError(engine, "Could not init SDL. %s", SDL_GetError());
        return PICCOLO_NIL_VAL();
    }

    w = PICCOLO_AS_NUM(argv[1]);
    h = PICCOLO_AS_NUM(argv[2]);
    pw = PICCOLO_AS_NUM(argv[3]);
    ph = PICCOLO_AS_NUM(argv[4]);
    win = SDL_CreateWindow(winName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w * pw, h * ph, 0);
    if(!win) {
        piccolo_runtimeError(engine, "Could not open window. %s", SDL_GetError());
        return PICCOLO_NIL_VAL();
    }

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetVSync(renderer, 1);
    if(!renderer) {
        piccolo_runtimeError(engine, "Could not init renderer. %s", SDL_GetError());
        return PICCOLO_NIL_VAL();
    }
    end = false;

    initTex(engine, &scr, w, h);
    cr = cg = cb = 1;
    modified = malloc(w * h);
    for(int i = 0; i < w * h; i++)
        modified[i] = false;

    for(int i = 0; i < 256; i++)
        keyDown[i] = false;
        
    return PICCOLO_NIL_VAL();
}

FN_SIGNATURE(shouldClose) {
    return PICCOLO_BOOL_VAL(end);
}

FN_SIGNATURE(endFrame) {

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

    SDL_Event ev;
    while(SDL_PollEvent(&ev)) {
        if(ev.type == SDL_QUIT) {
            end = true;
            freeTex(engine, &scr);
            return PICCOLO_NIL_VAL();
        }
        if(ev.type == SDL_KEYDOWN) {
            int key = ev.key.keysym.sym;
            if(key >= 256)
                break;
            keyDown[key] = true;
        }
        if(ev.type == SDL_KEYUP) {
            int key = ev.key.keysym.sym;
            if(key >= 256)
                break;
            keyDown[key] = false;
        }
    }

    SDL_Rect r;
    r.w = pw;
    r.h = ph;
    for(int x = 0; x < scr.w; x++) {
        for(int y = 0; y < scr.h; y++) {
            int idx = x + y * scr.w;
            if(modified[idx]) {
                struct pixel* p = &scr.dat[idx];
                r.x = x * pw;
                r.y = y * ph;
                SDL_SetRenderDrawColor(renderer, p->r * 255.0f, p->g * 255.0f, p->b * 255.0f, 255);
                SDL_RenderFillRect(renderer, &r);
            }
            modified[idx] = false;
        }
    }

    SDL_RenderPresent(renderer);

    return PICCOLO_NIL_VAL();
}

FN_SIGNATURE(keyPressed) {
    int code = PICCOLO_AS_NUM(argv[0]);
    return PICCOLO_BOOL_VAL(keyDown[code]);
}

#define TEX_FN \
    struct tex* tex = &scr; \
    if(target != NULL) \
        tex = target;

void plotPixel_(struct tex* tex, int x, int y, float r, float g, float b) {
    if(x < 0 || x >= tex->w || y < 0 || y >= tex->h)
        return;
    int idx = x + y * tex->w;
    if(r < 0)
        r = 0;
    if(r > 1)
        r = 1;
    tex->dat[idx].r = r;
    if(g < 0)
        g = 0;
    if(g > 1)
        g = 1;
    tex->dat[idx].g = g;
    if(b < 0)
        b = 0;
    if(b > 1)
        b = 1;
    tex->dat[idx].b = b;
    if(tex == &scr)
        modified[idx] = true;
}

const char* plotPixelParamName[5] = {
    "X",
    "Y",
    "R",
    "G",
    "B"
};
FN_SIGNATURE(plotPixel) {
    TEX_FN
    if(argc != 5) {
        piccolo_runtimeError(engine, "Expected 5 args.");
        return PICCOLO_NIL_VAL();
    }
    for(int i = 0; i < 5; i++) {
        if(!PICCOLO_IS_NUM(argv[i])) {
            piccolo_runtimeError(engine, "%s must be a number.", plotPixelParamName[i]);
            return PICCOLO_NIL_VAL();
        }
    }
    int x = PICCOLO_AS_NUM(argv[0]);
    int y = PICCOLO_AS_NUM(argv[1]);
    float r = PICCOLO_AS_NUM(argv[2]);
    float g = PICCOLO_AS_NUM(argv[3]);
    float b = PICCOLO_AS_NUM(argv[4]);
    plotPixel_(tex, x, y, r, g, b);
    return PICCOLO_NIL_VAL();
}

const char* fillParamNames[3] = {
    "R",
    "G",
    "B"
};
FN_SIGNATURE(fill) {
    TEX_FN
    if(argc != 3) {
        piccolo_runtimeError(engine, "Expected 3 args.");
        return PICCOLO_NIL_VAL();
    }
    for(int i = 0; i < 3; i++) {
        if(!PICCOLO_IS_NUM(argv[i])) {
            piccolo_runtimeError(engine, "%s must be a number.", fillParamNames[i]);
            return PICCOLO_NIL_VAL();
        }
    }
    float r = PICCOLO_AS_NUM(argv[0]);
    float g = PICCOLO_AS_NUM(argv[1]);
    float b = PICCOLO_AS_NUM(argv[2]);
    cr = r;
    cg = g;
    cb = b;
    return PICCOLO_NIL_VAL();
}

const char* rectParamNames[4] = {
    "X",
    "Y",
    "W",
    "H"
};
FN_SIGNATURE(rect) {
    TEX_FN
    if(argc != 4) {
        piccolo_runtimeError(engine, "Expected 4 args.");
        return PICCOLO_NIL_VAL();
    }
    for(int i = 0; i < 4; i++) {
        if(!PICCOLO_IS_NUM(argv[i])) {
            piccolo_runtimeError(engine, "%s must be a number.", rectParamNames[i]);
            return PICCOLO_NIL_VAL();
        }
    }
    int x = PICCOLO_AS_NUM(argv[0]);
    int y = PICCOLO_AS_NUM(argv[1]);
    int w = PICCOLO_AS_NUM(argv[2]);
    int h = PICCOLO_AS_NUM(argv[3]);
    for(int i = 0; i < w; i++) {
        for(int j = 0; j < h; j++) {
            plotPixel_(tex, x + i, y + j, cr, cg, cb);
        }
    }
    return PICCOLO_NIL_VAL();
}


FN_SIGNATURE(text) {
    TEX_FN
    if(argc != 3) {
        piccolo_runtimeError(engine, "Expected 3 args.");
        return PICCOLO_NIL_VAL();
    }
    if(!PICCOLO_IS_STRING(argv[0])) {
        piccolo_runtimeError(engine, "Text must be a string.");
        return PICCOLO_NIL_VAL();
    }
    if(!PICCOLO_IS_NUM(argv[1]) || !PICCOLO_IS_NUM(argv[2])) {
        piccolo_runtimeError(engine, "X and Y must be numbers.");
        return PICCOLO_NIL_VAL();
    }
    const char* text = ((struct piccolo_ObjString*)PICCOLO_AS_OBJ(argv[0]))->string;
    int x = PICCOLO_AS_NUM(argv[1]);
    int y = PICCOLO_AS_NUM(argv[2]);
    int cx = x, cy = y;
    while(*text != '\0') {
        char c = *text;
        if(c == '\n') {
            cy += 8;
            cx = x;
        } else {
            for(int i = 0; i < 8; i++) {
                for(int j = 0; j < 8; j++) {
                    if(font[c * 8 + i] & (1 << (8 - j))) {
                        plotPixel_(tex, cx + j, cy + i, cr, cg, cb);
                    }
                }
            }
            cx += 8;
        }
        text++;
    }
    return PICCOLO_NIL_VAL();
}

FN_SIGNATURE(width) {
    TEX_FN
    if(argc != 0) {
        piccolo_runtimeError(engine, "Expected no args.");
        return PICCOLO_NIL_VAL();
    }
    return PICCOLO_NUM_VAL(tex->w);
}

FN_SIGNATURE(height) {
    TEX_FN
    if(argc != 0) {
        piccolo_runtimeError(engine, "Expected no args.");
        return PICCOLO_NIL_VAL();
    }
    return PICCOLO_NUM_VAL(tex->h);
}

FN_SIGNATURE(loadTex) {
    if(argc != 1) {
        piccolo_runtimeError(engine, "Expected 1 arg.");
        return PICCOLO_NIL_VAL();
    }
    if(!PICCOLO_IS_STRING(argv[0])) {
        piccolo_runtimeError(engine, "Filename must be a string.");
        return PICCOLO_NIL_VAL();
    }
    const char* filename = ((struct piccolo_ObjString*)PICCOLO_AS_OBJ(argv[0]))->string;
    int w, h, n;
    unsigned char* data = stbi_load(filename, &w, &h, &n, 0);
    if(!data) {
        piccolo_runtimeError(engine, "Could not load texture.");
        return PICCOLO_NIL_VAL();
    }
    struct piccolo_ObjNativeStruct* natStruct = PICCOLO_ALLOCATE_NATIVE_STRUCT(engine, struct tex, "texture");
    struct tex* tex = PICCOLO_GET_PAYLOAD(natStruct, struct tex);
    initTex(engine, tex, w, h);
    for(int x = 0; x < w; x++) {
        for(int y = 0; y < h; y++) {
            int i = (y * w + x) * n;
            plotPixel_(tex, x, y, data[i] / 255.0, data[i + 1] / 255.0, data[i + 2] / 255.0);
        }
    }
    free(data);
    return PICCOLO_OBJ_VAL(natStruct);
}   

struct pixel* sampleTex(struct tex* tex, int x, int y) {
    return &tex->dat[y * tex->w + x];
}

FN_SIGNATURE(sampleR) {
    TEX_FN
    if(argc != 2) {
        piccolo_runtimeError(engine, "Expected 2 args.");
        return PICCOLO_NIL_VAL();
    }
    if(!PICCOLO_IS_NUM(argv[0]) || !PICCOLO_IS_NUM(argv[1])) {
        piccolo_runtimeError(engine, "X and Y must be numbers.");
        return PICCOLO_NIL_VAL();
    }
    float x = PICCOLO_AS_NUM(argv[0]) * tex->w;
    while(x > tex->w) {
        x -= tex->w;
    }
    while(x < 0) {
        x += tex->w;
    }
    float y = PICCOLO_AS_NUM(argv[1]) * tex->h;
    while(y > tex->h) {
        y -= tex->h;
    }
    while(y < 0) {
        y += tex->h;
    }
    int xi = (int)x;
    int yi = (int)y;
    return PICCOLO_NUM_VAL(sampleTex(tex, xi, yi)->r);
}

FN_SIGNATURE(sampleG) {
    TEX_FN
    if(argc != 2) {
        piccolo_runtimeError(engine, "Expected 2 args.");
        return PICCOLO_NIL_VAL();
    }
    if(!PICCOLO_IS_NUM(argv[0]) || !PICCOLO_IS_NUM(argv[1])) {
        piccolo_runtimeError(engine, "X and Y must be numbers.");
        return PICCOLO_NIL_VAL();
    }
    float x = PICCOLO_AS_NUM(argv[0]) * tex->w;
    while(x > tex->w) {
        x -= tex->w;
    }
    while(x < 0) {
        x += tex->w;
    }
    float y = PICCOLO_AS_NUM(argv[1]) * tex->h;
    while(y > tex->h) {
        y -= tex->h;
    }
    while(y < 0) {
        y += tex->h;
    }
    int xi = (int)x;
    int yi = (int)y;
    return PICCOLO_NUM_VAL(sampleTex(tex, xi, yi)->g);
}

FN_SIGNATURE(sampleB) {
    TEX_FN
    if(argc != 2) {
        piccolo_runtimeError(engine, "Expected 2 args.");
        return PICCOLO_NIL_VAL();
    }
    if(!PICCOLO_IS_NUM(argv[0]) || !PICCOLO_IS_NUM(argv[1])) {
        piccolo_runtimeError(engine, "X and Y must be numbers.");
        return PICCOLO_NIL_VAL();
    }
    float x = PICCOLO_AS_NUM(argv[0]) * tex->w;
    while(x > tex->w) {
        x -= tex->w;
    }
    while(x < 0) {
        x += tex->w;
    }
    float y = PICCOLO_AS_NUM(argv[1]) * tex->h;
    while(y > tex->h) {
        y -= tex->h;
    }
    while(y < 0) {
        y += tex->h;
    }
    int xi = (int)x;
    int yi = (int)y;
    return PICCOLO_NUM_VAL(sampleTex(tex, xi, yi)->b);
}

FN_SIGNATURE(setTarget) {
    if(!PICCOLO_IS_NATIVE_STRUCT(argv[0])) {
        piccolo_runtimeError(engine, "Target must be a texture.");
        return PICCOLO_NIL_VAL();
    }
    struct piccolo_ObjNativeStruct* natStruct = PICCOLO_AS_OBJ(argv[0]);
    struct tex* tex = PICCOLO_GET_PAYLOAD(natStruct, struct tex);
    target = tex;
    return PICCOLO_NIL_VAL();
}

FN_SIGNATURE(popTarget) {
    target = NULL;
    return PICCOLO_NIL_VAL();
}