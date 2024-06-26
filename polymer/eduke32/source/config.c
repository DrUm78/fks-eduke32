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

/*
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
*/

#include "baselayer.h"
#include "duke3d.h"
#include "scriplib.h"
#include "osd.h"
#include "osdcmds.h"
#include "osdfuncs.h"

// we load this in to get default button and key assignments
// as well as setting up function mappings

#define __SETUP__   // JBF 20031211
#include "_functio.h"

/*
===================
=
= CONFIG_FunctionNameToNum
=
===================
*/

hashtable_t h_gamefuncs    = { NUMGAMEFUNCTIONS<<1, NULL };

int32_t CONFIG_FunctionNameToNum(char *func)
{
    int32_t i;

    i = hash_find(&h_gamefuncs,func);

    if (i < 0)
    {
        char *str = Bstrtolower(Bstrdup(func));
        i = hash_find(&h_gamefuncs,str);
        Bfree(str);

        return i;
    }

    return i;
}

/*
===================
=
= CONFIG_FunctionNumToName
=
===================
*/

char *CONFIG_FunctionNumToName(int32_t func)
{
    if ((unsigned)func >= (unsigned)NUMGAMEFUNCTIONS)
        return NULL;
    return gamefunctions[func];
}

/*
===================
=
= CONFIG_AnalogNameToNum
=
===================
*/


int32_t CONFIG_AnalogNameToNum(char *func)
{

    if (!Bstrcasecmp(func,"analog_turning"))
    {
        return analog_turning;
    }
    if (!Bstrcasecmp(func,"analog_strafing"))
    {
        return analog_strafing;
    }
    if (!Bstrcasecmp(func,"analog_moving"))
    {
        return analog_moving;
    }
    if (!Bstrcasecmp(func,"analog_lookingupanddown"))
    {
        return analog_lookingupanddown;
    }

    return -1;
}


char *CONFIG_AnalogNumToName(int32_t func)
{
    switch (func)
    {
    case analog_turning:
        return "analog_turning";
    case analog_strafing:
        return "analog_strafing";
    case analog_moving:
        return "analog_moving";
    case analog_lookingupanddown:
        return "analog_lookingupanddown";
    }

    return NULL;
}


/*
===================
=
= CONFIG_SetDefaults
=
===================
*/

void CONFIG_SetDefaultKeys(int32_t type)
{
    int32_t i,f;

    Bmemset(ud.config.KeyboardKeys, 0xff, sizeof(ud.config.KeyboardKeys));
    Bmemset(&KeyBindings,0,sizeof(KeyBindings));
    Bmemset(&MouseBindings,0,sizeof(MouseBindings));

    if (type == 1)
    {
        for (i=0; i < (int32_t)(sizeof(oldkeydefaults)/sizeof(oldkeydefaults[0])); i+=3)
        {
            f = CONFIG_FunctionNameToNum((char *)oldkeydefaults[i+0]);
            if (f == -1) continue;
            ud.config.KeyboardKeys[f][0] = KB_StringToScanCode((char *)oldkeydefaults[i+1]);
            ud.config.KeyboardKeys[f][1] = KB_StringToScanCode((char *)oldkeydefaults[i+2]);

            if (f == gamefunc_Show_Console) OSD_CaptureKey(ud.config.KeyboardKeys[f][0]);
            else CONFIG_MapKey(f, ud.config.KeyboardKeys[f][0], 0, ud.config.KeyboardKeys[f][1], 0);
        }
        return;
    }

    for (i=0; i < (int32_t)(sizeof(keydefaults)/sizeof(keydefaults[0])); i+=3)
    {
        f = CONFIG_FunctionNameToNum(keydefaults[i+0]);
        if (f == -1) continue;
        ud.config.KeyboardKeys[f][0] = KB_StringToScanCode(keydefaults[i+1]);
        ud.config.KeyboardKeys[f][1] = KB_StringToScanCode(keydefaults[i+2]);

        if (f == gamefunc_Show_Console) OSD_CaptureKey(ud.config.KeyboardKeys[f][0]);
        else CONFIG_MapKey(f, ud.config.KeyboardKeys[f][0], 0, ud.config.KeyboardKeys[f][1], 0);
    }
}

