/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include "world.h"
#include "settings.h"
#include "race.h"
#include "game.h"
#include "difficulty.h"
#include <iostream>


struct ComparisionDistance
{
    ComparisionDistance(const s32 &index) : center(index)
    {
    }

    bool operator()(const s32 &index1, const s32 &index2) const
    {
        return Maps::GetApproximateDistance(center, index1) < Maps::GetApproximateDistance(center, index2);
    }

    s32 center;
};

Maps::IndexesDistance::IndexesDistance(s32 from, s32 center, uint32_t dist, int sort)
{
    MapsIndexes results;
    GetAroundIndexes(center, dist, sort, results);
    Assign(from, results, sort);
}

Maps::IndexesDistance::IndexesDistance(s32 from, const Indexes &indexes, int sort)
{
    Assign(from, indexes, sort);
}

void Maps::IndexesDistance::Assign(s32 from, const Indexes &indexes, int sort)
{
    reserve(indexes.size());

    for (int indexe : indexes)
        push_back(IndexDistance(indexe, GetApproximateDistance(from, indexe)));

    if (1 == sort)
        std::sort(begin(), end(), IndexDistance::Shortest);
    else if (2 == sort)
        std::sort(begin(), end(), IndexDistance::Longest);
}

bool TileIsObject(s32 index, int obj)
{
    return obj == world.GetTiles(index).GetObject();
}

bool TileIsObjects(s32 index, const u8 *objs)
{
    while (objs && *objs)
    {
        if (*objs == world.GetTiles(index).GetObject()) return true;
        ++objs;
    }
    return false;
}

Maps::Indexes &MapsIndexesFilteredObjects(Maps::Indexes &indexes, const u8 *objs)
{
    indexes.resize(distance(indexes.begin(),
                            remove_if(indexes.begin(), indexes.end(),
                                      [&](auto &tile) {
                                          return !TileIsObjects(tile, objs);
                                      })));

    return indexes;
}

Maps::Indexes &MapsIndexesFilteredObject(Maps::Indexes &indexes, int obj)
{
    indexes.resize(distance(indexes.begin(),
                            remove_if(indexes.begin(), indexes.end(),
                                      [&](auto &tile) {
                                          return !TileIsObject(tile, obj);
                                      })));
    return indexes;
}

std::string Maps::SizeString(int size)
{
    std::string mapsize[] = {"Unknown", _("maps|Small"), _("maps|Medium"), _("maps|Large"), _("maps|Extra Large"),
                             _("maps|Custom Size")};

    switch ((mapsize_t) size)
    {
        case mapsize_t::SMALL:
            return mapsize[1];
        case mapsize_t::MEDIUM:
            return mapsize[2];
        case mapsize_t::LARGE:
            return mapsize[3];
        case mapsize_t::XLARGE:
            return mapsize[4];
        default:
            break;
    }

    return mapsize[5];
}

std::string Maps::GetMinesName(int type)
{
    switch (type)
    {
        case Resource::ORE:
            return _("Ore Mine");
        case Resource::SULFUR:
            return _("Sulfur Mine");
        case Resource::CRYSTAL:
            return _("Crystal Mine");
        case Resource::GEMS:
            return _("Gems Mine");
        case Resource::GOLD:
            return _("Gold Mine");
        default:
            break;
    }

    return _("Mine");
}

s32 Maps::GetDirectionIndex(s32 from, int vector)
{
    switch (vector)
    {
        case Direction::TOP:
            return from - world.w();
        case Direction::TOP_RIGHT:
            return from - world.w() + 1;
        case Direction::RIGHT:
            return from + 1;
        case Direction::BOTTOM_RIGHT:
            return from + world.w() + 1;
        case Direction::BOTTOM:
            return from + world.w();
        case Direction::BOTTOM_LEFT:
            return from + world.w() - 1;
        case Direction::LEFT:
            return from - 1;
        case Direction::TOP_LEFT:
            return from - world.w() - 1;
        default:
            break;
    }

    return -1;
}


Point Maps::GetPoint(s32 index)
{
    return Point(index % world.w(), index / world.w());
}

bool Maps::isValidAbsPoint(const Point &pt)
{
    return isValidAbsPoint(pt.x, pt.y);
}

bool Maps::isValidAbsIndex(s32 ii)
{
    return 0 <= ii && ii < world.w() * world.h();
}

