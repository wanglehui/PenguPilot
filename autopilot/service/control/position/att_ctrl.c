/*___________________________________________________
 |  _____                       _____ _ _       _    |
 | |  __ \                     |  __ (_) |     | |   |
 | | |__) |__ _ __   __ _ _   _| |__) || | ___ | |_  |
 | |  ___/ _ \ '_ \ / _` | | | |  ___/ | |/ _ \| __| |
 | | |  |  __/ | | | (_| | |_| | |   | | | (_) | |_  |
 | |_|   \___|_| |_|\__, |\__,_|_|   |_|_|\___/ \__| |
 |                   __/ |                           |
 |  GNU/Linux based |___/  Multi-Rotor UAV Autopilot |
 |___________________________________________________|
  
 Attitude Controller Implementation

 Copyright (C) 2014 Tobias Simon, Integrated Communication Systems Group, TU Ilmenau

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details. */


#include <stdint.h>
#include <opcd_interface.h>
#include <threadsafe_types.h>
#include <util.h>

#include "att_ctrl.h"
#include "../util/pid.h"
#include "../../util/math/conv.h"
#include "../../filters/filter.h"


static tsfloat_t angle_p;
static tsfloat_t angle_i;
static tsfloat_t angle_i_max;
static tsfloat_t angle_d;
static tsfloat_t biases[2];
static tsfloat_t angles_max;
static pid_controller_t controllers[2];


void att_ctrl_init(void)
{
   ASSERT_ONCE();

   /* load parameters: */
   opcd_param_t params[] =
   {
      {"p", &angle_p.value},
      {"i", &angle_i.value},
      {"i_max", &angle_i_max.value},
      {"d", &angle_d.value},
      {"pitch_bias", &biases[0]},
      {"roll_bias", &biases[1]},
      {"angles_max", &angles_max},
      OPCD_PARAMS_END
   };
   opcd_params_apply("controllers.attitude.", params);
   
   /* initialize controllers: */
   FOR_EACH(i, controllers)
      pid_init(&controllers[i], &angle_p, &angle_i, &angle_d, &angle_i_max);
}


void att_ctrl_reset(void)
{
   FOR_EACH(i, controllers)
      pid_reset(&controllers[i]);
}


void att_ctrl_step(vec2_t *ctrl, vec2_t *err, const float dt, const vec2_t *pos, const vec2_t *speed, const vec2_t *setp)
{
   FOR_EACH(i, controllers)
   {
      err->ve[i] =   sym_limit(setp->ve[i], deg2rad(tsfloat_get(&angles_max))) 
                   + deg2rad(tsfloat_get(&biases[i])) - pos->ve[i];
      ctrl->ve[i] = pid_control(&controllers[i], err->ve[i], speed->ve[i], dt);
   }
}

