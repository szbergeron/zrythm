/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <string.h>

#include "audio/engine.h"
#include "audio/ext_port.h"
#include "audio/hardware_processor.h"
#include "gui/widgets/active_hardware_mb.h"
#include "gui/widgets/active_hardware_popover.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ActiveHardwarePopoverWidget,
  active_hardware_popover_widget,
  GTK_TYPE_POPOVER)

static void
on_closed (ActiveHardwarePopoverWidget *self,
               gpointer    user_data)
{
  active_hardware_mb_widget_refresh (self->owner);
}

static void
get_controllers (
  ActiveHardwarePopoverWidget * self,
  char ** controllers,
  int *   num_controllers)
{
#if 0
  ExtPort * ports[EXT_PORTS_MAX];
  int size = 0;
  ext_ports_get (
    TYPE_EVENT, FLOW_OUTPUT, 1, ports, &size);

  for (int i = 0; i < size; i++)
    {
      controllers[(*num_controllers)++] =
        g_strdup (ports[i]->full_name);
    }
  ext_ports_free (ports, size);
#endif

  HardwareProcessor * processor =
    AUDIO_ENGINE->hw_in_processor;

  /* force a rescan just in case */
  hardware_processor_rescan_ext_ports (processor);
  for (int i = 0;
       i <
         (self->owner->is_midi ?
            processor->num_ext_midi_ports :
            processor->num_ext_audio_ports);
       i++)
    {
      ExtPort * port =
        self->owner->is_midi ?
          processor->ext_midi_ports[i] :
          processor->ext_audio_ports[i];
      controllers[(*num_controllers)++] =
        ext_port_get_id (port);
    }
}

/**
 * Finds checkbutton with given label.
 */
static GtkWidget *
find_checkbutton (
  ActiveHardwarePopoverWidget * self,
  const char * label)
{
  GList *children, *iter;
  GtkButton * chkbtn;

  children =
    gtk_container_get_children (
      GTK_CONTAINER (self->controllers_box));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      if (!GTK_IS_CHECK_BUTTON (iter->data))
        continue;

      chkbtn = GTK_BUTTON (iter->data);
      if (!strcmp (gtk_button_get_label (chkbtn),
                   label))
        {
          g_list_free (children);
          return GTK_WIDGET (chkbtn);
        }
    }

  g_list_free (children);
  return NULL;
}


static void
setup (
  ActiveHardwarePopoverWidget * self)
{
  char * controllers[60];
  int num_controllers = 0;
  int i;
  GtkWidget * chkbtn;

  /* remove pre-existing controllers */
  z_gtk_container_destroy_all_children (
    GTK_CONTAINER (self->controllers_box));

  /* scan controllers and add them */
  get_controllers (
    self, controllers, &num_controllers);
  for (i = 0; i < num_controllers; i++)
    {
      chkbtn =
        gtk_check_button_new_with_label (
          controllers[i]);
      gtk_widget_set_visible (chkbtn, 1);
      gtk_container_add (
        GTK_CONTAINER (self->controllers_box),
        chkbtn);
    }

  /* fetch saved controllers and tick them if they
   * exist */
  gchar ** saved_controllers =
    self->owner->is_midi ?
      g_settings_get_strv (
        S_P_GENERAL_ENGINE,
        "midi-controllers") :
      g_settings_get_strv (
        S_P_GENERAL_ENGINE,
        "audio-inputs");
  char * tmp;
  i = 0;
  while ((tmp = saved_controllers[i]) != NULL)
    {
      /* find checkbutton matching saved controller */
      chkbtn = find_checkbutton (self, tmp);

      if (chkbtn)
        {
          /* tick it */
          gtk_toggle_button_set_active (
            GTK_TOGGLE_BUTTON (chkbtn), 1);
        }

      i++;
    }

  /* cleanup */
  for (i = 0; i < num_controllers; i++)
    g_free (controllers[i]);
}

static void
on_rescan (
  GtkButton * btn,
  ActiveHardwarePopoverWidget * self)
{
  setup (self);
}

ActiveHardwarePopoverWidget *
active_hardware_popover_widget_new (
  ActiveHardwareMbWidget * owner)
{
  ActiveHardwarePopoverWidget * self =
    g_object_new (
      ACTIVE_HARDWARE_POPOVER_WIDGET_TYPE, NULL);

  self->owner = owner;

  setup (self);

  return self;
}

static void
active_hardware_popover_widget_class_init (
  ActiveHardwarePopoverWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "active_hardware_popover.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ActiveHardwarePopoverWidget, x)

  BIND_CHILD (rescan);
  BIND_CHILD (controllers_box);

#undef BIND_CHILD

  gtk_widget_class_bind_template_callback (
    klass, on_closed);
}

static void
active_hardware_popover_widget_init (
  ActiveHardwarePopoverWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (
    G_OBJECT (self->rescan), "clicked",
    G_CALLBACK (on_rescan), self);
}
