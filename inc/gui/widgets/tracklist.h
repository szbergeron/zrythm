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

#ifndef __GUI_WIDGETS_TRACKLIST_H__
#define __GUI_WIDGETS_TRACKLIST_H__

#include <gtk/gtk.h>

#define USE_WIDE_HANDLE 1

#define TRACKLIST_WIDGET_TYPE \
  (tracklist_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TracklistWidget, tracklist_widget,
  Z, TRACKLIST_WIDGET, GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TRACKLIST MW_TIMELINE_PANEL->tracklist

typedef struct _TrackWidget TrackWidget;
typedef struct _DragDestBoxWidget DragDestBoxWidget;
typedef struct _ChordTrackWidget ChordTrackWidget;
typedef struct Track InstrumentTrack;
typedef struct Tracklist Tracklist;

/**
 * The TracklistWidget holds all the Track's
 * in the Project.
 *
 * It is a box with all the pinned tracks, plus a
 * scrolled window at the end containing unpinned
 * tracks and the DragDestBoxWidget at the end.
 */
typedef struct _TracklistWidget
{
  GtkBox              parent_instance;

  /** The scrolled window for unpinned tracks. */
  GtkScrolledWindow * unpinned_scroll;

  /** Box to hold pinned tracks. */
  GtkBox *            pinned_box;

  /** Box inside unpinned scroll. */
  GtkBox *            unpinned_box;

  /**
   * Widget for drag and dropping plugins in to
   * create new tracks.
   */
  DragDestBoxWidget *   ddbox;

  /**
   * Internal tracklist.
   */
  Tracklist *           tracklist;

  /** Size group to set the pinned track box and
   * the pinned timeline to the same height. */
  GtkSizeGroup *       pinned_size_group;
  GtkSizeGroup *       unpinned_size_group;

  /** Cache. */
  GdkRectangle         last_allocation;

  bool                 setup;
} TracklistWidget;

/**
 * Sets up the TracklistWidget.
 */
void
tracklist_widget_setup (
  TracklistWidget * self,
  Tracklist * tracklist);

/**
 * Prepare for finalization.
 */
void
tracklist_widget_tear_down (
  TracklistWidget * self);

/**
 * Makes sure all the tracks for channels marked as
 * visible are visible.
 */
void
tracklist_widget_update_track_visibility (
  TracklistWidget *self);


/**
 * Gets hit TrackWidget and the given coordinates.
 */
TrackWidget *
tracklist_widget_get_hit_track (
  TracklistWidget *  self,
  double            x,
  double            y);

/**
 * Handle ctrl+shift+scroll.
 */
void
tracklist_widget_handle_vertical_zoom_scroll (
  TracklistWidget *          self,
  GtkEventControllerScroll * scroll_controller);

/**
 * Deletes all tracks and re-adds them.
 */
void
tracklist_widget_hard_refresh (
  TracklistWidget * self);

/**
 * @}
 */

#endif