void CONFIG_SetDefaults(void)
{
    // JBF 20031211
    int32_t i;

    ud.config.scripthandle = -1;
    ud.config.ScreenWidth = 320;
    ud.config.ScreenHeight = 240;
    ud.config.ScreenMode = 0;
	ud.config.ScreenScaleMode = 0;
#if defined(POLYMOST) && defined(USE_OPENGL)
    ud.config.ScreenBPP = 32;
#else
    ud.config.ScreenBPP = 8;
#endif
#if defined(FUNKEYS)
	ud.config.ScreenWidth = 320;
	ud.config.ScreenHeight = 240;
	ud.config.ScreenBPP = 8;
#endif
    ud.config.useprecache = 1;
    ud.config.ForceSetup = 1;
    ud.config.NoAutoLoad = 1;
    ud.config.AmbienceToggle = 1;
    ud.config.AutoAim = 1;
    ud.config.FXDevice = 0;
    ud.config.FXVolume = 225;
#if defined(_WIN32) || defined(FUNKEYS)
    ud.config.MixRate = 44100;
#else
    ud.config.MixRate = 48000;
#endif
    ud.config.MouseBias = 0;
    ud.config.MouseDeadZone = 0;
    ud.config.MusicDevice = 0;
    ud.config.MusicToggle = 1;
    ud.config.MusicVolume = 195;
    g_myAimMode = g_player[0].ps->aim_mode = 1;
    ud.config.NumBits = 16;
    ud.config.NumChannels = 2;
    ud.config.NumVoices = 32;
    ud.config.ReverseStereo = 0;
    ud.auto_run = 1;
    ud.config.ShowOpponentWeapons = 0;
    ud.config.SmoothInput = 1;
    ud.config.SoundToggle = 1;
    ud.althud = 1;
    ud.automsg = 0;
    ud.autovote = 0;
    ud.brightness = 8;
    ud.camerasprite = -1;
    ud.color = 0;
    ud.crosshair = 1;
    ud.crosshairscale = 100;
    ud.obituaries = 1;
    ud.democams = 1;
    ud.detail = 1;
    ud.drawweapon = 1;
    ud.idplayers = 1;
    ud.levelstats = 0;
    ud.lockout = 0;
    ud.m_ffire = 1;
    ud.m_marker = 1;
    ud.mouseaiming = 0;
    ud.mouseflip = 1;
    ud.msgdisptime = 120;
    ud.pwlockout[0] = '\0';
    ud.runkey_mode = 0;
    ud.screen_size = 4;
    ud.screen_tilting = 1;
    ud.shadows = 1;
    ud.statusbarmode = 0;
    ud.statusbarscale = 100;
    ud.team = 0;
    ud.viewbob = 1;
    ud.weaponsway = 1;
    ud.weaponswitch = 3;	// new+empty
    ud.angleinterpolation = 0;
    ud.config.UseJoystick = 1;
    ud.config.UseMouse = 0;
    ud.config.VoiceToggle = 5; // bitfield, 1 = local, 2 = dummy, 4 = other players in DM
    ud.display_bonus_screen = 1;
    ud.show_level_text = 1;
    ud.configversion = 0;
    ud.weaponscale = 100;
    ud.textscale = 100;

    ud.config.CheckForUpdates = 1;

    Bstrcpy(ud.rtsname, "DUKE.RTS");
    Bstrcpy(szPlayerName, "Duke");

    Bstrcpy(ud.ridecule[0], "An inspiration for birth control.");
    Bstrcpy(ud.ridecule[1], "You're gonna die for that!");
    Bstrcpy(ud.ridecule[2], "It hurts to be you.");
    Bstrcpy(ud.ridecule[3], "Lucky Son of a Bitch.");
    Bstrcpy(ud.ridecule[4], "Hmmm....Payback time.");
    Bstrcpy(ud.ridecule[5], "You bottom dwelling scum sucker.");
    Bstrcpy(ud.ridecule[6], "Damn, you're ugly.");
    Bstrcpy(ud.ridecule[7], "Ha ha ha...Wasted!");
    Bstrcpy(ud.ridecule[8], "You suck!");
    Bstrcpy(ud.ridecule[9], "AARRRGHHHHH!!!");

    // JBF 20031211

    CONFIG_SetDefaultKeys(0);

    memset(ud.config.MouseFunctions, -1, sizeof(ud.config.MouseFunctions));
    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        ud.config.MouseFunctions[i][0] = CONFIG_FunctionNameToNum(mousedefaults[i]);
        CONTROL_MapButton(ud.config.MouseFunctions[i][0], i, 0, controldevice_mouse);
        if (i>=4) continue;
        ud.config.MouseFunctions[i][1] = CONFIG_FunctionNameToNum(mouseclickeddefaults[i]);
        CONTROL_MapButton(ud.config.MouseFunctions[i][1], i, 1, controldevice_mouse);
    }

    memset(ud.config.MouseDigitalFunctions, -1, sizeof(ud.config.MouseDigitalFunctions));
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        ud.config.MouseAnalogueScale[i] = 65536;
        CONTROL_SetAnalogAxisScale(i, ud.config.MouseAnalogueScale[i], controldevice_mouse);

        ud.config.MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i*2]);
        ud.config.MouseDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(mousedigitaldefaults[i*2+1]);
        CONTROL_MapDigitalAxis(i, ud.config.MouseDigitalFunctions[i][0], 0, controldevice_mouse);
        CONTROL_MapDigitalAxis(i, ud.config.MouseDigitalFunctions[i][1], 1, controldevice_mouse);

        ud.config.MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(mouseanalogdefaults[i]);
        CONTROL_MapAnalogAxis(i, ud.config.MouseAnalogueAxes[i], controldevice_mouse);
    }
    CONTROL_MouseSensitivity = DEFAULTMOUSESENSITIVITY;

    memset(ud.config.JoystickFunctions, -1, sizeof(ud.config.JoystickFunctions));
    for (i=0; i<MAXJOYBUTTONS; i++)
    {
        ud.config.JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(joystickdefaults[i]);
        ud.config.JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(joystickclickeddefaults[i]);
        CONTROL_MapButton(ud.config.JoystickFunctions[i][0], i, 0, controldevice_joystick);
        CONTROL_MapButton(ud.config.JoystickFunctions[i][1], i, 1, controldevice_joystick);
    }

    memset(ud.config.JoystickDigitalFunctions, -1, sizeof(ud.config.JoystickDigitalFunctions));
    for (i=0; i<MAXJOYAXES; i++)
    {
		if(i == 0)
		{
	        ud.config.JoystickAnalogueScale[i] = 253952; //65536;
	        ud.config.JoystickAnalogueDead[i] = 1000;
	        ud.config.JoystickAnalogueSaturate[i] = 9500;
		}
		else if(i == 1)
		{
	        ud.config.JoystickAnalogueScale[i] = 253952; //65536;
	        ud.config.JoystickAnalogueDead[i] = 312;
	        ud.config.JoystickAnalogueSaturate[i] = 6250;
		}
		else
		{
	        ud.config.JoystickAnalogueScale[i] = 65536;
	        ud.config.JoystickAnalogueDead[i] = 1000;
	        ud.config.JoystickAnalogueSaturate[i] = 9500;
		}
        CONTROL_SetAnalogAxisScale(i, ud.config.JoystickAnalogueScale[i], controldevice_joystick);

        ud.config.JoystickDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(joystickdigitaldefaults[i*2]);
        ud.config.JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(joystickdigitaldefaults[i*2+1]);
        CONTROL_MapDigitalAxis(i, ud.config.JoystickDigitalFunctions[i][0], 0, controldevice_joystick);
        CONTROL_MapDigitalAxis(i, ud.config.JoystickDigitalFunctions[i][1], 1, controldevice_joystick);

        ud.config.JoystickAnalogueAxes[i] = CONFIG_AnalogNameToNum(joystickanalogdefaults[i]);
        CONTROL_MapAnalogAxis(i, ud.config.JoystickAnalogueAxes[i], controldevice_joystick);
    }
}
/*
===================
=
= CONFIG_ReadKeys
=
===================
*/

