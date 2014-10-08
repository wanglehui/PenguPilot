#!/usr/bin/env python
"""
  ___________________________________________________
 |  _____                       _____ _ _       _    |
 | |  __ \                     |  __ (_) |     | |   |
 | | |__) |__ _ __   __ _ _   _| |__) || | ___ | |_  |
 | |  ___/ _ \ '_ \ / _` | | | |  ___/ | |/ _ \| __| |
 | | |  |  __/ | | | (_| | |_| | |   | | | (_) | |_  |
 | |_|   \___|_| |_|\__, |\__,_|_|   |_|_|\___/ \__| |
 |                   __/ |                           |
 |  GNU/Linux based |___/  Multi-Rotor UAV Autopilot |
 |___________________________________________________|
  
 Elevation Map Service

 Copyright (C) 2014 Tobias Simon, Integrated Communication Systems Group, TU Ilmenau

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details. """


from scl import generate_map
from misc import daemonize, RateTimer
from gps_msgpack import *
from msgpack import loads, dumps
from srtm import SrtmElevMap


def main(name):
   elev_map = SrtmElevMap()
   socket_map = generate_map(name)
   gps_socket = socket_map['gps']
   elev_socket = socket_map['elev']
   while True:
      gps = loads(gps_socket.recv())
      try:
         elev = elev_map.lookup((gps[LON], gps[LAT]))
         elev_socket.send(dumps([elev]))
      except:
         pass


daemonize('elevmap', main)