bool Maps::isValidAbsPoint(s32 x, s32 y)
{
    return 0 <= x && world.w() > x && 0 <= y && world.h() > y;
}

/* convert maps point to index maps */
s32 Maps::GetIndexFromAbsPoint(const Point &mp)
{
    return GetIndexFromAbsPoint(mp.x, mp.y);
}

s32 Maps::GetIndexFromAbsPoint(s32 px, s32 py)
{
    s32 res = py * world.w() + px;

    if (px < 0 || py < 0)
    {
        VERBOSE("Maps::GetIndexFromAbsPoint: error coods, " << "x: " << px << ", y: " << py);
        return -1;
    }

    return res;
}

Maps::Indexes Maps::GetAllIndexes()
{
    Indexes result;
    result.assign(world.w() * world.h(), 0);

    for (auto it = result.begin(); it != result.end(); ++it)
        *it = distance(result.begin(), it);
    return result;
}

void Maps::GetAroundIndexes(s32 center, Indexes &result)
{
    result.clear();
    if (!isValidAbsIndex(center))
    {
        return;
    }
    const auto &directions = Direction::All();

    Size wSize(world.w(), world.h());
    for (int direction : directions)
    {
        if (isValidDirection(center, direction, wSize))
            result.push_back(GetDirectionIndex(center, direction));
    }
}

void Maps::GetAroundIndexes(s32 center, int dist, bool sort, Maps::Indexes &results)
{
    results.clear();

    const Point cp = GetPoint(center);

    for (s32 xx = cp.x - dist; xx <= cp.x + dist; ++xx)
        for (s32 yy = cp.y - dist; yy <= cp.y + dist; ++yy)
        {
            if (isValidAbsPoint(xx, yy) && (xx != cp.x || yy != cp.y))
                results.push_back(GetIndexFromAbsPoint(xx, yy));
        }

    if (sort)
        std::sort(results.begin(), results.end(), ComparisionDistance(center));

    return;
}

Maps::Indexes Maps::GetDistanceIndexes(s32 center, int dist)
{
    Indexes results;
    results.reserve(dist * 6);

    const Point cp = GetPoint(center);

    for (s32 xx = cp.x - dist; xx <= cp.x + dist; ++xx)
    {
        if (isValidAbsPoint(xx, cp.y - dist))
            results.push_back(GetIndexFromAbsPoint(xx, cp.y - dist));
        if (isValidAbsPoint(xx, cp.y + dist))
            results.push_back(GetIndexFromAbsPoint(xx, cp.y + dist));
    }

    for (s32 yy = cp.y - dist + 1; yy < cp.y + dist; ++yy)
    {
        if (isValidAbsPoint(cp.x - dist, yy))
            results.push_back(GetIndexFromAbsPoint(cp.x - dist, yy));
        if (isValidAbsPoint(cp.x + dist, yy))
            results.push_back(GetIndexFromAbsPoint(cp.x + dist, yy));
    }

    return results;
}

void Maps::ClearFog(s32 index, int scoute, int color)
{
    if (0 == scoute || !isValidAbsIndex(index))
        return;
    const Point center = GetPoint(index);
    const Settings &conf = Settings::Get();

    // AI advantage
    if (world.GetKingdom(color).isControlAI())
    {
        switch (conf.GameDifficulty())
        {
            case Difficulty::NORMAL:
                scoute += 2;
                break;
            case Difficulty::HARD:
                scoute += 3;
                break;
            case Difficulty::EXPERT:
                scoute += 4;
                break;
            case Difficulty::IMPOSSIBLE:
                scoute += 6;
                break;
            default:
                break;
        }
    }

    int colors = conf.ExtUnionsAllowViewMaps() ? Players::GetPlayerFriends(color) : color;

    for (s32 y = center.y - scoute; y <= center.y + scoute; ++y)
        for (s32 x = center.x - scoute; x <= center.x + scoute; ++x)
            if (isValidAbsPoint(x, y) &&
                (scoute + scoute / 2) >= abs(x - center.x) + abs(y - center.y))
                world.GetTiles(GetIndexFromAbsPoint(x, y)).ClearFog(colors);
}

Maps::Indexes Maps::ScanAroundObjects(s32 center, const u8 *objs)
{
    Indexes results;
    GetAroundIndexes(center, results);
    return MapsIndexesFilteredObjects(results, objs);
}


