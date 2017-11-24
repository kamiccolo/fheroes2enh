/***************************************************************************
 *   Copyright (C) 2008 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <sstream>
#include <string>

#include "tools.h"
#include "types.h"
#include "system.h"
#include "error.h"
#include "display.h"

Display::Display() = default;


Display::~Display() = default;

void Display::SetVideoMode(int w, int h, bool fullscreen)
{
    u32 flags = System::GetRenderFlags();

    if (fullscreen)
        flags |= SDL_FULLSCREEN;

    surface = SDL_SetVideoMode(w, h, 0, flags);

    if (!surface)
        Error::Except(__FUNCTION__, SDL_GetError());
}

Size Display::GetSize() const
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if(window)
    {
        int dw, dh;
        SDL_GetWindowSize(window, &dw, &dh);
        return Size(dw, dh);
    }

    return Size(0, 0);
#else
    return Size(w(), h());
#endif
}

void Display::Flip()
{
    SDL_Flip(surface);
}

void Display::Present()
{
    SDL_Flip(surface);
}

void Display::ToggleFullScreen()
{
    SDL_WM_ToggleFullScreen(surface);
}

void Display::SetCaption(const char *str)
{
    SDL_WM_SetCaption(str, nullptr);
}

void Display::SetIcons(Surface &icons)
{
    SDL_WM_SetIcon(icons(), nullptr);
}

Size Display::GetMaxMode(bool rotate) const
{
    Size result;
    SDL_Rect **modes = SDL_ListModes(nullptr, SDL_ANYFORMAT);

    if (modes == (SDL_Rect **) 0 ||
        modes == (SDL_Rect **) -1)
    {
        ERROR("GetMaxMode: " << "no modes available");
    } else
    {
        int max = 0;
        int cur = 0;

        for (int ii = 0; modes[ii]; ++ii)
        {
            if (max < modes[ii]->w * modes[ii]->h)
            {
                max = modes[ii]->w * modes[ii]->h;
                cur = ii;
            }
        }

        result.w = modes[cur]->w;
        result.h = modes[cur]->h;

        if (rotate && result.w < result.h)
            std::swap(result.w, result.h);
    }

    return result;
}

std::string Display::GetInfo() const
{
    std::ostringstream os;
    char namebuf[12];

    os << "Display::" << "GetInfo: " <<
       GetString(GetSize()) << ", " <<
       "driver: " << SDL_VideoDriverName(namebuf, 12);

    return os.str();
}

Surface Display::GetSurface(const Rect &rt) const
{
    Surface res(rt, GetFormat());
    Blit(rt, Point(0, 0), res);
    return res; //Surface(SDL_DisplayFormat(res()));
}

void Display::Clear()
{
    Fill(ColorBlack);
}

/* hide system cursor */
void Display::HideCursor()
{
    SDL_ShowCursor(SDL_DISABLE);
}

/* show system cursor */
void Display::ShowCursor()
{
    SDL_ShowCursor(SDL_ENABLE);
}

void Display::Fade(const Surface &top, const Surface &back, const Point &pt, int level, int delay)
{
    Surface shadow = top.GetSurface();
    int alpha = 255;
    const int step = 10;
    const int min = step + 5;
    const int delay2 = (delay * step) / (alpha - min);

    while (alpha > min + level)
    {
        back.Blit(*this);
        shadow.SetAlphaMod(alpha);
        shadow.Blit(*this);
        Flip();
        alpha -= step;
        DELAY(delay2);
    }
}

void Display::Fade(int delay)
{
    Surface top = GetSurface();
    Surface back(GetSize(), false);
    back.Fill(ColorBlack);
    Fade(top, back, Point(0, 0), 5, delay);
    Blit(back);
    Flip();
}

void Display::Rise(const Surface &top, const Surface &back, const Point &pt, int level, int delay)
{
    Surface shadow = top.GetSurface();
    int alpha = 0;
    const int step = 10;
    const int max = level - step;
    const int delay2 = (delay * step) / max;

    while (alpha < max)
    {
        back.Blit(*this);
        shadow.SetAlphaMod(alpha);
        shadow.Blit(*this);
        Flip();
        alpha += step;
        DELAY(delay2);
    }
}

void Display::Rise(int delay)
{
    Surface top = GetSurface();
    Surface back(GetSize(), false);
    back.Fill(ColorBlack);
    Rise(top, back, Point(0, 0), 250, delay);
    Blit(top);
    Flip();
}

/* get video display */
Display &Display::Get()
{
    static Display inside;
    return inside;
}


bool Display::isDisplay() const
{
    return true;
}

Surface Display::GetSurface() const
{
    return GetSurface(Rect(Point(0, 0), GetSize()));
}


Texture::Texture(const Surface &sf)
{
    Set(SDL_DisplayFormatAlpha(sf()));
    //Set(SDL_DisplayFormat(sf()));
}
