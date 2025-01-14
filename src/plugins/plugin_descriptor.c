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

#include <stdlib.h>

#include "audio/track.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_manager.h"
#include "plugins/plugin.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#include <lv2/instance-access/instance-access.h>

PluginDescriptor *
plugin_descriptor_new (void)
{
  PluginDescriptor * self =
    object_new (PluginDescriptor);
  self->schema_version =
    PLUGIN_DESCRIPTOR_SCHEMA_VERSION;

  return self;
}

const char *
plugin_protocol_to_str (
  PluginProtocol prot)
{
  for (size_t i = 0;
       i < G_N_ELEMENTS (plugin_protocol_strings);
       i++)
    {
      if (plugin_protocol_strings[i].val ==
            (int64_t) prot)
        {
          return
            plugin_protocol_strings[i].str;
        }
    }
  g_return_val_if_reached (NULL);
}

/**
 * Clones the plugin descriptor.
 */
void
plugin_descriptor_copy (
  PluginDescriptor *       dest,
  const PluginDescriptor * src)
{
  g_return_if_fail (src->schema_version > 0);
  dest->schema_version = src->schema_version;
  dest->author = g_strdup (src->author);
  dest->name = g_strdup (src->name);
  dest->website = g_strdup (src->website);
  dest->category_str = g_strdup (src->category_str);
  dest->category = src->category;
  dest->num_audio_ins = src->num_audio_ins;
  dest->num_midi_ins = src->num_midi_ins;
  dest->num_audio_outs = src->num_audio_outs;
  dest->num_midi_outs = src->num_midi_outs;
  dest->num_ctrl_ins = src->num_ctrl_ins;
  dest->num_ctrl_outs = src->num_ctrl_outs;
  dest->num_cv_ins = src->num_cv_ins;
  dest->num_cv_outs = src->num_cv_outs;
  dest->arch = src->arch;
  dest->protocol = src->protocol;
  dest->path = g_strdup (src->path);
  dest->uri = g_strdup (src->uri);
  dest->min_bridge_mode = src->min_bridge_mode;
  dest->has_custom_ui = src->has_custom_ui;
  dest->ghash = src->ghash;
}

/**
 * Clones the plugin descriptor.
 */
PluginDescriptor *
plugin_descriptor_clone (
  const PluginDescriptor * src)
{
  PluginDescriptor * self =
    object_new (PluginDescriptor);

  plugin_descriptor_copy (self, src);

  return self;
}

