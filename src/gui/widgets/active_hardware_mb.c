/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/widgets/active_hardware_mb.h"
#include "gui/widgets/active_hardware_popover.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ActiveHardwareMbWidget,
  active_hardware_mb_widget,
  GTK_TYPE_BOX)

static void
on_create_popover (
  GtkMenuButton * button,
  gpointer        user_data)
{
  ActiveHardwareMbWidget * self =
    Z_ACTIVE_HARDWARE_MB_WIDGET (user_data);
  self->popover =
    active_hardware_popover_widget_new (self);
  gtk_menu_button_set_popover (
    button, GTK_WIDGET (self->popover));
}

#if 0
static void
set_label (ActiveHardwareMbWidget * self)
{
  gtk_label_set_text (
    self->label, _("Select..."));
}
#endif

/**
 * This is called when the popover closes.
 */
void
active_hardware_mb_widget_refresh (
  ActiveHardwareMbWidget * self)
{
  /*set_label (self);*/
  active_hardware_mb_widget_save_settings (self);
  if (self->callback)
    {
      self->callback (self->object);
    }
}

void
active_hardware_mb_widget_save_settings (
  ActiveHardwareMbWidget * self)
{
  GtkToggleButton * chkbtn;
  char * controllers[40];
  int num_controllers = 0;

  for (GtkWidget * child =
         gtk_widget_get_first_child (
           GTK_WIDGET (
             self->popover->controllers_box));
       child != NULL;
       child =
         gtk_widget_get_next_sibling (child))
    {
      if (!GTK_IS_CHECK_BUTTON (child))
        continue;

      chkbtn = GTK_TOGGLE_BUTTON (child);
      if (gtk_toggle_button_get_active (chkbtn))
        {
          controllers[num_controllers++] =
            g_strdup (
              gtk_button_get_label (
                GTK_BUTTON (chkbtn)));
        }
    }
  controllers[num_controllers] = NULL;

  int res =
    g_settings_set_strv (
      self->settings, self->key,
      (const char * const*) controllers);
  g_return_if_fail (res == 1);
}

void
active_hardware_mb_widget_setup (
  ActiveHardwareMbWidget * self,
  bool                     is_input,
  bool                     is_midi,
  GSettings *              settings,
  const char *             key)
{
  self->is_midi = is_midi;
  self->input = is_input;
  self->settings = settings;
  self->key = key;

  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->mbutton),
    is_input ?
      _("Click to enable inputs") :
      _("Click to enable outputs"));
}

ActiveHardwareMbWidget *
active_hardware_mb_widget_new (void)
{
  ActiveHardwareMbWidget * self =
    g_object_new (
      ACTIVE_HARDWARE_MB_WIDGET_TYPE, NULL);

  return self;
}

static void
active_hardware_mb_widget_class_init (
  ActiveHardwareMbWidgetClass * klass)
{
}

static void
active_hardware_mb_widget_init (
  ActiveHardwareMbWidget * self)
{
  self->mbutton =
    GTK_MENU_BUTTON (gtk_menu_button_new ());
  gtk_box_append (
    GTK_BOX (self),
    GTK_WIDGET (self->mbutton));

  gtk_menu_button_set_label (
    self->mbutton, _("Select..."));

  gtk_menu_button_set_create_popup_func (
    self->mbutton, on_create_popover, self,
    NULL);
}
