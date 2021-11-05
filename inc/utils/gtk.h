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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * GTK utils.
 */

#include "zrythm-config.h"

#ifndef __UTILS_GTK_H__
#define __UTILS_GTK_H__

#include <stdbool.h>

#include <gtk/gtk.h>
#ifdef HAVE_X11
#include <gdk/x11/gdkx.h>
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop

/**
 * @addtogroup utils
 *
 * @{
 */

#define DEFAULT_CLIPBOARD \
  gdk_display_get_clipboard ( \
    gdk_display_get_default ())

#define CREATE_MIDI_LEARN_MENU_ITEM(action) \
  z_gtk_create_menu_item ( \
    _("MIDI learn"), "midi-logo", action)

#define CREATE_CUT_MENU_ITEM(action) \
  z_gtk_create_menu_item ( \
    _("Cu_t"), "edit-cut", action)

#define CREATE_COPY_MENU_ITEM(action) \
  z_gtk_create_menu_item ( \
    _("_Copy"), "edit-copy", action)

#define CREATE_PASTE_MENU_ITEM(action) \
  z_gtk_create_menu_item ( \
    _("_Paste"), "edit-paste", action)

#define CREATE_DELETE_MENU_ITEM(action) \
  z_gtk_create_menu_item ( \
    _("_Delete"), "edit-delete", action)

#define CREATE_CLEAR_SELECTION_MENU_ITEM(action) \
  z_gtk_create_menu_item ( \
    /* TRANSLATORS: deselects everything */ \
    _("Cle_ar Selection"), "edit-clear", \
    action)

#define CREATE_SELECT_ALL_MENU_ITEM(action) \
  z_gtk_create_menu_item ( \
    _("Select A_ll"), "edit-select-all", \
    action)

#define CREATE_DUPLICATE_MENU_ITEM(action) \
  z_gtk_create_menu_item ( \
    _("Duplicate"), "edit-duplicate", action)

#define CREATE_MUTE_MENU_ITEM(action) \
  z_gtk_create_menu_item ( \
    _("Mute"), "mute", action)

#define CREATE_UNMUTE_MENU_ITEM(action) \
  z_gtk_create_menu_item ( \
    _("Unmute"), NULL, action)

#define z_gtk_assistant_set_current_page_complete( \
  assistant, complete) \
  gtk_assistant_set_page_complete ( \
    GTK_ASSISTANT (assistant), \
    gtk_assistant_get_nth_page ( \
      GTK_ASSISTANT (assistant), \
      gtk_assistant_get_current_page ( \
        GTK_ASSISTANT (assistant))), \
    complete);

#define Z_GDK_RGBA_INIT(r,g,b,a) \
  (GdkRGBA) \
  { .red = r, .green = g, .blue = b, .alpha = a  }


typedef enum IconType IconType;

enum ZGtkResize
{
  Z_GTK_NO_RESIZE,
  Z_GTK_RESIZE
};

enum ZGtkShrink
{
  Z_GTK_NO_SHRINK,
  Z_GTK_SHRINK
};

static inline GtkWidget *
z_gtk_notebook_get_current_page_widget (
  GtkNotebook * notebook)
{
  return
    gtk_notebook_get_nth_page (
      notebook,
      gtk_notebook_get_current_page (notebook));
}

static inline GtkWidget *
z_gtk_notebook_get_current_tab_label_widget (
  GtkNotebook * notebook)
{
  return
    gtk_notebook_get_tab_label (
      notebook,
      z_gtk_notebook_get_current_page_widget (
        notebook));
}

GdkMonitor *
z_gtk_get_primary_monitor (void);

int
z_gtk_get_primary_monitor_scale_factor (void);

int
z_gtk_get_primary_monitor_refresh_rate (void);

bool
z_gtk_is_wayland (void);

void
z_gtk_tree_view_remove_all_columns (
  GtkTreeView * treeview);

/**
 * @note Bumps reference, must be decremented after
 * calling.
 */
void
z_gtk_widget_remove_all_children (
  GtkWidget * widget);

void
z_gtk_widget_destroy_all_children (
  GtkWidget * widget);

void
z_gtk_widget_remove_children_of_type (
  GtkWidget * widget,
  GType       type);