void CONFIG_ReadKeys(void)
{
    int32_t i;
    int32_t numkeyentries;
    int32_t function;
    char keyname1[80];
    char keyname2[80];
    kb_scancode key1,key2;

    if (ud.config.scripthandle < 0) return;

    numkeyentries = SCRIPT_NumberEntries(ud.config.scripthandle,"KeyDefinitions");

    Bmemset(&KeyBindings,0,sizeof(KeyBindings));

    for (i=0; i<numkeyentries; i++)
    {
        function = CONFIG_FunctionNameToNum(SCRIPT_Entry(ud.config.scripthandle,"KeyDefinitions", i));
        if (function != -1)
        {
            memset(keyname1,0,sizeof(keyname1));
            memset(keyname2,0,sizeof(keyname2));
            SCRIPT_GetDoubleString
            (
                ud.config.scripthandle,
                "KeyDefinitions",
                SCRIPT_Entry(ud.config.scripthandle, "KeyDefinitions", i),
                keyname1,
                keyname2
            );
            key1 = 0xff;
            key2 = 0xff;
            if (keyname1[0])
            {
                key1 = (uint8_t) KB_StringToScanCode(keyname1);
            }
            if (keyname2[0])
            {
                key2 = (uint8_t) KB_StringToScanCode(keyname2);
            }
            ud.config.KeyboardKeys[function][0] = key1;
            ud.config.KeyboardKeys[function][1] = key2;
        }
    }

    for (i=0; i<NUMGAMEFUNCTIONS; i++)
    {
        if (i == gamefunc_Show_Console)
            OSD_CaptureKey(ud.config.KeyboardKeys[i][0]);
        else
            CONFIG_MapKey(i, ud.config.KeyboardKeys[i][0], 0, ud.config.KeyboardKeys[i][1], 0);
    }
}

// wrapper for CONTROL_MapKey(), generates key bindings to reflect changes to keyboard setup
void CONFIG_MapKey(int32_t which, kb_scancode key1, kb_scancode oldkey1, kb_scancode key2, kb_scancode oldkey2)
{
    int32_t i, j, k;
    int32_t ii[] = { key1, key2, oldkey1, oldkey2 };
    char buf[64];

    CONTROL_MapKey(which, key1, key2);

    for (k = 0; (unsigned)k < (sizeof(ii) / sizeof(ii[0])); k++)
    {
        if (ii[k] == 0xff || !ii[k]) continue;

        for (j=0; ConsoleKeys[j].name; j++)
            if (ii[k] == ConsoleKeys[j].id)
                break;
        if (ConsoleKeys[j].name)
            KeyBindings[ii[k]].key=Bstrdup(ConsoleKeys[j].name);

        KeyBindings[ii[k]].repeat = 1;
        KeyBindings[ii[k]].cmd[0] = 0;
        tempbuf[0] = 0;

        for (i=NUMGAMEFUNCTIONS-1; i>=0; i--)
        {
            if (ud.config.KeyboardKeys[i][0] == ii[k] || ud.config.KeyboardKeys[i][1] == ii[k])
            {
                Bsprintf(buf,"gamefunc_%s; ",CONFIG_FunctionNumToName(i));
                Bstrcat(tempbuf,buf);
            }
        }
        Bstrncpy(KeyBindings[ii[k]].cmd,tempbuf, MAXBINDSTRINGLENGTH-1);

        i = Bstrlen(KeyBindings[ii[k]].cmd);
        if (i)
            KeyBindings[ii[k]].cmd[i-2] = 0; // cut off the trailing "; "
    }
}

