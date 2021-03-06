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


        xfwm4    - (c) 2002-2011 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <glib.h>
#include <libxfce4util/libxfce4util.h>

#include "display.h"
#include "screen.h"
#include "mywindow.h"
#include "client.h"
#include "stacking.h"
#include "netwm.h"
#include "transients.h"
#include "frame.h"
#include "focus.h"

static guint raise_timeout = 0;

void
clientApplyStackList (ScreenInfo *screen_info)
{
    Window *xwinstack;
    guint nwindows;
    gint i;

    DBG ("applying stack list");
    nwindows = g_list_length (screen_info->windows_stack);

    i = 0;
    xwinstack = g_new (Window, nwindows + 4);
    xwinstack[i++] = MYWINDOW_XWINDOW (screen_info->sidewalk[0]);
    xwinstack[i++] = MYWINDOW_XWINDOW (screen_info->sidewalk[1]);
    xwinstack[i++] = MYWINDOW_XWINDOW (screen_info->sidewalk[2]);
    xwinstack[i++] = MYWINDOW_XWINDOW (screen_info->sidewalk[3]);

    if (nwindows)
    {
        GList *list;
        Client *c = NULL;

        for (list = g_list_last(screen_info->windows_stack); list; list = g_list_previous (list))
        {
            c = (Client *) list->data;
            xwinstack[i++] = c->frame;
            DBG ("  [%i] \"%s\" (0x%lx)", i, c->name, c->window);
        }
    }

    XRestackWindows (myScreenGetXDisplay (screen_info), xwinstack, (int) nwindows + 4);

    g_free (xwinstack);
}

Client *
clientGetLowestTransient (Client * c)
{
    ScreenInfo *screen_info;
    Client *lowest_transient, *c2;
    GList *list;

    g_return_val_if_fail (c != NULL, NULL);

    TRACE ("entering clientGetLowestTransient");

    lowest_transient = NULL;
    screen_info = c->screen_info;
    for (list = screen_info->windows_stack; list; list = g_list_next (list))
    {
        c2 = (Client *) list->data;
        if ((c2 != c) && clientIsTransientFor (c2, c))
        {
            lowest_transient = c2;
            break;
        }
    }
    return lowest_transient;
}

Client *
clientGetHighestTransientOrModalFor (Client * c)
{
    ScreenInfo *screen_info;
    Client *highest_transient;
    Client *c2;
    GList *list;

    g_return_val_if_fail (c != NULL, NULL);
    TRACE ("entering clientGetHighestTransientOrModalFor");

    screen_info = c->screen_info;
    highest_transient = NULL;
    for (list = screen_info->windows_stack; list; list = g_list_next (list))
    {
        c2 = (Client *) list->data;
        if (c2)
        {
            if (clientIsTransientOrModalFor (c, c2))
            {
                highest_transient = c2;
            }
        }
    }

    return highest_transient;
}

Client *
clientGetTopMostForGroup (Client * c)
{
    ScreenInfo *screen_info;
    Client *top_most;
    Client *c2;
    GList *list;

    g_return_val_if_fail (c != NULL, NULL);
    TRACE ("entering clientGetTopMostForGroup");

    screen_info = c->screen_info;
    top_most = NULL;
    for (list = screen_info->windows_stack; list; list = g_list_next (list))
    {
        c2 = (Client *) list->data;
        if (c2 != c)
        {
            if (clientSameGroup (c, c2))
            {
                top_most = c2;
            }
        }
    }

    return top_most;
}

gboolean
clientIsTopMost (Client *c)
{
    ScreenInfo *screen_info;
    GList *list, *list2;
    Client *c2;

    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("entering clientIsTopMost");

    screen_info = c->screen_info;

    list = g_list_find (screen_info->windows_stack, (gconstpointer) c);
    if (list)
    {
        list2 = g_list_next (list);
        while (list2)
        {
            c2 = (Client *) list2->data;
            if (FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_VISIBLE) && (c2->win_layer == c->win_layer))
            {
                return FALSE;
            }
            list2 = g_list_next (list2);
        }
    }
    return TRUE;
}

