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

#include "audio/channel.h"
#include "audio/audio_region.h"
#include "audio/clip.h"
#include "audio/fade.h"
#include "audio/pool.h"
#include "audio/stretcher.h"
#include "audio/tempo_track.h"
#include "audio/track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "project.h"
#include "utils/audio.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm_app.h"

/**
 * Creates a region for audio data.
 *
 * @param pool_id The pool ID. This is used when
 *   creating clone regions (non-main) and must be
 *   -1 when creating a new clip.
 * @param filename Filename, if loading from
 *   file, otherwise NULL.
 * @param read_from_pool Whether to save the given
 *   @a filename or @a frames to pool and read the
 *   data from the pool. Only used if @a filename or
 *   @a frames is given.
 * @param frames Float array, if loading from
 *   float array, otherwise NULL.
 * @param nframes Number of frames per channel.
 * @param clip_name Name of audio clip, if not
 *   loading from file.
 * @param bit_depth Bit depth, if using \ref frames.
 */
ZRegion *
audio_region_new (
  const int        pool_id,
  const char *     filename,
  bool             read_from_pool,
  const float *    frames,
  const long       nframes,
  const char *     clip_name,
  const channels_t channels,
  BitDepth         bit_depth,
  const Position * start_pos,
  unsigned int     track_name_hash,
  int              lane_pos,
  int              idx_inside_lane)
{
  ZRegion * self = object_new (ZRegion);
  /*ArrangerObject * obj =*/
    /*(ArrangerObject *) self;*/

  g_return_val_if_fail (
    start_pos && start_pos->frames >= 0, NULL);

  self->id.type = REGION_TYPE_AUDIO;
  self->pool_id = -1;
  self->read_from_pool = read_from_pool;

  AudioClip * clip = NULL;
  int recording = 0;
  if (pool_id == -1)
    {
      if (filename)
        {
          clip =
            audio_clip_new_from_file (filename);
        }
      else if (frames)
        {
          g_return_val_if_fail (clip_name, NULL);
          clip =
            audio_clip_new_from_float_array (
              frames, nframes, channels, bit_depth,
              clip_name);
        }
      else
        {
          clip =
            audio_clip_new_recording (
              2, nframes, clip_name);
          recording = 1;
        }
      g_return_val_if_fail (clip, NULL);

      if (read_from_pool)
        {
          self->pool_id =
            audio_pool_add_clip (
              AUDIO_POOL, clip);
          g_warn_if_fail (self->pool_id > -1);
        }
      else
        {
          self->clip = clip;
        }
    }
  else
    {
      self->pool_id = pool_id;
      clip = AUDIO_POOL->clips[pool_id];
      g_warn_if_fail (clip && clip->frames);
    }

  /* set end pos to sample end */
  Position end_pos;
  position_set_to_pos (
    &end_pos, start_pos);
  position_add_frames (
    &end_pos, clip->num_frames);

  /* init split points */
  self->split_points_size = 1;
  self->split_points =
    object_new_n (
      self->split_points_size, Position);
  self->num_split_points = 0;

  /* init APs */
  self->aps_size = 2;
  self->aps =
    object_new_n (
      self->aps_size, AutomationPoint *);

  self->gain = 1.f;

  /* init */
  region_init (
    self, start_pos, &end_pos, track_name_hash,
    lane_pos, idx_inside_lane);

  (void) recording;
  g_warn_if_fail (audio_region_get_clip (self));
  /*if (!recording)*/
    /*audio_clip_write_to_pool (clip);*/

  return self;
}

/**
 * Returns the audio clip associated with the
 * Region.
 */
AudioClip *
audio_region_get_clip (
  const ZRegion * self)
{
  g_return_val_if_fail (
    (!self->read_from_pool && self->clip)
    || (self->read_from_pool && self->pool_id >= 0),
    NULL);
  g_return_val_if_fail (
    self->id.type == REGION_TYPE_AUDIO, NULL);

  AudioClip * clip = NULL;
  if (G_LIKELY (self->read_from_pool))
    {
      g_return_val_if_fail (
        self->pool_id >= 0, NULL);
      clip =
        audio_pool_get_clip (
          AUDIO_POOL, self->pool_id);
    }
  else
    {
      clip = self->clip;
    }

  g_return_val_if_fail (
    clip && clip->frames && clip->num_frames > 0,
    NULL);

  return clip;
}

