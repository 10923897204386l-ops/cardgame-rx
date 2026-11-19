#include "render.h"
/* Original 5x7 bitmap lettering and geometric art; no external assets. */
static const char *glyph(char c) {
    switch(c) {
    case 'A': return "01110100011000111111100011000110001";
    case '2': return "01110100010000100010001000100011111";
    case '3': return "11110000010000101110000010000111110";
    case '4': return "00010001100101010010111110001000010";
    case '5': return "11111100001000011110000010000111110";
    case '6': return "01110100001000011110100011000101110";
    case '7': return "11111000010001000100010000100001000";
    case '8': return "01110100011000101110100011000101110";
    case '9': return "01110100011000101111000010000101110";
    case '1': return "00100011000010000100001000010001110";
    case '0': return "01110100011001110101110011000101110";
    case 'J': return "00111000100001000010000101001001100";
    case 'Q': return "01110100011000110001101011001001101";
    case 'K': return "10001100101010011000101001001010001";
    case 'B': return "11110100011000111110100011000111110";
    case 'C': return "01111100001000010000100001000001111";
    case 'D': return "11110100011000110001100011000111110";
    case 'E': return "11111100001000011110100001000011111";
    case 'F': return "11111100001000011110100001000010000";
    case 'G': return "01111100001000010111100011000101111";
    case 'H': return "10001100011000111111100011000110001";
    case 'I': return "01110001000010000100001000010001110";
    case 'L': return "10000100001000010000100001000011111";
    case 'M': return "10001110111010110101100011000110001";
    case 'N': return "10001110011010110011100011000110001";
    case 'O': return "01110100011000110001100011000101110";
    case 'P': return "11110100011000111110100001000010000";
    case 'R': return "11110100011000111110101001001010001";
    case 'S': return "01111100001000001110000010000111110";
    case 'T': return "11111001000010000100001000010000100";
    case 'U': return "10001100011000110001100011000101110";
    case 'V': return "10001100011000110001100010101000100";
    case 'W': return "10001100011000110101101011010101010";
    case 'X': return "10001100010101000100010101000110001";
    case 'Y': return "10001100010101000100001000010000100";
    case '-': return "00000000000000011111000000000000000";
    default: return "00000000000000000000000000000000000";
    }
}
static void color(SDL_Renderer *r,SDL_Color c) { SDL_SetRenderDrawColor(r,c.r,c.g,c.b,255); }
static void fill(SDL_Renderer *r,int x,int y,int w,int h) { SDL_Rect b={x,y,w,h}; SDL_RenderFillRect(r,&b); }
void render_round_fill(SDL_Renderer *r,Rect b,int radius,SDL_Color c) {
    if (radius <= 0) { color(r,c); fill(r,b.x,b.y,b.w,b.h); return; }
    if (radius*2 > b.w) radius = b.w/2;
    if (radius*2 > b.h) radius = b.h/2;
    color(r,c);
    fill(r,b.x+radius,b.y,b.w-2*radius,b.h);
    fill(r,b.x,b.y+radius,b.w,b.h-2*radius);
    for (int dy = 0; dy < radius; ++dy) for (int dx = 0; dx < radius; ++dx) {
        int x = radius - 1 - dx, y = radius - 1 - dy;
        if (x*x + y*y <= radius*radius) {
            SDL_RenderDrawPoint(r,b.x+dx,b.y+dy);
            SDL_RenderDrawPoint(r,b.x+b.w-1-dx,b.y+dy);
            SDL_RenderDrawPoint(r,b.x+dx,b.y+b.h-1-dy);
            SDL_RenderDrawPoint(r,b.x+b.w-1-dx,b.y+b.h-1-dy);
        }
    }
}
void render_round_outline(SDL_Renderer *r,Rect b,int radius,int thickness,SDL_Color c) {
    if (thickness < 1) thickness = 1;
    for (int i = 0; i < thickness; ++i) {
        Rect o = {b.x-i,b.y-i,b.w+2*i,b.h+2*i};
        if (radius <= 0) {
            color(r,c); SDL_Rect s={o.x,o.y,o.w,o.h}; SDL_RenderDrawRect(r,&s);
            continue;
        }
        int rr = radius + i;
        if (rr*2 > o.w) rr = o.w/2;
        if (rr*2 > o.h) rr = o.h/2;
        color(r,c);
        SDL_RenderDrawLine(r,o.x+rr,o.y,o.x+o.w-rr-1,o.y);
        SDL_RenderDrawLine(r,o.x+rr,o.y+o.h-1,o.x+o.w-rr-1,o.y+o.h-1);
        SDL_RenderDrawLine(r,o.x,o.y+rr,o.x,o.y+o.h-rr-1);
        SDL_RenderDrawLine(r,o.x+o.w-1,o.y+rr,o.x+o.w-1,o.y+o.h-rr-1);
        for (int dy = 0; dy <= rr; ++dy) for (int dx = 0; dx <= rr; ++dx) {
            int d = dx*dx + dy*dy;
            if (d <= rr*rr && d >= (rr-1)*(rr-1)) {
                SDL_RenderDrawPoint(r,o.x+rr-dx,o.y+rr-dy);
                SDL_RenderDrawPoint(r,o.x+o.w-rr-1+dx,o.y+rr-dy);
                SDL_RenderDrawPoint(r,o.x+rr-dx,o.y+o.h-rr-1+dy);
                SDL_RenderDrawPoint(r,o.x+o.w-rr-1+dx,o.y+o.h-rr-1+dy);
            }
        }
    }
}
void render_text(SDL_Renderer *r,int x,int y,const char *s,int scale) {
    for (;*s;++s,x+=6*scale) {
        const char *g=glyph(*s);
        for(int row=0;row<7;++row) for(int col=0;col<5;++col)
            if(g[row*5+col]=='1') fill(r,x+col*scale,y+row*scale,scale,scale);
    }
}
static SDL_Color ink(Suit s) { return s==HEARTS || s==DIAMONDS ? (SDL_Color){174,24,42,255} : (SDL_Color){22,25,31,255}; }
static void suit(SDL_Renderer *r,Suit s,int x,int y,int scale) {
    const char *g;
    switch(s) {
    case HEARTS: g="01010111111111111111011100010000000"; break;
    case DIAMONDS: g="00100011101111111111011100010000000"; break;
    case CLUBS: g="00100011100010011111111110010001110"; break;
    default: g="00100011101111111111010100010001110"; break;
    }
    for(int row=0;row<7;++row) for(int col=0;col<5;++col)
        if(g[row*5+col]=='1') fill(r,x+col*scale,y+row*scale,scale,scale);
}
BackTheme back_theme(bool alt) {
    return alt ? (BackTheme){{105,27,50,255},{245,194,119,255},true} : (BackTheme){{23,46,92,255},{148,192,225,255},false};
}
void render_card(SDL_Renderer *r,Rect b,const CardState *c,BackTheme theme) {
    render_round_fill(r,b,8,(SDL_Color){20,28,28,255});
    render_round_fill(r,(Rect){b.x+1,b.y+1,b.w-2,b.h-2},7,(SDL_Color){255,249,226,255});
    if (!c->face_up) {
        render_round_fill(r,(Rect){b.x+4,b.y+4,b.w-8,b.h-8},5,theme.base); color(r,theme.ink);
        for(int y=7;y<b.h-7;y+=8) for(int x=7;x<b.w-7;x+=8) {
            if(theme.diamonds) { SDL_RenderDrawLine(r,b.x+x,b.y+y-2,b.x+x+2,b.y+y); SDL_RenderDrawLine(r,b.x+x+2,b.y+y,b.x+x,b.y+y+2); SDL_RenderDrawLine(r,b.x+x,b.y+y+2,b.x+x-2,b.y+y); SDL_RenderDrawLine(r,b.x+x-2,b.y+y,b.x+x,b.y+y-2); }
            else fill(r,b.x+x,b.y+y,2,2);
        }
        return;
    }
    color(r,ink(c->card.suit));
    const char *rank[]={"","A","2","3","4","5","6","7","8","9","10","J","Q","K"};
    render_text(r,b.x+4,b.y+4,rank[c->card.rank],2);
    suit(r,c->card.suit,b.x+(c->card.rank==10?28:16),b.y+4,2);
    if(c->card.rank<=10) {
        /* Conventional symmetric pip fields, kept below the index. */
        static const unsigned patterns[11]={0,1u<<7,(1u<<1)|(1u<<13),(1u<<1)|(1u<<7)|(1u<<13),
            (1u<<0)|(1u<<2)|(1u<<12)|(1u<<14),
            (1u<<0)|(1u<<2)|(1u<<7)|(1u<<12)|(1u<<14),
            (1u<<0)|(1u<<2)|(1u<<6)|(1u<<8)|(1u<<12)|(1u<<14),
            (1u<<0)|(1u<<2)|(1u<<4)|(1u<<6)|(1u<<8)|(1u<<12)|(1u<<14),
            (1u<<0)|(1u<<2)|(1u<<4)|(1u<<6)|(1u<<8)|(1u<<10)|(1u<<12)|(1u<<14),
            (1u<<0)|(1u<<2)|(1u<<3)|(1u<<5)|(1u<<7)|(1u<<9)|(1u<<11)|(1u<<12)|(1u<<14),
            (1u<<0)|(1u<<2)|(1u<<3)|(1u<<5)|(1u<<4)|(1u<<10)|(1u<<9)|(1u<<11)|(1u<<12)|(1u<<14)};
        for(int i=0;i<15;++i) if(patterns[c->card.rank]&(1u<<i))
            suit(r,c->card.suit,b.x+b.w/2-5+(i%3-1)*19,b.y+27+(i/3)*14,2);
    } else {
        /* Temporary original court portrait: crown, face and robed shoulders. */
        int x=b.x+b.w/2;
        color(r,(SDL_Color){190,135,38,255}); fill(r,x-13,b.y+30,26,7);
        fill(r,x-13,b.y+25,5,8); fill(r,x-2,b.y+23,5,10); fill(r,x+8,b.y+25,5,8);
        color(r,(SDL_Color){235,183,126,255}); fill(r,x-10,b.y+38,20,21);
        color(r,ink(c->card.suit)); fill(r,x-5,b.y+44,2,2); fill(r,x+4,b.y+44,2,2);
        fill(r,x-18,b.y+62,36,29); color(r,(SDL_Color){239,197,78,255});
        fill(r,x-2,b.y+62,4,29); render_text(r,x-5,b.y+72,rank[c->card.rank],1);
    }
}
static void slot(SDL_Renderer *r,Rect b) {
    color(r,(SDL_Color){119,163,137,255}); SDL_Rect s={b.x,b.y,b.w,b.h}; SDL_RenderDrawRect(r,&s);
}
void render_scene(SDL_Renderer *r,const Scene *s,const Layout *l,BackTheme theme,bool stress) {
    color(r,(SDL_Color){22,83,64,255}); SDL_RenderClear(r);
    color(r,(SDL_Color){16,34,32,255}); fill(r,0,0,640,l->status_h);
    color(r,(SDL_Color){255,249,226,255});
    render_text(r,12,7,stress?"CARDGAME RX - STRESS PREVIEW":"CARDGAME RX - STATIC PREVIEW",2);
    const Pile *top[]={&s->stock,&s->waste,NULL,&s->foundations[0],&s->foundations[1],&s->foundations[2],&s->foundations[3]};
    for(unsigned col=0;col<7;++col) if(top[col]) {
        Rect b=layout_slot(l,col,false); slot(r,b); const CardState *c=pile_top(top[col]);
        if(c) render_card(r,b,c,theme);
        else { color(r,(SDL_Color){119,163,137,255}); suit(r,(Suit)(col-3),b.x+b.w/2-5,b.y+40,2); }
    }
    for(unsigned col=0;col<7;++col) {
        slot(r,layout_slot(l,col,true));
        for(size_t i=0;i<s->tableau[col].count;++i) {
            Rect b,visible; if(layout_card(l,&s->tableau[col],col,i,&b,&visible)) render_card(r,b,&s->tableau[col].cards[i],theme);
        }
    }
}
