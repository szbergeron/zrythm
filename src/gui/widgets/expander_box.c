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

#include "gui/widgets/expander_box.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (
  ExpanderBoxWidget,
  expander_box_widget,
  GTK_TYPE_BOX)

#define GET_PRIVATE(s) \
  ExpanderBoxWidgetPrivate * prv = \
     expander_box_widget_get_private (s);

static void
on_clicked (
  GtkButton *button,
  ExpanderBoxWidget * self)
{
  GET_PRIVATE (self);

  bool revealed =
    !gtk_revealer_get_reveal_child (
      prv->revealer);
  gtk_revealer_set_reveal_child (
    prv->revealer, revealed);
  gtk_widget_set_visible (
    GTK_WIDGET (prv->revealer), revealed);

  if (prv->reveal_cb)
    {
      prv->reveal_cb (
        self, revealed, prv->user_data);
    }
}

/**
 * Gets the private.
 */
ExpanderBoxWidgetPrivate *
expander_box_widget_get_private (
  ExpanderBoxWidget * self)
{
  return expander_box_widget_get_instance_private (
    self);
}

/**
 * Reveals or hides the expander box's contents.
 */
void
expander_box_widget_set_reveal (
  ExpanderBoxWidget * self,
  int                 reveal)
{
  GET_PRIVATE (self);
  gtk_revealer_set_reveal_child (
    prv->revealer, reveal);
  gtk_widget_set_visible (
    GTK_WIDGET (prv->revealer), reveal);
}

/**
 * Sets the label to show.
 */
void
expander_box_widget_set_label (
  ExpanderBoxWidget * self,
  const char *        label)
{
  GET_PRIVATE (self);

  gtk_label_set_text (
    prv->btn_label, label);
}

void
expander_box_widget_set_orientation (
  ExpanderBoxWidget * self,
  GtkOrientation      orientation)
{
  ExpanderBoxWidgetPrivate * prv =
    expander_box_widget_get_private (self);

  /* set the main orientation */
  prv->orientation = orientation;
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (self),
    orientation);

  /* set the orientation of the box inside the
   * expander button */
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (prv->btn_box),
    orientation == GTK_ORIENTATION_HORIZONTAL ?
    GTK_ORIENTATION_VERTICAL :
    GTK_ORIENTATION_HORIZONTAL);

  /* set the label angle */
  /* TODO */
#if 0
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_label_set_angle (
      prv->btn_label, 90.0);
  else
    gtk_label_set_angle (
      prv->btn_label, 0.0);
#endif

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gtk_widget_set_hexpand (
        GTK_WIDGET (prv->btn_label), 0);
      gtk_widget_set_vexpand (
        GTK_WIDGET (prv->btn_label), 1);
    }
  else
    {
      gtk_widget_set_hexpand (
        GTK_WIDGET (prv->btn_label), 1);
      gtk_widget_set_vexpand (
        GTK_WIDGET (prv->btn_label), 0);
    }
}

/**
 * Sets the icon name to show.
 */
void
expander_box_widget_set_icon_name (
  ExpanderBoxWidget * self,
  const char *        icon_name)
{
  GET_PRIVATE (self);

  gtk_image_set_from_icon_name (
    prv->btn_img, icon_name);
}

void
expander_box_widget_set_vexpand (
  ExpanderBoxWidget * self,
  bool                expand)
{
  GET_PRIVATE (self);

  gtk_widget_set_vexpand (
    GTK_WIDGET (prv->content), expand);
}

void
expander_box_widget_set_reveal_callback (
  ExpanderBoxWidget * self,
  ExpanderBoxRevealFunc   cb,
  void *                  user_data)
{
  GET_PRIVATE (self);

  prv->reveal_cb = cb;
  prv->user_data = user_data;
}

void
expander_box_widget_add_content (
  ExpanderBoxWidget * self,
  GtkWidget *         content)
{
  ExpanderBoxWidgetPrivate * prv =
    expander_box_widget_get_private (self);
  gtk_box_append (prv->content, content);
}

ExpanderBoxWidget *
expander_box_widget_new (
  const char * label,
  const char * icon_name,
  GtkOrientation orientation)
{
  ExpanderBoxWidget * self =
    g_object_new (EXPANDER_BOX_WIDGET_TYPE,
                  "visible", 1,
                  NULL);

  expander_box_widget_set_icon_name (
    self, icon_name);
  expander_box_widget_set_orientation (
    self, orientation);
  expander_box_widget_set_label (
    self, label);

  return self;
}

static void
expander_box_widget_class_init (
  ExpanderBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "expander_box.ui");
  gtk_widget_class_set_css_name (
    klass, "expander-box");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child_private ( \
    klass, ExpanderBoxWidget, x)

  BIND_CHILD (button);
  BIND_CHILD (revealer);
  BIND_CHILD (content);

#undef BIND_CHILD
}

static void
expander_box_widget_init (ExpanderBoxWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  GET_PRIVATE (self);

  prv->btn_label =
    GTK_LABEL (gtk_label_new ("Label"));
  gtk_widget_set_halign (
    GTK_WIDGET (prv->btn_label),
    GTK_ALIGN_START);
  prv->btn_img =
    GTK_IMAGE (
      gtk_image_new_from_icon_name ("plugins"));
  GtkWidget * box =
    gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append (
    GTK_BOX (box),
    GTK_WIDGET (prv->btn_label));
  gtk_box_append (
    GTK_BOX (box),
    GTK_WIDGET (
      gtk_separator_new (GTK_ORIENTATION_VERTICAL)));
  gtk_box_append (
    GTK_BOX (box),
    GTK_WIDGET (prv->btn_img));
  gtk_button_set_child (
    prv->button, GTK_WIDGET (box));
  prv->btn_box = GTK_BOX (box);

  g_signal_connect (
    G_OBJECT (prv->button), "clicked",
    G_CALLBACK (on_clicked), self);
}