void Maps::ScanAroundObject(s32 center, int obj, MapsIndexes &resultsScan)
{
    GetAroundIndexes(center, resultsScan);
    MapsIndexesFilteredObject(resultsScan, obj);
}

Maps::Indexes Maps::ScanAroundObject(s32 center, uint32_t dist, int obj)
{
    Indexes results;
    GetAroundIndexes(center, dist, true, results);
    return MapsIndexesFilteredObject(results, obj);
}

Maps::Indexes Maps::ScanAroundObjects(s32 center, uint32_t dist, const u8 *objs)
{
    Indexes results;
    GetAroundIndexes(center, dist, true, results);
    return MapsIndexesFilteredObjects(results, objs);
}

Maps::Indexes Maps::GetObjectPositions(int obj, bool check_hero)
{
    Indexes results = GetAllIndexes();
    MapsIndexesFilteredObject(results, obj);

    if (check_hero && obj != MP2::OBJ_HEROES)
    {
        const Indexes &v = GetObjectPositions(MP2::OBJ_HEROES, false);
        for (int it : v)
        {
            const Heroes *hero = world.GetHeroes(GetPoint(it));
            if (hero && obj == hero->GetMapsObject())
                results.push_back(it);
        }
    }

    return results;
}

Maps::Indexes Maps::GetObjectPositions(s32 center, int obj, bool check_hero)
{
    Indexes results = GetObjectPositions(obj, check_hero);
    sort(results.begin(), results.end(), ComparisionDistance(center));
    return results;
}

Maps::Indexes Maps::GetObjectsPositions(const u8 *objs)
{
    Indexes results = GetAllIndexes();
    return MapsIndexesFilteredObjects(results, objs);
}

bool MapsTileIsUnderProtection(s32 from, s32 index) /* from: center, index: monster */
{
    bool result = false;
    const Maps::Tiles &tile1 = world.GetTiles(from);
    const Maps::Tiles &tile2 = world.GetTiles(index);

    if (tile1.isWater() != tile2.isWater())
    {
        return result;
    }
    /* if monster can attack to */
    result = (tile2.GetPassable() & Direction::Get(index, from)) &&
             (tile1.GetPassable() & Direction::Get(from, index));

    if (!result)
    {
        /* h2 specific monster attack: BOTTOM_LEFT impassable! */
        if (Direction::BOTTOM_LEFT == Direction::Get(index, from) &&
            (Direction::LEFT & tile2.GetPassable()) && (Direction::TOP & tile1.GetPassable()))
            result = true;
        else
            /* h2 specific monster attack: BOTTOM_RIGHT impassable! */
        if (Direction::BOTTOM_RIGHT == Direction::Get(index, from) &&
            (Direction::RIGHT & tile2.GetPassable()) && (Direction::TOP & tile1.GetPassable()))
            result = true;
    }

    return result;
}

bool Maps::IsNearTiles(s32 index1, s32 index2)
{
    return DIRECTION_ALL & Direction::Get(index1, index2);
}

bool Maps::TileIsUnderProtection(s32 center)
{
    return MP2::OBJ_MONSTER == world.GetTiles(center).GetObject() || !GetTilesUnderProtection(center).empty();
}

namespace
{
    Maps::Indexes tilesIndexesUnderProtection;
}

Maps::Indexes Maps::GetTilesUnderProtection(s32 center)
{

    ScanAroundObject(center, MP2::OBJ_MONSTER, tilesIndexesUnderProtection);

    if (!tilesIndexesUnderProtection.empty())
    {
        auto removeIt = remove_if(
                tilesIndexesUnderProtection.begin(), tilesIndexesUnderProtection.end(),
                [&](auto &it) {
                    return !MapsTileIsUnderProtection(it, center);
                });
        int distVal = distance(tilesIndexesUnderProtection.begin(),
                               removeIt);
        tilesIndexesUnderProtection.resize(distVal);
    }

    if (MP2::OBJ_MONSTER == world.GetTiles(center).GetObject())
        tilesIndexesUnderProtection.push_back(center);

    return tilesIndexesUnderProtection;
}

uint32_t Maps::GetApproximateDistance(s32 index1, s32 index2)
{
    const Size sz(GetPoint(index1) - GetPoint(index2));
    return max(sz.w, sz.h);
}

