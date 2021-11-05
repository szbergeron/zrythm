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

#include "actions/port_connection_action.h"
#include "audio/port_connections_manager.h"
#include "gui/widgets/port_connections_tree.h"
#include "project.h"
#include "utils/error.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PortConnectionsTreeWidget,
  port_connections_tree_widget,
  GTK_TYPE_BOX)

enum
{
  COL_ENABLED,
  COL_SRC_PATH,
  COL_DEST_PATH,
  COL_MULTIPLIER,

  COL_SRC_PORT,
  COL_DEST_PORT,

  NUM_COLS
};

static void
on_enabled_toggled (
  GtkCellRendererToggle * cell,
  gchar                 * path_str,
  PortConnectionsTreeWidget *  self)
{
  GtkTreeModel * model = self->tree_model;
  GtkTreeIter  iter;
  GtkTreePath *path =
    gtk_tree_path_new_from_string (path_str);

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);

  /* get ports and toggled */
  gboolean enabled;
  Port * src_port = NULL, * dest_port = NULL;
  gtk_tree_model_get (
    model, &iter,
    COL_ENABLED, &enabled,
    COL_SRC_PORT, &src_port,
    COL_DEST_PORT, &dest_port,
    -1);

  /* get new value */
  enabled ^= 1;

  /* set new value on widget */
  gtk_list_store_set (
    GTK_LIST_STORE (model), &iter,
    COL_ENABLED, enabled, -1);

  /* clean up */
  gtk_tree_path_free (path);

  /* perform an undoable action */
  GError * err = NULL;
  bool ret =
    port_connection_action_perform_enable (
      &src_port->id, &dest_port->id, enabled, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err,
        _("Failed to enable connection from %s to "
        "%s"),
        src_port->id.label,
        dest_port->id.label);
    }
}

static GtkTreeModel *
create_model ()
{
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store =
    gtk_list_store_new (
      NUM_COLS,
      G_TYPE_BOOLEAN,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_POINTER,
      G_TYPE_POINTER);

  /* add data to the list store */
  for (int i = 0;
       i < PORT_CONNECTIONS_MGR->num_connections;
       i++)
    {
      PortConnection * conn =
        PORT_CONNECTIONS_MGR->connections[i];

      /* skip locked connections */
      if (conn->locked)
        continue;

      Port * src_port =
        port_find_from_identifier (conn->src_id);
      Port * dest_port =
        port_find_from_identifier (conn->dest_id);
      g_return_val_if_fail (
        IS_PORT_AND_NONNULL (src_port)
        && IS_PORT_AND_NONNULL (dest_port),
        NULL);

      char src_path[600];
      port_get_full_designation (
        src_port, src_path);
      char dest_path[600];
      port_get_full_designation (
        dest_port, dest_path);

      /* get multiplier */
      char mult_str[40];
      sprintf (
        mult_str, "%.4f", (double) conn->multiplier);

      gtk_list_store_append (
        store, &iter);
      gtk_list_store_set (
        store, &iter,
        COL_ENABLED, conn->enabled,
        COL_SRC_PATH, src_path,
        COL_DEST_PATH, dest_path,
        COL_MULTIPLIER, mult_str,
        COL_SRC_PORT, src_port,
        COL_DEST_PORT, dest_port,
        -1);
    }

  return GTK_TREE_MODEL (store);
}

static void
show_context_menu (
  PortConnectionsTreeWidget * self)
{
  GMenu * menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem =
    z_gtk_create_menu_item (
      _("Delete"), NULL,
      "app.port-connection-remove");
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    GTK_WIDGET (self), menu);
}

