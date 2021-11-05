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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/channel.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/chord_key.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChordEditorSpaceWidget,
  chord_editor_space_widget,
  GTK_TYPE_BOX)

#define DEFAULT_PX_PER_KEY 12

/**
 * Links scroll windows after all widgets have been
 * initialized.
 */
static void
link_scrolls (
  ChordEditorSpaceWidget * self)
{
  /* link note keys v scroll to arranger v scroll */
  if (self->chord_keys_scroll)
    {
      gtk_scrolled_window_set_vadjustment (
        self->chord_keys_scroll,
        gtk_scrolled_window_get_vadjustment (
          GTK_SCROLLED_WINDOW (
            self->arranger_scroll)));
    }

  /* link ruler h scroll to arranger h scroll */
  if (MW_CLIP_EDITOR_INNER->ruler_scroll)
    {
      gtk_scrolled_window_set_hadjustment (
        MW_CLIP_EDITOR_INNER->ruler_scroll,
        gtk_scrolled_window_get_hadjustment (
          self->arranger_scroll));
    }
}

/**
 * See CLIP_EDITOR_INNER_WIDGET_ADD_TO_SIZEGROUP.
 */
void
chord_editor_space_widget_update_size_group (
  ChordEditorSpaceWidget * self,
  int                     visible)
{
  clip_editor_inner_widget_add_to_left_of_ruler_sizegroup (
    MW_CLIP_EDITOR_INNER,
    GTK_WIDGET (self->chord_keys_box),
    visible);
}

static void
on_realize (
  GtkWidget * widget,
  ChordEditorSpaceWidget * self)
{
}

int
chord_editor_space_widget_get_chord_height (
  ChordEditorSpaceWidget * self)
{
  return
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self->chord_keys[0]));
}

int
chord_editor_space_widget_get_all_chords_height (
  ChordEditorSpaceWidget * self)
{
  return
    CHORD_EDITOR->num_chords *
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self->chord_keys[0]));
}

void
chord_editor_space_widget_refresh (
  ChordEditorSpaceWidget * self)
{
  link_scrolls (self);
}

void
chord_editor_space_widget_setup (
  ChordEditorSpaceWidget * self)
{
  if (self->arranger)
    {
      arranger_widget_setup (
        Z_ARRANGER_WIDGET (self->arranger),
        ARRANGER_WIDGET_TYPE_CHORD,
        SNAP_GRID_EDITOR);
    }

  for (int i = 0; i < CHORD_EDITOR->num_chords; i++)
    {
      self->chord_keys[i] =
        chord_key_widget_new (CHORD_EDITOR->chords[i]);
      GtkBox * box =
        GTK_BOX (
          gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                       0));
      gtk_widget_set_visible (
        GTK_WIDGET (box), 1);
      gtk_box_append (
        box, GTK_WIDGET (self->chord_keys[i]));
      gtk_widget_add_css_class (
        GTK_WIDGET (box), "chord_key");
      gtk_box_append (
        self->chord_keys_box,
        GTK_WIDGET (box));
      self->chord_key_boxes[i] = box;
    }

  chord_editor_space_widget_refresh (self);
}

static void
chord_editor_space_widget_init (
  ChordEditorSpaceWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->arranger->type =
    ARRANGER_WIDGET_TYPE_CHORD;

  self->arranger_and_keys_vsize_group =
    gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (
    self->arranger_and_keys_vsize_group,
    GTK_WIDGET (self->arranger));
  gtk_size_group_add_widget (
    self->arranger_and_keys_vsize_group,
    GTK_WIDGET (self->chord_keys_box));

  g_signal_connect (
    G_OBJECT (self), "realize",
    G_CALLBACK (on_realize),  self);
}

static void
chord_editor_space_widget_class_init (
  ChordEditorSpaceWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass,
    "chord_editor_space.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ChordEditorSpaceWidget, x)

  BIND_CHILD (arranger_scroll);
  BIND_CHILD (arranger_viewport);
  BIND_CHILD (arranger);
  BIND_CHILD (chord_keys_box);
  BIND_CHILD (chord_keys_scroll);

#undef BIND_CHILD
}
