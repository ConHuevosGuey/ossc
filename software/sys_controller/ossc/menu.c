//
// Copyright (C) 2015-2016  Markus Hiienkari <mhiienka@niksula.hut.fi>
//
// This file is part of Open Source Scan Converter project.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <string.h>
#include "menu.h"
#include "lcd.h"
#include "tvp7002.h"

extern char row1[LCD_ROW_LEN+1], row2[LCD_ROW_LEN+1], menu_row1[LCD_ROW_LEN+1], menu_row2[LCD_ROW_LEN+1];
extern avconfig_t tc;
extern alt_u32 remote_code;
extern alt_u8 menu_active;

//TODO: move to separate source file(s)
extern alt_u16 rc_keymap[22];
#define lcd_write_menu() lcd_write((char*)&menu_row1, (char*)&menu_row2)
#define lcd_write_status() lcd_write((char*)&row1, (char*)&row2)

static const char *off_on_desc[] = { "Off", "On" };
static const char *video_lpf_desc[] = { "Auto", "Off", "95MHz (HDTV II)", "35MHz (HDTV I)", "16MHz (EDTV)", "9MHz (SDTV)" };
static const char *ypbpr_cs_desc[] = { "Rec. 601", "Rec. 709" };
static const char *s480p_mode_desc[] = { "Auto", "DTV 480p", "VGA 640x480" };
static const char *sync_lpf_desc[] = { "Off", "33MHz (min)", "10MHz (med)", "2.5MHz (max)" };
static const char *l3_mode_desc[] = { "Generic 16:9", "Generic 4:3", "320x240 optim.", "256x240 optim." };
static const char *tx_mode_desc[] = { "HDMI", "DVI" };
static const char *sl_mode_desc[] = { "Off", "Horizontal", "Vertical" };
static const char *sl_id_desc[] = { "Even", "Odd" };

static void sampler_phase_disp(alt_u8 v) { sniprintf(menu_row2, LCD_ROW_LEN+1, "%d deg", (v*1125)/100); }
static void sync_vth_disp(alt_u8 v) { sniprintf(menu_row2, LCD_ROW_LEN+1, "%d mV", (v*1127)/100); }
static void vsync_thold_disp(alt_u8 v) { sniprintf(menu_row2, LCD_ROW_LEN+1, "%u.%u us", (unsigned)(((1000000U*v)/(clkrate[REFCLK_INTCLK]/1000))/1000), (unsigned)((((1000000U*v)/(clkrate[REFCLK_INTCLK]/1000))%1000)/100)); }
static void sl_str_disp(alt_u8 v) { sniprintf(menu_row2, LCD_ROW_LEN+1, "%u%%", ((v+1)*625)/100); }
static void lines_disp(alt_u8 v) { sniprintf(menu_row2, LCD_ROW_LEN+1, "%u lines", v); }
static void pixels_disp(alt_u8 v) { sniprintf(menu_row2, LCD_ROW_LEN+1, "%u pixels", v); }

MENU(menu_vinputproc, P99_PROTECT({ \
    { "Video LPF",          OPT_AVCONFIG_SELECTION, { .sel = { &tc.video_lpf,   SETTING_ITEM(video_lpf_desc) } } },
    { "YPbPr in ColSpa",    OPT_AVCONFIG_SELECTION, { .sel = { &tc.ypbpr_cs,    SETTING_ITEM(ypbpr_cs_desc) } } },
    { "Auto lev. ctrl",     OPT_AVCONFIG_SELECTION, { .sel = { &tc.en_alc,      SETTING_ITEM(off_on_desc) } } },
}))

MENU(menu_sampling, P99_PROTECT({ \
    { "Sampling phase",     OPT_AVCONFIG_NUMVALUE,  { .num = { &tc.sampler_phase, 0, 31, sampler_phase_disp } } },
    { "480p in sampler",    OPT_AVCONFIG_SELECTION, { .sel = { &tc.s480p_mode,  SETTING_ITEM(s480p_mode_desc) } } },
    //{ "Modeparam editor", OPT_SUBMENU,            { .sub =  NULL } },
}))

