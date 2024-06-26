//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "compat.h"
#include "osdcmds.h"
#include "baselayer.h"
#include "duke3d.h"
#include "premap.h"
#include "menus.h"
#include "osd.h"
#include "osdfuncs.h"
#include "gamedef.h"

#include <ctype.h>
#include <limits.h>
#include "enet/enet.h"

extern int32_t voting, g_doQuickSave;
struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat;
float r_ambientlight = 1.0, r_ambientlightrecip = 1.0;
extern int32_t althud_numbertile, althud_numberpal, althud_shadows, althud_flashing, hud_glowingquotes;
extern int32_t hud_showmapname;
extern int32_t r_maxfps;
extern uint32_t g_frameDelay;
extern int32_t demorec_diffs_cvar, demorec_force_cvar, demorec_seeds_cvar, demoplay_diffs, demoplay_showsync;
extern int32_t demorec_difftics_cvar, demorec_diffcompress_cvar, demorec_synccompress_cvar;
extern void G_CheckPlayerColor(int32_t *color,int32_t prev_color);

static inline int32_t osdcmd_quit(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    G_GameQuit();
    return OSDCMD_OK;
}

static int32_t osdcmd_changelevel(const osdfuncparm_t *parm)
{
    int32_t volume=0,level;
    char *p;

    if (!VOLUMEONE)
    {
        if (parm->numparms != 2) return OSDCMD_SHOWHELP;

        volume = strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
        level = strtol(parm->parms[1], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    }
    else
    {
        if (parm->numparms != 1) return OSDCMD_SHOWHELP;

        level = strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    }

    if (volume < 0) return OSDCMD_SHOWHELP;
    if (level < 0) return OSDCMD_SHOWHELP;

    if (!VOLUMEONE)
    {
        if (volume > g_numVolumes)
        {
            OSD_Printf("changelevel: invalid volume number (range 1-%d)\n",g_numVolumes);
            return OSDCMD_OK;
        }
    }

    if (level > MAXLEVELS || MapInfo[volume *MAXLEVELS+level].filename == NULL)
    {
        OSD_Printf("changelevel: invalid level number\n");
        return OSDCMD_SHOWHELP;
    }

    if (numplayers > 1)
    {
        if (g_netServer)
            Net_NewGame(volume,level);
        else if (voting == -1)
        {
            ud.m_volume_number = volume;
            ud.m_level_number = level;

            if (g_player[myconnectindex].ps->i)
            {
                int32_t i;

                for (i=0; i<MAXPLAYERS; i++)
                {
                    g_player[i].vote = 0;
                    g_player[i].gotvote = 0;
                }

                g_player[myconnectindex].vote = g_player[myconnectindex].gotvote = 1;

                voting = myconnectindex;

                tempbuf[0] = PACKET_MAP_VOTE_INITIATE;
                tempbuf[1] = myconnectindex;
                tempbuf[2] = ud.m_volume_number;
                tempbuf[3] = ud.m_level_number;

                enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(tempbuf, 4, ENET_PACKET_FLAG_RELIABLE));
            }
            if ((GametypeFlags[ud.m_coop] & GAMETYPE_PLAYERSFRIENDLY) && !(GametypeFlags[ud.m_coop] & GAMETYPE_TDM))
                ud.m_noexits = 0;

            g_player[myconnectindex].ps->gm |= MODE_MENU;
            ChangeToMenu(603);
        }
        return OSDCMD_OK;
    }
    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        // in-game behave like a cheat
        osdcmd_cheatsinfo_stat.cheatnum = 2;
        osdcmd_cheatsinfo_stat.volume   = volume;
        osdcmd_cheatsinfo_stat.level    = level;
    }
    else
    {
        // out-of-game behave like a menu command
        osdcmd_cheatsinfo_stat.cheatnum = -1;

        ud.m_volume_number = volume;
        ud.m_level_number = level;

        ud.m_monsters_off = ud.monsters_off = 0;

        ud.m_respawn_items = 0;
        ud.m_respawn_inventory = 0;

        ud.multimode = 1;

        G_NewGame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
        if (G_EnterLevel(MODE_GAME)) G_BackToMenu();
    }

    return OSDCMD_OK;
}

static CACHE1D_FIND_REC *findfiles = NULL;
static int32_t numfiles = 0;

static void clearfilenames(void)
{
    klistfree(findfiles);
    findfiles = NULL;
    numfiles = 0;
}

static int32_t getfilenames(char *path)
{
    CACHE1D_FIND_REC *r;

    clearfilenames();
    findfiles = klistpath(path,"*.MAP",CACHE1D_FIND_FILE);
    for (r = findfiles; r; r=r->next) numfiles++;
    return(0);
}

static int32_t osdcmd_map(const osdfuncparm_t *parm)
{
    int32_t i;
    CACHE1D_FIND_REC *r;
    char filename[256];

    if (parm->numparms != 1)
    {
        int32_t maxwidth = 0;

        getfilenames("/");

        for (r=findfiles; r!=NULL; r=r->next)
            maxwidth = max((unsigned)maxwidth,Bstrlen(r->name));

        if (maxwidth > 0)
        {
            int32_t x = 0, count = 0;
            maxwidth += 3;
            OSD_Printf(OSDTEXT_RED "Map listing:\n");
            for (r=findfiles; r!=NULL; r=r->next)
            {
                OSD_Printf("%-*s",maxwidth,r->name);
                x += maxwidth;
                count++;
                if (x > OSD_GetCols() - maxwidth)
                {
                    x = 0;
                    OSD_Printf("\n");
                }
            }
            if (x) OSD_Printf("\n");
            OSD_Printf(OSDTEXT_RED "Found %d maps\n",numfiles);
        }

        return OSDCMD_SHOWHELP;
    }

#if 0
    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }
#endif

    strcpy(filename,parm->parms[0]);
    if (strchr(filename,'.') == 0)
        strcat(filename,".map");

    if ((i = kopen4loadfrommod(filename,0)) < 0)
    {
        OSD_Printf(OSD_ERROR "map: file \"%s\" not found.\n", filename);
        return OSDCMD_OK;
    }
    kclose(i);

    boardfilename[0] = '/';
    boardfilename[1] = 0;
    strcat(boardfilename, filename);

    if (numplayers > 1)
    {
        if (g_netServer)
        {
            Net_SendUserMapName();
            ud.m_volume_number = 0;
            ud.m_level_number = 7;
            Net_NewGame(ud.m_volume_number, ud.m_level_number);
        }
        else if (voting == -1)
        {
            Net_SendUserMapName();

            ud.m_volume_number = 0;
            ud.m_level_number = 7;

            if (g_player[myconnectindex].ps->i)
            {
                int32_t i;

                for (i=0; i<MAXPLAYERS; i++)
                {
                    g_player[i].vote = 0;
                    g_player[i].gotvote = 0;
                }

                g_player[myconnectindex].vote = g_player[myconnectindex].gotvote = 1;
                voting = myconnectindex;

                tempbuf[0] = PACKET_MAP_VOTE_INITIATE;
                tempbuf[1] = myconnectindex;
                tempbuf[2] = ud.m_volume_number;
                tempbuf[3] = ud.m_level_number;

                enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(tempbuf, 4, ENET_PACKET_FLAG_RELIABLE));
            }
            if ((GametypeFlags[ud.m_coop] & GAMETYPE_PLAYERSFRIENDLY) && !(GametypeFlags[ud.m_coop] & GAMETYPE_TDM))
                ud.m_noexits = 0;

            g_player[myconnectindex].ps->gm |= MODE_MENU;
            ChangeToMenu(603);
        }
        return OSDCMD_OK;
    }

    osdcmd_cheatsinfo_stat.cheatnum = -1;
    ud.m_volume_number = 0;
    ud.m_level_number = 7;

    ud.m_monsters_off = ud.monsters_off = 0;

    ud.m_respawn_items = 0;
    ud.m_respawn_inventory = 0;

    ud.multimode = 1;

    G_NewGame(ud.m_volume_number,ud.m_level_number,ud.m_player_skill);
    if (G_EnterLevel(MODE_GAME)) G_BackToMenu();

    return OSDCMD_OK;
}

