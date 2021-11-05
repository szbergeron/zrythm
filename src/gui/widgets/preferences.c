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

#include "zrythm-config.h"

#include <locale.h>

#include "audio/engine.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/active_hardware_mb.h"
#include "gui/widgets/file_chooser_button.h"
#include "gui/widgets/preferences.h"
#include "plugins/plugin_gtk.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/localization.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PreferencesWidget,
  preferences_widget,
  GTK_TYPE_DIALOG)

typedef struct CallbackData
{
  PreferencesWidget * preferences_widget;
  SubgroupInfo *      info;
  char *              key;
} CallbackData;

#define KEY_IS(a,b,c) \
  (string_is_equal (group, _(a)) && \
  string_is_equal (subgroup, _(b)) && \
  string_is_equal (key, c))

static void
on_simple_string_entry_changed (
  GtkEditable *  editable,
  CallbackData * data)
{
  char * str =
    gtk_editable_get_chars (editable, 0, -1);
  g_return_if_fail (str);
  g_settings_set_string (
    data->info->settings, data->key, str);
  g_free (str);
}

static void
on_path_entry_changed (
  GtkEditable *  editable,
  CallbackData * data)
{
  char * str =
    gtk_editable_get_chars (editable, 0, -1);
  g_return_if_fail (str);
  char ** split_str =
    g_strsplit (str, G_SEARCHPATH_SEPARATOR_S, 0);
  g_settings_set_strv (
    data->info->settings, data->key,
    (const char * const *) split_str);
  g_strfreev (split_str);
  g_free (str);
}

static void
on_enum_combo_box_active_changed (
  GtkComboBox *  combo,
  CallbackData * data)
{
  g_settings_set_enum (
    data->info->settings, data->key,
    gtk_combo_box_get_active (combo));
}

static void
on_string_combo_box_active_changed (
  GtkComboBoxText *  combo,
  CallbackData * data)
{
  g_settings_set_string (
    data->info->settings, data->key,
    gtk_combo_box_text_get_active_text (combo));
}

static void
on_backends_combo_box_active_changed (
  GtkComboBox *  combo,
  CallbackData * data)
{
  g_settings_set_enum (
    data->info->settings, data->key,
    atoi (gtk_combo_box_get_active_id (combo)));
}

static void
on_file_set (
  GtkNativeDialog* dialog,
  gint response_id,
  gpointer user_data)
{
  CallbackData * data = (CallbackData *) user_data;
  GtkFileChooser * fc = GTK_FILE_CHOOSER (dialog);

  GFile * file = gtk_file_chooser_get_file (fc);
  char * str =
    g_file_get_path (file);
  g_settings_set_string (
    data->info->settings, data->key, str);
  g_free (str);
  g_object_unref (file);
}

static void
font_scale_adjustment_changed (
  GtkAdjustment * adjustment,
  void *          data)
{
  double factor =
    gtk_adjustment_get_value (adjustment);
  zrythm_app_set_font_scale (zrythm_app, factor);
}

static void
on_closure_notify_delete_data (
  CallbackData * data,
  GClosure *     closure)
{
  free (data);
}

/** Path type. */
typedef enum PathType
{
  /** Not a path. */
  PATH_TYPE_NONE,

  /** Single entry separated by G_SEARCHPATH_SEPARATOR_S. */
  PATH_TYPE_ENTRY,

  /** File chooser button. */
  PATH_TYPE_FILE,

  /** File chooser button for directories. */
  PATH_TYPE_DIRECTORY,
} PathType;

/**
 * Returns if the key is a path or not.
 */
static PathType
get_path_type (
  const char * group,
  const char * subgroup,
  const char * key)
{
  if (KEY_IS ("General", "Paths", "zrythm-dir"))
    {
      return PATH_TYPE_DIRECTORY;
    }
  else if (
    KEY_IS (
      "Plugins", "Paths",
      "vst-search-paths-windows") ||
    KEY_IS (
      "Plugins", "Paths", "sfz-search-paths") ||
    KEY_IS (
      "Plugins", "Paths", "sf2-search-paths"))
    {
      return PATH_TYPE_ENTRY;
    }

  return PATH_TYPE_NONE;
}

