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

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "audio/port.h"
#include "audio/port_connection.h"
#include "gui/widgets/bar_slider.h"
#include "utils/cairo.h"
#include "utils/objects.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  BarSliderWidget,
  bar_slider_widget,
  GTK_TYPE_DRAWING_AREA)

/**
 * Get the real value.
 */
static float
get_real_val (
  BarSliderWidget * self,
  bool              snapped)
{
  if (self->type == BAR_SLIDER_TYPE_PORT_MULTIPLIER)
    {
      PortConnection * conn =
        (PortConnection *) self->object;
      return conn->multiplier;
    }
  else
    {
      if (snapped && self->snapped_getter)
        {
          return
            self->snapped_getter (self->object);
        }
      else
        {
          return self->getter (self->object);
        }
    }
}

/**
 * Macro to get real value from bar_slider value.
 */
#define REAL_VAL_FROM_BAR_SLIDER(bar_slider) \
  (self->min + (float) bar_slider * \
   (self->max - self->min))

/**
 * Converts from real value to bar_slider value
 */
#define BAR_SLIDER_VAL_FROM_REAL(real) \
  (((float) real - self->min) / \
   (self->max - self->min))

/**
 * Sets real val
 */
static void
set_real_val (
  BarSliderWidget * self,
  float             real_val)
{
  if (self->type == BAR_SLIDER_TYPE_PORT_MULTIPLIER)
    {
      PortConnection * connection =
        (PortConnection *) self->object;
      connection->multiplier = real_val;
    }
  else
    {
      (*self->setter)(self->object, real_val);
    }
}

/**
 * Draws the bar_slider.
 */
static void
bar_slider_draw_cb (
  GtkDrawingArea * drawing_area,
  cairo_t *        cr,
  int              width,
  int              height,
  gpointer         user_data)
{
  BarSliderWidget * self =
    Z_BAR_SLIDER_WIDGET (user_data);
  GtkWidget * widget = GTK_WIDGET (drawing_area);

  GtkStyleContext * context =
    gtk_widget_get_style_context (
      GTK_WIDGET (drawing_area));

  gtk_render_background (
    context, cr, 0, 0, width, height);

  const float real_min = self->min;
  const float real_max = self->max;
  const float real_zero = self->zero;
  const float real_val = get_real_val (self, true);

  /* get absolute values in pixels */
  /*const float min_px = 0.f;*/
  /*const float max_px = width;*/
  const float zero_px =
    ((real_zero - real_min) /
     (real_max - real_min)) *
    (float) width;
  const float val_px =
    ((real_val - real_min) /
     (real_max - real_min)) *
    (float) width;

  cairo_set_source_rgba (cr, 1, 1, 1, 0.3);

  /* draw from val to zero */
  if (real_val < real_zero)
    {
      cairo_rectangle (
        cr, (double) val_px, 0,
        (double) (zero_px - val_px), height);
    }
  /* draw from zero to val */
  else
    {
      cairo_rectangle (
        cr, (double) zero_px, 0,
        (double) (val_px - zero_px), height);
    }
  cairo_fill (cr);

  char str[3000];
  if (!self->show_value)
    {
      sprintf (
        str, "%s%s",
        self->prefix, self->suffix);
    }
  else if (self->decimals == 0)
    {
      sprintf (
        str, "%s%d%s",
        self->prefix,
        (int) (self->convert_to_percentage ?
          real_val * 100 : real_val),
        self->suffix);
    }
  else if (self->decimals < 5)
    {
      sprintf (
        str,
        "%s%.*f%s",
        self->prefix,
        self->decimals,
        (double)
        (self->convert_to_percentage ?
         real_val * 100.f : real_val),
        self->suffix);
    }
  else
    {
      g_warn_if_reached ();
    }
  int we;
  cairo_set_source_rgba (cr, 1, 1, 1, 1);
  z_cairo_get_text_extents_for_widget (
    widget, self->layout, str, &we, NULL);
  if (width < we)
    {
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        we + Z_CAIRO_TEXT_PADDING * 2,
        height);
    }
  z_cairo_draw_text_full (
    cr, widget, self->layout, str,
    width / 2 - we / 2,
    Z_CAIRO_TEXT_PADDING);

  if (self->hover)
    {
      cairo_set_source_rgba (cr, 1,1,1, 0.12 );
      cairo_rectangle (cr, 0, 0, width, height);
      cairo_fill (cr);
    }
}