MENU(menu_sync, P99_PROTECT({ \
    { "Analog sync LPF",    OPT_AVCONFIG_SELECTION, { .sel = { &tc.sync_lpf,    SETTING_ITEM(sync_lpf_desc) } } },
    { "Analog sync Vth",    OPT_AVCONFIG_NUMVALUE,  { .num = { &tc.sync_vth,    0, 31, sync_vth_disp } } },
    { "Vsync threshold",    OPT_AVCONFIG_NUMVALUE,  { .num = { &tc.vsync_thold, 10, 200, vsync_thold_disp } } },
    { "H-PLL Pre-Coast",    OPT_AVCONFIG_NUMVALUE,  { .num = { &tc.pre_coast,   0, 5, lines_disp } } },
    { "H-PLL Post-Coast",   OPT_AVCONFIG_NUMVALUE,  { .num = { &tc.post_coast,  0, 5, lines_disp } } },
}))

MENU(menu_output, P99_PROTECT({ \
    { "240p/288p lineX3",   OPT_AVCONFIG_SELECTION, { .sel = { &tc.linemult_target, SETTING_ITEM(off_on_desc) } } },
    { "Linetriple mode",    OPT_AVCONFIG_SELECTION, { .sel = { &tc.l3_mode,     SETTING_ITEM(l3_mode_desc) } } },
    //{ "Interlace passt.",            OPT_AVCONFIG_SELECTION, { .sel = { &tc.s480p_mode, SETTING_ITEM(s480p_desc) } } },
    { "TX mode",            OPT_AVCONFIG_SELECTION, { .sel = { &tc.tx_mode,     SETTING_ITEM(tx_mode_desc) } } },
}))

MENU(menu_postproc, P99_PROTECT({ \
    { "Scanlines",          OPT_AVCONFIG_SELECTION, { .sel = { &tc.sl_mode,     SETTING_ITEM(sl_mode_desc) } } },
    { "Scanline str.",      OPT_AVCONFIG_NUMVALUE,  { .num = { &tc.sl_str,      0, 15, sl_str_disp } } },
    { "Scanline id.",       OPT_AVCONFIG_SELECTION, { .sel = { &tc.sl_id,       SETTING_ITEM(sl_id_desc) } } },
    { "Horizontal mask",    OPT_AVCONFIG_NUMVALUE,  { .num = { &tc.h_mask,      0, 63, pixels_disp } } },
    { "Vertical mask",      OPT_AVCONFIG_NUMVALUE,  { .num = { &tc.v_mask,      0, 63, pixels_disp } } },
}))

MENU(menu_main, P99_PROTECT({ \
    { "Video in proc",      OPT_SUBMENU,            { .sub = &menu_vinputproc } }, \
    { "Sampling opt.",      OPT_SUBMENU,            { .sub = &menu_sampling } }, \
    { "Sync opt.",          OPT_SUBMENU,            { .sub = &menu_sync } }, \
    { "Output opt.",        OPT_SUBMENU,            { .sub = &menu_output } }, \
    { "Post-proc.",         OPT_SUBMENU,            { .sub = &menu_postproc } }, \
    { "Fw. update",         OPT_FUNC_CALL,          { .fun = { fw_update, "OK - pls restart" } } }, \
    { "Reset settings",     OPT_FUNC_CALL,          { .fun = { set_default_avconfig, "Reset done" } } }, \
    { "Save settings",      OPT_FUNC_CALL,          { .fun = { write_userdata, "Saved" } } }, \
}))

// Max 2 levels currently
menunavi navi[] = {{&menu_main, 0}, {NULL, 0}};
alt_u8 navlvl = 0;