void
z_gtk_overlay_add_if_not_exists (
  GtkOverlay * overlay,
  GtkWidget *  widget);

/**
 * Returns the primary or secondary label of the
 * given GtkMessageDialog.
 *
 * @param secondary 0 for primary, 1 for secondary.
 */
GtkLabel *
z_gtk_message_dialog_get_label (
  GtkMessageDialog * self,
  const int          secondary);

/**
 * Configures a simple value-text combo box using
 * the given model.
 */
void
z_gtk_configure_simple_combo_box (
  GtkComboBox * cb,
  GtkTreeModel * model);

/**
 * Sets the icon name and optionally text.
 */
void
z_gtk_button_set_icon_name_and_text (
  GtkButton *  btn,
  const char * name,
  const char * text,
  bool         icon_first,
  GtkOrientation orientation,
  int          spacing);

/**
 * Creates a toggle button with the given icon name.
 */
GtkToggleButton *
z_gtk_toggle_button_new_with_icon (
  const char * name);

/**
 * Creates a toggle button with the given icon name.
 */
GtkToggleButton *
z_gtk_toggle_button_new_with_icon_and_text (
  const char * name,
  const char * text,
  bool         icon_first,
  GtkOrientation orientation,
  int          spacing);

/**
 * Creates a button with the given icon name and
 * text.
 */
GtkButton *
z_gtk_button_new_with_icon_and_text (
  const char * name,
  const char * text,
  bool         icon_first,
  GtkOrientation orientation,
  int          spacing);

/**
 * Creates a button with the given resource name as icon.
 */
GtkButton *
z_gtk_button_new_with_resource (
  IconType  icon_type,
  const char * name);

/**
 * Creates a toggle button with the given resource name as
 * icon.
 */
GtkToggleButton *
z_gtk_toggle_button_new_with_resource (
  IconType  icon_type,
  const char * name);

#define z_gtk_create_menu_item( \
  lbl_name,icn_name,action_name) \
  z_gtk_create_menu_item_full ( \
    lbl_name, icn_name, action_name)

/**
 * Creates a menu item.
 */
GMenuItem *
z_gtk_create_menu_item_full (
  const gchar *   label_name,
  const gchar *   icon_name,
  const char *    detailed_action);

/**
 * Returns a pointer stored at the given selection.
 */
void *
z_gtk_get_single_selection_pointer (
  GtkTreeView * tv,
  int           column);

#if 0
/**
 * Returns the label from a given GtkMenuItem.
 *
 * The menu item must have a box with an optional
 * icon and a label inside.
 */
GtkLabel *
z_gtk_get_label_from_menu_item (
  GtkMenuItem * mi);
#endif

/**
 * Gets the tooltip for the given action on the
 * given widget.
 *
 * If the action is valid, an orange text showing
 * the accelerator will be added to the tooltip.
 *
 * @return A new string that must be free'd with
 *   g_free().
 */
char *
z_gtk_get_tooltip_for_action (
  const char * detailed_action,
  const char * tooltip);

/**
 * Sets the tooltip for the given action on the
 * given widget.
 *
 * If the action is valid, an orange text showing
 * the accelerator will be added to the tooltip.
 */
void
z_gtk_widget_set_tooltip_for_action (
  GtkWidget *  widget,
  const char * detailed_action,
  const char * tooltip);

/**
 * Sets the tooltip and finds the accel keys and
 * appends them to the tooltip in small text.
 */
void
z_gtk_set_tooltip_for_actionable (
  GtkActionable * actionable,
  const char *    tooltip);

#if 0
/**
 * Changes the size of the icon inside tool buttons.
 */
void
z_gtk_tool_button_set_icon_size (
  GtkToolButton * toolbutton,
  GtkIconSize     icon_size);
#endif

/**
 * Removes the given style class from the widget.
 */
void
z_gtk_widget_remove_style_class (
  GtkWidget   *widget,
  const gchar *class_name);

/**
 * Gets the GdkDevice for a GtkWidget.
 */
static inline GdkDevice *
z_gtk_widget_get_device (
  GtkWidget * widget)
{
  return (gdk_seat_get_pointer (
    gdk_display_get_default_seat (
      gtk_widget_get_display (widget))));
}