static void
on_motion_enter (
  GtkEventControllerMotion * controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  BarSliderWidget * self =
    Z_BAR_SLIDER_WIDGET (user_data);
  self->hover = true;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_motion_leave (
  GtkEventControllerMotion * controller,
  gpointer                   user_data)
{
  BarSliderWidget * self =
    Z_BAR_SLIDER_WIDGET (user_data);
  self->hover = false;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
drag_begin (
  GtkGestureDrag * gesture,
  double           offset_x,
  double           offset_y,
  BarSliderWidget * self)
{
  if (!self->editable)
    return;

  self->start_x = offset_x;

  if (self->init_setter)
    {
      int width =
        gtk_widget_get_allocated_width (
          GTK_WIDGET (self));
      double normalized_val =
        ui_get_normalized_draggable_value (
          width,
          BAR_SLIDER_VAL_FROM_REAL (
            get_real_val (self, false)),
          self->start_x, self->start_x,
          self->start_x, 1.0, self->mode);
      self->init_setter (
        self->object,
        REAL_VAL_FROM_BAR_SLIDER (normalized_val));
    }
}

static void
drag_update (
  GtkGestureDrag *gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  BarSliderWidget * self)
{
  if (!self->editable)
    return;

  int width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (self));
  double new_normalized_val =
    ui_get_normalized_draggable_value (
      width,
      BAR_SLIDER_VAL_FROM_REAL (
        get_real_val (self, false)),
      self->start_x, self->start_x + offset_x,
      self->start_x + self->last_x,
      1.0, self->mode);
  set_real_val (
    self,
    REAL_VAL_FROM_BAR_SLIDER (new_normalized_val));
  self->last_x = offset_x;
  gtk_widget_queue_draw ((GtkWidget *)self);
}

static void
drag_end (
  GtkGestureDrag *gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  BarSliderWidget * self)
{
  if (!self->editable)
    return;

  if (self->end_setter)
    {
      int width =
        gtk_widget_get_allocated_width (
          GTK_WIDGET (self));
      double normalized_val =
        ui_get_normalized_draggable_value (
          width,
          BAR_SLIDER_VAL_FROM_REAL (
            get_real_val (self, false)),
          self->start_x, self->start_x + offset_x,
          self->start_x + self->last_x,
          1.0, self->mode);
      self->end_setter (
        self->object,
        REAL_VAL_FROM_BAR_SLIDER (normalized_val));
    }

  self->last_x = 0;
  self->start_x = 0;
}

static void
recreate_pango_layouts (
  BarSliderWidget * self)
{
  if (PANGO_IS_LAYOUT (self->layout))
    g_object_unref (self->layout);

  self->layout =
    z_cairo_create_pango_layout_from_string (
      (GtkWidget *) self, Z_CAIRO_FONT,
      PANGO_ELLIPSIZE_NONE, -1); }

static void
bar_slider_widget_on_size_allocate (
  GtkWidget *          widget,
  GdkRectangle *       allocation,
  BarSliderWidget * self)
{
  recreate_pango_layouts (self);
}

#if 0
static void
on_screen_changed (
  GtkWidget *          widget,
  GdkScreen *          previous_screen,
  BarSliderWidget * self)
{
  recreate_pango_layouts (self);
}
#endif

/**
 * Creates a bar slider widget for floats.
 */
BarSliderWidget *
_bar_slider_widget_new (
  BarSliderType type,
  float (*get_val)(void *),
  void (*set_val)(void *, float),
  void * object,
  float    min,
  float    max,
  int    w,
  int    h,
  float    zero,
  int    convert_to_percentage,
  int       decimals,
  UiDragMode mode,
  const char * prefix,
  const char * suffix)
{
  g_warn_if_fail (object);

  BarSliderWidget * self =
    g_object_new (BAR_SLIDER_WIDGET_TYPE, NULL);
  self->type = type;
  self->getter = get_val;
  self->setter = set_val;
  self->object = object;
  self->min = min;
  self->max = max;
  self->hover = 0;
  self->zero = zero; /* default 0.05f */
  self->last_x = 0;
  self->start_x = 0;
  strcpy (self->suffix, suffix ? suffix : "");
  strcpy (self->prefix, prefix ? prefix : "");
  self->decimals = decimals;
  self->mode = mode;
  self->convert_to_percentage =
    convert_to_percentage;

  /* set size */
  gtk_widget_set_size_request (
    GTK_WIDGET (self), w, h);
  gtk_widget_set_hexpand (
    GTK_WIDGET (self), 1);
  gtk_widget_set_vexpand (
    GTK_WIDGET (self), 1);

  gtk_drawing_area_set_draw_func (
    GTK_DRAWING_AREA (self), bar_slider_draw_cb,
    self, NULL);

  GtkEventController * motion_controller =
    gtk_event_controller_motion_new ();
  gtk_widget_add_controller (
    GTK_WIDGET (self), motion_controller);
  g_signal_connect (
    G_OBJECT (motion_controller), "enter",
    G_CALLBACK (on_motion_enter), self);
  g_signal_connect (
    G_OBJECT(self), "leave",
    G_CALLBACK (on_motion_leave), self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-begin",
    G_CALLBACK (drag_begin), self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update), self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end), self);
  return self;
}

static void
finalize (
  BarSliderWidget * self)
{
  g_return_if_fail (Z_IS_BAR_SLIDER_WIDGET (self));

  object_free_w_func_and_null (
    g_object_unref, self->layout);

  G_OBJECT_CLASS (
    bar_slider_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
bar_slider_widget_init (
  BarSliderWidget * self)
{
  self->show_value = 1;
  self->editable = 1;

  /* make it able to notify */
  self->drag =
    GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->drag));

  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);

#if 0
  g_signal_connect (
    G_OBJECT (self), "screen-changed",
    G_CALLBACK (on_screen_changed),  self);
#endif
  g_signal_connect (
    G_OBJECT (self), "size-allocate",
    G_CALLBACK (
      bar_slider_widget_on_size_allocate),
    self);
}

static void
bar_slider_widget_class_init (
  BarSliderWidgetClass * _klass)
{
  GObjectClass * klass =
    G_OBJECT_CLASS (_klass);

  klass->finalize = (GObjectFinalizeFunc) finalize;
}