static bool
should_be_hidden (
  const char * group,
  const char * subgroup,
  const char * key)
{
  return
#ifndef _WOE32
    KEY_IS (
      "Plugins", "Paths",
      "vst-search-paths-windows") ||
#endif
#ifndef HAVE_CARLA
    KEY_IS (
      "Plugins", "Paths", "sfz-search-paths") ||
    KEY_IS (
      "Plugins", "Paths", "sf2-search-paths") ||
#endif
    (AUDIO_ENGINE->audio_backend !=
       AUDIO_BACKEND_SDL &&
     KEY_IS (
       "General", "Engine",
       "sdl-audio-device-name")) ||
    (!audio_backend_is_rtaudio (
       AUDIO_ENGINE->audio_backend) &&
     KEY_IS (
       "General", "Engine",
       "rtaudio-audio-device-name")) ||
    (AUDIO_ENGINE->audio_backend ==
       AUDIO_BACKEND_JACK &&
     KEY_IS (
       "General", "Engine", "sample-rate")) ||
    (AUDIO_ENGINE->audio_backend ==
       AUDIO_BACKEND_JACK &&
     KEY_IS (
       "General", "Engine", "buffer-size"));
}

static void
get_range_vals (
  GVariant * range,
  GVariant * current_var,
  const GVariantType * type,
  double *   lower,
  double *   upper,
  double *   current)
{
  GVariant * range_vals =
    g_variant_get_child_value (range, 1);
  range_vals =
    g_variant_get_child_value (range_vals, 0);
  GVariant * lower_var =
    g_variant_get_child_value (range_vals, 0);
  GVariant * upper_var =
    g_variant_get_child_value (range_vals, 1);

#define TYPE_EQUALS(type2) \
  string_is_equal ( \
    (const char *) type,  \
    (const char *) G_VARIANT_TYPE_##type2)

  if (TYPE_EQUALS (INT32))
    {
      *lower =
        (double) g_variant_get_int32 (lower_var);
      *upper =
        (double) g_variant_get_int32 (upper_var);
      *current =
        (double)
        g_variant_get_int32 (current_var);
    }
  else if (TYPE_EQUALS (UINT32))
    {
      *lower =
        (double)
        g_variant_get_uint32 (lower_var);
      *upper =
        (double)
        g_variant_get_uint32 (upper_var);
      *current =
        (double)
        g_variant_get_uint32 (current_var);
    }
  else if (TYPE_EQUALS (DOUBLE))
    {
      *lower =
        g_variant_get_double (lower_var);
      *upper =
        (double)
        g_variant_get_double (upper_var);
      *current =
        (double)
        g_variant_get_double (current_var);
    }
#undef TYPE_EQUALS

  g_variant_unref (range_vals);
  g_variant_unref (lower_var);
  g_variant_unref (upper_var);
}

static GtkWidget *
make_control (
  PreferencesWidget * self,
  int                 group_idx,
  int                 subgroup_idx,
  const char *        key)
{
  SubgroupInfo * info =
    &self->subgroup_infos[group_idx][subgroup_idx];
  const char * group = info->group_name;
  const char * subgroup = info->name;
  GSettingsSchemaKey * schema_key =
    g_settings_schema_get_key (
      info->schema, key);
  const GVariantType * type =
    g_settings_schema_key_get_value_type (
      schema_key);
  GVariant * current_var =
    g_settings_get_value (info->settings, key);
  GVariant * range =
    g_settings_schema_key_get_range (
      schema_key);

#if 0
  g_message ("%s",
    g_variant_get_type_string (current_var));
#endif

#define TYPE_EQUALS(type2) \
  string_is_equal ( \
    (const char *) type,  \
    (const char *) G_VARIANT_TYPE_##type2)

  GtkWidget * widget = NULL;
  if (KEY_IS (
        "General", "Engine",
        "rtaudio-audio-device-name") ||
      KEY_IS (
        "General", "Engine",
        "sdl-audio-device-name"))
    {
      widget = gtk_combo_box_text_new ();
      ui_setup_device_name_combo_box (
        GTK_COMBO_BOX_TEXT (widget));
      CallbackData * data =
        object_new (CallbackData);
      data->info = info;
      data->preferences_widget = self;
      data->key = g_strdup (key);
      g_signal_connect_data (
        G_OBJECT (widget), "changed",
        G_CALLBACK (
          on_string_combo_box_active_changed),
        data,
        (GClosureNotify)
          on_closure_notify_delete_data,
        G_CONNECT_AFTER);
    }
  else if (KEY_IS (
             "General", "Engine", "midi-backend"))
    {
      widget = gtk_combo_box_new ();
      ui_setup_midi_backends_combo_box (
        GTK_COMBO_BOX (widget));
      CallbackData * data =
        object_new (CallbackData);
      data->info = info;
      data->preferences_widget = self;
      data->key = g_strdup (key);
      g_signal_connect_data (
        G_OBJECT (widget), "changed",
        G_CALLBACK (
          on_backends_combo_box_active_changed),
        data,
        (GClosureNotify)
          on_closure_notify_delete_data,
        G_CONNECT_AFTER);
    }
  else if (KEY_IS (
             "General", "Engine", "audio-backend"))
    {
      widget = gtk_combo_box_new ();
      ui_setup_audio_backends_combo_box (
        GTK_COMBO_BOX (widget));
      CallbackData * data =
        object_new (CallbackData);
      data->info = info;
      data->preferences_widget = self;
      data->key = g_strdup (key);
      g_signal_connect_data (
        G_OBJECT (widget), "changed",
        G_CALLBACK (
          on_backends_combo_box_active_changed),
        data,
        (GClosureNotify)
          on_closure_notify_delete_data,
        G_CONNECT_AFTER);
    }
  else if (KEY_IS (
        "General", "Engine",
        "audio-inputs"))
    {
      widget =
        g_object_new (
          ACTIVE_HARDWARE_MB_WIDGET_TYPE, NULL);
      active_hardware_mb_widget_setup (
        Z_ACTIVE_HARDWARE_MB_WIDGET (widget),
        F_INPUT, F_NOT_MIDI, S_P_GENERAL_ENGINE,
        "audio-inputs");
    }
  else if (KEY_IS (
        "General", "Engine",
        "midi-controllers"))
    {
      widget =
        g_object_new (
          ACTIVE_HARDWARE_MB_WIDGET_TYPE, NULL);
      active_hardware_mb_widget_setup (
        Z_ACTIVE_HARDWARE_MB_WIDGET (widget),
        F_INPUT, F_MIDI, S_P_GENERAL_ENGINE,
        "midi-controllers");
    }
  else if (KEY_IS ("UI", "General", "font-scale"))
    {
      double lower = 0.f, upper = 1.f, current = 0.f;
      get_range_vals (
        range, current_var, type, &lower, &upper, &current);
      widget =
        gtk_box_new (
          GTK_ORIENTATION_HORIZONTAL, 2);
      gtk_widget_set_visible (widget, true);
      GtkWidget * scale =
        gtk_scale_new_with_range (
          GTK_ORIENTATION_HORIZONTAL, lower, upper,
          0.1);
      gtk_widget_set_visible (scale, true);
      gtk_widget_set_hexpand (scale, true);
      gtk_scale_add_mark (
        GTK_SCALE (scale), 1.0, GTK_POS_TOP, NULL);
      gtk_box_append (
        GTK_BOX (widget), scale);
      GtkAdjustment * adj =
        gtk_range_get_adjustment (
          GTK_RANGE (scale));
      gtk_adjustment_set_value (adj, current);
      g_settings_bind (
        info->settings, key, adj, "value",
        G_SETTINGS_BIND_DEFAULT);
      g_signal_connect (
        adj, "value-changed",
        G_CALLBACK (font_scale_adjustment_changed),
        NULL);
    }
  else if (TYPE_EQUALS (BOOLEAN))
    {
      widget = gtk_switch_new ();
      g_settings_bind (
        info->settings, key, widget, "state",
        G_SETTINGS_BIND_DEFAULT);
    }
  else if (TYPE_EQUALS (INT32) ||
           TYPE_EQUALS (UINT32) ||
           TYPE_EQUALS (DOUBLE))
    {
      double lower = 0.f, upper = 1.f, current = 0.f;
      get_range_vals (
        range, current_var, type, &lower, &upper, &current);
      GtkAdjustment * adj =
        gtk_adjustment_new (
          current, lower, upper, 1.0, 1.0, 1.0);
      widget =
        gtk_spin_button_new (
          adj, 1, TYPE_EQUALS (DOUBLE) ? 3 : 0);
      g_settings_bind (
        info->settings, key, widget, "value",
        G_SETTINGS_BIND_DEFAULT);
    }
  else if (TYPE_EQUALS (STRING))
    {
      PathType path_type =
        get_path_type (
          info->group_name, info->name, key);
      if (path_type == PATH_TYPE_DIRECTORY ||
          path_type == PATH_TYPE_FILE)
        {
          widget =
            GTK_WIDGET (
              file_chooser_button_widget_new (
                GTK_WINDOW (MAIN_WINDOW),
                path_type == PATH_TYPE_DIRECTORY ?
                _("Select a folder") :
                _("Select a file"),
                path_type == PATH_TYPE_DIRECTORY ?
                GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER :
                GTK_FILE_CHOOSER_ACTION_OPEN));
          char * path =
            g_settings_get_string (
              info->settings, key);
          file_chooser_button_widget_set_current_dir (
            Z_FILE_CHOOSER_BUTTON_WIDGET (widget),
            path);
          g_free (path);
          CallbackData * data =
            object_new (CallbackData);
          data->info = info;
          data->preferences_widget = self;
          data->key = g_strdup (key);
          file_chooser_button_widget_set_response_callback (
            Z_FILE_CHOOSER_BUTTON_WIDGET (widget),
            G_CALLBACK (on_file_set), data,
            (GClosureNotify)
              on_closure_notify_delete_data);
        }
      else if (path_type == PATH_TYPE_NONE)
        {
          /* map enums */
          const char ** strv = NULL;
          const cyaml_strval_t * cyaml_strv = NULL;
          size_t size = 0;

#define SET_STRV_IF_MATCH(a,b,c,arr_name) \
  if (KEY_IS (a,b,c)) \
    { \
      strv = arr_name; \
      size = G_N_ELEMENTS (arr_name); \
    }

#define SET_STRV_IF_MATCH_W_COUNT(a,b,c,arr_name, \
  count) \
  if (KEY_IS (a,b,c)) \
    { \
      strv = arr_name; \
      size = count; \
    }

#define SET_STRV_FROM_CYAML_IF_MATCH( \
  a,b,c,arr_name) \
  if (KEY_IS (a,b,c)) \
    { \
      cyaml_strv = arr_name; \
      size = G_N_ELEMENTS (arr_name); \
    }

          SET_STRV_IF_MATCH (
            "General", "Engine", "audio-backend",
            audio_backend_str);
          SET_STRV_IF_MATCH (
            "General", "Engine", "midi-backend",
            midi_backend_str);
          SET_STRV_IF_MATCH (
            "General", "Engine", "sample-rate",
            sample_rate_str);
          SET_STRV_IF_MATCH (
            "General", "Engine", "buffer-size",
            buffer_size_str);
          SET_STRV_FROM_CYAML_IF_MATCH (
            "Editing", "Audio", "fade-algorithm",
            curve_algorithm_strings);
          SET_STRV_FROM_CYAML_IF_MATCH (
            "Editing", "Automation",
            "curve-algorithm",
            curve_algorithm_strings);
          SET_STRV_IF_MATCH_W_COUNT (
            "UI", "General", "language",
            localization_get_language_strings_w_codes (),
            NUM_LL_LANGUAGES);
          SET_STRV_IF_MATCH (
            "UI", "General", "graphic-detail",
            ui_detail_str);
          SET_STRV_IF_MATCH (
            "DSP", "Pan", "pan-algorithm",
            pan_algorithm_str);
          SET_STRV_IF_MATCH (
            "DSP", "Pan", "pan-law",
            pan_law_str);

#undef SET_STRV_IF_MATCH

          if (strv || cyaml_strv)
            {
              widget =
                gtk_combo_box_text_new ();
              for (size_t i = 0; i < size; i++)
                {
                  if (cyaml_strv)
                    {
                      gtk_combo_box_text_append (
                        GTK_COMBO_BOX_TEXT (widget),
                        cyaml_strv[i].str,
                        _(cyaml_strv[i].str));
                    }
                  else if (strv)
                    {
                      gtk_combo_box_text_append (
                        GTK_COMBO_BOX_TEXT (widget),
                        strv[i], _(strv[i]));
                    }
                }
              gtk_combo_box_set_active (
                GTK_COMBO_BOX (widget),
                g_settings_get_enum (
                  info->settings, key));
              CallbackData * data =
                object_new (CallbackData);
              data->info = info;
              data->preferences_widget = self;
              data->key = g_strdup (key);
              g_signal_connect_data (
                G_OBJECT (widget), "changed",
                G_CALLBACK (
                  on_enum_combo_box_active_changed),
                data,
                (GClosureNotify)
                  on_closure_notify_delete_data,
                G_CONNECT_AFTER);
            }
          /* else if not a string array */
          else
            {
              /* create basic string entry control */
              widget = gtk_entry_new ();
              char * current_val =
                g_settings_get_string (
                  info->settings, key);
              gtk_editable_set_text (
                GTK_EDITABLE (widget), current_val);
              g_free (current_val);
              CallbackData * data =
                object_new (CallbackData);
              data->info = info;
              data->preferences_widget = self;
              data->key = g_strdup (key);
              g_signal_connect_data (
                G_OBJECT (widget), "changed",
                G_CALLBACK (
                  on_simple_string_entry_changed),
                data,
                (GClosureNotify)
                  on_closure_notify_delete_data,
                G_CONNECT_AFTER);
            }
        }
    }
  else if (TYPE_EQUALS (STRING_ARRAY))
    {
      if (get_path_type (
            info->group_name, info->name, key) ==
            PATH_TYPE_ENTRY)
        {
          widget = gtk_entry_new ();
          char ** paths =
            g_settings_get_strv (
              info->settings, key);
          char * joined_str =
            g_strjoinv (G_SEARCHPATH_SEPARATOR_S, paths);
          gtk_editable_set_text (
            GTK_EDITABLE (widget), joined_str);
          g_free (joined_str);
          g_strfreev (paths);
          CallbackData * data =
            object_new (CallbackData);
          data->info = info;
          data->preferences_widget = self;
          data->key = g_strdup (key);
          g_signal_connect_data (
            G_OBJECT (widget), "changed",
            G_CALLBACK (on_path_entry_changed),
            data,
            (GClosureNotify)
              on_closure_notify_delete_data,
            G_CONNECT_AFTER);
        }
    }
#if 0
  else if (string_is_equal (
             g_variant_get_type_string (
               current_var), "ai"))
    {
    }
#endif

#undef TYPE_EQUALS

  g_warn_if_fail (widget);

  return widget;
}