#if 0
static inline GdkWindow *
z_gtk_widget_get_root_gdk_window (
  GtkWidget * widget)
{
  GdkScreen * screen =
    z_gtk_widget_get_screen (widget);
  return
    gdk_screen_get_root_window (screen);
}
#endif

#if 0
static inline void
z_gtk_widget_get_global_coordinates (
  GtkWidget * widget,
  int *       x,
  int *       y)
{
  GdkDevice * dev =
    z_gtk_widget_get_device (widget);
  GdkWindow * win =
    z_gtk_widget_get_root_gdk_window (widget);
  gdk_window_get_device_position (
    win, dev, x, y, NULL);
}

static inline void
z_gtk_widget_get_global_coordinates_double (
  GtkWidget * widget,
  double *    x,
  double *    y)
{
  GdkDevice * dev =
    z_gtk_widget_get_device (widget);
  GdkWindow * win =
    z_gtk_widget_get_root_gdk_window (widget);
  gdk_window_get_device_position_double (
    win, dev, x, y, NULL);
}

/**
 * Wraps the cursor to the given global coordinates.
 */
static inline void
z_gtk_warp_cursor_to (
  GtkWidget * widget, int x, int y)
{
  GdkDevice * dev =
    z_gtk_widget_get_device (widget);
  GdkScreen * screen =
    z_gtk_widget_get_screen (widget);
  gdk_device_warp (dev, screen, x, y);
}
#endif

static inline GdkSurface *
z_gtk_widget_get_surface (
  GtkWidget * widget)
{
  GtkNative * native =
    gtk_widget_get_native (widget);
  return gtk_native_get_surface (native);
}

/**
 * Sets the GdkModifierType given for the widget.
 *
 * Used in eg. drag_motion events to check if
 * Ctrl is held.
 */
static inline void
z_gtk_widget_get_mask (
  GtkWidget * widget,
  GdkModifierType * mask)
{
  gdk_surface_get_device_position (
    z_gtk_widget_get_surface (widget),
    z_gtk_widget_get_device (widget),
    NULL, NULL, mask);
}

/**
 * Returns if the keyval is an Alt key.
 */
static inline int
z_gtk_keyval_is_alt (
  const guint keyval)
{
  return
    keyval == GDK_KEY_Alt_L ||
    keyval == GDK_KEY_Alt_R ||
    keyval == GDK_KEY_Meta_L ||
    keyval == GDK_KEY_Meta_R;
}

/**
 * Returns if the keyval is a Control key.
 */
static inline int
z_gtk_keyval_is_ctrl (
  const guint keyval)
{
  return
    keyval == GDK_KEY_Control_L ||
    keyval == GDK_KEY_Control_R;
}

/**
 * Returns if the keyval is an arrow key.
 */
static inline int
z_gtk_keyval_is_arrow (
  const guint keyval)
{
  return
    keyval == GDK_KEY_Left ||
    keyval == GDK_KEY_Right ||
    keyval == GDK_KEY_Down ||
    keyval == GDK_KEY_Up;
}

/**
 * Returns if the keyval is a Shift key.
 */
static inline int
z_gtk_keyval_is_shift (
  const guint keyval)
{
  return
    keyval == GDK_KEY_Shift_L ||
    keyval == GDK_KEY_Shift_R;
}

/**
 * Returns the nth child of a container.
 */
GtkWidget *
z_gtk_widget_get_nth_child (
  GtkWidget * widget,
  int         index);

/**
 * Sets the ellipsize mode of each text cell
 * renderer in the combo box.
 */
void
z_gtk_combo_box_set_ellipsize_mode (
  GtkComboBox * self,
  PangoEllipsizeMode ellipsize);

/**
 * Sets the given emblem to the button, or unsets
 * the emblem if \ref emblem_icon is NULL.
 */
void
z_gtk_button_set_emblem (
  GtkButton *  btn,
  const char * emblem_icon);

/**
 * Makes the given notebook foldable.
 *
 * The pages of the notebook must all be wrapped
 * in GtkBox's.
 */
