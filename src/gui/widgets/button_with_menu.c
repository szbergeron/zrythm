/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "gui/widgets/button_with_menu.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ButtonWithMenuWidget,
  button_with_menu_widget,
  GTK_TYPE_BOX)

void
button_with_menu_widget_set_menu_model (
  ButtonWithMenuWidget * self,
  GMenuModel *           gmenu_model)
{
  gtk_menu_button_set_menu_model (
    self->menu_btn, gmenu_model);
}

/**
 * This must only be called once to set up the
 * widget.
 *
 * @param btn The main button.
 * @param menu Optional GtkMenu to set for the
 *   arrow button.
 * @param menu Optional GMenuModel to set for the
 *   arrow button.
 */
void
button_with_menu_widget_setup (
  ButtonWithMenuWidget * self,
  GtkButton *            btn,
  GMenuModel *           gmenu_model,
  bool                   downward_arrow,
  int                    height,
  const char *           btn_tooltip_text,
  const char *           menu_tooltip_text)
{
  gtk_box_set_homogeneous (
    GTK_BOX (self), false);

  gtk_box_append (
    GTK_BOX (self), GTK_WIDGET (btn));

  /* add arrow */
  self->menu_btn =
    GTK_MENU_BUTTON (gtk_menu_button_new ());
  gtk_menu_button_set_icon_name (
    GTK_MENU_BUTTON (self->menu_btn),
    downward_arrow ?
      "arrow-down-small" : "arrow-up-small");
  gtk_box_append (
    GTK_BOX (self),
    GTK_WIDGET (self->menu_btn));
  /* TODO write CSS rule to set image size to 6 */
  gtk_widget_add_css_class (
    GTK_WIDGET (self->menu_btn), "arrow-button");

  gtk_widget_add_css_class (
    GTK_WIDGET (self), "linked");
  gtk_widget_add_css_class (
    GTK_WIDGET (self), "button-with-menu");

  int width;
  gtk_widget_get_size_request (
    GTK_WIDGET (btn), &width, NULL);
  gtk_widget_set_size_request (
    GTK_WIDGET (btn), width, height);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->menu_btn), -1, height);

  gtk_widget_set_tooltip_text (
    GTK_WIDGET (btn), btn_tooltip_text);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->menu_btn), menu_tooltip_text);

  if (gmenu_model)
    {
      button_with_menu_widget_set_menu_model (
        self, gmenu_model);
    }
}

static void
finalize (
  ButtonWithMenuWidget * self)
{
  G_OBJECT_CLASS (
    button_with_menu_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
button_with_menu_widget_init (
  ButtonWithMenuWidget * self)
{
}

static void
button_with_menu_widget_class_init (
  ButtonWithMenuWidgetClass * _klass)
{
  GObjectClass * klass = G_OBJECT_CLASS (_klass);

  klass->finalize = (GObjectFinalizeFunc) finalize;
}