static void
add_subgroup (
  PreferencesWidget * self,
  int                 group_idx,
  int                 subgroup_idx,
  GtkSizeGroup *      size_group)
{
  SubgroupInfo * info =
    &self->subgroup_infos[group_idx][subgroup_idx];

  const char * localized_subgroup_name =
    info->name;
  g_message (
    "adding subgroup %s (%s)",
    info->name, localized_subgroup_name);

  /* create a section for the subgroup */
  GtkWidget * page_box =
    gtk_notebook_get_nth_page (
      self->group_notebook, info->group_idx);
  GtkWidget * label =
    plugin_gtk_new_label (
      localized_subgroup_name, true, false,
      0.f, 0.5f);
  gtk_widget_set_visible (label, true);
  gtk_widget_set_parent (label, page_box);

  char ** keys =
    g_settings_schema_list_keys (info->schema);
  int i = 0;
  int num_controls = 0;
  char * key;
  while ((key = keys[i++]))
    {
      GSettingsSchemaKey * schema_key =
        g_settings_schema_get_key (
          info->schema, key);
      /* note: this is already translated */
      const char * summary =
        g_settings_schema_key_get_summary (
          schema_key);
      /* note: this is already translated */
      const char * description =
        g_settings_schema_key_get_description (
          schema_key);

      if (string_is_equal (key, "info") ||
          should_be_hidden (
            info->group_name, info->name, key))
        continue;

      g_message ("adding control for %s", key);

      /* create a box to add controls */
      GtkWidget * box =
        gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
      gtk_widget_set_visible (box, true);
      gtk_widget_set_parent (box, page_box);

      /* add label */
      GtkWidget * lbl =
        plugin_gtk_new_label (
          summary, false, false, 1.f, 0.5f);
      gtk_widget_set_visible (lbl, true);
      gtk_box_append (GTK_BOX (box), lbl);
      gtk_size_group_add_widget (
        size_group, lbl);

      /* add control */
      GtkWidget * widget =
        make_control (
          self, group_idx, subgroup_idx, key);
      if (widget)
        {
          gtk_widget_set_visible (widget, true);
          if (GTK_IS_SWITCH (widget))
            {
              gtk_widget_set_halign (
                widget, GTK_ALIGN_START);
            }
          else
            {
              gtk_widget_set_hexpand (widget, true);
            }
          gtk_widget_set_tooltip_text (
            widget, description);
          gtk_box_append (GTK_BOX (box), widget);
          num_controls++;
        }
      else
        {
          g_warning ("no widget for %s", key);
        }
    }

  /* Remove label if no controls added */
  if (num_controls == 0)
    {
      g_object_unref (label);
    }
}

