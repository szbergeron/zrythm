/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "helpers/zrythm.h"

#include <glib.h>
#include <locale.h>

static void
test_empty_save_load ()
{
  int ret;
  g_assert_nonnull (PROJECT);

  /* save the project */
  ret =
    project_save (
      PROJECT, PROJECT->dir, 0, 0);
  g_assert_cmpint (ret, ==, 0);
  char * prj_file =
    g_build_filename (
      PROJECT->dir, PROJECT_FILE, NULL);

  /* reload it */
  ret =
    project_load (prj_file, 0);
  g_assert_cmpint (ret, ==, 0);

  /* resave it */
  ret =
    project_save (
      PROJECT, PROJECT->dir, 0, 0);
  g_assert_cmpint (ret, ==, 0);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/project/"

  g_test_add_func (
    TEST_PREFIX "test empty save load",
    (GTestFunc) test_empty_save_load);

  return g_test_run ();
}
