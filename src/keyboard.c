/*      $Id$

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2, or (at your option)
        any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., Inc., 51 Franklin Street, Fifth Floor, Boston,
        MA 02110-1301, USA.


        oroborus - (c) 2001 Ken Lynch
        xfwm4    - (c) 2002-2011 Olivier Fourdan,
                       2008 Jannis Pohlmann

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxfce4util/libxfce4util.h>
#include "keyboard.h"

#define MODIFIER_MASK (GDK_SHIFT_MASK | \
                       GDK_CONTROL_MASK | \
                       GDK_MOD1_MASK | \
                       GDK_MOD2_MASK | \
                       GDK_MOD3_MASK | \
                       GDK_MOD4_MASK | \
                       GDK_MOD5_MASK)

#define IGNORE_MASK (0x2000 | \
                     GDK_LOCK_MASK | \
                     GDK_HYPER_MASK | \
                     GDK_SUPER_MASK | \
                     GDK_META_MASK | \
                     NumLockMask | \
                     ScrollLockMask)

unsigned int AltMask;
unsigned int MetaMask;
unsigned int NumLockMask;
unsigned int ScrollLockMask;
unsigned int SuperMask;
unsigned int HyperMask;

static KeyCode
getKeycode (Display *dpy, const char *str)
{
    GdkModifierType keysym;

    gtk_accelerator_parse (str, &keysym, NULL);
    return XKeysymToKeycode (dpy, keysym);
}

guint
getModifierMap (const char *str)
{
    guint map;

    gtk_accelerator_parse (str, NULL, &map);

    if ((map & GDK_SUPER_MASK) == GDK_SUPER_MASK)
    {
        map |= SuperMask;
    }

    if ((map & GDK_HYPER_MASK) == GDK_HYPER_MASK)
    {
        map |= HyperMask;
    }

    if ((map & GDK_META_MASK) == GDK_META_MASK)
    {
        map |= MetaMask;
    }

    return map & MODIFIER_MASK & ~IGNORE_MASK;
}

void
parseKeyString (Display * dpy, MyKey * key, const char *str)
{
    g_return_if_fail (key != NULL);

    TRACE ("entering parseKeyString");
    TRACE ("key string=%s", str);

    key->keycode = 0;
    key->modifier = 0;

    if (str == NULL)
    {
        return;
    }

    if (!g_ascii_strcasecmp (str, "none"))
    {
        return;
    }

    key->keycode = getKeycode (dpy, str);
    key->modifier = getModifierMap (str);

    TRACE ("keycode = 0x%x, modifier = 0x%x", key->keycode, key->modifier);
}

gboolean
grabKey (Display * dpy, MyKey * key, Window w)
{
    int status;

    TRACE ("entering grabKey");

    status = GrabSuccess;
    if (key->keycode)
    {
        if (key->modifier != 0)
        {
            status |=
                XGrabKey (dpy, key->keycode,
                                        key->modifier, w,
                                        TRUE, GrabModeAsync, GrabModeSync);
        }

        /* Here we grab all combinations of well known modifiers */
        status |=
            XGrabKey (dpy, key->keycode,
                                    key->modifier | ScrollLockMask, w,
                                    TRUE, GrabModeAsync, GrabModeAsync);
        status |=
            XGrabKey (dpy, key->keycode,
                                    key->modifier | NumLockMask, w,
                                    TRUE, GrabModeAsync, GrabModeAsync);
        status |=
            XGrabKey (dpy, key->keycode,
                                    key->modifier | LockMask, w,
                                    TRUE, GrabModeAsync, GrabModeAsync);
        status |=
            XGrabKey (dpy, key->keycode,
                                    key->modifier | ScrollLockMask | NumLockMask, w,
                                    TRUE, GrabModeAsync, GrabModeAsync);
        status |=
            XGrabKey (dpy, key->keycode,
                                    key->modifier | ScrollLockMask | LockMask, w,
                                    TRUE, GrabModeAsync, GrabModeAsync);
        status |=
            XGrabKey (dpy, key->keycode,
                                    key->modifier | LockMask | NumLockMask, w,
                                    TRUE, GrabModeAsync, GrabModeAsync);
        status |=
            XGrabKey (dpy, key->keycode,
                                    key->modifier | ScrollLockMask | LockMask | NumLockMask, w,
                                    TRUE, GrabModeAsync, GrabModeAsync);
    }

    return (status == GrabSuccess);
}

void
ungrabKeys (Display * dpy, Window w)
{
    TRACE ("entering ungrabKeys");

    XUngrabKey (dpy, AnyKey, AnyModifier, w);
}

