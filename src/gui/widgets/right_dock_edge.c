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

#include "audio/control_room.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/monitor_section.h"
#include "gui/widgets/panel_file_browser.h"
#include "gui/widgets/plugin_browser.h"
#include "gui/widgets/right_dock_edge.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  RightDockEdgeWidget, right_dock_edge_widget,
  GTK_TYPE_BOX)

static void
on_notebook_switch_page (
  GtkNotebook *         notebook,
  GtkWidget *           page,
  guint                 page_num,
  RightDockEdgeWidget * self)
{
  g_debug (
    "setting right dock page to %u", page_num);

  g_settings_set_int (
    S_UI, "right-panel-tab", (int) page_num);
}

void
right_dock_edge_widget_setup (
  RightDockEdgeWidget * self)
{
  foldable_notebook_widget_setup (
    self->right_notebook,
    MW_CENTER_DOCK->center_right_paned,
    GTK_POS_RIGHT);

  monitor_section_widget_setup (
    self->monitor_section, CONTROL_ROOM);

  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (
      self->right_notebook);
  int page_num =
    g_settings_get_int (S_UI, "right-panel-tab");
  gtk_notebook_set_current_page (
    notebook, page_num);

  g_signal_connect (
    G_OBJECT (notebook), "switch-page",
    G_CALLBACK (on_notebook_switch_page), self);
}

static void
right_dock_edge_widget_init (
  RightDockEdgeWidget * self)
{
  g_type_ensure (MONITOR_SECTION_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  GtkWidget * img;
  GtkBox * box;
  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (
      self->right_notebook);

  /* add plugin browser */
  self->plugin_browser =
    plugin_browser_widget_new ();
  img =
    gtk_image_new_from_icon_name (
      "plugin-solid");
  gtk_widget_set_tooltip_text (
    img, _("Plugin Browser"));
  box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (
    GTK_BOX (box),
    GTK_WIDGET (self->plugin_browser));
  gtk_notebook_prepend_page (
    notebook, GTK_WIDGET (box), img);
  gtk_notebook_set_tab_detachable (
    notebook, GTK_WIDGET (box), true);
  gtk_notebook_set_tab_reorderable (
    notebook, GTK_WIDGET (box), true);
  self->plugin_browser_box = box;

  /* add file browser */
  self->file_browser =
    panel_file_browser_widget_new ();
#if 0
  GdkPixbuf * pixbuf =
    gtk_icon_theme_load_icon_for_scale (
      gtk_icon_theme_get_default (),
      /* the scale only accepts integers and we want
       * 24, but if we pass 24 and 1 a different
       * icon is loaded, so load the 12 icon and
       * scale it 2 times */
      "folder-music-line",
      12, 2,
      GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
#endif
  img =
    gtk_image_new_from_icon_name (
      "folder-music-line");
  gtk_widget_set_tooltip_text (
    img, _("File Browser"));
  box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (
    GTK_BOX (box),
    GTK_WIDGET (self->file_browser));
  gtk_widget_set_visible (
    GTK_WIDGET (box), 1);
  gtk_notebook_append_page (
    notebook, GTK_WIDGET (box), img);
  gtk_notebook_set_tab_detachable (
    notebook, GTK_WIDGET (box), true);
  gtk_notebook_set_tab_reorderable (
    notebook, GTK_WIDGET (box), true);
  self->file_browser_box = box;

  /* add control room */
  self->monitor_section =
    monitor_section_widget_new ();
#if 0
  pixbuf =
    gtk_icon_theme_load_icon_for_scale (
      gtk_icon_theme_get_default (),
      /* the scale only accepts integers and we want
       * 24, but if we pass 24 and 1 a different
       * icon is loaded, so load the 12 icon and
       * scale it 2 times */
      "speaker",
      12, 2,
      GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
#endif
  img =
    gtk_image_new_from_icon_name ("speaker");
  gtk_widget_set_tooltip_text (
    img, _("Monitor Section"));
  box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (
    GTK_BOX (box),
    GTK_WIDGET (self->monitor_section));
  gtk_widget_set_visible (
    GTK_WIDGET (box), 1);
  gtk_notebook_append_page (
    notebook, GTK_WIDGET (box), img);
  gtk_notebook_set_tab_detachable (
    notebook, GTK_WIDGET (box), true);
  gtk_notebook_set_tab_reorderable (
    notebook, GTK_WIDGET (box), true);
  self->monitor_section_box = box;

  /* add file browser button */
  GtkButton * tb =
    GTK_BUTTON (
      gtk_button_new_from_icon_name ("hdd"));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (tb),
    _("Show file browser"));
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (tb),
    "app.show-file-browser");
  gtk_notebook_set_action_widget (
    notebook, GTK_WIDGET (tb),
    GTK_PACK_END);

  gtk_notebook_set_current_page (
    notebook, 0);
  gtk_notebook_set_tab_pos (
    notebook, GTK_POS_RIGHT);
}

static void
right_dock_edge_widget_class_init (
  RightDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "right_dock_edge.ui");

  gtk_widget_class_set_css_name (
    klass, "right-dock-edge");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    RightDockEdgeWidget, \
    x)

  BIND_CHILD (right_notebook);
}
