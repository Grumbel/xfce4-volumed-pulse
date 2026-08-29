/*
 *  xfce4-volumed - Volume management daemon for XFCE 4
 *
 *  Copyright © 2009 Steve Dodier <sidnioulz@gmail.com>
 *  Copyright © 2012 Lionel Le Folgoc <lionel@lefolgoc.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gio/gio.h>
#include <gdk/gdk.h>
#include <keybinder.h>

#include "xvd_keys.h"
#include "xvd_pulse.h"

/* Delay before rebinding after a keyboard device change or resume. Gives
 * the X server time to finish device setup / mapping. */
#define XVD_KEYS_REBIND_DELAY_MS 500

typedef struct
{
  const char       *keystring;
  KeybinderHandler  handler;
} XvdKeyBinding;

static void xvd_raise_handler (const char *keystring, void *Inst);
static void xvd_lower_handler (const char *keystring, void *Inst);
static void xvd_mute_handler (const char *keystring, void *Inst);
static void xvd_mic_mute_handler (const char *keystring, void *Inst);

/*
 * All modifier combinations for each multimedia key. Binding every variant
 * avoids the keys being swallowed when a modifier is held (lp #665146).
 */
static const XvdKeyBinding xvd_key_bindings[] = {
  /* Raise volume */
  { "XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Ctrl>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Alt>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Super>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Shift>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Ctrl><Shift>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Ctrl><Alt>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Ctrl><Super>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Alt><Shift>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Alt><Super>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Shift><Super>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Ctrl><Shift><Super>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Ctrl><Shift><Alt>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Ctrl><Alt><Super>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Shift><Alt><Super>XF86AudioRaiseVolume", xvd_raise_handler },
  { "<Ctrl><Shift><Alt><Super>XF86AudioRaiseVolume", xvd_raise_handler },

  /* Lower volume */
  { "XF86AudioLowerVolume", xvd_lower_handler },
  { "<Ctrl>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Alt>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Super>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Shift>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Ctrl><Shift>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Ctrl><Alt>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Ctrl><Super>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Alt><Shift>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Alt><Super>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Shift><Super>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Ctrl><Shift><Super>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Ctrl><Shift><Alt>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Ctrl><Alt><Super>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Shift><Alt><Super>XF86AudioLowerVolume", xvd_lower_handler },
  { "<Ctrl><Shift><Alt><Super>XF86AudioLowerVolume", xvd_lower_handler },

  /* Mute */
  { "XF86AudioMute", xvd_mute_handler },
  { "<Ctrl>XF86AudioMute", xvd_mute_handler },
  { "<Alt>XF86AudioMute", xvd_mute_handler },
  { "<Super>XF86AudioMute", xvd_mute_handler },
  { "<Shift>XF86AudioMute", xvd_mute_handler },
  { "<Ctrl><Shift>XF86AudioMute", xvd_mute_handler },
  { "<Ctrl><Alt>XF86AudioMute", xvd_mute_handler },
  { "<Ctrl><Super>XF86AudioMute", xvd_mute_handler },
  { "<Alt><Shift>XF86AudioMute", xvd_mute_handler },
  { "<Alt><Super>XF86AudioMute", xvd_mute_handler },
  { "<Shift><Super>XF86AudioMute", xvd_mute_handler },
  { "<Ctrl><Shift><Super>XF86AudioMute", xvd_mute_handler },
  { "<Ctrl><Shift><Alt>XF86AudioMute", xvd_mute_handler },
  { "<Ctrl><Alt><Super>XF86AudioMute", xvd_mute_handler },
  { "<Shift><Alt><Super>XF86AudioMute", xvd_mute_handler },
  { "<Ctrl><Shift><Alt><Super>XF86AudioMute", xvd_mute_handler },

  /* Mic mute */
  { "XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Ctrl>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Alt>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Super>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Shift>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Ctrl><Shift>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Ctrl><Alt>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Ctrl><Super>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Alt><Shift>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Alt><Super>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Shift><Super>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Ctrl><Shift><Super>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Ctrl><Shift><Alt>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Ctrl><Alt><Super>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Shift><Alt><Super>XF86AudioMicMute", xvd_mic_mute_handler },
  { "<Ctrl><Shift><Alt><Super>XF86AudioMicMute", xvd_mic_mute_handler },
};

static XvdInstance *keys_instance = NULL;
static guint        rebind_timeout_id = 0;
static gulong       seat_device_added_id = 0;
static gulong       seat_device_removed_id = 0;
static GDBusProxy  *login1_proxy = NULL;
static gulong       login1_signal_id = 0;

static void
xvd_raise_handler (const char *keystring, void *Inst)
{
  XvdInstance *xvd_inst = (XvdInstance *) Inst;

  g_debug ("The RaiseVolume key was pressed.");

  xvd_update_volume (xvd_inst, XVD_UP);
}

static void
xvd_lower_handler (const char *keystring, void *Inst)
{
  XvdInstance *xvd_inst = (XvdInstance *) Inst;

  g_debug ("The LowerVolume key was pressed.");

  xvd_update_volume (xvd_inst, XVD_DOWN);
}

static void
xvd_mute_handler (const char *keystring, void *Inst)
{
  XvdInstance *xvd_inst = (XvdInstance *) Inst;

  g_debug ("The Mute key was pressed.");

  xvd_toggle_mute (xvd_inst);
}

static void
xvd_mic_mute_handler (const char *keystring, void *Inst)
{
  XvdInstance *xvd_inst = (XvdInstance *) Inst;

  g_debug ("The MicMute key was pressed.");

  xvd_toggle_mic_mute (xvd_inst);
}