void Maps::MinimizeAreaForCastle(const Point &center)
{
    // reset castle ID
    for (s32 yy = -3; yy < 2; ++yy)
        for (s32 xx = -2; xx < 3; ++xx)
        {
            Tiles &tile = world.GetTiles(center.x + xx, center.y + yy);

            if (MP2::OBJN_RNDCASTLE == tile.GetObject() ||
                MP2::OBJN_RNDTOWN == tile.GetObject() ||
                MP2::OBJN_CASTLE == tile.GetObject())
                tile.SetObject(MP2::OBJ_ZERO);
        }

    // set minimum area castle ID
    for (s32 yy = -1; yy < 1; ++yy)
        for (s32 xx = -2; xx < 3; ++xx)
        {
            Tiles &tile = world.GetTiles(center.x + xx, center.y + yy);

            // skip angle
            if (yy == -1 && (xx == -2 || xx == 2)) continue;

            tile.SetObject(MP2::OBJN_CASTLE);
        }

    // restore center ID
    world.GetTiles(center.x, center.y).SetObject(MP2::OBJ_CASTLE);
}

/* correct sprites for RND castles */
void Maps::UpdateRNDSpriteForCastle(const Point &center, int race, bool castle)
{
    /*
    castle size: T and B - sprite, S - shadow, XX - center

                  T0
          S1S1T1T1T1T1T1
        S2S2S2T2T2T2T2T2
          S3S3B1B1XXB1B1
            S4B2B2  B2B2
    */
    Indexes coords;
    coords.reserve(21);

    // T0
    if (castle) coords.push_back(GetIndexFromAbsPoint(center.x, center.y - 3));
    // T1
    coords.push_back(GetIndexFromAbsPoint(center.x - 2, center.y - 2));
    coords.push_back(GetIndexFromAbsPoint(center.x - 1, center.y - 2));
    coords.push_back(GetIndexFromAbsPoint(center.x, center.y - 2));
    coords.push_back(GetIndexFromAbsPoint(center.x + 1, center.y - 2));
    coords.push_back(GetIndexFromAbsPoint(center.x + 2, center.y - 2));
    // T2
    coords.push_back(GetIndexFromAbsPoint(center.x - 2, center.y - 1));
    coords.push_back(GetIndexFromAbsPoint(center.x - 1, center.y - 1));
    coords.push_back(GetIndexFromAbsPoint(center.x, center.y - 1));
    coords.push_back(GetIndexFromAbsPoint(center.x + 1, center.y - 1));
    coords.push_back(GetIndexFromAbsPoint(center.x + 2, center.y - 1));
    // B1
    coords.push_back(GetIndexFromAbsPoint(center.x - 2, center.y));
    coords.push_back(GetIndexFromAbsPoint(center.x - 1, center.y));
    coords.push_back(GetIndexFromAbsPoint(center.x, center.y));
    coords.push_back(GetIndexFromAbsPoint(center.x + 1, center.y));
    coords.push_back(GetIndexFromAbsPoint(center.x + 2, center.y));
    // B2
    coords.push_back(GetIndexFromAbsPoint(center.x - 2, center.y + 1));
    coords.push_back(GetIndexFromAbsPoint(center.x - 1, center.y + 1));
    coords.push_back(GetIndexFromAbsPoint(center.x, center.y + 1));
    coords.push_back(GetIndexFromAbsPoint(center.x + 1, center.y + 1));
    coords.push_back(GetIndexFromAbsPoint(center.x + 2, center.y + 1));

    Tiles &tile_center = world.GetTiles(center.x, center.y);

    // correct only RND town and castle
    switch (tile_center.GetObject())
    {
        case MP2::OBJ_RNDTOWN:
        case MP2::OBJ_RNDCASTLE:
            break;

        default:
            return;
    }

    // modify all rnd sprites
    for (Indexes::const_iterator
                 it = coords.begin(); it != coords.end(); ++it)
    {
        if (!isValidAbsIndex(*it))
            continue;
        TilesAddon *addon = world.GetTiles(*it).FindObject(MP2::OBJ_RNDCASTLE);
        if (!addon)
            continue;
        addon->object -= 12;

        switch (race)
        {
            case Race::KNGT:
                break;
            case Race::BARB:
                addon->index += 32;
                break;
            case Race::SORC:
                addon->index += 64;
                break;
            case Race::WRLK:
                addon->index += 96;
                break;
            case Race::WZRD:
                addon->index += 128;
                break;
            case Race::NECR:
                addon->index += 160;
                break;
            default:
                break;
        }
    }
}