/**
 * Sets the clip ID on the region and updates any
 * references.
 */
void
audio_region_set_clip_id (
  ZRegion * self,
  int       clip_id)
{
  self->pool_id = clip_id;

  /* TODO update identifier - needed? */
}

/**
 * Replaces the region's frames from \ref
 * start_frames with \ref frames.
 *
 * @param duplicate_clip Whether to duplicate the
 *   clip (eg, when other regions refer to it).
 * @param frames Frames, interleaved.
 */
void
audio_region_replace_frames (
  ZRegion * self,
  float *   frames,
  size_t    start_frame,
  size_t    num_frames,
  bool      duplicate_clip)
{
  AudioClip * clip = audio_region_get_clip (self);
  g_return_if_fail (clip);

  if (duplicate_clip)
    {
      g_warn_if_reached ();

      /* TODO delete */
      int prev_id = clip->pool_id;
      int id =
        audio_pool_duplicate_clip (
          AUDIO_POOL, clip->pool_id,
          F_NO_WRITE_FILE);
      g_return_if_fail (id != prev_id);
      clip = audio_pool_get_clip (AUDIO_POOL, id);
      g_return_if_fail (clip);

      self->pool_id = clip->pool_id;
    }

  dsp_copy (
    &clip->frames[start_frame * clip->channels],
    frames, num_frames * clip->channels);

  audio_clip_write_to_pool (
    clip, false, F_NOT_BACKUP);

  self->last_clip_change = g_get_monotonic_time ();
}

static void
timestretch_buf (
  Track *      self,
  ZRegion *    r,
  AudioClip *  clip,
  size_t      in_frame_offset,
  double       timestretch_ratio,
  float *      lbuf_after_ts,
  float *      rbuf_after_ts,
  unsigned int out_frame_offset,
  ssize_t      frames_to_process)
{
  g_return_if_fail (
    r && self->rt_stretcher);
  stretcher_set_time_ratio (
    self->rt_stretcher, 1.0 / timestretch_ratio);
  size_t in_frames_to_process =
    (size_t)
    (frames_to_process * timestretch_ratio);
  g_message (
    "%s: in frame offset %zd, out frame offset %u, "
    "in frames to process %zu, "
    "out frames to process %zd",
    __func__, in_frame_offset, out_frame_offset,
    in_frames_to_process, frames_to_process);
  g_return_if_fail (
    (long)
    (in_frame_offset + in_frames_to_process) <=
      clip->num_frames);
  ssize_t retrieved =
    stretcher_stretch (
      self->rt_stretcher,
      &clip->ch_frames[0][in_frame_offset],
      clip->channels == 1 ?
        &clip->ch_frames[0][in_frame_offset] :
        &clip->ch_frames[1][in_frame_offset],
      in_frames_to_process,
      &lbuf_after_ts[out_frame_offset],
      &rbuf_after_ts[out_frame_offset],
      (size_t) frames_to_process);
  g_warn_if_fail (retrieved == frames_to_process);
}

/**
 * Fills audio data from the region.
 *
 * @note The caller already splits calls to this
 *   function at each sub-loop inside the region,
 *   so region loop related logic is not needed.
 *
 * @param g_start_frames Global start frame.
 * @param cycle_start_offset The start frame offset
 *   from 0 in this cycle.
 * @param nframes Number of frames at start
 *   Position.
 * @param stereo_ports StereoPorts to fill.
 */