static void
xvd_keys_unbind_all (void)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (xvd_key_bindings); i++)
    keybinder_unbind (xvd_key_bindings[i].keystring, xvd_key_bindings[i].handler);
}

static void
xvd_keys_bind_all (void)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (xvd_key_bindings); i++)
    {
      if (!keybinder_bind (xvd_key_bindings[i].keystring,
                           xvd_key_bindings[i].handler,
                           keys_instance))
        {
          g_debug ("Failed to bind key: %s", xvd_key_bindings[i].keystring);
        }
    }
}

static void
xvd_keys_rebind (void)
{
  g_debug ("Rebinding volume keys");
  xvd_keys_unbind_all ();
  xvd_keys_bind_all ();
}

static gboolean
xvd_keys_rebind_timeout_cb (gpointer user_data)
{
  (void) user_data;

  rebind_timeout_id = 0;
  xvd_keys_rebind ();
  return G_SOURCE_REMOVE;
}

static void
xvd_keys_schedule_rebind (void)
{
  if (rebind_timeout_id != 0)
    g_source_remove (rebind_timeout_id);

  rebind_timeout_id = g_timeout_add (XVD_KEYS_REBIND_DELAY_MS,
                                     xvd_keys_rebind_timeout_cb,
                                     NULL);
}

static void
xvd_keys_device_changed (GdkSeat   *seat,
                         GdkDevice *device,
                         gpointer   user_data)
{
  (void) seat;
  (void) user_data;

  if (gdk_device_get_source (device) != GDK_SOURCE_KEYBOARD)
    return;

  g_debug ("Keyboard device changed (%s), scheduling key rebind",
           gdk_device_get_name (device));
  xvd_keys_schedule_rebind ();
}

static void
xvd_keys_connect_device_signals (void)
{
  GdkDisplay *display;
  GdkSeat    *seat;

  display = gdk_display_get_default ();
  if (display == NULL)
    return;

  seat = gdk_display_get_default_seat (display);
  if (seat == NULL)
    return;

  seat_device_added_id = g_signal_connect (seat, "device-added",
                                          G_CALLBACK (xvd_keys_device_changed),
                                          NULL);
  seat_device_removed_id = g_signal_connect (seat, "device-removed",
                                            G_CALLBACK (xvd_keys_device_changed),
                                            NULL);
}

static void
xvd_keys_disconnect_device_signals (void)
{
  GdkDisplay *display;
  GdkSeat    *seat;

  if (seat_device_added_id == 0 && seat_device_removed_id == 0)
    return;

  display = gdk_display_get_default ();
  if (display == NULL)
    return;

  seat = gdk_display_get_default_seat (display);
  if (seat == NULL)
    return;

  if (seat_device_added_id != 0)
    {
      g_signal_handler_disconnect (seat, seat_device_added_id);
      seat_device_added_id = 0;
    }

  if (seat_device_removed_id != 0)
    {
      g_signal_handler_disconnect (seat, seat_device_removed_id);
      seat_device_removed_id = 0;
    }
}

static void
xvd_keys_login1_signal_cb (GDBusProxy  *proxy,
                           const gchar *sender_name,
                           const gchar *signal_name,
                           GVariant    *parameters,
                           gpointer     user_data)
{
  gboolean preparing;

  (void) proxy;
  (void) sender_name;
  (void) user_data;

  if (g_strcmp0 (signal_name, "PrepareForSleep") != 0)
    return;

  if (!g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(b)")))
    return;

  g_variant_get (parameters, "(b)", &preparing);

  /* preparing == TRUE: about to sleep; FALSE: resumed from sleep */
  if (!preparing)
    {
      g_debug ("System resumed from sleep, scheduling key rebind");
      xvd_keys_schedule_rebind ();
    }
}

static void
xvd_keys_connect_login1 (void)
{
  GError *error = NULL;

  login1_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                NULL,
                                                "org.freedesktop.login1",
                                                "/org/freedesktop/login1",
                                                "org.freedesktop.login1.Manager",
                                                NULL,
                                                &error);
  if (login1_proxy == NULL)
    {
      g_debug ("Could not connect to logind (PrepareForSleep unavailable): %s",
               error != NULL ? error->message : "unknown error");
      g_clear_error (&error);
      return;
    }

  login1_signal_id = g_signal_connect (login1_proxy, "g-signal",
                                       G_CALLBACK (xvd_keys_login1_signal_cb),
                                       NULL);
}

static void
xvd_keys_disconnect_login1 (void)
{
  if (login1_proxy == NULL)
    return;

  if (login1_signal_id != 0)
    {
      g_signal_handler_disconnect (login1_proxy, login1_signal_id);
      login1_signal_id = 0;
    }

  g_clear_object (&login1_proxy);
}

void
xvd_keys_init (XvdInstance *Inst)
{
  keys_instance = Inst;

  keybinder_init ();
  xvd_keys_bind_all ();
  xvd_keys_connect_device_signals ();
  xvd_keys_connect_login1 ();
}

void
xvd_keys_release (XvdInstance *Inst)
{
  (void) Inst;

  if (rebind_timeout_id != 0)
    {
      g_source_remove (rebind_timeout_id);
      rebind_timeout_id = 0;
    }

  xvd_keys_disconnect_login1 ();
  xvd_keys_disconnect_device_signals ();
  xvd_keys_unbind_all ();
  keys_instance = NULL;
}