static int32_t osdcmd_god(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        osdcmd_cheatsinfo_stat.cheatnum = 0;
    }
    else
    {
        OSD_Printf("god: Not in a single-player game.\n");
    }

    return OSDCMD_OK;
}

static int32_t osdcmd_noclip(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        osdcmd_cheatsinfo_stat.cheatnum = 20;
    }
    else
    {
        OSD_Printf("noclip: Not in a single-player game.\n");
    }

    return OSDCMD_OK;
}

static int32_t osdcmd_restartsound(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    S_MusicShutdown();
    S_SoundShutdown();

    S_SoundStartup();
    S_MusicStartup();

    FX_StopAllSounds();
    S_ClearSoundLocks();

    if (ud.config.MusicToggle == 1)
    {
        if (ud.recstat != 2 && g_player[myconnectindex].ps->gm&MODE_GAME)
        {
            if (MapInfo[(uint8_t)g_musicIndex].musicfn != NULL)
                S_PlayMusic(&MapInfo[(uint8_t)g_musicIndex].musicfn[0],g_musicIndex);
        }
        else S_PlayMusic(&EnvMusicFilename[0][0],MAXVOLUMES*MAXLEVELS);
    }

    return OSDCMD_OK;
}

static int32_t osdcmd_restartvid(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    resetvideomode();
    if (setgamemode(ud.config.ScreenMode,ud.config.ScreenWidth,ud.config.ScreenHeight,ud.config.ScreenBPP))
        G_GameExit("restartvid: Reset failed...\n");
    onvideomodechange(ud.config.ScreenBPP>8);
    G_UpdateScreenArea();

    return OSDCMD_OK;
}

static int32_t osdcmd_vidmode(const osdfuncparm_t *parm)
{
    int32_t newbpp = ud.config.ScreenBPP, newwidth = ud.config.ScreenWidth,
            newheight = ud.config.ScreenHeight, newfs = ud.config.ScreenMode;
    if (parm->numparms < 1 || parm->numparms > 4) return OSDCMD_SHOWHELP;

    switch (parm->numparms)
    {
    case 1: // bpp switch
        newbpp = Batol(parm->parms[0]);
        break;
    case 2: // res switch
        newwidth = Batol(parm->parms[0]);
        newheight = Batol(parm->parms[1]);
        break;
    case 3: // res & bpp switch
    case 4:
        newwidth = Batol(parm->parms[0]);
        newheight = Batol(parm->parms[1]);
        newbpp = Batol(parm->parms[2]);
        if (parm->numparms == 4)
            newfs = (Batol(parm->parms[3]) != 0);
        break;
    }

    if (setgamemode(newfs,newwidth,newheight,newbpp))
    {
        initprintf("vidmode: Mode change failed!\n");
        if (setgamemode(ud.config.ScreenMode, ud.config.ScreenWidth, ud.config.ScreenHeight, ud.config.ScreenBPP))
            G_GameExit("vidmode: Reset failed!\n");
    }
    ud.config.ScreenBPP = newbpp;
    ud.config.ScreenWidth = newwidth;
    ud.config.ScreenHeight = newheight;
    ud.config.ScreenMode = newfs;
    onvideomodechange(ud.config.ScreenBPP>8);
    G_UpdateScreenArea();
    return OSDCMD_OK;
}

static int32_t osdcmd_spawn(const osdfuncparm_t *parm)
{
    uint16_t cstat=0,picnum=0;
    char pal=0;
    int16_t ang=0;
    int16_t set=0, idx;
    vec3_t vect;

    if (numplayers > 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME))
    {
        OSD_Printf("spawn: Can't spawn sprites in multiplayer games or demos\n");
        return OSDCMD_OK;
    }

    switch (parm->numparms)
    {
    case 7: // x,y,z
        vect.x = Batol(parm->parms[4]);
        vect.y = Batol(parm->parms[5]);
        vect.z = Batol(parm->parms[6]);
        set |= 8;
    case 4: // ang
        ang = Batol(parm->parms[3]) & 2047;
        set |= 4;
    case 3: // cstat
        cstat = (uint16_t)Batol(parm->parms[2]);
        set |= 2;
    case 2: // pal
        pal = (uint8_t)Batol(parm->parms[1]);
        set |= 1;
    case 1: // tile number
        if (isdigit(parm->parms[0][0]))
        {
            picnum = (uint16_t)Batol(parm->parms[0]);
        }
        else
        {
            int32_t i,j;
            for (j=0; j<2; j++)
            {
                for (i=0; i<g_numLabels; i++)
                {
                    if (
                        (j == 0 && !Bstrcmp(label+(i<<6),     parm->parms[0])) ||
                        (j == 1 && !Bstrcasecmp(label+(i<<6), parm->parms[0]))
                    )
                    {
                        picnum = (uint16_t)labelcode[i];
                        break;
                    }
                }
                if (i<g_numLabels) break;
            }
            if (i==g_numLabels)
            {
                OSD_Printf("spawn: Invalid tile label given\n");
                return OSDCMD_OK;
            }
        }

        if (picnum >= MAXTILES)
        {
            OSD_Printf("spawn: Invalid tile number\n");
            return OSDCMD_OK;
        }
        break;
    default:
        return OSDCMD_SHOWHELP;
    }

    idx = A_Spawn(g_player[myconnectindex].ps->i, (int16_t)picnum);
    if (set & 1) sprite[idx].pal = (uint8_t)pal;
    if (set & 2) sprite[idx].cstat = (int16_t)cstat;
    if (set & 4) sprite[idx].ang = ang;
    if (set & 8)
    {
        if (setsprite(idx, &vect) < 0)
        {
            OSD_Printf("spawn: Sprite can't be spawned into null space\n");
            deletesprite(idx);
        }
    }

    return OSDCMD_OK;
}

static int32_t osdcmd_setvar(const osdfuncparm_t *parm)
{
    int32_t i, varval;
    char varname[256];

    if (parm->numparms != 2) return OSDCMD_SHOWHELP;

    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    strcpy(varname,parm->parms[1]);
    varval = Batol(varname);
    i = hash_find(&h_gamevars,varname);
    if (i >= 0)
        varval=Gv_GetVar(i, g_player[myconnectindex].ps->i, myconnectindex);

    strcpy(varname,parm->parms[0]);
    i = hash_find(&h_gamevars,varname);
    if (i >= 0)
        Gv_SetVar(i, varval, g_player[myconnectindex].ps->i, myconnectindex);
    return OSDCMD_OK;
}