static void
add_group (
  PreferencesWidget * self,
  int                 group_idx)
{
  GSettingsSchemaSource * source =
    g_settings_schema_source_get_default ();
  char ** non_relocatable;
  g_settings_schema_source_list_schemas (
    source, 1, &non_relocatable, NULL);

  /* loop once to get the max subgroup index and
   * group name */
  char * schema_str;
  const char * group_name = NULL;
  const char * subgroup_name = NULL;
  int i = 0;
  int max_subgroup_idx = 0;
  while ((schema_str = non_relocatable[i++]))
    {
      if (!string_contains_substr (
            schema_str,
            GSETTINGS_ZRYTHM_PREFIX ".preferences"))
        continue;

      /* get the preferences.x.y schema */
      GSettingsSchema * schema =
        g_settings_schema_source_lookup (
          source, schema_str, 1);
      g_return_if_fail (schema);

      GSettings * settings =
        g_settings_new (schema_str);
      GVariant * info_val =
        g_settings_get_value (
          settings, "info");
      int this_group_idx =
        (int)
        g_variant_get_int32 (
          g_variant_get_child_value (
            info_val, 0));

      if (this_group_idx != group_idx)
        continue;

      GSettingsSchemaKey * info_key =
        g_settings_schema_get_key (
          schema, "info");
      /* note: this is already translated */
      group_name =
        g_settings_schema_key_get_summary (info_key);
      /* note: this is already translated */
      subgroup_name =
        g_settings_schema_key_get_description (
          info_key);

      /* get max subgroup index */
      int subgroup_idx =
        (int)
        g_variant_get_int32 (
          g_variant_get_child_value (
            info_val, 1));
      SubgroupInfo * nfo =
        &self->subgroup_infos[
          group_idx][subgroup_idx];
      nfo->schema = schema;
      nfo->settings = settings;
      nfo->group_name = group_name;
      nfo->name = subgroup_name;
      nfo->group_idx = group_idx;
      nfo->subgroup_idx = subgroup_idx;
      if (subgroup_idx > max_subgroup_idx)
        max_subgroup_idx = subgroup_idx;
    }

  const char * localized_group_name = group_name;
  g_message (
    "adding group %s (%s)",
    group_name, localized_group_name);

  /* create a page for the group */
  GtkWidget * box =
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_visible (box, true);
  z_gtk_widget_set_margin (box, 4);
  gtk_notebook_append_page (
    self->group_notebook, box,
    plugin_gtk_new_label (
      localized_group_name, true, false, 0.f, 0.5f));

  /* create a sizegroup for the labels */
  GtkSizeGroup * size_group =
    gtk_size_group_new (
      GTK_SIZE_GROUP_HORIZONTAL);

  /* add each subgroup */
  for (int j = 0; j <= max_subgroup_idx; j++)
    {
      add_subgroup (self, group_idx, j, size_group);
    }
}

