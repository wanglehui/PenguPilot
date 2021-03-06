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
  
 Raspberry Pi Quadrotor Platform

 Copyright (C) 2014 Tobias Simon, Integrated Communication Systems Group, TU Ilmenau
 Copyright (C) 2013 Alexander Barth, Control Engineering Group, TU Ilmenau
 Copyright (C) 2013 Benjamin Jahn, Control Engineering Group, TU Ilmenau

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details. */


#include <stddef.h>
#include <unistd.h>
#include <malloc.h>
#include <math.h>

#include <logger.h>
#include <util.h>
#include <i2c/i2c.h>

#include "platform.h"
#include "generic_platform.h"
#include "inv_coupling.h"
#include "drotek_9150.h"
#include "force_to_esc.h"
#include "../sensors/i2cxl/i2cxl_reader.h"
#include "../sensors/ms5611/ms5611_reader.h"
#include "../actuators/afroi2c_pwms/afroi2c_pwms.h"
#include "../util/math/quat.h"


#define N_MOTORS 4


static i2c_bus_t i2c_bus;
static uint8_t motors_map[N_MOTORS] = {0, 1, 2, 3};
static drotek_9150_t marg;


static int read_marg(marg_data_t *marg_data)
{
   return drotek_9150_read(marg_data, &marg);
}


int pi_quad_init(platform_t *plat, int override_hw)
{
   ASSERT_ONCE();
   THROW_BEGIN();

   float rpm_min = 1340.99f;
   float rpm_max = 7107.6f;
   plat->rpm_square_min = rpm_min * rpm_min;
   plat->rpm_square_max = rpm_max * rpm_max;

   /* arm length */
   const float length = 0.2025f;

   /* F = c * rpm ^ 2 */
   const float c = 1.5866e-007f;

   /* tau = d * rpm ^ 2 */
   const float d = 4.0174e-009f;

   const float imtx1 = 1.0f / (4.0f * c);
   const float imtx2 = 1.0f / (2.0f * c * length);
   const float imtx3 = 1.0f / (4.0f * d);

   plat->imtx1 = imtx1;
   plat->imtx2 = imtx2;
   plat->imtx3 = imtx3;

   /* inverse coupling matrix for ARCADE quadrotor: */
   const float icmatrix[4 * N_MOTORS] =
   {         /* gas     roll    pitch    yaw */
      /* m1 */ imtx1,   0.0f, -imtx2, -imtx3,
      /* m2 */ imtx1,   0.0f,  imtx2, -imtx3,
      /* m4 */ imtx1,  imtx2,   0.0f,  imtx3,
      /* m3 */ imtx1, -imtx2,   0.0f,  imtx3
   };

   LOG(LL_INFO, "ic matrix [gas pitch roll yaw]");
   FOR_N(i, 4)
   {
      LOG(LL_INFO, "[m%d]: %.0f, %.0f, %.0f, %.0f", i,
          icmatrix[i*4], icmatrix[i*4+1],
          icmatrix[i*4+2], icmatrix[i*4+3]);
   }
   LOG(LL_INFO, "setting platform parameters");
   
  
   plat->max_thrust_n = 40.0f;
   plat->mass_kg = 1.125f;
   plat->n_motors = N_MOTORS;

   LOG(LL_INFO, "initializing inverse coupling matrix");
   inv_coupling_init(N_MOTORS, icmatrix);
      
   THROW_ON_ERR(generic_platform_init(plat));

   if (!override_hw)
   {
      LOG(LL_INFO, "initializing i2c bus");
      THROW_ON_ERR(i2c_bus_open(&i2c_bus, "/dev/i2c-1"));
      plat->priv = &i2c_bus;

      LOG(LL_INFO, "initializing MARG sensor cluster");
      THROW_ON_ERR(drotek_9150_init(&marg, &i2c_bus));
      plat->read_marg = read_marg;
 
      LOG(LL_INFO, "initializing i2cxl sonar sensor");
      THROW_ON_ERR(i2cxl_reader_init(&i2c_bus));
      plat->read_ultra = i2cxl_reader_get_alt;
      
      LOG(LL_INFO, "initializing ms5611 barometric pressure sensor");
      THROW_ON_ERR(ms5611_reader_init(&i2c_bus));
      plat->read_baro = ms5611_reader_get_alt;
   
      LOG(LL_INFO, "initializing motors via afroi2c bridge");
      THROW_ON_ERR(afroi2c_pwms_init(&i2c_bus, motors_map, N_MOTORS));
      plat->write_motors = afroi2c_pwms_write;
      ac_init(&plat->ac, 0.1f, 1.0f, 12.0f, 17.0f, c, N_MOTORS, force_to_esc_setup3, 0.0f);
   }

   LOG(LL_INFO, "pi_quadro platform initialized");
   THROW_END();
}