static int32_t osdcmd_addlogvar(const osdfuncparm_t *parm)
{
    int32_t i;
    char varname[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    strcpy(varname,parm->parms[0]);
    i = hash_find(&h_gamevars,varname);
    if (i >= 0)
        OSD_Printf("%s = %d\n", varname, Gv_GetVar(i, g_player[myconnectindex].ps->i, myconnectindex));
    return OSDCMD_OK;
}

static int32_t osdcmd_setactorvar(const osdfuncparm_t *parm)
{
    int32_t i, varval, ID;
    char varname[256];

    if (parm->numparms != 3) return OSDCMD_SHOWHELP;

    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    ID=Batol(parm->parms[0]);
    if (ID>=MAXSPRITES)
    {
        OSD_Printf("Invalid sprite ID\n");
        return OSDCMD_OK;
    }

    varval = Batol(parm->parms[2]);
    strcpy(varname,parm->parms[2]);
    varval = Batol(varname);
    i = hash_find(&h_gamevars,varname);
    if (i >= 0)
        varval=Gv_GetVar(i, g_player[myconnectindex].ps->i, myconnectindex);

    strcpy(varname,parm->parms[1]);
    i = hash_find(&h_gamevars,varname);
    if (i >= 0)
        Gv_SetVar(i, varval, ID, -1);
    return OSDCMD_OK;
}

static int32_t osdcmd_addpath(const osdfuncparm_t *parm)
{
    char pathname[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strcpy(pathname,parm->parms[0]);
    addsearchpath(pathname);
    return OSDCMD_OK;
}

static int32_t osdcmd_initgroupfile(const osdfuncparm_t *parm)
{
    char file[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    strcpy(file,parm->parms[0]);
    initgroupfile(file);
    return OSDCMD_OK;
}

static int32_t osdcmd_cmenu(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1) return OSDCMD_SHOWHELP;
    if (numplayers > 1)
    {
        OSD_Printf("cmenu: disallowed in multiplayer\n");
        return OSDCMD_OK;
    }
    else
    {
        ChangeToMenu(Batol(parm->parms[0]));
    }

    return OSDCMD_OK;
}

extern void G_SetCrosshairColor(int32_t r, int32_t g, int32_t b);
extern palette_t CrosshairColors;

static int32_t osdcmd_crosshaircolor(const osdfuncparm_t *parm)
{
    int32_t r, g, b;

    if (parm->numparms != 3)
    {
        OSD_Printf("crosshaircolor: r:%d g:%d b:%d\n",CrosshairColors.r,CrosshairColors.g,CrosshairColors.b);
        return OSDCMD_SHOWHELP;
    }
    r = atol(parm->parms[0]);
    g = atol(parm->parms[1]);
    b = atol(parm->parms[2]);
    G_SetCrosshairColor(r,g,b);
    OSD_Printf("%s\n", parm->raw);
    return OSDCMD_OK;
}

/*
static int32_t osdcmd_setbrightness(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
    {
//        OSD_Printf("\"setbri\" \"%d\"\n",ud.brightness>>2);
        return OSDCMD_SHOWHELP;
    }
    ud.brightness = atoi(parm->parms[0])<<2;
    setbrightness(ud.brightness>>2,&g_player[screenpeek].ps->palette[0],0);
    OSD_Printf("setbrightness %d\n",ud.brightness>>2);
    return OSDCMD_OK;
}
*/

static int32_t osdcmd_give(const osdfuncparm_t *parm)
{
    int32_t i;

    if (numplayers != 1 || (g_player[myconnectindex].ps->gm & MODE_GAME) == 0 ||
            g_player[myconnectindex].ps->dead_flag != 0)
    {
        OSD_Printf("give: Cannot give while dead or not in a single-player game.\n");
        return OSDCMD_OK;
    }

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if (!Bstrcasecmp(parm->parms[0], "all"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = 1;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "health"))
    {
        sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health<<1;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "weapons"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = 21;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "ammo"))
    {
        for (i=MAX_WEAPONS-(VOLUMEONE?6:1)-1; i>=PISTOL_WEAPON; i--)
            P_AddAmmo(i,g_player[myconnectindex].ps,g_player[myconnectindex].ps->max_ammo_amount[i]);
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "armor"))
    {
        g_player[myconnectindex].ps->inv_amount[GET_SHIELD] = 100;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "keys"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = 23;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "inventory"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = 22;
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

void onvideomodechange(int32_t newmode)
{
    uint8_t *pal;
    extern int32_t g_crosshairSum;

    if (newmode)
    {
        if (g_player[screenpeek].ps->palette == palette ||
                g_player[screenpeek].ps->palette == waterpal ||
                g_player[screenpeek].ps->palette == titlepal ||
                g_player[screenpeek].ps->palette == animpal ||
                g_player[screenpeek].ps->palette == endingpal ||
                g_player[screenpeek].ps->palette == drealms ||
                g_player[screenpeek].ps->palette == slimepal)
            pal = g_player[screenpeek].ps->palette;
        else
            pal = palette;
    }
    else
    {
        pal = g_player[screenpeek].ps->palette;
    }

#ifdef POLYMER
    if (getrendermode() == 4)
    {
        int32_t i = 0;

        while (i < MAXSPRITES)
        {
            if (actor[i].lightptr)
            {
                polymer_deletelight(actor[i].lightId);
                actor[i].lightptr = NULL;
                actor[i].lightId = -1;
            }
            i++;
        }
    }
#endif

    setbrightness(ud.brightness>>2, pal, 0);
    g_restorePalette = 1;
    g_crosshairSum = 0;
}

static int32_t osdcmd_name(const osdfuncparm_t *parm)
{
    char namebuf[32];

    if (parm->numparms != 1)
    {
        OSD_Printf("\"name\" is \"%s\"\n",szPlayerName);
        return OSDCMD_SHOWHELP;
    }

    Bstrcpy(tempbuf,parm->parms[0]);

    while (Bstrlen(OSD_StripColors(namebuf,tempbuf)) > 10)
        tempbuf[Bstrlen(tempbuf)-1] = '\0';

    Bstrncpy(szPlayerName,tempbuf,sizeof(szPlayerName)-1);
    szPlayerName[sizeof(szPlayerName)-1] = '\0';

    OSD_Printf("name %s\n",szPlayerName);

    Net_SendClientInfo();

    return OSDCMD_OK;
}

static int32_t osdcmd_button(const osdfuncparm_t *parm)
{
    char *p = (char *)parm->name+9;  // skip "gamefunc_"
//    if (g_player[myconnectindex].ps->gm == MODE_GAME) // only trigger these if in game
    extinput[CONFIG_FunctionNameToNum(p)] = 1; // FIXME
    return OSDCMD_OK;
}

keydef_t ConsoleKeys[]=
{
    { "Escape", 0x1 },
    { "1", 0x2 },
    { "2", 0x3 },
    { "3", 0x4 },
    { "4", 0x5 },
    { "5", 0x6 },
    { "6", 0x7 },
    { "7", 0x8 },
    { "8", 0x9 },
    { "9", 0xa },
    { "0", 0xb },
    { "-", 0xc },
    { "=", 0xd },
    { "BakSpc", 0xe },
    { "Tab", 0xf },
    { "Q", 0x10 },
    { "W", 0x11 },
    { "E", 0x12 },
    { "R", 0x13 },
    { "T", 0x14 },
    { "Y", 0x15 },
    { "U", 0x16 },
    { "I", 0x17 },
    { "O", 0x18 },
    { "P", 0x19 },
    { "[", 0x1a },
    { "]", 0x1b },
    { "Enter", 0x1c },
    { "LCtrl", 0x1d },
    { "A", 0x1e },
    { "S", 0x1f },
    { "D", 0x20 },
    { "F", 0x21 },
    { "G", 0x22 },
    { "H", 0x23 },
    { "J", 0x24 },
    { "K", 0x25 },
    { "L", 0x26 },
    { "SemiColon", 0x27 },
    { "'", 0x28 },
    { "Tilde", 0x29 },
    { "LShift", 0x2a },
    { "\\", 0x2b },
    { "Z", 0x2c },
    { "X", 0x2d },
    { "C", 0x2e },
    { "V", 0x2f },
    { "B", 0x30 },
    { "N", 0x31 },
    { "M", 0x32 },
    { ",", 0x33 },
    { ".", 0x34 },
    { "/", 0x35 },
    { "RShift", 0x36 },
    { "Kpad*", 0x37 },
    { "LAlt", 0x38 },
    { "Space", 0x39 },
    { "CapLck", 0x3a },
    { "F1", 0x3b },
    { "F2", 0x3c },
    { "F3", 0x3d },
    { "F4", 0x3e },
    { "F5", 0x3f },
    { "F6", 0x40 },
    { "F7", 0x41 },
    { "F8", 0x42 },
    { "F9", 0x43 },
    { "F10", 0x44 },
    { "NumLck", 0x45 },
    { "ScrLck", 0x46 },
    { "Kpad7", 0x47 },
    { "Kpad8", 0x48 },
    { "Kpad9", 0x49 },
    { "Kpad-", 0x4a },
    { "Kpad4", 0x4b },
    { "Kpad5", 0x4c },
    { "Kpad6", 0x4d },
    { "Kpad+", 0x4e },
    { "Kpad1", 0x4f },
    { "Kpad2", 0x50 },
    { "Kpad3", 0x51 },
    { "Kpad0", 0x52 },
    { "Kpad.", 0x53 },
    { "F11", 0x57 },
    { "F12", 0x58 },
    { "KpdEnt", 0x9c },
    { "RCtrl", 0x9d },
    { "Kpad/", 0xb5 },
    { "RAlt", 0xb8 },
    { "PrtScn", 0xb7 },
    { "Pause", 0xc5 },
    { "Home", 0xc7 },
    { "Up", 0xc8 },
    { "PgUp", 0xc9 },
    { "Left", 0xcb },
    { "Right", 0xcd },
    { "End", 0xcf },
    { "Down", 0xd0 },
    { "PgDn", 0xd1 },
    { "Insert", 0xd2 },
    { "Delete", 0xd3 },

    {0,0}
};

char *ConsoleButtons[] = { "mouse1", "mouse2", "mouse3", "mouse4", "mwheelup", "mwheeldn", "mouse5", "mouse6", "mouse7", "mouse8" };

static int32_t osdcmd_bind(const osdfuncparm_t *parm)
{
    int32_t i, j;

    if (parm->numparms==1&&!Bstrcasecmp(parm->parms[0],"showkeys"))
    {
        for (i=0; ConsoleKeys[i].name; i++)
            OSD_Printf("%s\n",ConsoleKeys[i].name);
        for (i=0; i<MAXMOUSEBUTTONS; i++)
            OSD_Printf("%s\n",ConsoleButtons[i]);
        return OSDCMD_OK;
    }

    if (parm->numparms==0)
    {
        int32_t j=0;

        OSD_Printf("Current key bindings:\n");
        for (i=0; i<MAXBOUNDKEYS; i++)
            if (KeyBindings[i].cmd[0] && KeyBindings[i].key)
            {
                j++;
                OSD_Printf("%-9s %s\"%s\"\n",KeyBindings[i].key, KeyBindings[i].repeat?"":"norepeat ", KeyBindings[i].cmd);
            }

        for (i=0; i<MAXMOUSEBUTTONS; i++)
            if (MouseBindings[i].cmd[0] && MouseBindings[i].key)
            {
                j++;
                OSD_Printf("%-9s %s\"%s\"\n",MouseBindings[i].key, MouseBindings[i].repeat?"":"norepeat ",MouseBindings[i].cmd);
            }

        if (j == 0)
            OSD_Printf("No binds found.\n");

        return OSDCMD_OK;
    }

    for (i=0; ConsoleKeys[i].name; i++)
        if (!Bstrcasecmp(parm->parms[0],ConsoleKeys[i].name))
            break;

    if (!ConsoleKeys[i].name)
    {
        for (i=0; i<MAXMOUSEBUTTONS; i++)
            if (!Bstrcasecmp(parm->parms[0],ConsoleButtons[i]))
                break;
        if (i >= MAXMOUSEBUTTONS)
            return OSDCMD_SHOWHELP;

        if (parm->numparms < 2)
        {
            OSD_Printf("%-9s %s\"%s\"\n",ConsoleButtons[i], MouseBindings[i].repeat?"":"norepeat ",MouseBindings[i].cmd);
            return OSDCMD_OK;
        }

        j = 1;

        MouseBindings[i].repeat = 1;
        if (parm->numparms >= 2 && !Bstrcasecmp(parm->parms[j],"norepeat"))
        {
            MouseBindings[i].repeat = 0;
            j++;
        }

        Bstrcpy(tempbuf,parm->parms[j++]);
        for (; j<parm->numparms; j++)
        {
            Bstrcat(tempbuf," ");
            Bstrcat(tempbuf,parm->parms[j++]);
        }
        Bstrncpy(MouseBindings[i].cmd,tempbuf, MAXBINDSTRINGLENGTH-1);

        MouseBindings[i].key=ConsoleButtons[i];
        if (!OSD_ParsingScript())
            OSD_Printf("%s\n",parm->raw);
        return OSDCMD_OK;
    }

    if (parm->numparms < 2)
    {
        OSD_Printf("%-9s %s\"%s\"\n",ConsoleKeys[i].name, KeyBindings[ConsoleKeys[i].id].repeat?"":"norepeat ", KeyBindings[ConsoleKeys[i].id].cmd);
        return OSDCMD_OK;
    }

    j = 1;

    KeyBindings[ConsoleKeys[i].id].repeat = 1;
    if (parm->numparms >= 2 && !Bstrcasecmp(parm->parms[j],"norepeat"))
    {
        KeyBindings[ConsoleKeys[i].id].repeat = 0;
        j++;
    }

    Bstrcpy(tempbuf,parm->parms[j++]);
    for (; j<parm->numparms; j++)
    {
        Bstrcat(tempbuf," ");
        Bstrcat(tempbuf,parm->parms[j++]);
    }
    Bstrncpy(KeyBindings[ConsoleKeys[i].id].cmd,tempbuf, MAXBINDSTRINGLENGTH-1);

    KeyBindings[ConsoleKeys[i].id].key=ConsoleKeys[i].name;

    {
        char *cp = tempbuf;

        // Populate the keyboard config menu based on the bind.
        // Take care of processing one-to-many bindings properly, too.
        while ((cp = Bstrstr(cp, "gamefunc_")))
        {
            char *semi;

            cp += 9;  // skip the "gamefunc_"

            semi = Bstrchr(cp, ';');
            if (semi)
                *semi = 0;

            j = CONFIG_FunctionNameToNum(cp);

            if (semi)
                cp = semi+1;

            if (j != -1)
            {
                ud.config.KeyboardKeys[j][1] = ud.config.KeyboardKeys[j][0];
                ud.config.KeyboardKeys[j][0] = ConsoleKeys[i].id;

            CONTROL_MapKey(j, ConsoleKeys[i].id, ud.config.KeyboardKeys[j][0]);
            }
        }
    }

    if (!OSD_ParsingScript())
        OSD_Printf("%s\n",parm->raw);

    return OSDCMD_OK;
}

static int32_t osdcmd_unbindall(const osdfuncparm_t *parm)
{
    int32_t i;

    UNREFERENCED_PARAMETER(parm);

    for (i=0; i<MAXBOUNDKEYS; i++)
        if (KeyBindings[i].cmd[0])
            KeyBindings[i].cmd[0] = 0;

    for (i=0; i<MAXMOUSEBUTTONS; i++)
        if (MouseBindings[i].cmd[0])
            MouseBindings[i].cmd[0] = 0;

    for (i=0; i<NUMGAMEFUNCTIONS; i++)
    {
        ud.config.KeyboardKeys[i][0] = ud.config.KeyboardKeys[i][1] = 0xff;
        CONTROL_MapKey(i, ud.config.KeyboardKeys[i][0], ud.config.KeyboardKeys[i][1]);
    }

    if (!OSD_ParsingScript())
        OSD_Printf("unbound all controls\n");

    return OSDCMD_OK;
}

static int32_t osdcmd_unbind(const osdfuncparm_t *parm)
{
    int32_t i;

    if (parm->numparms < 1) return OSDCMD_SHOWHELP;

    for (i=0; ConsoleKeys[i].name; i++)
        if (!Bstrcasecmp(parm->parms[0],ConsoleKeys[i].name))
            break;

    if (!ConsoleKeys[i].name)
    {
        for (i=0; i<MAXMOUSEBUTTONS; i++)
            if (!Bstrcasecmp(parm->parms[0],ConsoleButtons[i]))
                break;

        if (i >= MAXMOUSEBUTTONS)
            return OSDCMD_SHOWHELP;

        MouseBindings[i].repeat = 0;
        MouseBindings[i].cmd[0] = 0;

        OSD_Printf("unbound %s\n",ConsoleButtons[i]);

        return OSDCMD_OK;
    }

    KeyBindings[ConsoleKeys[i].id].repeat = 0;
    KeyBindings[ConsoleKeys[i].id].cmd[0] = 0;

    OSD_Printf("unbound key %s\n",ConsoleKeys[i].name);

    return OSDCMD_OK;
}

static int32_t osdcmd_quicksave(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        OSD_Printf("quicksave: not in a game.\n");
    else g_doQuickSave = 1;
    return OSDCMD_OK;
}

static int32_t osdcmd_quickload(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        OSD_Printf("quickload: not in a game.\n");
    else g_doQuickSave = 2;
    return OSDCMD_OK;
}

static int32_t osdcmd_screenshot(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
//    KB_ClearKeysDown();
    screencapture("duke0000.tga",0);
    return OSDCMD_OK;
}

/*
static int32_t osdcmd_savestate(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (MapInfo[ud.volume_number*MAXLEVELS+ud.level_number].savedstate == NULL)
        MapInfo[ud.volume_number*MAXLEVELS+ud.level_number].savedstate = Bcalloc(1,sizeof(mapstate_t));
    G_SaveMapState(MapInfo[ud.volume_number*MAXLEVELS+ud.level_number].savedstate);
    return OSDCMD_OK;
}

static int32_t osdcmd_restorestate(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    if (MapInfo[ud.volume_number*MAXLEVELS+ud.level_number].savedstate)
        G_RestoreMapState(MapInfo[ud.volume_number*MAXLEVELS+ud.level_number].savedstate);
    return OSDCMD_OK;
}

static int32_t osdcmd_inittimer(const osdfuncparm_t *parm)
{
    int32_t j;

    if (parm->numparms != 1)
    {
        OSD_Printf("%dHz timer\n",g_timerTicsPerSecond);
        return OSDCMD_SHOWHELP;
    }

    j = atol(parm->parms[0]);
    if (g_timerTicsPerSecond == j)
        return OSDCMD_OK;
    uninittimer();
    inittimer(j);
    g_timerTicsPerSecond = j;

    OSD_Printf("%s\n",parm->raw);
    return OSDCMD_OK;
}
*/

static int32_t osdcmd_disconnect(const osdfuncparm_t *parm)
{
    extern int32_t g_netDisconnect;
    UNREFERENCED_PARAMETER(parm);
    g_netDisconnect = 1;
    return OSDCMD_OK;
}

static int32_t osdcmd_connect(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    Net_Connect(parm->parms[0]);
    G_BackToMenu();
    return OSDCMD_OK;
}

static int32_t osdcmd_password(const osdfuncparm_t *parm)
{
    extern char g_netPassword[32];

    if (parm->numparms < 1)
    {
        Bmemset(g_netPassword, 0, sizeof(g_netPassword));
        return OSDCMD_OK;
    }
    Bstrncpy(g_netPassword, (char *)(parm->raw) + 9, sizeof(g_netPassword)-1);

    return OSDCMD_OK;
}

static int32_t osdcmd_listplayers(const osdfuncparm_t *parm)
{
    ENetPeer *currentPeer;
    char ipaddr[32];

    if (parm->numparms != 0)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        initprintf("You are not the server.\n");
        return OSDCMD_OK;
    }

    for (currentPeer = g_netServer -> peers;
            currentPeer < & g_netServer -> peers [g_netServer -> peerCount];
            ++ currentPeer)
    {
        if (currentPeer -> state != ENET_PEER_STATE_CONNECTED)
            continue;

        enet_address_get_host_ip(&currentPeer->address, ipaddr, sizeof(ipaddr));
        initprintf("%x %s %s\n", currentPeer->address.host, ipaddr,
                   g_player[(intptr_t)currentPeer->data].user_name);
    }

    return OSDCMD_OK;
}

static int32_t osdcmd_kick(const osdfuncparm_t *parm)
{
    ENetPeer *currentPeer;
    uint32_t hexaddr;

    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        initprintf("You are not the server.\n");
        return OSDCMD_OK;
    }

    for (currentPeer = g_netServer -> peers;
            currentPeer < & g_netServer -> peers [g_netServer -> peerCount];
            ++ currentPeer)
    {
        if (currentPeer -> state != ENET_PEER_STATE_CONNECTED)
            continue;

        sscanf(parm->parms[0],"%" PRIxPTR "", &hexaddr);

        if (currentPeer->address.host == hexaddr)
        {
            initprintf("Kicking %x (%s)\n", currentPeer->address.host,
                       g_player[(intptr_t)currentPeer->data].user_name);
            enet_peer_disconnect(currentPeer, DISC_KICKED);
            return OSDCMD_OK;
        }
    }

    initprintf("Player %s not found!\n", parm->parms[0]);
    return OSDCMD_OK;
}