#define IS_CAT(x) \
  (descr->category == PC_##x)

/**
 * Returns if the Plugin is an instrument or not.
 */
bool
plugin_descriptor_is_instrument (
  const PluginDescriptor * descr)
{
  if (descr->num_midi_ins == 0 ||
      descr->num_audio_outs == 0)
    {
      return false;
    }

  if (descr->category == PC_INSTRUMENT)
    {
      return true;
    }
  else
    {
      return
        /* if VSTs are instruments their category
         * must be INSTRUMENT, otherwise they are
         * not */
        descr->protocol != PROT_VST &&
        descr->category == ZPLUGIN_CATEGORY_NONE &&
        descr->num_midi_ins > 0 &&
        descr->num_audio_outs > 0;
    }
}

/**
 * Returns if the Plugin is an effect or not.
 */
bool
plugin_descriptor_is_effect (
  PluginDescriptor * descr)
{

  return
    (descr->category > ZPLUGIN_CATEGORY_NONE &&
     (IS_CAT (DELAY) ||
      IS_CAT (REVERB) ||
      IS_CAT (DISTORTION) ||
      IS_CAT (WAVESHAPER) ||
      IS_CAT (DYNAMICS) ||
      IS_CAT (AMPLIFIER) ||
      IS_CAT (COMPRESSOR) ||
      IS_CAT (ENVELOPE) ||
      IS_CAT (EXPANDER) ||
      IS_CAT (GATE) ||
      IS_CAT (LIMITER) ||
      IS_CAT (FILTER) ||
      IS_CAT (ALLPASS_FILTER) ||
      IS_CAT (BANDPASS_FILTER) ||
      IS_CAT (COMB_FILTER) ||
      IS_CAT (EQ) ||
      IS_CAT (MULTI_EQ) ||
      IS_CAT (PARA_EQ) ||
      IS_CAT (HIGHPASS_FILTER) ||
      IS_CAT (LOWPASS_FILTER) ||
      IS_CAT (GENERATOR) ||
      IS_CAT (CONSTANT) ||
      IS_CAT (OSCILLATOR) ||
      IS_CAT (MODULATOR) ||
      IS_CAT (CHORUS) ||
      IS_CAT (FLANGER) ||
      IS_CAT (PHASER) ||
      IS_CAT (SIMULATOR) ||
      IS_CAT (SIMULATOR_REVERB) ||
      IS_CAT (SPATIAL) ||
      IS_CAT (SPECTRAL) ||
      IS_CAT (PITCH) ||
      IS_CAT (UTILITY) ||
      IS_CAT (ANALYZER) ||
      IS_CAT (CONVERTER) ||
      IS_CAT (FUNCTION) ||
      IS_CAT (MIXER))) ||
    (descr->category == ZPLUGIN_CATEGORY_NONE &&
     descr->num_audio_ins > 0 &&
     descr->num_audio_outs > 0);
}

/**
 * Returns if the Plugin is a modulator or not.
 */
int
plugin_descriptor_is_modulator (
  PluginDescriptor * descr)
{
  return
    (descr->category == ZPLUGIN_CATEGORY_NONE ||
     (descr->category > ZPLUGIN_CATEGORY_NONE &&
      (IS_CAT (ENVELOPE) ||
       IS_CAT (GENERATOR) ||
       IS_CAT (CONSTANT) ||
       IS_CAT (OSCILLATOR) ||
       IS_CAT (MODULATOR) ||
       IS_CAT (UTILITY) ||
       IS_CAT (CONVERTER) ||
       IS_CAT (FUNCTION)))) &&
    descr->num_cv_outs > 0;
}

/**
 * Returns if the Plugin is a midi modifier or not.
 */
int
plugin_descriptor_is_midi_modifier (
  PluginDescriptor * descr)
{
  return
    (descr->category > ZPLUGIN_CATEGORY_NONE &&
     descr->category == PC_MIDI) ||
    (descr->category == ZPLUGIN_CATEGORY_NONE &&
     descr->num_midi_ins > 0 &&
     descr->num_midi_outs > 0 &&
     descr->protocol != PROT_VST);
}

#undef IS_CAT

/**
 * Returns the ZPluginCategory matching the given
 * string.
 */
ZPluginCategory
plugin_descriptor_string_to_category (
  const char * str)
{
  ZPluginCategory category = ZPLUGIN_CATEGORY_NONE;

#define CHECK_CAT(term,cat) \
  if (g_strrstr (str, term)) \
    category = PC_##cat

  /* add category */
  CHECK_CAT ("Delay", DELAY);
  CHECK_CAT ("Reverb", REVERB);
  CHECK_CAT ("Distortion", DISTORTION);
  CHECK_CAT ("Waveshaper", WAVESHAPER);
  CHECK_CAT ("Dynamics", DYNAMICS);
  CHECK_CAT ("Amplifier", AMPLIFIER);
  CHECK_CAT ("Compressor", COMPRESSOR);
  CHECK_CAT ("Envelope", ENVELOPE);
  CHECK_CAT ("Expander", EXPANDER);
  CHECK_CAT ("Gate", GATE);
  CHECK_CAT ("Limiter", LIMITER);
  CHECK_CAT ("Filter", FILTER);
  CHECK_CAT ("Allpass", ALLPASS_FILTER);
  CHECK_CAT ("Bandpass", BANDPASS_FILTER);
  CHECK_CAT ("Comb", COMB_FILTER);
  CHECK_CAT ("Equaliser", EQ);
  CHECK_CAT ("Equalizer", EQ);
  CHECK_CAT ("Multiband", MULTI_EQ);
  CHECK_CAT ("Para", PARA_EQ);
  CHECK_CAT ("Highpass", HIGHPASS_FILTER);
  CHECK_CAT ("Lowpass", LOWPASS_FILTER);
  CHECK_CAT ("Generator", GENERATOR);
  CHECK_CAT ("Constant", CONSTANT);
  CHECK_CAT ("Instrument", INSTRUMENT);
  CHECK_CAT ("Oscillator", OSCILLATOR);
  CHECK_CAT ("MIDI", MIDI);
  CHECK_CAT ("Modulator", MODULATOR);
  CHECK_CAT ("Chorus", CHORUS);
  CHECK_CAT ("Flanger", FLANGER);
  CHECK_CAT ("Phaser", PHASER);
  CHECK_CAT ("Simulator", SIMULATOR);
  CHECK_CAT ("SimulatorReverb", SIMULATOR_REVERB);
  CHECK_CAT ("Spatial", SPATIAL);
  CHECK_CAT ("Spectral", SPECTRAL);
  CHECK_CAT ("Pitch", PITCH);
  CHECK_CAT ("Utility", UTILITY);
  CHECK_CAT ("Analyser", ANALYZER);
  CHECK_CAT ("Analyzer", ANALYZER);
  CHECK_CAT ("Converter", CONVERTER);
  CHECK_CAT ("Function", FUNCTION);
  CHECK_CAT ("Mixer", MIXER);

#undef CHECK_CAT

  return category;
}

char *
plugin_descriptor_category_to_string (
  ZPluginCategory category)
{

#define RET_STRING(term,cat) \
  if (category == PC_##cat) \
    return g_strdup (term)

  /* add category */
  RET_STRING ("Delay", DELAY);
  RET_STRING ("Reverb", REVERB);
  RET_STRING ("Distortion", DISTORTION);
  RET_STRING ("Waveshaper", WAVESHAPER);
  RET_STRING ("Dynamics", DYNAMICS);
  RET_STRING ("Amplifier", AMPLIFIER);
  RET_STRING ("Compressor", COMPRESSOR);
  RET_STRING ("Envelope", ENVELOPE);
  RET_STRING ("Expander", EXPANDER);
  RET_STRING ("Gate", GATE);
  RET_STRING ("Limiter", LIMITER);
  RET_STRING ("Filter", FILTER);
  RET_STRING ("Allpass", ALLPASS_FILTER);
  RET_STRING ("Bandpass", BANDPASS_FILTER);
  RET_STRING ("Comb", COMB_FILTER);
  RET_STRING ("Equaliser", EQ);
  RET_STRING ("Equalizer", EQ);
  RET_STRING ("Multiband", MULTI_EQ);
  RET_STRING ("Para", PARA_EQ);
  RET_STRING ("Highpass", HIGHPASS_FILTER);
  RET_STRING ("Lowpass", LOWPASS_FILTER);
  RET_STRING ("Generator", GENERATOR);
  RET_STRING ("Constant", CONSTANT);
  RET_STRING ("Instrument", INSTRUMENT);
  RET_STRING ("Oscillator", OSCILLATOR);
  RET_STRING ("MIDI", MIDI);
  RET_STRING ("Modulator", MODULATOR);
  RET_STRING ("Chorus", CHORUS);
  RET_STRING ("Flanger", FLANGER);
  RET_STRING ("Phaser", PHASER);
  RET_STRING ("Simulator", SIMULATOR);
  RET_STRING ("SimulatorReverb", SIMULATOR_REVERB);
  RET_STRING ("Spatial", SPATIAL);
  RET_STRING ("Spectral", SPECTRAL);
  RET_STRING ("Pitch", PITCH);
  RET_STRING ("Utility", UTILITY);
  RET_STRING ("Analyser", ANALYZER);
  RET_STRING ("Analyzer", ANALYZER);
  RET_STRING ("Converter", CONVERTER);
  RET_STRING ("Function", FUNCTION);
  RET_STRING ("Mixer", MIXER);

#undef RET_STRING

  return g_strdup ("Other");
}

/**
 * Returns if the given plugin identifier can be
 * dropped in a slot of the given type.
 */
bool
plugin_descriptor_is_valid_for_slot_type (
  PluginDescriptor * self,
  int                slot_type,
  int                track_type)
{
  switch (slot_type)
    {
    case PLUGIN_SLOT_INSERT:
      if (track_type == TRACK_TYPE_MIDI)
        {
          return self->num_midi_outs > 0;
        }
      else
        {
          return self->num_audio_outs > 0;
        }
    case PLUGIN_SLOT_MIDI_FX:
      return self->num_midi_outs > 0;
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      return
        track_type == TRACK_TYPE_INSTRUMENT &&
        plugin_descriptor_is_instrument (self);
    default:
      break;
    }

  g_return_val_if_reached (false);
}

/**
 * Returns whether the two descriptors describe
 * the same plugin, ignoring irrelevant fields.
 */
bool
plugin_descriptor_is_same_plugin (
  const PluginDescriptor * a,
  const PluginDescriptor * b)
{
  return a->arch == b->arch &&
    a->protocol == b->protocol &&
    string_is_equal (a->path, b->path) &&
    string_is_equal (a->uri, b->uri) &&
    a->unique_id == b->unique_id &&
    a->ghash == b->ghash;
}

/**
 * Returns if the Plugin has a supported custom
 * UI.
 */
bool
plugin_descriptor_has_custom_ui (
  const PluginDescriptor * self)
{
  switch (self->protocol)
    {
    case PROT_LV2:
      {
        return
          lv2_plugin_pick_most_preferable_ui (
            self->uri, NULL, NULL, true);
      }
      break;
    case PROT_VST:
    case PROT_VST3:
    case PROT_AU:
#ifdef HAVE_CARLA
      return
        carla_native_plugin_has_custom_ui (self);
#else
      return false;
#endif
      break;
    default:
      return false;
      break;
    }

  g_return_val_if_reached (false);
}

/**
 * Returns the minimum bridge mode required for this
 * plugin.
 */
CarlaBridgeMode
plugin_descriptor_get_min_bridge_mode (
  const PluginDescriptor * self)
{
  if (self->protocol == PROT_LV2)
    {
      /* TODO if the UI and DSP binary is the same
       * file, bridge the whole plugin */
      LilvNode * lv2_uri =
        lilv_new_uri (LILV_WORLD, self->uri);
      const LilvPlugin * lilv_plugin =
        lilv_plugins_get_by_uri (
          LILV_PLUGINS, lv2_uri);
      lilv_node_free (lv2_uri);
      LilvUIs * uis =
        lilv_plugin_get_uis (lilv_plugin);
      const LilvUI * picked_ui;
      const LilvNode * picked_ui_type;
      bool needs_bridging =
        lv2_plugin_pick_ui (
          uis, LV2_PLUGIN_UI_FOR_BRIDGING,
          &picked_ui, &picked_ui_type);
      if (needs_bridging)
        {
          const LilvNode * ui_uri =
            lilv_ui_get_uri (picked_ui);
          LilvNodes * ui_required_features =
            lilv_world_find_nodes (
              LILV_WORLD, ui_uri,
              PM_GET_NODE (
                LV2_CORE__requiredFeature),
              NULL);
          if (lilv_nodes_contains (
                ui_required_features,
                PM_GET_NODE (LV2_DATA_ACCESS_URI)) ||
              lilv_nodes_contains (
                ui_required_features,
                PM_GET_NODE (
                  LV2_INSTANCE_ACCESS_URI)) ||
              lilv_node_equals (
                picked_ui_type,
                PM_GET_NODE (LV2_UI__Qt4UI)) ||
              lilv_node_equals (
                picked_ui_type,
                PM_GET_NODE (LV2_UI__Qt5UI)) ||
              lilv_node_equals (
                picked_ui_type,
                PM_GET_NODE (LV2_UI__GtkUI)) ||
              lilv_node_equals (
                picked_ui_type,
                PM_GET_NODE (LV2_UI__Gtk3UI)))
            {
              return CARLA_BRIDGE_FULL;
            }
          else
            {
              return CARLA_BRIDGE_UI;
            }
          lilv_nodes_free (ui_required_features);
        }
      else /* does not need bridging */
        {
          return CARLA_BRIDGE_NONE;
        }
      lilv_uis_free (uis);
    }
  else if (self->arch == ARCH_32)
    {
      return CARLA_BRIDGE_FULL;
    }
  else
    {
      return CARLA_BRIDGE_NONE;
    }

  g_return_val_if_reached (CARLA_BRIDGE_NONE);
}

void
plugin_descriptor_free (
  PluginDescriptor * self)
{
  g_free_and_null (self->author);
  g_free_and_null (self->name);
  g_free_and_null (self->website);
  g_free_and_null (self->category_str);
  g_free_and_null (self->path);
  g_free_and_null (self->uri);

  object_zero_and_free (self);
}