Client *
clientGetNextTopMost (ScreenInfo *screen_info, guint layer, Client * exclude)
{
    Client *top, *c;
    GList *list;

    TRACE ("entering clientGetNextTopMost");

    top = NULL;
    for (list = screen_info->windows_stack; list; list = g_list_next (list))
    {
        c = (Client *) list->data;
        TRACE ("*** stack window \"%s\" (0x%lx), layer %i", c->name, c->window, (int) c->win_layer);
        if (!exclude || (c != exclude))
        {
            if (c->win_layer > layer)
            {
                top = c;
                break;
            }
        }
    }

    return top;
}

Client *
clientGetBottomMost (ScreenInfo *screen_info, guint layer, Client * exclude)
{
    Client *bot, *c;
    GList *list;

    TRACE ("entering clientGetBottomMost");

    bot = NULL;
    for (list = screen_info->windows_stack; list; list = g_list_next (list))
    {
        c = (Client *) list->data;
        if (c)
        {
            TRACE ("*** stack window \"%s\" (0x%lx), layer %i", c->name,
                c->window, (int) c->win_layer);
            if (!exclude || (c != exclude))
            {
                if (c->win_layer < layer)
                {
                    bot = c;
                }
                else if (c->win_layer >= layer)
                {
                    break;
                }
            }
        }
    }
    return bot;
}

/*
 * This function does the same as XQueryPointer but w/out the race
 * conditions caused by querying the X server
 */
Client *
clientAtPosition (ScreenInfo *screen_info, int x, int y, GList * exclude_list)
{
    GList *list;
    Client *c, *c2;

    TRACE ("entering clientAtPosition");

    c = NULL;
    for (list = g_list_last (screen_info->windows_stack); list; list = g_list_previous (list))
    {
        c2 = (Client *) list->data;
        if ((frameX (c2) <= x) && (frameX (c2) + frameWidth (c2) >= x)
            && (frameY (c2) <= y) && (frameY (c2) + frameHeight (c2) >= y))
        {
            if (clientSelectMask (c2, NULL, SEARCH_INCLUDE_SKIP_PAGER | SEARCH_INCLUDE_SKIP_TASKBAR, WINDOW_REGULAR_FOCUSABLE)
                && !g_list_find (exclude_list, (gconstpointer) c2))
            {
                c = c2;
                break;
            }
        }
    }

    return c;
}

