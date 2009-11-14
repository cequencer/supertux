//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __GROWUP_H__
#define __GROWUP_H__

#include "object/moving_sprite.hpp"
#include "supertux/physic.hpp"
#include "supertux/direction.hpp"

class GrowUp : public MovingSprite, private UsesPhysic
{
public:
  GrowUp(Direction direction = RIGHT);
  virtual GrowUp* clone() const { return new GrowUp(*this); }

  virtual void update(float elapsed_time);
  virtual void collision_solid(const CollisionHit& hit);
  virtual HitResponse collision(GameObject& other, const CollisionHit& hit);
  void do_jump();
};

#endif
