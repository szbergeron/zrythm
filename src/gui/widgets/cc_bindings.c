/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/cc_bindings.h"
#include "gui/widgets/cc_bindings_tree.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  CcBindingsWidget, cc_bindings_widget, GTK_TYPE_BOX)

/**
 * Refreshes the cc_bindings widget.
 */
void
cc_bindings_widget_refresh (
  CcBindingsWidget * self)
{
  cc_bindings_tree_widget_refresh (
    self->bindings_tree);
}

CcBindingsWidget *
cc_bindings_widget_new ()
{
  CcBindingsWidget * self =
    g_object_new (CC_BINDINGS_WIDGET_TYPE, NULL);

  self->bindings_tree =
    cc_bindings_tree_widget_new ();
  gtk_box_append (
    GTK_BOX (self),
    GTK_WIDGET (self->bindings_tree));
  gtk_widget_set_vexpand (
    GTK_WIDGET (self->bindings_tree), 1);

  return self;
}

static void
cc_bindings_widget_class_init (
  CcBindingsWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "cc-bindings");
}

static void
cc_bindings_widget_init (
  CcBindingsWidget * self)
{
}