void
z_gtk_setup_foldable_notebook (
  GtkNotebook * notebook);

/**
 * Sets the margin on all 4 sides on the widget.
 */
void
z_gtk_widget_set_margin (
  GtkWidget * widget,
  int         margin);

GtkFlowBoxChild *
z_gtk_flow_box_get_selected_child (
  GtkFlowBox * self);

/**
 * Callback to use for simple directory links.
 */
bool
z_gtk_activate_dir_link_func (
  GtkLabel * label,
  char *     uri,
  void *     data);

GtkSourceLanguageManager *
z_gtk_source_language_manager_get (void);

/**
 * Makes the given GtkNotebook detachable to
 * a new window.
 */
void
z_gtk_notebook_make_detachable (
  GtkNotebook * notebook,
  GtkWindow *   parent_window);

/**
 * Wraps the message area in a scrolled window.
 */
void
z_gtk_message_dialog_wrap_message_area_in_scroll (
  GtkMessageDialog * dialog,
  int                min_width,
  int                min_height);

/**
 * Returns the full text contained in the text
 * buffer.
 *
 * Must be free'd using g_free().
 */
char *
z_gtk_text_buffer_get_full_text (
  GtkTextBuffer * buffer);

/**
 * Generates a screenshot image for the given
 * widget.
 *
 * See gdk_pixbuf_savev() for the parameters.
 *
 * @param accept_fallback Whether to accept a
 *   fallback "no image" pixbuf.
 * @param[out] ret_dir Placeholder for directory to
 *   be deleted after using the screenshot.
 * @param[out] ret_path Placeholder for absolute
 *   path to the screenshot.
 */
void
z_gtk_generate_screenshot_image (
  GtkWidget *  widget,
  GtkWidget *  child,
  const char * type,
  char **      option_keys,
  char **      option_values,
  char **      ret_dir,
  char **      ret_path,
  bool         accept_fallback);

/**
 * Sets the action target of the given GtkActionable
 * to be binded to the given setting.
 *
 * Mainly used for binding GSettings keys to toggle
 * buttons.
 */
void
z_gtk_actionable_set_action_from_setting (
  GtkActionable * actionable,
  GSettings *     settings,
  const char *    key);

/**
 * Returns column number or -1 if not found or on
 * error.
 */
int
z_gtk_tree_view_column_get_column_id (
  GtkTreeViewColumn * col);

bool
z_gtk_is_event_button (
  GdkEvent * ev);

/**
 * Gets the visible rectangle from the scrolled
 * window's adjustments.
 */
void
z_gtk_scrolled_window_get_visible_rect (
  GtkScrolledWindow * scroll,
  graphene_rect_t *   rect);

void
z_gtk_graphene_rect_t_to_gdk_rectangle (
  GdkRectangle *    rect,
  graphene_rect_t * grect);

/**
 * Mimics the blocking behavior.
 *
 * @return The response ID.
 */
int
z_gtk_dialog_run (
  GtkDialog * dialog,
  bool        destroy_on_close);

void
z_gtk_show_context_menu_from_g_menu (
  GtkWidget * widget,
  GMenu *     menu);

/**
 * Returns the bitmask of the selected action
 * during a drop (eg, GDK_ACTION_COPY).
 */
GdkDragAction
z_gtk_drop_target_get_selected_action (
  GtkDropTarget * drop_target);

GtkIconTheme *
z_gtk_icon_theme_get_default (void);

char *
z_gtk_file_chooser_get_filename (
  GtkFileChooser * file_chooser);

void
z_gtk_file_chooser_set_file_from_path (
  GtkFileChooser * file_chooser,
  const char *     filename);

/**
 * Returns the text on the clipboard, or NULL if
 * there is nothing or the content is not text.
 */
char *
z_gdk_clipboard_get_text (
  GdkClipboard * clipboard);

#ifdef HAVE_X11
Window
z_gtk_window_get_x11_xid (
  GtkWindow * window);
#endif

GdkTexture *
z_gdk_texture_new_from_icon_name (
  const char * icon_name,
  int          size,
  int          scale);

void
z_gtk_print_graphene_rect (
  graphene_rect_t * rect);

/**
 * @}
 */
#endif