static void
on_right_click (
  GtkGestureClick * gesture,
  gint                   n_press,
  gdouble                x_dbl,
  gdouble                y_dbl,
  PortConnectionsTreeWidget * self)
{
  if (n_press != 1)
    return;

  g_message ("right click");

  GtkTreePath *path;
  GtkTreeViewColumn *column;

  int x, y;
  gtk_tree_view_convert_widget_to_bin_window_coords (
    self->tree, (int) x_dbl, (int) y_dbl, &x, &y);

  GtkTreeSelection * selection =
    gtk_tree_view_get_selection (self->tree);
  if (!gtk_tree_view_get_path_at_pos (
        GTK_TREE_VIEW (self->tree), x, y,
        &path, &column, NULL, NULL))
    {
      g_message ("no path at position %d %d", x, y);
      return;
    }

  gtk_tree_selection_unselect_all (selection);
  gtk_tree_selection_select_path (selection, path);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    GTK_TREE_MODEL (self->tree_model),
    &iter, path);
  GValue init_value = G_VALUE_INIT;
  GValue value = init_value;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->tree_model),
    &iter, COL_SRC_PORT, &value);
  self->src_port = g_value_get_pointer (&value);
  value = init_value;
  gtk_tree_model_get_value (
    GTK_TREE_MODEL (self->tree_model),
    &iter, COL_DEST_PORT, &value);
  self->dest_port = g_value_get_pointer (&value);

  gtk_tree_path_free (path);

  show_context_menu (self);
}

static void
tree_view_setup (
  PortConnectionsTreeWidget * self,
  GtkTreeView *         tree_view)
{
  /* init tree view */
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;

  /* column for checkbox */
  renderer = gtk_cell_renderer_toggle_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("On"), renderer,
      "active", COL_ENABLED,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);
  g_signal_connect (
    renderer, "toggled",
    G_CALLBACK (on_enabled_toggled), self);

  /* column for src path */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Source"), renderer,
      "text", COL_SRC_PATH,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);
  g_object_set (
    renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_column_set_resizable (
    GTK_TREE_VIEW_COLUMN (column), true);
  gtk_tree_view_column_set_min_width (
    GTK_TREE_VIEW_COLUMN (column), 120);
  gtk_tree_view_column_set_expand (
    GTK_TREE_VIEW_COLUMN (column), true);

  /* column for src path */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Destination"), renderer,
      "text", COL_DEST_PATH,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);
  g_object_set (
    renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_column_set_resizable (
    GTK_TREE_VIEW_COLUMN (column), true);
  gtk_tree_view_column_set_min_width (
    GTK_TREE_VIEW_COLUMN (column), 120);
  gtk_tree_view_column_set_expand (
    GTK_TREE_VIEW_COLUMN (column), true);

  /* column for multiplier */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Multiplier"), renderer,
      "text", COL_MULTIPLIER,
      NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);

  /* connect right click handler */
  GtkGestureClick * mp =
    GTK_GESTURE_CLICK (
      gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (mp), "pressed",
    G_CALLBACK (on_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (tree_view),
    GTK_EVENT_CONTROLLER (mp));
}

/**
 * Refreshes the tree model.
 */
void
port_connections_tree_widget_refresh (
  PortConnectionsTreeWidget * self)
{
  GtkTreeModel * model = self->tree_model;
  self->tree_model = create_model ();
  gtk_tree_view_set_model (
    self->tree, self->tree_model);

  if (model)
    {
      g_object_unref (model);
    }
}

PortConnectionsTreeWidget *
port_connections_tree_widget_new ()
{
  PortConnectionsTreeWidget * self =
    g_object_new (
      PORT_CONNECTIONS_TREE_WIDGET_TYPE, NULL);

  /* setup tree */
  tree_view_setup (self, self->tree);

  return self;
}

static void
port_connections_tree_widget_class_init (
  PortConnectionsTreeWidgetClass * _klass)
{
}

static void
port_connections_tree_widget_init (
  PortConnectionsTreeWidget * self)
{
  self->scroll =
    GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_box_append (
    GTK_BOX (self),
    GTK_WIDGET (self->scroll));

  self->tree =
    GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_scrolled_window_set_child (
    self->scroll, GTK_WIDGET (self->tree));
}