void
clientRaise (Client * c, Window wsibling)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Client *c2, *c3, *client_sibling;
    GList *transients;
    GList *sibling;
    GList *list1, *list2;
    GList *windows_stack_copy;

    g_return_if_fail (c != NULL);

    TRACE ("entering clientRaise");

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    client_sibling = NULL;
    transients = NULL;
    sibling = NULL;

    if (c == screen_info->last_raise)
    {
        TRACE ("client \"%s\" (0x%lx) already raised", c->name, c->window);
        return;
    }
    TRACE ("raising client \"%s\" (0x%lx) over (0x%lx)", c->name, c->window, wsibling);

    /*
     * If the raised window is the one that has focus, fine, we can
     * release the grab we have on it since there is no use for it
     * anymore.
     *
     * However, if the raised window is not the focused one, then we
     * end up with some kind of indermination, so we need to regrab
     * the buttons for the user to be able to raise or focus the window
     * by clicking inside.
     */

    if (c == clientGetFocus ())
    {
        clientPassGrabMouseButton (c);
    }
    else
    {
        clientPassGrabMouseButton (NULL);
    }

    if (g_list_length (screen_info->windows_stack) < 1)
    {
        return;
    }

    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MANAGED))
    {
        /* Copy the existing window stack temporarily as reference */
        windows_stack_copy = g_list_copy (screen_info->windows_stack);
        /* Search for the window that will be just on top of the raised window  */
        if (wsibling)
        {
            c2 = myDisplayGetClientFromWindow (display_info, wsibling, SEARCH_FRAME | SEARCH_WINDOW);
            if (c2)
            {
                sibling = g_list_find (screen_info->windows_stack, (gconstpointer) c2);
                if (sibling)
                {
                    list1 = g_list_next (sibling);
                    if (list1)
                    {
                        client_sibling = (Client *) list1->data;
                        /* Do not place window under higher layers though */
                        if ((client_sibling) && (client_sibling->win_layer < c->win_layer))
                        {
                            client_sibling = NULL;
                        }
                    }
                }
            }
        }
        if (!client_sibling)
        {
            client_sibling = clientGetNextTopMost (screen_info, c->win_layer, c);
        }
        screen_info->windows_stack = g_list_remove (screen_info->windows_stack, (gconstpointer) c);
        if (client_sibling)
        {
            /* If there is one, look for its place in the list */
            sibling = g_list_find (screen_info->windows_stack, (gconstpointer) client_sibling);
            /* Place the raised window just before it */
            screen_info->windows_stack = g_list_insert_before (screen_info->windows_stack, sibling, c);
        }
        else
        {
            /* There will be no window on top of the raised window, so place it at the end of list */
            screen_info->windows_stack = g_list_append (screen_info->windows_stack, c);
        }
        /* Now, look for transients, transients of transients, etc. */
        for (list1 = windows_stack_copy; list1; list1 = g_list_next (list1))
        {
            c2 = (Client *) list1->data;
            if (c2)
            {
                if ((c2 != c) && clientIsTransientOrModalFor (c2, c) && (c2->win_layer <= c->win_layer))
                {
                    transients = g_list_append (transients, c2);
                    if (sibling)
                    {
                        /* Make sure client_sibling is not c2 otherwise we create a circular linked list */
                        if (client_sibling != c2)
                        {
                            /* Place the transient window just before sibling */
                            screen_info->windows_stack = g_list_remove (screen_info->windows_stack, (gconstpointer) c2);
                            screen_info->windows_stack = g_list_insert_before (screen_info->windows_stack, sibling, c2);
                        }
                    }
                    else
                    {
                        /* There will be no window on top of the transient window, so place it at the end of list */
                        screen_info->windows_stack = g_list_remove (screen_info->windows_stack, (gconstpointer) c2);
                        screen_info->windows_stack = g_list_append (screen_info->windows_stack, c2);
                    }
                }
                else
                {
                    for (list2 = transients; list2; list2 = g_list_next (list2))
                    {
                        c3 = (Client *) list2->data;
                        if ((c3 != c2) && clientIsTransientOrModalFor (c2, c3))
                        {
                            transients = g_list_append (transients, c2);
                            if (sibling)
                            {
                                /* Again, make sure client_sibling is not c2 to avoid a circular linked list */
                                if (client_sibling != c2)
                                {
                                    /* Place the transient window just before sibling */
                                    screen_info->windows_stack = g_list_remove (screen_info->windows_stack, (gconstpointer) c2);
                                    screen_info->windows_stack = g_list_insert_before (screen_info->windows_stack, sibling, c2);
                                }
                            }
                            else
                            {
                                /* There will be no window on top of the transient window, so place it at the end of list */
                                screen_info->windows_stack = g_list_remove (screen_info->windows_stack, (gconstpointer) c2);
                                screen_info->windows_stack = g_list_append (screen_info->windows_stack, c2);
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (transients)
        {
            g_list_free (transients);
        }
        if (windows_stack_copy)
        {
            g_list_free (windows_stack_copy);
        }
        /* Now, screen_info->windows_stack contains the correct window stack
           We still need to tell the X Server to reflect the changes
         */
        clientApplyStackList (screen_info);
        clientSetNetClientList (c->screen_info, display_info->atoms[NET_CLIENT_LIST_STACKING], screen_info->windows_stack);
        screen_info->last_raise = c;
    }
}

void
clientLower (Client * c, Window wsibling)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Client *c2, *client_sibling;
    GList *sibling;
    GList *list;
    gint position;

    g_return_if_fail (c != NULL);

    TRACE ("entering clientLower");
    TRACE ("lowering client \"%s\" (0x%lx) below (0x%lx)", c->name, c->window, wsibling);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    client_sibling = NULL;
    sibling = NULL;
    c2 = NULL;

    if (g_list_length (screen_info->windows_stack) < 1)
    {
        return;
    }

    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MANAGED))
    {
        if (clientIsTransientOrModalForGroup (c))
        {
            client_sibling = clientGetTopMostForGroup (c);
        }
        else if (clientIsTransient (c))
        {
            client_sibling = clientGetTransient (c);
        }
        else if (wsibling)
        {
            c2 = myDisplayGetClientFromWindow (display_info, wsibling, SEARCH_FRAME | SEARCH_WINDOW);
            if (c2)
            {
                sibling = g_list_find (screen_info->windows_stack, (gconstpointer) c2);
                if (sibling)
                {
                    list = g_list_previous (sibling);
                    if (list)
                    {
                        client_sibling = (Client *) list->data;
                        /* Do not place window above lower layers though */
                        if ((client_sibling) && (client_sibling->win_layer > c->win_layer))
                        {
                            client_sibling = NULL;
                        }
                    }
                }
            }
        }
        if ((!client_sibling) ||
            (client_sibling && (client_sibling->win_layer < c->win_layer)))
        {
            client_sibling = clientGetBottomMost (screen_info, c->win_layer, c);
        }
        if (client_sibling != c)
        {
            screen_info->windows_stack = g_list_remove (screen_info->windows_stack, (gconstpointer) c);
            /* Paranoid check to avoid circular linked list */
            if (client_sibling)
            {
                sibling = g_list_find (screen_info->windows_stack, (gconstpointer) client_sibling);
                position = g_list_position (screen_info->windows_stack, sibling) + 1;

                screen_info->windows_stack = g_list_insert (screen_info->windows_stack, c, position);
                TRACE ("lowest client is \"%s\" (0x%lx) at position %i",
                        client_sibling->name, client_sibling->window, position);
            }
            else
            {
                screen_info->windows_stack = g_list_prepend (screen_info->windows_stack, c);
            }
        }
        /* Now, screen_info->windows_stack contains the correct window stack
           We still need to tell the X Server to reflect the changes
         */
        clientApplyStackList (screen_info);
        clientSetNetClientList (screen_info, display_info->atoms[NET_CLIENT_LIST_STACKING], screen_info->windows_stack);
        clientPassGrabMouseButton (NULL);
        clientPassFocus (screen_info, c, NULL);
        if (screen_info->last_raise == c)
        {
            screen_info->last_raise = NULL;
        }
    }
}

gboolean
clientAdjustFullscreenLayer (Client *c, gboolean set)
{
    g_return_val_if_fail (c, FALSE);

    TRACE ("entering clientAdjustFullscreenLayer");
    TRACE ("%s fullscreen layer for  \"%s\" (0x%lx)", set ? "Setting" : "Unsetting", c->name, c->window);

    if (set)
    {
        if (FLAG_TEST(c->xfwm_flags, XFWM_FLAG_LEGACY_FULLSCREEN)
            || FLAG_TEST(c->flags, CLIENT_FLAG_FULLSCREEN))
        {
            clientSetLayer (c, WIN_LAYER_FULLSCREEN);
            return TRUE;
        }
    }
    else if (c->win_layer == WIN_LAYER_FULLSCREEN)
    {
        if (FLAG_TEST(c->flags, CLIENT_FLAG_FULLSCREEN))
        {
            TRACE ("Moving \"%s\" (0x%lx) to initial layer %d", c->name, c->window, c->fullscreen_old_layer);
            clientSetLayer (c, c->fullscreen_old_layer);
            return TRUE;
        }
        if (FLAG_TEST(c->xfwm_flags, XFWM_FLAG_LEGACY_FULLSCREEN))
        {
            TRACE ("Moving \"%s\" (0x%lx) to layer %d", c->name, c->window, WIN_LAYER_FULLSCREEN);
            clientSetLayer (c, WIN_LAYER_NORMAL);
            return TRUE;
        }
    }
    return FALSE;
}

void
clientAddToList (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    TRACE ("entering clientAddToList");

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    myDisplayAddClient (display_info, c);

    screen_info->client_count++;
    if (screen_info->clients)
    {
        c->prev = screen_info->clients->prev;
        c->next = screen_info->clients;
        screen_info->clients->prev->next = c;
        screen_info->clients->prev = c;
    }
    else
    {
        screen_info->clients = c;
        c->next = c;
        c->prev = c;
    }

    TRACE ("adding window \"%s\" (0x%lx) to windows list", c->name, c->window);
    screen_info->windows = g_list_append (screen_info->windows, c);
    screen_info->windows_stack = g_list_append (screen_info->windows_stack, c);

    clientSetNetClientList (screen_info, display_info->atoms[NET_CLIENT_LIST], screen_info->windows);
    clientSetNetClientList (screen_info, display_info->atoms[WIN_CLIENT_LIST], screen_info->windows);

    FLAG_SET (c->xfwm_flags, XFWM_FLAG_MANAGED);
}

void
clientRemoveFromList (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    TRACE ("entering clientRemoveFromList");

    FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_MANAGED);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    myDisplayRemoveClient (display_info, c);

    g_assert (screen_info->client_count > 0);
    screen_info->client_count--;
    if (screen_info->client_count == 0)
    {
        screen_info->clients = NULL;
    }
    else
    {
        c->next->prev = c->prev;
        c->prev->next = c->next;
        if (c == screen_info->clients)
        {
            screen_info->clients = screen_info->clients->next;
        }
    }

    TRACE ("removing window \"%s\" (0x%lx) from windows list", c->name, c->window);
    screen_info->windows = g_list_remove (screen_info->windows, c);

    TRACE ("removing window \"%s\" (0x%lx) from screen_info->windows_stack list", c->name, c->window);
    screen_info->windows_stack = g_list_remove (screen_info->windows_stack, c);

    clientSetNetClientList (screen_info, display_info->atoms[NET_CLIENT_LIST], screen_info->windows);
    clientSetNetClientList (screen_info, display_info->atoms[WIN_CLIENT_LIST], screen_info->windows);
    clientSetNetClientList (screen_info, display_info->atoms[NET_CLIENT_LIST_STACKING], screen_info->windows_stack);

    FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_MANAGED);
}