void
audio_region_fill_stereo_ports (
  ZRegion *     r,
  long          g_start_frames,
  nframes_t     cycle_start_offset,
  nframes_t     nframes,
  StereoPorts * stereo_ports)
{
  ArrangerObject * r_obj =  (ArrangerObject *) r;
  AudioClip * clip = audio_region_get_clip (r);
  g_return_if_fail (clip);
  Track * track =
    arranger_object_get_track (r_obj);

  /* restretch if necessary */
  Position g_start_pos;
  position_from_frames (
    &g_start_pos, g_start_frames);
  bpm_t cur_bpm =
    tempo_track_get_bpm_at_pos (
      P_TEMPO_TRACK, &g_start_pos);
  double timestretch_ratio = 1.0;
  bool needs_rt_timestretch = false;
  if (region_get_musical_mode (r) &&
      !math_floats_equal (
        clip->bpm, cur_bpm))
    {
      needs_rt_timestretch = true;
      timestretch_ratio =
        (double) cur_bpm / (double) clip->bpm;
      g_message (
        "timestretching: "
        "(cur bpm %f clip bpm %f) %f",
        (double) cur_bpm, (double) clip->bpm,
        timestretch_ratio);
    }

  /* buffers after timestretch */
  float lbuf_after_ts[nframes];
  float rbuf_after_ts[nframes];
  dsp_fill (lbuf_after_ts, 0, nframes);
  dsp_fill (rbuf_after_ts, 0, nframes);

#if 0
  double r_local_ticks_at_start =
    g_start_pos.ticks - r->base.pos.ticks;
  double r_len =
    arranger_object_get_length_in_ticks (r_obj);
  double ratio =
    r_local_ticks_at_start / r_len;
  g_message ("ratio %f", ratio);
#endif

  long r_local_frames_at_start =
    region_timeline_frames_to_local (
      r, g_start_frames, F_NORMALIZE);

#if 0
  Position r_local_pos_at_start;
  position_from_frames (
    &r_local_pos_at_start, r_local_frames_at_start);
  Position r_local_pos_at_end;
  position_from_frames (
    &r_local_pos_at_end,
    r_local_frames_at_start + nframes);

  g_message ("region");
  position_print_range (
    &r->base.pos, &r->base.end_pos);
  g_message ("g start pos");
  position_print (&g_start_pos);
  g_message ("region local pos start/end");
  position_print_range (
    &r_local_pos_at_start, &r_local_pos_at_end);
#endif

  size_t buff_index_start =
    (size_t) clip->num_frames + 16;
  size_t buff_size = 0;
  unsigned int prev_offset = cycle_start_offset;
  for (nframes_t j =
         (r_local_frames_at_start < 0) ?
           - r_local_frames_at_start : 0;
       j < nframes; j++)
    {
      long current_local_frame =
        cycle_start_offset + j;
      long r_local_pos =
        region_timeline_frames_to_local (
          r, g_start_frames + j, F_NORMALIZE);
      if (r_local_pos < 0 ||
          j > AUDIO_ENGINE->block_length)
        {
          g_critical (
            "invalid r_local_pos %ld, j %u, "
            "g_start_frames %ld, nframes %u",
            r_local_pos, j, g_start_frames, nframes);
          return;
        }

      ssize_t buff_index =
        (ssize_t) r_local_pos;

#define STRETCH \
timestretch_buf ( \
track, r, clip, buff_index_start, \
timestretch_ratio, \
lbuf_after_ts, rbuf_after_ts, \
prev_offset, \
(current_local_frame - prev_offset) + 1)

      /* if we are starting at a new
       * point in the audio clip */
      if (needs_rt_timestretch)
        {
          buff_index =
            (ssize_t)
            (buff_index * timestretch_ratio);
          if (buff_index <
                (ssize_t) buff_index_start)
            {
              g_message (
                "buff index (%zd) < "
                "buff index start (%zd)",
                buff_index,
                buff_index_start);
              /* set the start point (
               * used when
               * timestretching) */
              buff_index_start =
                (size_t) buff_index;

              /* timestretch the material
               * up to this point */
              if (buff_size > 0)
                {
                  g_message (
                    "buff size (%zd) > 0",
                    buff_size);
                  STRETCH;
                  prev_offset = current_local_frame;
                }
              buff_size = 0;
            }
          /* else if last sample */
          else if (j == (nframes - 1))
            {
              STRETCH;
              prev_offset = current_local_frame;
            }
          else
            {
              buff_size++;
            }
        }
      /* else if no need for timestretch */
      else
        {
          z_return_if_fail_cmp (buff_index, >=, 0);
          if (G_UNLIKELY (
                buff_index >= clip->num_frames))
            {
              g_critical (
                "Buffer index %zd exceeds %ld "
                "frames in clip '%s'",
                buff_index, clip->num_frames,
                clip->name);
              return;
            }
          lbuf_after_ts[j] =
            clip->ch_frames[0][buff_index];
          rbuf_after_ts[j] =
            clip->channels == 1 ?
            clip->ch_frames[0][buff_index] :
            clip->ch_frames[1][buff_index];
        }
    }

  /* apply gain */
  if (!math_floats_equal (r->gain, 1.f))
    {
      dsp_mul_k2 (
        &lbuf_after_ts[0], r->gain, nframes);
      dsp_mul_k2 (
        &rbuf_after_ts[0], r->gain, nframes);
    }

  /* copy frames */
  dsp_copy (
    &stereo_ports->l->buf[cycle_start_offset],
    &lbuf_after_ts[0], nframes);
  dsp_copy (
    &stereo_ports->r->buf[cycle_start_offset],
    &rbuf_after_ts[0], nframes);

  /* apply fades */
  long num_frames_in_fade_in_area =
    r_obj->fade_in_pos.frames;
  long num_frames_in_fade_out_area =
    r_obj->end_pos.frames -
      (r_obj->fade_out_pos.frames +
       r_obj->pos.frames);
  for (nframes_t j = 0; j < nframes; j++)
    {
      long current_cycle_frame =
        cycle_start_offset + j;
      long current_local_frame =
        (g_start_frames + current_cycle_frame) -
        r_obj->pos.frames;

      /* skip to fade out if not in any fade area */
      if (G_LIKELY (
            current_local_frame >=
              r_obj->fade_in_pos.frames
            &&
            current_local_frame <
              r_obj->fade_out_pos.frames))
        {
          j +=
            r_obj->fade_out_pos.frames -
            current_local_frame;
          j--;
          continue;
        }

      /* if inside fade in */
      if (current_local_frame >= 0 &&
          current_local_frame <
            r_obj->fade_in_pos.frames)
        {
          z_return_if_fail_cmp (
            num_frames_in_fade_in_area, >, 0);
          z_return_if_fail_cmp (
            current_local_frame, <=,
            num_frames_in_fade_in_area);
          float fade_in =
            (float)
            fade_get_y_normalized (
              (double) current_local_frame /
              (double)
              num_frames_in_fade_in_area,
              &r_obj->fade_in_opts, 1);

          stereo_ports->l->buf[current_cycle_frame] *=
            fade_in;
          stereo_ports->r->buf[current_cycle_frame] *=
            fade_in;
        }
      /* if inside fade out */
      if (current_local_frame >=
            r_obj->fade_out_pos.frames)
        {
          z_return_if_fail_cmp (
            num_frames_in_fade_out_area, >, 0);
          long num_frames_from_fade_out_start =
            current_local_frame -
              r_obj->fade_out_pos.frames;
          z_return_if_fail_cmp (
            num_frames_from_fade_out_start, <=,
            num_frames_in_fade_out_area);
          float fade_out =
            (float)
            fade_get_y_normalized (
              (double)
              num_frames_from_fade_out_start /
              (double)
              num_frames_in_fade_out_area,
              &r_obj->fade_out_opts, 0);

          stereo_ports->l->buf[current_cycle_frame] *=
            fade_out;
          stereo_ports->r->buf[current_cycle_frame] *=
            fade_out;
        }
    }
}

float
audio_region_detect_bpm (
  ZRegion * self,
  GArray *  candidates)
{
  AudioClip * clip = audio_region_get_clip (self);
  g_return_val_if_fail (clip, 0.f);

  return
    audio_detect_bpm (
      clip->ch_frames[0], (size_t) clip->num_frames,
      (unsigned int) AUDIO_ENGINE->sample_rate,
      candidates);
}

bool
audio_region_validate (
  ZRegion * self)
{
  long loop_len =
    arranger_object_get_loop_length_in_frames (
      (ArrangerObject *) self);

  AudioClip * clip =
    audio_region_get_clip (self);
  g_return_val_if_fail (clip, false);

  /* verify that the loop does not contain more
   * frames than available in the clip */
  z_return_val_if_fail_cmp (
    loop_len, <=, clip->num_frames, false);

  return true;
}

/**
 * Frees members only but not the audio region
 * itself.
 *
 * Regions should be free'd using region_free.
 */
void
audio_region_free_members (ZRegion * self)
{
  object_free_w_func_and_null (
    audio_clip_free, self->clip);
}