static int32_t osdcmd_kickban(const osdfuncparm_t *parm)
{
    ENetPeer *currentPeer;
    uint32_t hexaddr;

    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        initprintf("You are not the server.\n");
        return OSDCMD_OK;
    }

    for (currentPeer = g_netServer -> peers;
            currentPeer < & g_netServer -> peers [g_netServer -> peerCount];
            ++ currentPeer)
    {
        if (currentPeer -> state != ENET_PEER_STATE_CONNECTED)
            continue;

        sscanf(parm->parms[0],"%" PRIxPTR "", &hexaddr);

        // TODO: implement banning logic

        if (currentPeer->address.host == hexaddr)
        {
            char ipaddr[32];

            enet_address_get_host_ip(&currentPeer->address, ipaddr, sizeof(ipaddr));
            initprintf("Host %s is now banned.\n", ipaddr);
            initprintf("Kicking %x (%s)\n", currentPeer->address.host,
                       g_player[(intptr_t)currentPeer->data].user_name);
            enet_peer_disconnect(currentPeer, DISC_BANNED);
            return OSDCMD_OK;
        }
    }

    initprintf("Player %s not found!\n", parm->parms[0]);
    return OSDCMD_OK;
}

static int32_t osdcmd_cvar_set_game(const osdfuncparm_t *parm)
{
    int32_t r = osdcmd_cvar_set(parm);

    if (r != OSDCMD_OK) return r;

    if (!Bstrcasecmp(parm->name, "r_maxfps"))
    {
        if (r_maxfps) g_frameDelay = (1000/r_maxfps);
        else g_frameDelay = 0;

        return r;
    }
    else if (!Bstrcasecmp(parm->name, "r_ambientlight"))
    {
        if (r_ambientlight == 0)
            r_ambientlightrecip = 256.f;
        else r_ambientlightrecip = 1.f/r_ambientlight;

        return r;
    }
    else if (!Bstrcasecmp(parm->name, "in_mouse"))
    {
        CONTROL_MouseEnabled = (ud.config.UseMouse && CONTROL_MousePresent);

        return r;
    }
    else if (!Bstrcasecmp(parm->name, "in_joystick"))
    {
        CONTROL_JoystickEnabled = (ud.config.UseJoystick && CONTROL_JoyPresent);

        return r;
    }
    else if (!Bstrcasecmp(parm->name, "vid_gamma"))
    {
        ud.brightness = GAMMA_CALC;
        ud.brightness <<= 2;
        setbrightness(ud.brightness>>2,&g_player[myconnectindex].ps->palette[0],0);

        return r;
    }
    else if (!Bstrcasecmp(parm->name, "vid_brightness"))
    {
        setbrightness(ud.brightness>>2,&g_player[myconnectindex].ps->palette[0],0);

        return r;
    }
    else if (!Bstrcasecmp(parm->name, "vid_contrast"))
    {
        setbrightness(ud.brightness>>2,&g_player[myconnectindex].ps->palette[0],0);

        return r;
    }
    else if (!Bstrcasecmp(parm->name, "hud_scale"))
    {
        G_UpdateScreenArea();

        return r;
    }
    else if (!Bstrcasecmp(parm->name, "skill"))
    {
        if (numplayers > 1)
            return r;

        ud.m_player_skill = ud.player_skill;

        return r;
    }
    else if (!Bstrcasecmp(parm->name, "color"))
    {
        G_CheckPlayerColor((int32_t *)&ud.color,-1);
        g_player[0].ps->palookup = g_player[0].pcolor = ud.color;

        return r;
    }

    return r;
}