/*
===================
=
= CONFIG_SetupMouse
=
===================
*/

void CONFIG_SetupMouse(void)
{
    int32_t i;
    char str[80];
    char temp[80];
    int32_t scale;

    if (ud.config.scripthandle < 0) return;

    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        Bsprintf(str,"MouseButton%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
            ud.config.MouseFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseButtonClicked%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
            ud.config.MouseFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        Bsprintf(str,"MouseAnalogAxes%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_AnalogNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.MouseAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.MouseDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseDigitalAxes%d_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.MouseDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"MouseAnalogScale%d",i);
        scale = ud.config.MouseAnalogueScale[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.MouseAnalogueScale[i] = scale;
    }

    {
        tempbuf[0] = 0;
        SCRIPT_GetString(ud.config.scripthandle, "Controls","Mouse_Sensitivity",&tempbuf[0]);
        if (tempbuf[0]) CONTROL_MouseSensitivity = atof(tempbuf);
    }

    for (i=0; i<MAXMOUSEBUTTONS; i++)
    {
        CONTROL_MapButton(ud.config.MouseFunctions[i][0], i, 0, controldevice_mouse);
        CONTROL_MapButton(ud.config.MouseFunctions[i][1], i, 1,  controldevice_mouse);
    }
    for (i=0; i<MAXMOUSEAXES; i++)
    {
        CONTROL_MapAnalogAxis(i, ud.config.MouseAnalogueAxes[i], controldevice_mouse);
        CONTROL_MapDigitalAxis(i, ud.config.MouseDigitalFunctions[i][0], 0,controldevice_mouse);
        CONTROL_MapDigitalAxis(i, ud.config.MouseDigitalFunctions[i][1], 1,controldevice_mouse);
        CONTROL_SetAnalogAxisScale(i, ud.config.MouseAnalogueScale[i], controldevice_mouse);
    }
}

/*
===================
=
= CONFIG_SetupJoystick
=
===================
*/

void CONFIG_SetupJoystick(void)
{
    int32_t i;
    char str[80];
    char temp[80];
    int32_t scale;

    if (ud.config.scripthandle < 0) return;

    for (i=0; i<MAXJOYBUTTONS; i++)
    {
        Bsprintf(str,"JoystickButton%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
            ud.config.JoystickFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickButtonClicked%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle,"Controls", str,temp))
            ud.config.JoystickFunctions[i][1] = CONFIG_FunctionNameToNum(temp);
    }

    // map over the axes
    for (i=0; i<MAXJOYAXES; i++)
    {
        Bsprintf(str,"JoystickAnalogAxes%d",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_AnalogNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.JoystickAnalogueAxes[i] = CONFIG_AnalogNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%d_0",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.JoystickDigitalFunctions[i][0] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickDigitalAxes%d_1",i);
        temp[0] = 0;
        if (!SCRIPT_GetString(ud.config.scripthandle, "Controls", str,temp))
            if (CONFIG_FunctionNameToNum(temp) != -1 || (!temp[0] && CONFIG_FunctionNameToNum(temp) != -1))
                ud.config.JoystickDigitalFunctions[i][1] = CONFIG_FunctionNameToNum(temp);

        Bsprintf(str,"JoystickAnalogScale%d",i);
        scale = ud.config.JoystickAnalogueScale[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.JoystickAnalogueScale[i] = scale;

        Bsprintf(str,"JoystickAnalogDead%d",i);
        scale = ud.config.JoystickAnalogueDead[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.JoystickAnalogueDead[i] = scale;

        Bsprintf(str,"JoystickAnalogSaturate%d",i);
        scale = ud.config.JoystickAnalogueSaturate[i];
        SCRIPT_GetNumber(ud.config.scripthandle, "Controls", str,&scale);
        ud.config.JoystickAnalogueSaturate[i] = scale;
    }

    for (i=0; i<MAXJOYBUTTONS; i++)
    {
        CONTROL_MapButton(ud.config.JoystickFunctions[i][0], i, 0, controldevice_joystick);
        CONTROL_MapButton(ud.config.JoystickFunctions[i][1], i, 1,  controldevice_joystick);
    }
    for (i=0; i<MAXJOYAXES; i++)
    {
        CONTROL_MapAnalogAxis(i, ud.config.JoystickAnalogueAxes[i], controldevice_joystick);
        CONTROL_MapDigitalAxis(i, ud.config.JoystickDigitalFunctions[i][0], 0, controldevice_joystick);
        CONTROL_MapDigitalAxis(i, ud.config.JoystickDigitalFunctions[i][1], 1, controldevice_joystick);
        CONTROL_SetAnalogAxisScale(i, ud.config.JoystickAnalogueScale[i], controldevice_joystick);
    }
}

/*
===================
=
= CONFIG_ReadSetup
=
===================
*/
extern char *g_grpNamePtr;
extern void G_CheckPlayerColor(int32_t *color,int32_t prev_color);
extern palette_t CrosshairColors;
extern palette_t DefaultCrosshairColors;
extern char g_modDir[BMAX_PATH];
extern int32_t r_maxfps;
extern int32_t g_noSetup;
extern int32_t demorec_diffs_cvar, demoplay_diffs;
extern int32_t demorec_difftics_cvar, demorec_diffcompress_cvar, demorec_synccompress_cvar;
extern int32_t enableFramerateLimiter;

int32_t CONFIG_ReadSetup(void)
{
    int32_t dummy, i = 0;
    char commmacro[] = "CommbatMacro# ";
    extern int32_t g_forceWeaponChoice;
    char tempbuf[1024];

    CONTROL_ClearAssignments();
    CONFIG_SetDefaults();

    ud.config.setupread = 1;

    pathsearchmode = 1;
    if (SafeFileExists(setupfilename) && ud.config.scripthandle < 0)  // JBF 20031211
        ud.config.scripthandle = SCRIPT_Load(setupfilename);
    else if (SafeFileExists(SETUPFILENAME) && ud.config.scripthandle < 0)
    {
        Bsprintf(tempbuf,"The configuration file \"%s\" was not found. "
                 "Import configuration data from \"%s\"?",setupfilename,SETUPFILENAME);

        i=wm_ynbox("Import Configuration Settings",tempbuf);
        if (i) ud.config.scripthandle = SCRIPT_Load(SETUPFILENAME);
    }
    else if (SafeFileExists("duke3d.cfg") && ud.config.scripthandle < 0)
    {
        Bsprintf(tempbuf,"The configuration file \"%s\" was not found. "
                 "Import configuration data from \"duke3d.cfg\"?",setupfilename);

        i=wm_ynbox("Import Configuration Settings",tempbuf);
        if (i) ud.config.scripthandle = SCRIPT_Load("duke3d.cfg");
    }
    pathsearchmode = 0;

    if (ud.config.scripthandle < 0) return -1;

    if (ud.config.scripthandle >= 0)
    {
        char dummybuf[64];

        for (dummy = 0; dummy < 10; dummy++)
        {
            commmacro[13] = dummy+'0';
            SCRIPT_GetString(ud.config.scripthandle, "Comm Setup",commmacro,&ud.ridecule[dummy][0]);
        }

        SCRIPT_GetString(ud.config.scripthandle, "Comm Setup","PlayerName",&tempbuf[0]);

        while (Bstrlen(OSD_StripColors(dummybuf,tempbuf)) > 10)
            tempbuf[Bstrlen(tempbuf)-1] = '\0';

        Bstrncpy(szPlayerName,tempbuf,sizeof(szPlayerName)-1);
        szPlayerName[sizeof(szPlayerName)-1] = '\0';

        SCRIPT_GetString(ud.config.scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

        SCRIPT_GetNumber(ud.config.scripthandle, "Setup","ConfigVersion",&ud.configversion);
        SCRIPT_GetNumber(ud.config.scripthandle, "Setup","ForceSetup",&ud.config.ForceSetup);
        SCRIPT_GetNumber(ud.config.scripthandle, "Setup","NoAutoLoad",&ud.config.NoAutoLoad);

// #ifdef _WIN32
        if (g_noSetup == 0 && g_modDir[0] == '/')
        {
            struct stat st;
            SCRIPT_GetString(ud.config.scripthandle, "Setup","ModDir",&g_modDir[0]);

            if (Bstat(g_modDir, &st))
            {
                if ((st.st_mode & S_IFDIR) != S_IFDIR)
                {
                    initprintf("Invalid mod dir in cfg!\n");
                    Bsprintf(g_modDir,"/");
                }
            }
        }
// #endif

        {
            extern char defaultduke3dgrp[BMAX_PATH];
            if (!Bstrcmp(defaultduke3dgrp,"duke3d.grp"))
                SCRIPT_GetString(ud.config.scripthandle, "Setup","SelectedGRP",&g_grpNamePtr[0]);
        }

        if (!NAM)
        {
            SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "Out",&ud.lockout);
            SCRIPT_GetString(ud.config.scripthandle, "Screen Setup","Password",&ud.pwlockout[0]);
        }

        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenHeight",&ud.config.ScreenHeight);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenMode",&ud.config.ScreenMode);
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenWidth",&ud.config.ScreenWidth);
		SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenScaleMode", &ud.config.ScreenScaleMode);
		SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "FramerateLimiter", &enableFramerateLimiter);

#ifdef RENDERTYPEWIN
        {
            SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "WindowPositioning", (int32_t *)&windowpos);
            windowx = -1;
            SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "WindowPosX", (int32_t *)&windowx);
            windowy = -1;
            SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "WindowPosY", (int32_t *)&windowy);
        }
