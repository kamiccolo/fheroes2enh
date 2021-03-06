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

#include <iostream>

#include "system.h"
#include "tools.h"
#include "audio_music.h"
#include "audio_mixer.h"

#ifdef WITH_MIXER

#include "SDL_mixer.h"

namespace Music
{
    void Play(Mix_Music *mix, uint32_t id, bool loop);

    Mix_Music *music = nullptr;
    int fadein = 0;
    int fadeout = 0;
}

void Music::Play(Mix_Music *mix, uint32_t id, bool loop)
{
    Reset();

    int res = fadein ?
              Mix_FadeInMusic(mix, loop ? -1 : 0, fadein) : Mix_PlayMusic(mix, loop ? -1 : 0);

    if (res < 0)
    {
        ERROR(Mix_GetError());
    } else
        music = mix;
}

void Music::Play(const std::vector<u8> &v, bool loop)
{
    if (!Mixer::isValid() || v.empty())
        return;
    uint32_t id = CheckSum(v);
    SDL_RWops *rwops = SDL_RWFromConstMem(&v[0], v.size());

    Mix_Music *mix = Mix_LoadMUS_RW(rwops);

    SDL_FreeRW(rwops);
    Music::Play(mix, id, loop);
}

void Music::Play(const std::string &file, bool loop)
{
    if (!Mixer::isValid())
        return;
    uint32_t id = CheckSum(file);
    Mix_Music *mix = Mix_LoadMUS(file.c_str());

    if (!mix)
    {
        ERROR(Mix_GetError());
    } else
        Music::Play(mix, id, loop);
}

void Music::SetFadeIn(int f)
{
    fadein = f;
}

void Music::SetFadeOut(int f)
{
    fadeout = f;
}

u16 Music::Volume(s16 vol)
{
    return Mixer::isValid() ? (Mix_VolumeMusic(vol > MIX_MAX_VOLUME ? MIX_MAX_VOLUME : vol)) : 0;
}

void Music::Pause()
{
    if (music) Mix_PauseMusic();
}

void Music::Resume()
{
    if (music) Mix_ResumeMusic();
}

void Music::Reset()
{
    if (!music)
        return;
    if (fadeout)
    {
        while (!Mix_FadeOutMusic(fadeout) && Mix_PlayingMusic()) SDL_Delay(50);
    } else
        Mix_HaltMusic();

    Mix_FreeMusic(music);
    music = nullptr;
}

bool Music::isPlaying()
{
    return music && Mix_PlayingMusic();
}

bool Music::isPaused()
{
    return music && Mix_PausedMusic();
}

void Music::SetExtCommand(const std::string &)
{
}

#else

#include <algorithm>

namespace Music
{
    enum
    {
        UNUSED = 0, PLAY = 0x01, PAUSE = 0x02, LOOP = 0x04
    };
    string command;
}

struct info_t
{
    info_t() : status(0)
    {}

    string file;
    int status;
};

int callbackPlayMusic(void *ptr)
{
#if WIN32
    return 0;
#endif
    if (ptr && System::ShellCommand(nullptr))
    {
        info_t *info = reinterpret_cast<info_t *>(ptr);
        ostringstream os;
        os << Music::command << " " << info->file;

        info->status |= Music::PLAY;

        do
        {
            System::ShellCommand(os.str().c_str());
            DELAY(100);
        } while (info->status & Music::LOOP);

        info->status &= ~Music::PLAY;

        return 0;
    }

    return -1;
}

struct play_t : pair<SDL::Thread, info_t>
{
    play_t()
    {}

    bool operator==(const string &f) const
    { return f == second.file; }

    void Run(const info_t &info)
    {
        second = info;
        first.Create(callbackPlayMusic, &second);
    }

    void Run()
    { first.Create(callbackPlayMusic, &second); }

    void Stop()
    {
        if (System::GetEnvironment("MUSIC_WRAPPER")) RunMusicWrapper("stop");
        first.Kill();
        second.status = Music::UNUSED;
    }

    void RunMusicWrapper(const char *action)
    {
        ostringstream os;
        os << System::GetEnvironment("MUSIC_WRAPPER") << " " << action << " " << second.file;
        System::ShellCommand(os.str().c_str());
    }

    void Pause()
    {
        RunMusicWrapper("pause");
        second.status |= Music::PAUSE;
    }

    void Continue()
    {
        RunMusicWrapper("continue");
        second.status &= ~Music::PAUSE;
    }

    bool isPlay() const
    { return second.status & Music::PLAY; }

    bool isPaused() const
    { return second.status & Music::PAUSE; }

    static bool isPlaying(const play_t &p)
    { return p.isPlay() && !p.isPaused(); }

    static bool isRunning(const play_t &p)
    { return p.first.IsRun(); }

    static bool isFree(const play_t &p)
    { return p.second.status == Music::UNUSED; }
};

namespace Music
{
    list<play_t> musics;
    list<play_t>::iterator current = musics.end();
}

void Music::SetExtCommand(const string &cmd)
{
    command = cmd;
}

void Music::Play(const vector<u8> &v, bool loop)
{
}

void Music::Play(const string &f, bool loop)
{
    auto it = find(musics.begin(), musics.end(), f);

    // skip repeat
    if (it != musics.end())
    {
        if ((*it).isPaused())
        {
            Pause();
            current = it;
            Resume();
            DELAY(100);
        }

        if ((*it).isPlay()) return;
    }

    // stop run
    Pause();

    info_t info;
    info.file = f;
    info.status = loop ? LOOP : 0;

    it = find_if(musics.begin(), musics.end(), play_t::isFree);

    if (it == musics.end())
    {
        musics.push_back(play_t());
        it = --musics.end();
    }

    (*it).Run(info);
    current = it;
}

void Music::SetFadeIn(int f)
{
}

void Music::SetFadeOut(int f)
{
}

u16 Music::Volume(s16 vol)
{
    return 0;
}

void Music::Pause()
{
    if (!System::GetEnvironment("MUSIC_WRAPPER"))
        Reset();
    else if (current != musics.end() && (*current).isPlay() && !(*current).isPaused())
        (*current).Pause();
}

void Music::Resume()
{
    if (current != musics.end())
    {
        if (!System::GetEnvironment("MUSIC_WRAPPER"))
            (*current).Run();
        else if ((*current).isPlay() && (*current).isPaused())
            (*current).Continue();
    }
}

bool Music::isPlaying()
{
    auto it = find_if(musics.begin(), musics.end(), play_t::isPlaying);
    return it != musics.end();
}

bool Music::isPaused()
{
    return false;
}

void Music::Reset()
{
    auto it = find_if(musics.begin(), musics.end(), play_t::isRunning);

    if (it != musics.end())
        (*it).Stop();
}

#endif
