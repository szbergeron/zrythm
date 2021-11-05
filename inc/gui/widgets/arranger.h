/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Arranger base widget.
 */

#ifndef __GUI_WIDGETS_ARRANGER_H__
#define __GUI_WIDGETS_ARRANGER_H__

#include "gui/widgets/main_window.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#define ARRANGER_WIDGET_TYPE ( \
  arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ArrangerWidget,
  arranger_widget,
  Z, ARRANGER_WIDGET,
  GtkWidget)

typedef struct _ArrangerBgWidget ArrangerBgWidget;
typedef struct MidiNote MidiNote;
typedef struct SnapGrid SnapGrid;
typedef struct AutomationPoint AutomationPoint;

typedef struct _GtkEventControllerMotion
  GtkEventControllerMotion;
typedef struct ArrangerObject ArrangerObject;
typedef struct ArrangerSelections ArrangerSelections;
typedef struct EditorSettings EditorSettings;
typedef struct ObjectPool ObjectPool;
typedef struct _RulerWidget RulerWidget;
typedef enum ArrangerObjectType ArrangerObjectType;
typedef enum TransportDisplay TransportDisplay;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define ARRANGER_WIDGET_GET_ACTION(arr,actn) \
  (arr->action == UI_OVERLAY_ACTION_##actn)

typedef enum ArrangerCursor
{
  /** Invalid cursor. */
  ARRANGER_CURSOR_NONE,
  ARRANGER_CURSOR_SELECT,
  ARRANGER_CURSOR_EDIT,
  ARRANGER_CURSOR_AUTOFILL,
  ARRANGER_CURSOR_CUT,
  ARRANGER_CURSOR_ERASER,
  ARRANGER_CURSOR_AUDITION,
  ARRANGER_CURSOR_RAMP,
  ARRANGER_CURSOR_GRAB,
  ARRANGER_CURSOR_GRABBING,
  ARRANGER_CURSOR_RESIZING_L,
  ARRANGER_CURSOR_RESIZING_L_FADE,
  ARRANGER_CURSOR_STRETCHING_L,
  ARRANGER_CURSOR_RESIZING_L_LOOP,
  ARRANGER_CURSOR_RESIZING_R,
  ARRANGER_CURSOR_RESIZING_R_FADE,
  ARRANGER_CURSOR_STRETCHING_R,
  ARRANGER_CURSOR_RESIZING_R_LOOP,
  ARRANGER_CURSOR_RESIZING_UP,
  ARRANGER_CURSOR_RESIZING_UP_FADE_IN,
  ARRANGER_CURSOR_RESIZING_UP_FADE_OUT,
  ARRANGER_CURSOR_GRABBING_COPY,
  ARRANGER_CURSOR_GRABBING_LINK,
  ARRANGER_CURSOR_RANGE,
  ARRANGER_CURSOR_FADE_IN,
  ARRANGER_CURSOR_FADE_OUT,
  ARRANGER_CURSOR_RENAME,
} ArrangerCursor;

/**
 * Type of arranger.
 */
typedef enum ArrangerWidgetType
{
  ARRANGER_WIDGET_TYPE_TIMELINE,
  ARRANGER_WIDGET_TYPE_MIDI,
  ARRANGER_WIDGET_TYPE_MIDI_MODIFIER,
  ARRANGER_WIDGET_TYPE_AUDIO,
  ARRANGER_WIDGET_TYPE_CHORD,
  ARRANGER_WIDGET_TYPE_AUTOMATION,
} ArrangerWidgetType;

#if 0
typedef enum ArrangerWidgetHoverType
{
  ARRANGER_WIDGET_HOVER_TYPE_NONE,
  ARRANGER_WIDGET_HOVER_TYPE_TRACK,
  ARRANGER_WIDGET_HOVER_TYPE_TRACK_LANE,
  ARRANGER_WIDGET_HOVER_TYPE_AUTOMATION_TRACK,
} ArrangerWidgetHoverType;
#endif

/**
 * The arranger widget is a canvas that draws all
 * the arranger objects it contains.
 */
typedef struct _ArrangerWidget
{
  GtkWidget  parent_instance;

  /** Type of arranger this is. */
  ArrangerWidgetType type;

  GtkGestureDrag * drag;
  GtkGestureClick * click;
  GtkGestureClick * right_click;
  GtkEventControllerMotion * motion_controller;

  /** Used when dragging. */
  double         last_offset_x;
  double         last_offset_y;

  UiOverlayAction action;

  /** X-axis coordinate at start of drag. */
  double         start_x;

  /** Y-axis coordinate at start of drag. */
  double         start_y;

  /** X-axis coordinate at the start of the drag,
   * in pixels. */
  double         start_pos_px;

  /**
   * Whether a drag update operation started.
   *
   * drag_update will be skipped unless this is
   * true or gtk_drag_check_threshold() returns
   * true.
   */
  bool           drag_update_started;

  /** Whether an object exists, so we can use the
   * earliest_obj_start_pos. */
  int            earliest_obj_exists;

  /** Start Position of the earliest object
   * at the start of the drag. */
  Position       earliest_obj_start_pos;

  /**
   * Fade in/out position at start.
   *
   * Used when moving fade in/out points.
   */
  Position       fade_pos_at_start;

  /**
   * The object that was clicked in this drag
   * cycle, if any.
   *
   * This is the ArrangerObject that was clicked,
   * even though there could be more selected.
   */
  ArrangerObject * start_object;

  /** Object currently hovered. */
  ArrangerObject * hovered_object;

  /** Whether the start object was selected before
   * drag_begin. */
  int            start_object_was_selected;

  /**
   * A clone of the ArrangerSelections on drag
   * begin.
   *
   * When autofilling velocities, this is used to
   * store the affected objects before editing.
   *
   * This must contain clones only.
   */
  ArrangerSelections * sel_at_start;

  /**
   * Region on drag begin, if editing automation.
   */
  ZRegion *            region_at_start;

  /** Selections to delete, used with the eraser
   * tool. */
  ArrangerSelections * sel_to_delete;

  /** Start Position of the earliest object
   * currently. */
  //Position             earliest_obj_pos;

  /** The absolute (not snapped) Position at the
   * start of a drag, translated from start_x. */
  Position       start_pos;

  /** The absolute (not snapped) current diff in
   * ticks from the curr_pos to the start_pos. */
  double         curr_ticks_diff_from_start;

  /** The adjusted diff in ticks to use for moving
   * objects starting from their cached start
   * positions. */
  double         adj_ticks_diff;

  /** adj_ticks_diff in last cycle. */
  double         last_adj_ticks_diff;

  /** The absolute (not snapped) Position as of the
   * current action. */
  Position       curr_pos;

  Position       end_pos; ///< for moving regions
  gboolean       key_is_pressed;

  /** Current hovering positions. */
  double         hover_x;
  double         hover_y;

  /** Number of clicks in current action. */
  int            n_press;

  /** Associated SnapGrid. */
  SnapGrid *     snap_grid;

  /** Whether shift button is held down. */
  int            shift_held;

  /** Whether Ctrl button is held down. */
  int            ctrl_held;

  /** Whether Alt is currently held down. */
  int            alt_held;

  gint64         last_frame_time;

  /* ----- TIMELINE ------ */

  /** The number of visible tracks moved during a
   * moving operation between tracks up to the last
   * cycle. */
  int            visible_track_diff;

  /** The number of lanes moved during a
   * moving operation between lanes, up to the last
   * cycle. */
  int            lane_diff;

  /** Whether this TimelineArrangerWidget is for
   * the PinnedTracklist or not. */
  int            is_pinned;

  /**
   * 1 if resizing range.
   */
  int            resizing_range;

  /**
   * 1 if this is the first call to resize the range,
   * so range1 can be set.
   */
  int            resizing_range_start;

  /** Cache for chord object height, used during
   * child size allocation. */
  //int                  chord_obj_height;

  /* ----- END TIMELINE ----- */

  /* ------ MIDI (PIANO ROLL) ---- */

  /** The note currently hovering over */
  int            hovered_note;

  /* ------ END MIDI (PIANO ROLL) ---- */

  /* ------ MIDI MODIFIER ---- */

  /** 1-127. */
  int            start_vel_val;

  /**
   * Maximum Velocity diff applied in this action.
   *
   * Used in drag_end to create an UndableAction.
   * This can have any value, even greater than 127
   * and it will be clamped when applying it to
   * a Velocity.
   */
  int            vel_diff;

  /* ------ END MIDI MODIFIER ---- */

  /* ------- CHORD ------- */

  /** Index of the chord being hovered on. */
  int            hovered_chord_index;

  /* ------- END CHORD ------- */

  /* --- AUDIO --- */

  /**
   * Float value at start.
   *
   * Used when changing the audio region gain.
   */
  float          fval_at_start;

  double         dval_at_start;

  /* --- END AUDIO --- */

  /** Px the playhead was last drawn at, so we can
   * redraw this and the new px only when the
   * playhead changes position. */
  int            last_playhead_px;

  /** Set to 1 to redraw. */
  bool           redraw;

  cairo_t *      cached_cr;

  cairo_surface_t * cached_surface;

  /** Rectangle in the last call. */
  graphene_rect_t last_rect;

  /**
   * Whether the current selections can link
   * (ie, only regions are selected).
   *
   * To be set on drag begin.
   */
  bool           can_link;

  /** Whether a rectangle is highlighted for DND. */
  bool           is_highlighted;

  /** The rectangle to highlight. */
  GdkRectangle   highlight_rect;
  //GdkRectangle   prev_highlight_rect;
  //

  /** Last selection rectangle, used to redraw the
   * union of the new selection and this. */
  GdkRectangle   last_selection_rect;

  /**
   * Drag start button (primary, secondary, etc.).
   *
   * Can be tested against GDK_BUTTON_SECONDARY and
   * GDK_BUTTON_PRIMARY.
   */
  guint          drag_start_btn;

  /**
   * Whether this is the first time the widget is
   * drawn.
   *
   * This is used for loading back the scroll
   * positions saved in the project.
   */
  bool           first_draw;

  /** Cached setting. */
  TransportDisplay ruler_display;

  /**
   * Layout for drawing velocity text.
   *
   * TODO move to Velocity if parallel
   * processing is needed - no need now.
   */
  PangoLayout *  vel_layout;

  /**
   * Layout for drawing automation point text.
   *
   * TODO move to AutomationPoint if parallel
   * processing is needed - no need now.
   */
  PangoLayout *  ap_layout;

  /**
   * Layout for drawing audio editor text.
   */
  PangoLayout *  audio_layout;

#if 0
  /**
   * Dummy cairo surface to create new surfaces from.
   */
  cairo_surface_t * dummy_surface;

  /**
   * Thread pool for drawing in the background
   * instead of in the UI thread.
   *
   * The result will be applied during draw in the
   * UI thread.
   */
  GThreadPool *  draw_thread_pool;

  /**
   * Object pool for ArrangerDrawTaskData.
   *
   * Must only be accessed from the GTK thread.
   */
  ObjectPool *   draw_task_obj_pool;
#endif

  /**
   * Cached playhead x to draw.
   *
   * This is used to avoid queuing drawing at x and
   * then drawing after it (if playhead moved). The
   * playhead will be drawn at the location it
   * was when the draw was queued.
   */
  //int            queued_playhead_px;

} ArrangerWidget;

const char *
arranger_widget_get_type_str (
  ArrangerWidgetType type);

/**
 * Creates a timeline widget using the given
 * timeline data.
 */
void
arranger_widget_setup (
  ArrangerWidget *   self,
  ArrangerWidgetType type,
  SnapGrid *         snap_grid);

/**
 * Sets the cursor on the arranger and all of its
 * children.
 */
void
arranger_widget_set_cursor (
  ArrangerWidget * self,
  ArrangerCursor   cursor);

/**
 * Wrapper of the UI functions based on the arranger
 * type.
 */
int
arranger_widget_pos_to_px (
  ArrangerWidget * self,
  Position * pos,
  int        use_padding);

/**
 * Gets the cursor based on the current hover
 * position.
 */
ArrangerCursor
arranger_widget_get_cursor (
  ArrangerWidget * self);

/**
 * Figures out which cursor should be used based
 * on the current state and then sets it.
 */
void
arranger_widget_refresh_cursor (
  ArrangerWidget * self);

/**
 * Gets the corresponding scrolled window.
 */
GtkScrolledWindow *
arranger_widget_get_scrolled_window (
  ArrangerWidget * self);

/**
 * Get all objects currently present in the arranger.
 *
 * @param objs Array to fill in.
 * @param size Array size to fill in.
 */
void
arranger_widget_get_all_objects (
  ArrangerWidget *  self,
  ArrangerObject ** objs,
  int *             size);

/**
 * Wrapper for ui_px_to_pos depending on the
 * arranger type.
 */
void
arranger_widget_px_to_pos (
  ArrangerWidget * self,
  double           px,
  Position *       pos,
  bool             has_padding);

/**
 * Returns the current visible rectangle.
 *
 * @param rect The rectangle to fill in.
 */
void
arranger_widget_get_visible_rect (
  ArrangerWidget * self,
  GdkRectangle *   rect);

/**
 * Fills in the given array with the ArrangerObject's
 * of the given type that appear in the given
 * ranger.
 *
 * @param type The type of arranger objects to find,
 *   or -1 to look for all objects.
 * @param x X, or -1 to not check x.
 * @param y Y, or -1 to not check y.
 * @param array The array to fill.
 * @param array_size The size of the array to fill.
 */
void
arranger_widget_get_hit_objects_at_point (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  double             x,
  double             y,
  ArrangerObject **  array,
  int *              array_size);

/**
 * Fills in the given array with the ArrangerObject's
 * of the given type that appear in the given
 * ranger.
 *
 * @param rect The rectangle to search in.
 * @param type The type of arranger objects to find,
 *   or -1 to look for all objects.
 * @param array The array to fill.
 * @param array_size The size of the array to fill.
 */
void
arranger_widget_get_hit_objects_in_rect (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  GdkRectangle *     rect,
  ArrangerObject **  array,
  int *              array_size);

/**
 * Returns the ArrangerObject of the given type
 * at (x,y).
 *
 * @param type The arranger object type, or -1 to
 *   search for all types.
 * @param x X, or -1 to not check x.
 * @param y Y, or -1 to not check y.
 */
ArrangerObject *
arranger_widget_get_hit_arranger_object (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  const double       x,
  const double       y);

void
arranger_widget_select_all (
  ArrangerWidget *  self,
  bool              select,
  bool              fire_events);

/**
 * Returns if the arranger is in a moving-related
 * operation or starting a moving-related operation.
 *
 * Useful to know if we need transient widgets or
 * not.
 */
bool
arranger_widget_is_in_moving_operation (
  ArrangerWidget * self);

/**
 * Returns the ArrangerSelections for this
 * ArrangerWidget.
 */
ArrangerSelections *
arranger_widget_get_selections (
  ArrangerWidget * self);

/**
 * Queues a redraw of the whole visible arranger.
 */
void
arranger_widget_redraw_whole (
  ArrangerWidget * self);

/**
 * Only redraws the playhead part.
 */
void
arranger_widget_redraw_playhead (
  ArrangerWidget * self);

/**
 * Only redraws the given rectangle.
 */
void
arranger_widget_redraw_rectangle (
  ArrangerWidget * self,
  GdkRectangle *   rect);

SnapGrid *
arranger_widget_get_snap_grid (
  ArrangerWidget * self);

/**
 * Called from MainWindowWidget because some
 * events don't reach here.
 */
gboolean
arranger_widget_on_key_press (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  ArrangerWidget *        self);

void
arranger_widget_on_key_release (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  ArrangerWidget *        self);

/**
 * Scroll until the given object is visible.
 *
 * @param horizontal 1 for horizontal, 2 for
 *   vertical.
 * @param up Whether scrolling up or down.
 * @param padding Padding pixels.
 */
NONNULL
void
arranger_widget_scroll_until_obj (
  ArrangerWidget * self,
  ArrangerObject * obj,
  int              horizontal,
  int              up,
  int              left,
  double           padding);

/**
 * Toggles the mute status of the selection, based
 * on the mute status of the selected object.
 *
 * This creates an undoable action and executes it.
 */
void
arranger_widget_toggle_selections_muted (
  ArrangerWidget * self,
  ArrangerObject * clicked_object);

/**
 * Returns the earliest possible position allowed
 * in this arranger (eg, 1.1.0.0 for timeline).
 */
void
arranger_widget_get_min_possible_position (
  ArrangerWidget * self,
  Position *       pos);

/**
 * Sets the highlight rectangle.
 *
 * @param rect The rectangle, or NULL to
 *   unset/unhighlight.
 */
void
arranger_widget_set_highlight_rect (
  ArrangerWidget * self,
  GdkRectangle *   rect);

/**
 * Returns the EditorSettings corresponding to
 * the given arranger.
 */
EditorSettings *
arranger_widget_get_editor_settings (
  ArrangerWidget * self);

bool
arranger_widget_is_playhead_visible (
  ArrangerWidget * self);

NONNULL
PURE
RulerWidget *
arranger_widget_get_ruler (
  ArrangerWidget * self);

/**
 * Returns whether any arranger is in the middle
 * of an action.
 */
bool
arranger_widget_any_doing_action (void);

/**
 * Returns the playhead's x coordinate in absolute
 * coordinates.
 */
int
arranger_widget_get_playhead_px (
  ArrangerWidget * self);

#define arranger_widget_print_action(self) \
  g_debug ( \
    "action: %s", ui_overlay_strings[self->action])

/**
 * Returns true if MIDI arranger and track mode
 * is enabled.
 */
bool
arranger_widget_get_drum_mode_enabled (
  ArrangerWidget * self);

/**
 * @}
 */

#endif
