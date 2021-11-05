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

/**
 * \file
 */

#ifndef __GUI_WIDGETS_PIANO_ROLL_TOOLBAR_H__
#define __GUI_WIDGETS_PIANO_ROLL_TOOLBAR_H__

#include <gtk/gtk.h>

#define EDITOR_TOOLBAR_WIDGET_TYPE \
  (editor_toolbar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  EditorToolbarWidget,
  editor_toolbar_widget,
  Z, EDITOR_TOOLBAR_WIDGET,
  GtkBox)

#define MW_EDITOR_TOOLBAR \
  MW_CLIP_EDITOR->editor_toolbar

typedef struct _ToolboxWidget ToolboxWidget;
typedef struct _QuantizeMbWidget QuantizeMbWidget;
typedef struct _QuantizeBoxWidget QuantizeBoxWidget;
typedef struct _SnapBoxWidget SnapBoxWidget;
typedef struct _ButtonWithMenuWidget
  ButtonWithMenuWidget;
typedef struct _PlayheadScrollButtonsWidget
  PlayheadScrollButtonsWidget;

/**
 * The PianoRoll toolbar in the top.
 */
typedef struct _EditorToolbarWidget
{
  GtkBox              parent_instance;
  GtkComboBoxText *   chord_highlighting;
  SnapBoxWidget *     snap_box;
  QuantizeBoxWidget * quantize_box;
  GtkButton *     event_viewer_toggle;
  ButtonWithMenuWidget * functions_btn;
  GtkButton *         apply_function_btn;

  GtkSeparator *      sep_after_chord_highlight;
  GtkBox *       chord_highlight_box;

  PlayheadScrollButtonsWidget * playhead_scroll;

  GMenuModel *        midi_functions_menu;
  GMenuModel *        automation_functions_menu;
  GMenuModel *        audio_functions_menu;
} EditorToolbarWidget;

/**
 * Refreshes relevant widgets.
 */
void
editor_toolbar_widget_refresh (
  EditorToolbarWidget * self);

void
editor_toolbar_widget_setup (
  EditorToolbarWidget * self);

#endif