#endif

        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "ScreenBPP", &ud.config.ScreenBPP);
        if (ud.config.ScreenBPP < 8) ud.config.ScreenBPP = 32;

#ifdef POLYMER
        SCRIPT_GetNumber(ud.config.scripthandle, "Screen Setup", "Polymer", &dummy);
        if (dummy > 0 && ud.config.ScreenBPP >= 16) glrendmode = 4;
        else glrendmode = 3;
#endif

        /*

                    SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "Color",&ud.color);
                    G_CheckPlayerColor((int32_t *)&ud.color,-1);
                    g_player[0].ps->palookup = g_player[0].pcolor = ud.color;
                    tempbuf[0] = 0;
                    SCRIPT_GetString(ud.config.scripthandle, "Misc", "CrosshairColor",&tempbuf[0]);
                    if (tempbuf[0])
                    {
                        char *ptr = strtok(tempbuf,",");
                        palette_t temppal;
                        char *palptr = (char *)&temppal;

                        i = 0;
                        while (ptr != NULL && i < 3)
                        {
                            palptr[i++] = atoi(ptr);
                            ptr = strtok(NULL,",");
                        }
                        if (i == 3)
                        {
                            Bmemcpy(&CrosshairColors,&temppal,sizeof(palette_t));
                            DefaultCrosshairColors.f = 1;
                        }
                    }
        */

        SCRIPT_GetNumber(ud.config.scripthandle, "Misc", "Executions",&ud.executions);

        // weapon choices are defaulted in G_CheckCommandLine, which may override them
        if (!g_forceWeaponChoice)
            for (i=0; i<10; i++)
            {
                Bsprintf(buf,"WeaponChoice%d",i);
                dummy = -1;
                SCRIPT_GetNumber(ud.config.scripthandle, "Misc", buf, &dummy);
                if (dummy >= 0) g_player[0].wchoice[i] = dummy;
            }