gboolean
grabButton (Display * dpy, int button, int modifier, Window w)
{
    int status;

    TRACE ("entering grabButton");

    status = GrabSuccess;
    if (modifier == AnyModifier)
    {
        status |=
            XGrabButton (dpy, button, AnyModifier, w, FALSE,
                                ButtonPressMask|ButtonReleaseMask, GrabModeSync, GrabModeAsync,
                                None, None);
    }
    else
    {
        /* Here we grab all combinations of well known modifiers */
        status |=
            XGrabButton (dpy, button, modifier,
                                w, FALSE,
                                ButtonPressMask|ButtonReleaseMask, GrabModeSync, GrabModeAsync,
                                None, None);
        status |=
            XGrabButton (dpy, button, modifier | ScrollLockMask,
                                w, FALSE,
                                ButtonPressMask|ButtonReleaseMask, GrabModeSync, GrabModeAsync,
                                None, None);
        status |=
            XGrabButton (dpy, button, modifier | NumLockMask,
                                w, FALSE,
                                ButtonPressMask|ButtonReleaseMask, GrabModeSync, GrabModeAsync,
                                None, None);
        status |=
            XGrabButton (dpy, button, modifier | LockMask, w, FALSE,
                                ButtonPressMask|ButtonReleaseMask, GrabModeSync, GrabModeAsync,
                                None, None);
        status |=
            XGrabButton (dpy, button, modifier | ScrollLockMask | NumLockMask,
                                w, FALSE,
                                ButtonPressMask|ButtonReleaseMask, GrabModeSync, GrabModeAsync,
                                None, None);
        status |=
            XGrabButton (dpy, button, modifier | ScrollLockMask | LockMask,
                                w, FALSE,
                                ButtonPressMask|ButtonReleaseMask, GrabModeSync, GrabModeAsync,
                                None, None);
        status |=
            XGrabButton (dpy, button, modifier | LockMask | NumLockMask,
                                w, FALSE,
                                ButtonPressMask|ButtonReleaseMask, GrabModeSync, GrabModeAsync,
                                None, None);
        status |=
            XGrabButton (dpy, button,
                                modifier | ScrollLockMask | LockMask | NumLockMask,
                                w, FALSE,
                                ButtonPressMask|ButtonReleaseMask, GrabModeSync, GrabModeAsync,
                                None, None);
    }

    return (status == GrabSuccess);
}

void
ungrabButton (Display * dpy, int button, int modifier, Window w)
{
    TRACE ("entering ungrabKeys");

    TRACE ("entering grabButton");

    if (modifier == AnyModifier)
    {
        XUngrabButton (dpy, button, AnyModifier, w);
    }
    else
    {
        /* Here we ungrab all combinations of well known modifiers */
        XUngrabButton (dpy, button, modifier, w);
        XUngrabButton (dpy, button, modifier | ScrollLockMask, w);
        XUngrabButton (dpy, button, modifier | NumLockMask, w);
        XUngrabButton (dpy, button, modifier | LockMask, w);
        XUngrabButton (dpy, button, modifier | ScrollLockMask | NumLockMask, w);
        XUngrabButton (dpy, button, modifier | ScrollLockMask | LockMask, w);
        XUngrabButton (dpy, button, modifier | LockMask | NumLockMask, w);
        XUngrabButton (dpy, button, modifier | ScrollLockMask | LockMask | NumLockMask, w);
    }
}

void
initModifiers (Display * dpy)
{
    XModifierKeymap *modmap;
    KeySym *keymap;
    int min_keycode, max_keycode, keycode;
    int keysyms_per_keycode;
    int i;

    AltMask = 0;
    MetaMask = 0;
    NumLockMask = 0;
    ScrollLockMask = 0;
    SuperMask = 0;
    HyperMask = 0;
    keysyms_per_keycode = 0;
    min_keycode = 0;
    max_keycode = 0;

    XDisplayKeycodes (dpy, &min_keycode, &max_keycode);
    modmap = XGetModifierMapping (dpy);
    keymap = XGetKeyboardMapping (dpy, min_keycode, max_keycode - min_keycode + 1, &keysyms_per_keycode);

    if (modmap && keymap)
    {
        for (i = 3 * modmap->max_keypermod; i < 8 * modmap->max_keypermod; i++)
        {
            keycode = modmap->modifiermap[i];
            if ((keycode >= min_keycode) && (keycode <= max_keycode))
            {
                int j;
                KeySym *syms = keymap + (keycode - min_keycode) * keysyms_per_keycode;

                for (j = 0; j < keysyms_per_keycode; j++)
                {
                    if (!NumLockMask && (syms[j] == XK_Num_Lock))
                    {
                        NumLockMask = (1 << (i / modmap->max_keypermod));
                    }
                    else if (!ScrollLockMask && (syms[j] == XK_Scroll_Lock))
                    {
                        ScrollLockMask = (1 << (i / modmap->max_keypermod));
                    }
                    else if (!AltMask && ((syms[j] == XK_Alt_L) || (syms[j] == XK_Alt_R)))
                    {
                        AltMask = (1 << (i / modmap->max_keypermod));
                    }
                    else if (!SuperMask && ((syms[j] == XK_Super_L) || (syms[j] == XK_Super_R)))
                    {
                        SuperMask = (1 << (i / modmap->max_keypermod));
                    }
                    else if (!HyperMask && ((syms[j] == XK_Hyper_L) || (syms[j] == XK_Hyper_R)))
                    {
                        HyperMask = (1 << (i / modmap->max_keypermod));
                    }
                    else if (!MetaMask && ((syms[j] == XK_Meta_L) || (syms[j] == XK_Meta_R)))
                    {
                        MetaMask = (1 << (i / modmap->max_keypermod));
                    }
                }
            }
        }
    }

    /* Cleanup memory */
    if (modmap)
    {
        XFreeModifiermap (modmap);
    }

    if (keymap)
    {
        XFree (keymap);
    }

    /* In case we didn't find AltMask, use Mod1Mask */
    if (AltMask == 0)
    {
        AltMask = Mod1Mask;
    }
}