static int32_t osdcmd_cvar_set_multi(const osdfuncparm_t *parm)
{
    int32_t r = osdcmd_cvar_set_game(parm);

    if (r != OSDCMD_OK) return r;

    G_UpdatePlayerFromMenu();

    return r;
}

int32_t registerosdcommands(void)
{
    uint32_t i;

    cvar_t cvars_game[] =
    {
        { "crosshair", "crosshair: enable/disable crosshair", (void *)&ud.crosshair, CVAR_BOOL, 0, 1 },

        { "cl_autoaim", "cl_autoaim: enable/disable weapon autoaim", (void *)&ud.config.AutoAim, CVAR_INT|CVAR_MULTI, 0, 2 },
        { "cl_automsg", "cl_automsg: enable/disable automatically sending messages to all players", (void *)&ud.automsg, CVAR_BOOL, 0, 1 },
        { "cl_autorun", "cl_autorun", (void *)&ud.auto_run, CVAR_BOOL, 0, 1 },
        { "cl_autovote", "cl_autovote: enable/disable automatic voting", (void *)&ud.autovote, CVAR_INT, 0, 2 },

        { "cl_obituaries", "cl_obituaries: enable/disable multiplayer death messages", (void *)&ud.obituaries, CVAR_BOOL, 0, 1 },
        { "cl_democams", "cl_democams: enable/disable demo playback cameras", (void *)&ud.democams, CVAR_BOOL, 0, 1 },

        { "cl_idplayers", "cl_idplayers: enable/disable name display when aiming at opponents", (void *)&ud.idplayers, CVAR_BOOL, 0, 1 },

        { "cl_showcoords", "cl_showcoords: show your position in the game world", (void *)&ud.coords, CVAR_BOOL, 0, 1 },

        { "cl_viewbob", "cl_viewbob: enable/disable player head bobbing", (void *)&ud.viewbob, CVAR_BOOL, 0, 1 },

        { "cl_weaponsway", "cl_weaponsway: enable/disable player weapon swaying", (void *)&ud.weaponsway, CVAR_BOOL, 0, 1 },
        { "cl_weaponswitch", "cl_weaponswitch: enable/disable auto weapon switching", (void *)&ud.weaponswitch, CVAR_INT|CVAR_MULTI, 0, 3 },
        { "cl_angleinterpolation", "cl_angleinterpolation: enable/disable angle interpolation", (void *)&ud.angleinterpolation, CVAR_INT, 0, 256 },

        { "color", "color: changes player palette", (void *)&ud.color, CVAR_INT|CVAR_MULTI, 0, g_numRealPalettes },

        { "crosshairscale","crosshairscale: changes the size of the crosshair", (void *)&ud.crosshairscale, CVAR_INT, 10, 100 },

        { "demorec_diffs","demorec_diffs: enable/disable diff recording in demos",(void *)&demorec_diffs_cvar, CVAR_BOOL, 0, 1 },
        { "demorec_force","demorec_force: enable/disable forced demo recording",(void *)&demorec_force_cvar, CVAR_BOOL|CVAR_NOSAVE, 0, 1 },
        {
            "demorec_difftics","demorec_difftics <number>: sets game tic interval after which a diff is recorded",
            (void *)&demorec_difftics_cvar, CVAR_INT, 2, 60*(TICRATE/TICSPERFRAME)
        },
        { "demorec_diffcompress","demorec_diffcompress <number>: Compression method for diffs. (0: none, 1: KSLZW)",(void *)&demorec_diffcompress_cvar, CVAR_INT, 0, 1 },
        { "demorec_synccompress","demorec_synccompress <number>: Compression method for input. (0: none, 1: KSLZW)",(void *)&demorec_synccompress_cvar, CVAR_INT, 0, 1 },
        { "demorec_seeds","demorec_seeds: enable/disable recording of random seed for later sync checking",(void *)&demorec_seeds_cvar, CVAR_BOOL, 0, 1 },
        { "demoplay_diffs","demoplay_diffs: enable/disable application of diffs in demo playback",(void *)&demoplay_diffs, CVAR_BOOL, 0, 1 },
        { "demoplay_showsync","demoplay_showsync: enable/disable display of sync status",(void *)&demoplay_showsync, CVAR_BOOL, 0, 1 },

        { "hud_althud", "hud_althud: enable/disable alternate mini-hud", (void *)&ud.althud, CVAR_BOOL, 0, 1 },
        { "hud_bgstretch", "hud_bgstretch: enable/disable background image stretching in wide resolutions", (void *)&ud.bgstretch, CVAR_BOOL, 0, 1 },
        { "hud_messagetime", "hud_messagetime: length of time to display multiplayer chat messages", (void *)&ud.msgdisptime, CVAR_INT, 0, 3600 },
        { "hud_numbertile", "hud_numbertile: first tile in alt hud number set", (void *)&althud_numbertile, CVAR_INT, 0, MAXTILES-10 },
        { "hud_numberpal", "hud_numberpal: pal for alt hud numbers", (void *)&althud_numberpal, CVAR_INT, 0, MAXPALOOKUPS },
        { "hud_shadows", "hud_shadows: enable/disable althud shadows", (void *)&althud_shadows, CVAR_BOOL, 0, 1 },
        { "hud_flashing", "hud_flashing: enable/disable althud flashing", (void *)&althud_flashing, CVAR_BOOL, 0, 1 },
        { "hud_glowingquotes", "hud_glowingquotes: enable/disable \"glowing\" quote text", (void *)&hud_glowingquotes, CVAR_BOOL, 0, 1 },
        { "hud_scale","hud_scale: changes the hud scale", (void *)&ud.statusbarscale, CVAR_INT|CVAR_FUNCPTR, 10, 100 },
        { "hud_showmapname", "hud_showmapname: enable/disable map name display on load", (void *)&hud_showmapname, CVAR_BOOL, 0, 1 },
        { "hud_stats", "hud_stats: enable/disable level statistics display", (void *)&ud.levelstats, CVAR_BOOL, 0, 1 },
        { "hud_textscale", "hud_textscale: sets multiplayer chat message size", (void *)&ud.textscale, CVAR_INT, 100, 400 },
        { "hud_weaponscale","hud_weaponscale: changes the weapon scale", (void *)&ud.weaponscale, CVAR_INT, 10, 100 },

        { "in_joystick","in_joystick: enables input from the joystick if it is present",(void *)&ud.config.UseJoystick, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "in_mouse","in_mouse: enables input from the mouse if it is present",(void *)&ud.config.UseMouse, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },

        { "in_aimmode", "in_aimmode: 0:toggle, 1:hold to aim", (void *)&ud.mouseaiming, CVAR_BOOL, 0, 1 },
        {
            "in_mousebias", "in_mousebias: emulates the original mouse code's weighting of input towards whichever axis is moving the most at any given time",
            (void *)&ud.config.MouseBias, CVAR_INT, 0, 32
        },
        { "in_mousedeadzone", "in_mousedeadzone: amount of mouse movement to filter out", (void *)&ud.config.MouseDeadZone, CVAR_INT, 0, 512 },
        { "in_mouseflip", "in_mouseflip: invert vertical mouse movement", (void *)&ud.mouseflip, CVAR_BOOL, 0, 1 },
        { "in_mousemode", "in_mousemode: like pressing U.", (void *)&g_myAimMode, CVAR_BOOL, 0, 1 },
        { "in_mousesmoothing", "in_mousesmoothing: enable/disable mouse input smoothing", (void *)&ud.config.SmoothInput, CVAR_BOOL, 0, 1 },

        { "mus_enabled", "mus_enabled: enables/disables music", (void *)&ud.config.MusicToggle, CVAR_BOOL, 0, 1 },
        { "mus_volume", "mus_musvolume: controls volume of midi music", (void *)&ud.config.MusicVolume, CVAR_INT, 0, 255 },

        { "osdhightile", "osdhightile: enable/disable hires art replacements for console text", (void *)&osdhightile, CVAR_BOOL, 0, 1 },

        { "r_drawweapon", "r_drawweapon: enable/disable weapon drawing", (void *)&ud.drawweapon, CVAR_INT, 0, 2 },
        { "r_showfps", "r_showfps: show the frame rate counter", (void *)&ud.tickrate, CVAR_BOOL, 0, 1 },
        { "r_shadows", "r_shadows: enable/disable sprite and model shadows", (void *)&ud.shadows, CVAR_BOOL, 0, 1 },
        { "r_size", "r_size: change size of viewable area", (void *)&ud.screen_size, CVAR_INT, 0, 64 },
        { "r_precache", "r_precache: enable/disable the pre-level caching routine", (void *)&ud.config.useprecache, CVAR_BOOL, 0, 1 },

        { "r_ambientlight", "r_ambientlight: sets the global map light level",(void *)&r_ambientlight, CVAR_FLOAT|CVAR_FUNCPTR, 0, 10 },
        { "r_maxfps", "r_maxfps: limit the frame rate",(void *)&r_maxfps, CVAR_INT|CVAR_FUNCPTR, 0, 1000 },

        { "sensitivity","sensitivity <value>: changes the mouse sensitivity", (void *)&CONTROL_MouseSensitivity, CVAR_FLOAT|CVAR_FUNCPTR, 0, 25 },

        { "skill","skill <value>: changes the game skill setting", (void *)&ud.player_skill, CVAR_INT|CVAR_FUNCPTR|CVAR_NOMULTI, 0, 5 },

        { "snd_ambience", "snd_ambience: enables/disables ambient sounds", (void *)&ud.config.AmbienceToggle, CVAR_BOOL, 0, 1 },
        { "snd_duketalk", "snd_duketalk: enables/disables Duke's speech", (void *)&ud.config.VoiceToggle, CVAR_INT, 0, 5 },
        { "snd_enabled", "snd_enabled: enables/disables sound effects", (void *)&ud.config.SoundToggle, CVAR_BOOL, 0, 1 },
        { "snd_fxvolume", "snd_fxvolume: volume of sound effects", (void *)&ud.config.FXVolume, CVAR_INT, 0, 255 },
        { "snd_mixrate", "snd_mixrate: sound mixing rate", (void *)&ud.config.MixRate, CVAR_INT, 0, 48000 },
        { "snd_numbits", "snd_numbits: sound bits", (void *)&ud.config.NumBits, CVAR_INT, 8, 16 },
        { "snd_numchannels", "snd_numchannels: the number of sound channels", (void *)&ud.config.NumChannels, CVAR_INT, 0, 2 },
        { "snd_numvoices", "snd_numvoices: the number of concurrent sounds", (void *)&ud.config.NumVoices, CVAR_INT, 0, 96 },
        { "snd_reversestereo", "snd_reversestereo: reverses the stereo channels", (void *)&ud.config.ReverseStereo, CVAR_BOOL, 0, 16 },

        { "team","team <value>: change team in multiplayer", (void *)&ud.team, CVAR_INT|CVAR_MULTI, 0, 3 },

        { "vid_gamma","vid_gamma <gamma>: adjusts gamma ramp",(void *)&vid_gamma, CVAR_DOUBLE|CVAR_FUNCPTR, 0, 10 },
        { "vid_contrast","vid_contrast <gamma>: adjusts gamma ramp",(void *)&vid_contrast, CVAR_DOUBLE|CVAR_FUNCPTR, 0, 10 },
        { "vid_brightness","vid_brightness <gamma>: adjusts gamma ramp",(void *)&vid_brightness, CVAR_DOUBLE|CVAR_FUNCPTR, 0, 10 },
    };

    osdcmd_cheatsinfo_stat.cheatnum = -1;

    for (i=0; i<sizeof(cvars_game)/sizeof(cvars_game[0]); i++)
    {
        if (OSD_RegisterCvar(&cvars_game[i]))
            continue;

        switch (cvars_game[i].type & (CVAR_FUNCPTR|CVAR_MULTI))
        {
        case CVAR_FUNCPTR:
            OSD_RegisterFunction(cvars_game[i].name, cvars_game[i].helpstr, osdcmd_cvar_set_game);
            break;
        case CVAR_MULTI:
        case CVAR_FUNCPTR|CVAR_MULTI:
            OSD_RegisterFunction(cvars_game[i].name, cvars_game[i].helpstr, osdcmd_cvar_set_multi);
            break;
        default:
            OSD_RegisterFunction(cvars_game[i].name, cvars_game[i].helpstr, osdcmd_cvar_set);
            break;
        }
    }

    if (VOLUMEONE)
        OSD_RegisterFunction("changelevel","changelevel <level>: warps to the given level", osdcmd_changelevel);
    else
    {
        OSD_RegisterFunction("changelevel","changelevel <volume> <level>: warps to the given level", osdcmd_changelevel);
        OSD_RegisterFunction("map","map <mapfile>: loads the given user map", osdcmd_map);
    }

    OSD_RegisterFunction("addpath","addpath <path>: adds path to game filesystem", osdcmd_addpath);
    OSD_RegisterFunction("bind","bind <key> <string>: associates a keypress with a string of console input. Type \"bind showkeys\" for a list of keys and \"listsymbols\" for a list of valid console commands.", osdcmd_bind);
    OSD_RegisterFunction("cmenu","cmenu <#>: jumps to menu", osdcmd_cmenu);
    OSD_RegisterFunction("crosshaircolor","crosshaircolor: changes the crosshair color", osdcmd_crosshaircolor);

    OSD_RegisterFunction("connect","connect: connects to a multiplayer game", osdcmd_connect);
    OSD_RegisterFunction("disconnect","disconnect: disconnects from the local multiplayer game", osdcmd_disconnect);

    for (i=0; i<NUMGAMEFUNCTIONS; i++)
    {
        char *t;
        int32_t j;

        if (!Bstrcmp(gamefunctions[i],"Show_Console")) continue;

        Bsprintf(tempbuf,"gamefunc_%s",gamefunctions[i]);
        t = Bstrdup(tempbuf);
        for (j=Bstrlen(t); j>=0; j--)
            t[j] = Btolower(t[j]);
        Bstrcat(tempbuf,": game button");
        OSD_RegisterFunction(t,Bstrdup(tempbuf),osdcmd_button);
    }

    OSD_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    OSD_RegisterFunction("god","god: toggles god mode", osdcmd_god);

    OSD_RegisterFunction("initgroupfile","initgroupfile <path>: adds a grp file into the game filesystem", osdcmd_initgroupfile);
//    OSD_RegisterFunction("inittimer","debug", osdcmd_inittimer);

    OSD_RegisterFunction("kick","kick <id>: kicks a multiplayer client.  See listplayers.", osdcmd_kick);
    OSD_RegisterFunction("kickban","kickban <id>: kicks a multiplayer client and prevents them from reconnecting.  See listplayers.", osdcmd_kickban);

    OSD_RegisterFunction("listplayers","listplayers: lists currently connected multiplayer clients", osdcmd_listplayers);

    OSD_RegisterFunction("name","name: change your multiplayer nickname", osdcmd_name);
    OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

    OSD_RegisterFunction("password","password: sets multiplayer game password", osdcmd_password);

    OSD_RegisterFunction("quicksave","quicksave: performs a quick save", osdcmd_quicksave);
    OSD_RegisterFunction("quickload","quickload: performs a quick load", osdcmd_quickload);
    OSD_RegisterFunction("quit","quit: exits the game immediately", osdcmd_quit);
    OSD_RegisterFunction("exit","exit: exits the game immediately", osdcmd_quit);

    OSD_RegisterFunction("restartsound","restartsound: reinitializes the sound system",osdcmd_restartsound);
    OSD_RegisterFunction("restartvid","restartvid: reinitializes the video mode",osdcmd_restartvid);

    OSD_RegisterFunction("addlogvar","addlogvar <gamevar>: prints the value of a gamevar", osdcmd_addlogvar);
    OSD_RegisterFunction("setvar","setvar <gamevar> <value>: sets the value of a gamevar", osdcmd_setvar);
    OSD_RegisterFunction("setvarvar","setvarvar <gamevar1> <gamevar2>: sets the value of <gamevar1> to <gamevar2>", osdcmd_setvar);
    OSD_RegisterFunction("setactorvar","setactorvar <actor#> <gamevar> <value>: sets the value of <actor#>'s <gamevar> to <value>", osdcmd_setactorvar);
    OSD_RegisterFunction("screenshot","screenshot: takes a screenshot.  See r_scrcaptureformat.", osdcmd_screenshot);

    OSD_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);

    OSD_RegisterFunction("unbind","unbind <key>: unbinds a key", osdcmd_unbind);
    OSD_RegisterFunction("unbindall","unbindall: unbinds all keys", osdcmd_unbindall);

    OSD_RegisterFunction("vidmode","vidmode <xdim> <ydim> <bpp> <fullscreen>: change the video mode",osdcmd_vidmode);
//    OSD_RegisterFunction("savestate","",osdcmd_savestate);
//    OSD_RegisterFunction("restorestate","",osdcmd_restorestate);
    //baselayer_onvideomodechange = onvideomodechange;

    return 0;
}