#ifdef _WIN32
        SCRIPT_GetNumber(ud.config.scripthandle, "Updates", "CheckForUpdates", &ud.config.CheckForUpdates);
        SCRIPT_GetNumber(ud.config.scripthandle, "Updates", "LastUpdateCheck", &ud.config.LastUpdateCheck);
#endif

    }

//    CONFIG_ReadKeys();

    //CONFIG_SetupMouse(ud.config.scripthandle);
    //CONFIG_SetupJoystick(ud.config.scripthandle);
    ud.config.setupread = 1;
    return 0;
}

/*
===================
=
= CONFIG_WriteSetup
=
===================
*/

void CONFIG_WriteBinds(void) // save binds and aliases to <cfgname>_settings.cfg
{
    int32_t i;
    FILE *fp;
    char *ptr = Bstrdup(setupfilename);
    char tempbuf[128];

    if (!Bstrcmp(setupfilename, SETUPFILENAME))
        Bsprintf(tempbuf, "settings.cfg");
    else Bsprintf(tempbuf, "%s_settings.cfg", strtok(ptr, "."));

    fp = fopen(tempbuf, "wt");

    if (fp)
    {
        symbol_t *symb;

        fprintf(fp,"// this file automatically generated by EDuke32\n");
        fprintf(fp,"// these settings take precedence over your main cfg file\n");
        fprintf(fp,"// do not modify if you lack common sense\n");

        fprintf(fp,"unbindall\n");

        for (i=0; i<MAXBOUNDKEYS; i++)
            if (KeyBindings[i].cmd[0] && KeyBindings[i].key)
                fprintf(fp,"bind \"%s\"%s \"%s\"\n",KeyBindings[i].key,KeyBindings[i].repeat?"":" norepeat",KeyBindings[i].cmd);

        for (i=0; i<MAXMOUSEBUTTONS; i++)
            if (MouseBindings[i].cmd[0])
                fprintf(fp,"bind \"%s\"%s \"%s\"\n",MouseBindings[i].key,MouseBindings[i].repeat?"":" norepeat",MouseBindings[i].cmd);

        for (symb=symbols; symb!=NULL; symb=symb->next)
            if (symb->func == (void *)OSD_ALIAS)
                fprintf(fp,"alias \"%s\" \"%s\"\n", symb->name, symb->help);

        /*        for (i = 0; i < sizeof(cvar)/sizeof(cvarmappings); i++)
                    if (!(cvar[i].type&CVAR_NOSAVE))
                        fprintf(fp,"%s \"%d\"\n",cvar[i].name,*(int32_t*)cvar[i].var);
                        */
        OSD_WriteCvars(fp);
        fclose(fp);
        if (!Bstrcmp(setupfilename, SETUPFILENAME))
            Bsprintf(tempbuf, "Wrote settings.cfg\n");
        else Bsprintf(tempbuf,"Wrote %s_settings.cfg\n",ptr);
        OSD_Printf(tempbuf);
        Bfree(ptr);
        return;
    }

    if (!Bstrcmp(setupfilename, SETUPFILENAME))
        Bsprintf(tempbuf, "Error writing settings.cfg: %s\n", strerror(errno));
    else Bsprintf(tempbuf,"Error writing %s_settings.cfg: %s\n",ptr,strerror(errno));

    OSD_Printf(tempbuf);
    Bfree(ptr);
}

