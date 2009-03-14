// ------------------------------------------------------
// Protrekkr
// Based on Juan Antonio Arguelles Rius's NoiseTrekker.
//
// Copyright (C) 2008-2009 Franck Charlet.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL FRANCK CHARLET OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
// ------------------------------------------------------

// ------------------------------------------------------
// Includes
#include "include/draw_primitives.h"
#include "../extralibs/sdl_draw/include/sdl_draw.h"
#include "../include/ptk.h"

// ------------------------------------------------------
// Variables
extern SDL_Surface *Main_Screen;
extern SDL_Surface *Temp_PFONT;
extern SDL_Surface *Temp_LARGEPFONT;
extern SDL_Surface *Temp_SMALLPFONT;
extern SDL_Surface *Temp_NOTEPFONT;
extern SDL_Surface *Temp_NOTELARGEPFONT;
extern SDL_Surface *Temp_NOTESMALLPFONT;
extern char *Font_Ascii;
extern int Nbr_Letters;
extern int Font_Pos[256];
extern int Font_Size[256];
int FgColor;

// ------------------------------------------------------
// Functions
int Get_Char_Position(char *Ascii_Letters, int Max_Letters, char Letter);

// ------------------------------------------------------
// Primitives
void DrawLine(int x1, int y1, int x2, int y2)
{
    Draw_Line(Main_Screen, x1, y1, x2, y2, FgColor);
}

void DrawPixel(int x, int y, int Color)
{
    Draw_Pixel(Main_Screen, x, y, Color);
}

void DrawHLine(int y, int x1, int x2, int Color)
{
    Draw_HLine(Main_Screen, x1, y, x2, Color);
}

void DrawVLine(int x, int y1, int y2, int Color)
{
    Draw_VLine(Main_Screen, x, y1, y2, Color);
}

void SetColor(int color)
{
    FgColor = color;
}

void Fillrect(int x1, int y1, int x2, int y2)
{
    SDL_Rect Dst_Rect;
    Dst_Rect.x = x1;
    Dst_Rect.y = y1;
    Dst_Rect.w = x2 - x1;
    Dst_Rect.h = y2 - y1;
    SDL_FillRect(Main_Screen, &Dst_Rect, FgColor);
}

// ------------------------------------------------------
// Set the current palette
void UISetPalette(SDL_Color *Palette, int Amount)
{
    if(FONT_LOW)
    {
        SDL_SetPalette(FONT_LOW, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(FONT_LOW, SDL_LOGPAL, Palette, 0, Amount);
    }
    if(FONT)
    {
        SDL_SetPalette(FONT, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(FONT, SDL_LOGPAL, Palette, 0, Amount);
    }
    if(PFONT)
    {
        SDL_SetPalette(PFONT, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(PFONT, SDL_LOGPAL, Palette, 0, Amount);
    }
    if(SKIN303)
    {
        SDL_SetPalette(SKIN303, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(SKIN303, SDL_LOGPAL, Palette, 0, Amount);
    }
    if(LOGOPIC)
    {
        SDL_SetPalette(LOGOPIC, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(LOGOPIC, SDL_LOGPAL, Palette, 0, Amount);
    }
    if(Temp_PFONT)
    {
        SDL_SetPalette(Temp_PFONT, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(Temp_PFONT, SDL_LOGPAL, Palette, 0, Amount);
    }
    if(Temp_LARGEPFONT)
    {
        SDL_SetPalette(Temp_LARGEPFONT, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(Temp_LARGEPFONT, SDL_LOGPAL, Palette, 0, Amount);
    }
    if(Temp_SMALLPFONT)
    {
        SDL_SetPalette(Temp_SMALLPFONT, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(Temp_SMALLPFONT, SDL_LOGPAL, Palette, 0, Amount);
    }
    if(Temp_NOTEPFONT)
    {
        SDL_SetPalette(Temp_NOTEPFONT, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(Temp_NOTEPFONT, SDL_LOGPAL, Palette, 0, Amount);
    }
    if(Temp_NOTELARGEPFONT)
    {
        SDL_SetPalette(Temp_NOTELARGEPFONT, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(Temp_NOTELARGEPFONT, SDL_LOGPAL, Palette, 0, Amount);
    }
    if(Temp_NOTESMALLPFONT)
    {
        SDL_SetPalette(Temp_NOTESMALLPFONT, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(Temp_NOTESMALLPFONT, SDL_LOGPAL, Palette, 0, Amount);
    }
    if(POINTER)
    {
        SDL_SetPalette(POINTER, SDL_PHYSPAL, Palette, 0, Amount);
        SDL_SetPalette(POINTER, SDL_LOGPAL, Palette, 0, Amount);
    }

    SDL_SetPalette(Main_Screen, SDL_PHYSPAL, Palette, 0, Amount);
    SDL_SetPalette(Main_Screen, SDL_LOGPAL, Palette, 0, Amount);
}

// ------------------------------------------------------
// Copy a surface onto the main screen
void Copy(SDL_Surface *Source,
          int x, int y,
          int x1, int y1,
          int x2, int y2)
{
    SDL_Rect Src_Rect;
    SDL_Rect Dst_Rect;

    Dst_Rect.x = x;
    Dst_Rect.y = y;
    Dst_Rect.w = x2 - x1;
    Dst_Rect.h = y2 - y1;

    Src_Rect.x = x1;
    Src_Rect.y = y1;
    Src_Rect.w = Dst_Rect.w + 1;
    Src_Rect.h = Dst_Rect.h + 1;

    SDL_BlitSurface(Source, &Src_Rect, Main_Screen, &Dst_Rect);
}

void Copy_To_Surface(SDL_Surface *Source, SDL_Surface *dest,
                     int x, int y, int x1, int y1, int x2, int y2)
{
    SDL_Rect Src_Rect;
    SDL_Rect Dst_Rect;

    Src_Rect.x = x1;
    Src_Rect.y = y1;
    Src_Rect.w = (x2 - x1);
    Src_Rect.h = (y2 - y1);

    Dst_Rect.x = x;
    Dst_Rect.y = y;
    Dst_Rect.w = (x2 - x1);
    Dst_Rect.h = (y2 - y1);

    SDL_BlitSurface(Source, &Src_Rect, dest, &Dst_Rect);
}

// ------------------------------------------------------
// Print a string on the screen
void PrintXY(int x, int y, int Font_Type, char *String)
{
    int Idx;
    int i;
    SDL_Rect Src_Rect;
    SDL_Rect Dst_Rect;

    y += 2;

    Dst_Rect.y = y;
    Dst_Rect.h = Font_Height;
    Src_Rect.y = 0;
    Src_Rect.h = Font_Height;

    for(i = 0; i < (int) strlen(String); i++)
    {
        Idx = Get_Char_Position(Font_Ascii, Nbr_Letters, String[i]);
        Src_Rect.x = Font_Pos[Idx];
        Src_Rect.w = Font_Size[Idx];
        Dst_Rect.x = x;
        Dst_Rect.w = Src_Rect.w;
        if(Font_Type == USE_FONT)
        {
            SDL_BlitSurface(FONT, &Src_Rect, Main_Screen, &Dst_Rect);
        }
        else
        {
            SDL_BlitSurface(FONT_LOW, &Src_Rect, Main_Screen, &Dst_Rect);
        }
        x += Font_Size[Idx];
    }
}