GList *
clientGetStackList (ScreenInfo *screen_info)
{
    g_return_val_if_fail (screen_info, NULL);

    if (screen_info->windows_stack)
    {
        return g_list_copy (screen_info->windows_stack);
    }
    return NULL;
}

void
clientSetLastRaise (Client *c)
{
    ScreenInfo *screen_info;

    g_return_if_fail (c != NULL);

    screen_info = c->screen_info;
    screen_info->last_raise = c;
}

Client *
clientGetLastRaise (ScreenInfo *screen_info)
{
    g_return_val_if_fail (screen_info, NULL);
    return (screen_info->last_raise);
}

void
clientClearLastRaise (ScreenInfo *screen_info)
{
    g_return_if_fail (screen_info);
    screen_info->last_raise = NULL;
}

static gboolean
delayed_raise_cb (gpointer data)
{
    Client *c;

    TRACE ("entering delayed_raise_cb");

    clientClearDelayedRaise ();
    c = clientGetFocus ();

    if (c)
    {
        clientRaise (c, None);
    }
    return (TRUE);
}

void
clientClearDelayedRaise (void)
{
    if (raise_timeout)
    {
        g_source_remove (raise_timeout);
        raise_timeout = 0;
    }
}

void
clientResetDelayedRaise (ScreenInfo *screen_info)
{
    if (raise_timeout)
    {
        g_source_remove (raise_timeout);
    }
    raise_timeout = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                        screen_info->params->raise_delay,
                                        (GSourceFunc) delayed_raise_cb,
                                        NULL, NULL);
}