void CONFIG_WriteSetup(void)
{
    int32_t dummy;
    char tempbuf[1024];

    if (!ud.config.setupread) return;

    if (ud.config.scripthandle < 0)
        ud.config.scripthandle = SCRIPT_Init(setupfilename);

    SCRIPT_PutNumber(ud.config.scripthandle, "Misc", "Executions",++ud.executions,FALSE,FALSE);

    for (dummy=0; dummy<10; dummy++)
    {
        Bsprintf(buf,"WeaponChoice%d",dummy);
        SCRIPT_PutNumber(ud.config.scripthandle, "Misc",buf,g_player[myconnectindex].wchoice[dummy],FALSE,FALSE);
    }

    SCRIPT_PutNumber(ud.config.scripthandle, "Setup","ConfigVersion",BYTEVERSION_JF,FALSE,FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Setup", "ForceSetup",ud.config.ForceSetup,FALSE,FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Setup", "NoAutoLoad",ud.config.NoAutoLoad,FALSE,FALSE);

#ifdef POLYMER
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "Polymer",glrendmode == 4 && bpp > 8,FALSE,FALSE);
#endif

    if (!NAM)
    {
        SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "Out",ud.lockout,FALSE,FALSE);
        SCRIPT_PutString(ud.config.scripthandle, "Screen Setup", "Password",ud.pwlockout);
    }

    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenBPP",ud.config.ScreenBPP,FALSE,FALSE);  // JBF 20040523
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenHeight",ud.config.ScreenHeight,FALSE,FALSE);    // JBF 20031206
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenMode",ud.config.ScreenMode,FALSE,FALSE);    // JBF 20031206
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenWidth",ud.config.ScreenWidth,FALSE,FALSE);  // JBF 20031206
	SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "ScreenScaleMode", ud.config.ScreenScaleMode, FALSE, FALSE);
	SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "FramerateLimiter", enableFramerateLimiter, FALSE, FALSE);

#ifdef RENDERTYPEWIN
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "WindowPositioning", windowpos, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "WindowPosX", windowx, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Screen Setup", "WindowPosY", windowy, FALSE, FALSE);
#endif


#ifdef _WIN32
    SCRIPT_PutNumber(ud.config.scripthandle, "Updates", "CheckForUpdates", ud.config.CheckForUpdates, FALSE, FALSE);
    SCRIPT_PutNumber(ud.config.scripthandle, "Updates", "LastUpdateCheck", ud.config.LastUpdateCheck, FALSE, FALSE);