void Maps::UpdateSpritesFromTownToCastle(const Point &center)
{
    // correct area maps sprites
    Indexes coords;
    coords.reserve(15);

    // T1
    coords.push_back(GetIndexFromAbsPoint(center.x - 2, center.y - 2));
    coords.push_back(GetIndexFromAbsPoint(center.x - 1, center.y - 2));
    coords.push_back(GetIndexFromAbsPoint(center.x, center.y - 2));
    coords.push_back(GetIndexFromAbsPoint(center.x + 1, center.y - 2));
    coords.push_back(GetIndexFromAbsPoint(center.x + 2, center.y - 2));
    // T2
    coords.push_back(GetIndexFromAbsPoint(center.x - 2, center.y - 1));
    coords.push_back(GetIndexFromAbsPoint(center.x - 1, center.y - 1));
    coords.push_back(GetIndexFromAbsPoint(center.x, center.y - 1));
    coords.push_back(GetIndexFromAbsPoint(center.x + 1, center.y - 1));
    coords.push_back(GetIndexFromAbsPoint(center.x + 2, center.y - 1));
    // B1
    coords.push_back(GetIndexFromAbsPoint(center.x - 2, center.y));
    coords.push_back(GetIndexFromAbsPoint(center.x - 1, center.y));
    coords.push_back(GetIndexFromAbsPoint(center.x, center.y));
    coords.push_back(GetIndexFromAbsPoint(center.x + 1, center.y));
    coords.push_back(GetIndexFromAbsPoint(center.x + 2, center.y));

    // modify all town sprites
    for (Indexes::const_iterator
                 it = coords.begin(); it != coords.end(); ++it)
    {
        if (!isValidAbsIndex(*it))
            continue;
        TilesAddon *addon = world.GetTiles(*it).FindObject(MP2::OBJ_CASTLE);
        if (addon) addon->index -= 16;
    }
    // T0
    if (!isValidAbsIndex(GetIndexFromAbsPoint(center.x, center.y - 3) &&
                         isValidAbsIndex(GetIndexFromAbsPoint(center.x, center.y - 2))))
        return;
    TilesAddon *addon = world.GetTiles(GetIndexFromAbsPoint(center.x, center.y - 2)).FindObject(MP2::OBJ_CASTLE);
    if (!addon)
        return;
    world.GetTiles(GetIndexFromAbsPoint(center.x, center.y - 3)).AddonsPushLevel2(
            TilesAddon(addon->level, addon->uniq, addon->object, addon->index - 3));
}

int Maps::TileIsCoast(s32 center, int filter)
{
    int result = 0;
    const auto &directions = Direction::All();

    Size wSize(world.w(), world.h());
    for (int direction : directions)
        if ((direction & filter) && isValidDirection(center, direction, wSize) &&
            world.GetTiles(GetDirectionIndex(center, direction)).isWater())
            result |= direction;

    return result;
}

ByteVectorReader &operator>>(ByteVectorReader &sb, IndexObject &st)
{
    return sb >> st.Value.first >> st.Value.second;
}

ByteVectorWriter &operator<<(ByteVectorWriter &sb, const IndexObject &st)
{
    return sb << st.Value.first << st.Value.second;
}


ByteVectorWriter &operator<<(ByteVectorWriter &sb, const IndexDistance &st)
{
    return sb << st.first << st.second;
}

ByteVectorReader &operator>>(ByteVectorReader &sb, IndexDistance &st)
{
    return sb >> st.first >> st.second;
}

ByteVectorWriter &operator<<(ByteVectorWriter &sb, const ObjectColor &st)
{
    return sb << st.first << st.second;
}

ByteVectorReader &operator>>(ByteVectorReader &sb, ObjectColor &st)
{
    return sb >> st.first >> st.second;
}

ByteVectorWriter &operator<<(ByteVectorWriter &sb, const ResourceCount &st)
{
    return sb << st.first << st.second;
}
ByteVectorReader &operator>>(ByteVectorReader &sb, ResourceCount &st)
{
    return sb >> st.first >> st.second;
}