static void
on_window_closed (
  GtkWidget *object,
  PreferencesWidget * self)
{
  GtkWidget * dialog =
    gtk_message_dialog_new_with_markup (
      GTK_WINDOW (MAIN_WINDOW),
      GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_INFO,
      GTK_BUTTONS_OK,
      _("Some changes will only take "
      "effect after you restart %s"),
      PROGRAM_NAME);
  gtk_window_set_transient_for (
    GTK_WINDOW (dialog),
    GTK_WINDOW (self));
  gtk_window_set_modal (GTK_WINDOW (dialog), true);
  gtk_widget_show (dialog);

  MAIN_WINDOW->preferences_opened = false;
}

/**
 * Sets up the preferences widget.
 */
PreferencesWidget *
preferences_widget_new ()
{
  PreferencesWidget * self =
    g_object_new (
      PREFERENCES_WIDGET_TYPE,
      "title", _("Preferences"),
      NULL);

  for (int i = 0; i <= 6; i++)
    {
      add_group (self, i);
    }

  g_signal_connect (
    G_OBJECT (self), "destroy",
    G_CALLBACK (on_window_closed), self);

  return self;
}

static void
preferences_widget_class_init (
  PreferencesWidgetClass * _klass)
{
}

static void
preferences_widget_init (
  PreferencesWidget * self)
{
  self->group_notebook =
    GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->group_notebook), true);
  gtk_box_append (
    GTK_BOX (
      gtk_dialog_get_content_area (
        GTK_DIALOG (self))),
    GTK_WIDGET (self->group_notebook));
}