#endif

    for (dummy=0; dummy<MAXMOUSEBUTTONS; dummy++)
    {
        if (CONFIG_FunctionNumToName(ud.config.MouseFunctions[dummy][0]))
        {
            Bsprintf(buf,"MouseButton%d",dummy);
            SCRIPT_PutString(ud.config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(ud.config.MouseFunctions[dummy][0]));
        }

        if (dummy >= (MAXMOUSEBUTTONS-2)) continue;

        if (CONFIG_FunctionNumToName(ud.config.MouseFunctions[dummy][1]))
        {
            Bsprintf(buf,"MouseButtonClicked%d",dummy);
            SCRIPT_PutString(ud.config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(ud.config.MouseFunctions[dummy][1]));
        }
    }

    for (dummy=0; dummy<MAXMOUSEAXES; dummy++)
    {
        if (CONFIG_AnalogNumToName(ud.config.MouseAnalogueAxes[dummy]))
        {
            Bsprintf(buf,"MouseAnalogAxes%d",dummy);
            SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_AnalogNumToName(ud.config.MouseAnalogueAxes[dummy]));
        }

        if (CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[dummy][0]))
        {
            Bsprintf(buf,"MouseDigitalAxes%d_0",dummy);
            SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[dummy][0]));
        }

        if (CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[dummy][1]))
        {
            Bsprintf(buf,"MouseDigitalAxes%d_1",dummy);
            SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(ud.config.MouseDigitalFunctions[dummy][1]));
        }

        if (ud.config.MouseAnalogueScale[dummy] != 65536)
        {
            Bsprintf(buf,"MouseAnalogScale%d",dummy);
            SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, ud.config.MouseAnalogueScale[dummy], FALSE, FALSE);
        }
    }

    Bsprintf(tempbuf,"%.2f",CONTROL_MouseSensitivity);
    SCRIPT_PutString(ud.config.scripthandle,  "Controls","Mouse_Sensitivity",tempbuf);

    for (dummy=0; dummy<MAXJOYBUTTONS; dummy++)
    {
        if (CONFIG_FunctionNumToName(ud.config.JoystickFunctions[dummy][0]))
        {
            Bsprintf(buf,"JoystickButton%d",dummy);
            SCRIPT_PutString(ud.config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(ud.config.JoystickFunctions[dummy][0]));
        }

        if (CONFIG_FunctionNumToName(ud.config.JoystickFunctions[dummy][1]))
        {
            Bsprintf(buf,"JoystickButtonClicked%d",dummy);
            SCRIPT_PutString(ud.config.scripthandle,"Controls", buf, CONFIG_FunctionNumToName(ud.config.JoystickFunctions[dummy][1]));
        }
    }
    for (dummy=0; dummy<MAXJOYAXES; dummy++)
    {
        if (CONFIG_AnalogNumToName(ud.config.JoystickAnalogueAxes[dummy]))
        {
            Bsprintf(buf,"JoystickAnalogAxes%d",dummy);
            SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_AnalogNumToName(ud.config.JoystickAnalogueAxes[dummy]));
        }

        if (CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[dummy][0]))
        {
            Bsprintf(buf,"JoystickDigitalAxes%d_0",dummy);
            SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[dummy][0]));
        }

        if (CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[dummy][1]))
        {
            Bsprintf(buf,"JoystickDigitalAxes%d_1",dummy);
            SCRIPT_PutString(ud.config.scripthandle, "Controls", buf, CONFIG_FunctionNumToName(ud.config.JoystickDigitalFunctions[dummy][1]));
        }

        if (ud.config.JoystickAnalogueScale[dummy] != 65536)
        {
            Bsprintf(buf,"JoystickAnalogScale%d",dummy);
            SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, ud.config.JoystickAnalogueScale[dummy], FALSE, FALSE);
        }

        if (ud.config.JoystickAnalogueDead[dummy] != 1000)
        {
            Bsprintf(buf,"JoystickAnalogDead%d",dummy);
            SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, ud.config.JoystickAnalogueDead[dummy], FALSE, FALSE);
        }

        if (ud.config.JoystickAnalogueSaturate[dummy] != 9500)
        {
            Bsprintf(buf,"JoystickAnalogSaturate%d",dummy);
            SCRIPT_PutNumber(ud.config.scripthandle, "Controls", buf, ud.config.JoystickAnalogueSaturate[dummy], FALSE, FALSE);
        }
    }

    SCRIPT_PutString(ud.config.scripthandle, "Comm Setup","PlayerName",&szPlayerName[0]);
    SCRIPT_PutString(ud.config.scripthandle, "Comm Setup","RTSName",&ud.rtsname[0]);

    SCRIPT_PutString(ud.config.scripthandle, "Setup","SelectedGRP",&g_grpNamePtr[0]);

// #ifdef _WIN32
    if (g_noSetup == 0)
        SCRIPT_PutString(ud.config.scripthandle, "Setup","ModDir",&g_modDir[0]);
// #endif

    {
        char commmacro[] = "CommbatMacro# ";

        for (dummy = 0; dummy < 10; dummy++)
        {
            commmacro[13] = dummy+'0';
            SCRIPT_PutString(ud.config.scripthandle, "Comm Setup",commmacro,&ud.ridecule[dummy][0]);
        }
    }

    SCRIPT_Save(ud.config.scripthandle, setupfilename);
    SCRIPT_Free(ud.config.scripthandle);
    OSD_Printf("Wrote %s\n",setupfilename);
    CONFIG_WriteBinds();
}


int32_t CONFIG_GetMapBestTime(char *mapname)
{
    int32_t t = -1;
    char m[BMAX_PATH], *p;

    strcpy(m, mapname);
    p = strrchr(m, '/');
    if (!p) p = strrchr(m, '\\');
    if (p) Bmemmove(m, p, Bstrlen(p)+1);//strcpy(m, p);
    for (p=m; *p; p++) *p = tolower(*p);

    // cheap hack because SCRIPT_GetNumber doesn't like the slashes
    p = m;
    while (*p == '/') p++;

    if (!ud.config.setupread) return -1;
    if (ud.config.scripthandle < 0) return -1;

    SCRIPT_GetNumber(ud.config.scripthandle, "MapTimes", p, &t);
    return t;
}

int32_t CONFIG_SetMapBestTime(char *mapname, int32_t tm)
{
    char m[BMAX_PATH], *p;

    strcpy(m, mapname);
    p = strrchr(m, '/');
    if (!p) p = strrchr(m, '\\');
    if (p) strcpy(m, p);
    for (p=m; *p; p++) *p = tolower(*p);

    // cheap hack because SCRIPT_GetNumber doesn't like the slashes
    p = m;
    while (*p == '/') p++;

    if (ud.config.scripthandle < 0) ud.config.scripthandle = SCRIPT_Init(setupfilename);
    if (ud.config.scripthandle < 0) return -1;

    SCRIPT_PutNumber(ud.config.scripthandle, "MapTimes", p, tm, FALSE, FALSE);
    return 0;
}

/*
 * vim:ts=4:sw=4:
 */