void display_menu(alt_u8 forcedisp)
{
    menucode_id code;
    int retval = 0;

    if (remote_code == rc_keymap[RC_UP])
        code = PREV_PAGE;
    else if (remote_code == rc_keymap[RC_DOWN])
        code = NEXT_PAGE;
    else if (remote_code == rc_keymap[RC_RIGHT])
        code = VAL_PLUS;
    else if (remote_code == rc_keymap[RC_LEFT])
        code = VAL_MINUS;
    else if (remote_code == rc_keymap[RC_OK])
        code = OPT_SELECT;
    else if (remote_code == rc_keymap[RC_BACK])
        code = PREV_MENU;
    else
        code = NO_ACTION;

    if (!forcedisp && (code == NO_ACTION))
        return;

    // Parse menu control
    switch (code) {
    case PREV_PAGE:
        navi[navlvl].mp = (navi[navlvl].mp == 0) ? navi[navlvl].m->num_items-1 : (navi[navlvl].mp-1);
        break;
    case NEXT_PAGE:
        navi[navlvl].mp = (navi[navlvl].mp+1) % navi[navlvl].m->num_items;
        break;
    case PREV_MENU:
        if (navlvl > 0) {
            navlvl--;
        } else {
            menu_active = 0;
            lcd_write_status();
            return;
        }
        break;
    case OPT_SELECT:
        switch (navi[navlvl].m->items[navi[navlvl].mp].type) {
            case OPT_SUBMENU:
                if (navi[navlvl+1].m != navi[navlvl].m->items[navi[navlvl].mp].sub)
                    navi[navlvl+1].mp = 0;
                navi[navlvl+1].m = navi[navlvl].m->items[navi[navlvl].mp].sub;
                navlvl++;
                break;
            case OPT_FUNC_CALL:
                retval = navi[navlvl].m->items[navi[navlvl].mp].fun.f();
                break;
            default:
                break;
        }
        break;
    case VAL_MINUS:
        switch (navi[navlvl].m->items[navi[navlvl].mp].type) {
            case OPT_AVCONFIG_SELECTION:
                if (*(navi[navlvl].m->items[navi[navlvl].mp].sel.data) > 0)
                    *(navi[navlvl].m->items[navi[navlvl].mp].sel.data) = *(navi[navlvl].m->items[navi[navlvl].mp].sel.data) - 1;
                break;
            case OPT_AVCONFIG_NUMVALUE:
                if (*(navi[navlvl].m->items[navi[navlvl].mp].num.data) > navi[navlvl].m->items[navi[navlvl].mp].num.min)
                    *(navi[navlvl].m->items[navi[navlvl].mp].num.data) = *(navi[navlvl].m->items[navi[navlvl].mp].num.data) - 1;
                break;
            default:
                break;
        }
        break;
    case VAL_PLUS:
        switch (navi[navlvl].m->items[navi[navlvl].mp].type) {
            case OPT_AVCONFIG_SELECTION:
                if (*(navi[navlvl].m->items[navi[navlvl].mp].sel.data) < navi[navlvl].m->items[navi[navlvl].mp].sel.num_settings-1)
                    *(navi[navlvl].m->items[navi[navlvl].mp].sel.data) = *(navi[navlvl].m->items[navi[navlvl].mp].sel.data) + 1;
                break;
            case OPT_AVCONFIG_NUMVALUE:
                if (*(navi[navlvl].m->items[navi[navlvl].mp].num.data) < navi[navlvl].m->items[navi[navlvl].mp].num.max)
                    *(navi[navlvl].m->items[navi[navlvl].mp].num.data) = *(navi[navlvl].m->items[navi[navlvl].mp].num.data) + 1;
                break;
            default:
                break;
        }
        break;
    default:
        break;
    }

    // Generate menu text
    switch (navi[navlvl].m->items[navi[navlvl].mp].type) {
        case OPT_AVCONFIG_SELECTION:
            strncpy(menu_row1, navi[navlvl].m->items[navi[navlvl].mp].name, LCD_ROW_LEN+1);
            strncpy(menu_row2, navi[navlvl].m->items[navi[navlvl].mp].sel.setting_str[*(navi[navlvl].m->items[navi[navlvl].mp].sel.data)], LCD_ROW_LEN+1);
            break;
        case OPT_AVCONFIG_NUMVALUE:
            strncpy(menu_row1, navi[navlvl].m->items[navi[navlvl].mp].name, LCD_ROW_LEN+1);
            navi[navlvl].m->items[navi[navlvl].mp].num.f(*(navi[navlvl].m->items[navi[navlvl].mp].num.data));
            break;
        case OPT_SUBMENU:
            sniprintf(menu_row1, LCD_ROW_LEN+1,  "%s >", navi[navlvl].m->items[navi[navlvl].mp].name);
            menu_row2[0] = 0;
            break;
        case OPT_FUNC_CALL:
            sniprintf(menu_row1, LCD_ROW_LEN+1,  "<%s>", navi[navlvl].m->items[navi[navlvl].mp].name);
            if (code == OPT_SELECT)
                sniprintf(menu_row2, LCD_ROW_LEN+1, "%s", (retval==0) ? navi[navlvl].m->items[navi[navlvl].mp].fun.text_success : "Error");
            else
                menu_row2[0] = 0;
            break;
        default:
            break;
    }

    lcd_write_menu();

    return;
}
