//
// C Implementation: player/tux
//
// Description:
//
//
// Author: Tobias Glaesser <tobi.web@gmx.de> & Bill Kendrick, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <math.h>

#include "gameloop.h"
#include "globals.h"
#include "player.h"
#include "defines.h"
#include "scene.h"
#include "tile.h"
#include "screen.h"

texture_type tux_life;
std::vector<texture_type> tux_right;
std::vector<texture_type> tux_left;
texture_type smalltux_jump_left;
texture_type smalltux_jump_right;
texture_type smalltux_stand_left;
texture_type smalltux_stand_right;

texture_type bigtux_right[3];
texture_type bigtux_left[3];
texture_type bigtux_right_jump;
texture_type bigtux_left_jump;
texture_type ducktux_right;
texture_type ducktux_left;
texture_type skidtux_right;
texture_type skidtux_left;
texture_type firetux_right[3];
texture_type firetux_left[3];
texture_type bigfiretux_right[3];
texture_type bigfiretux_left[3];
texture_type bigfiretux_right_jump;
texture_type bigfiretux_left_jump;
texture_type duckfiretux_right;
texture_type duckfiretux_left;
texture_type skidfiretux_right;
texture_type skidfiretux_left;
texture_type cape_right[2];
texture_type cape_left[2];
texture_type bigcape_right[2];
texture_type bigcape_left[2];

void player_input_init(player_input_type* pplayer_input)
{
  pplayer_input->down = UP;
  pplayer_input->fire = UP;
  pplayer_input->left = UP;
  pplayer_input->old_fire = UP;
  pplayer_input->right = UP;
  pplayer_input->up = UP;
}

void
Player::init()
{
  base.width = 32;
  base.height = 32;

  size = SMALL;
  got_coffee = false;

  // FIXME: Make the start position configurable via the levelfile
  base.x = 100;
  base.y = 240;
  base.xm = 0;
  base.ym = 0;
  old_base = base;
  dir = RIGHT;
  duck = false;

  dying   = DYING_NOT;
  jumping = false;

  frame_main = 0;
  frame_ = 0;
  lives = 3;
  score = 0;
  distros = 0;

  player_input_init(&input);

  keymap.jump  = SDLK_UP;
  keymap.duck  = SDLK_DOWN;
  keymap.left  = SDLK_LEFT;
  keymap.right = SDLK_RIGHT;
  keymap.fire  = SDLK_LCTRL;

  timer_init(&invincible_timer,true);
  timer_init(&skidding_timer,true);
  timer_init(&safe_timer,true);
  timer_init(&frame_timer,true);

  physic.reset();
}

int
Player::key_event(SDLKey key, int state)
{
  if(key == keymap.right)
    {
      input.right = state;
      return true;
    }
  else if(key == keymap.left)
    {
      input.left = state;
      return true;
    }
  else if(key == keymap.jump)
    {
      input.up = state;
      return true;
    }
  else if(key == keymap.duck)
    {
      input.down = state;
      return true;
    }
  else if(key == keymap.fire)
    {
      input.fire = state;
      return true;
    }
  else
    return false;
}

void
Player::level_begin()
{
  base.x  = 100;
  base.y  = 240;
  base.xm = 0;
  base.ym = 0;
  old_base = base;
  previous_base = base;

  dying = DYING_NOT;

  player_input_init(&input);

  timer_init(&invincible_timer,true);
  timer_init(&skidding_timer,true);
  timer_init(&safe_timer,true);
  timer_init(&frame_timer,true);
  physic.reset();
}

void
Player::action()
{
  bool jumped_in_solid = false;

  /* --- HANDLE TUX! --- */

  if(!dying)
    handle_input();

  /* Move tux: */
  previous_base = base;

  physic.apply(base.x, base.y);

  if (!dying)
    {

      collision_swept_object_map(&old_base,&base);
      keep_in_bounds();

      /* Land: */


      if( !on_ground())
        {
          physic.enable_gravity(true);
          if(under_solid())
            {
              // fall down
              physic.set_velocity(physic.get_velocity_x(), 0);
              jumped_in_solid = true;
            }
        }
      else
        {
          /* Land: */
          if (physic.get_velocity_y() < 0)
            {
              base.y = (int)(((int)base.y / 32) * 32);
              physic.set_velocity(physic.get_velocity_x(), 0);
            }

          physic.enable_gravity(false);
          /* Reset score multiplier (for multi-hits): */
          player_status.score_multiplier = 1;
        }

      if(jumped_in_solid)
        {
          if (isbrick(base.x, base.y) ||
              isfullbox(base.x, base.y))
            {
              World::current()->trygrabdistro(base.x, base.y - 32,BOUNCE);
              World::current()->trybumpbadguy(base.x, base.y - 64);

              World::current()->trybreakbrick(base.x, base.y, size == SMALL);

              bumpbrick(base.x, base.y);
              World::current()->tryemptybox(base.x, base.y, RIGHT);
            }

          if (isbrick(base.x+ 31, base.y) ||
              isfullbox(base.x+ 31, base.y))
            {
              World::current()->trygrabdistro(base.x+ 31, base.y - 32,BOUNCE);
              World::current()->trybumpbadguy(base.x+ 31, base.y - 64);

              if(size == BIG)
                World::current()->trybreakbrick(base.x+ 31, base.y, size == SMALL);

              bumpbrick(base.x+ 31, base.y);
              World::current()->tryemptybox(base.x+ 31, base.y, LEFT);
            }
        }

      grabdistros();

      if (jumped_in_solid)
        {
          ++base.y;
          ++old_base.y;
          if(on_ground())
            {
              /* Make sure jumping is off. */
              jumping = false;
            }
        }

    }

  timer_check(&safe_timer);


  /* ---- DONE HANDLING TUX! --- */

  /* Handle invincibility timer: */


  if (get_current_music() == HERRING_MUSIC && !timer_check(&invincible_timer))
    {
      /*
         no, we are no more invincible
         or we were not in invincible mode
         but are we in hurry ?
       */


      if (timer_get_left(&time_left) < TIME_WARNING)
        {
          /* yes, we are in hurry
             stop the herring_song, prepare to play the correct
             fast level_song !
           */
          set_current_music(HURRYUP_MUSIC);
        }
      else
        {
          set_current_music(LEVEL_MUSIC);
        }

      /* start playing it */
      play_current_music();
    }

  /* Handle skidding: */

  // timer_check(&skidding_timer); // disabled

  /* End of level? */
  if (base.x >= World::current()->get_level()->endpos
      && World::current()->get_level()->endpos != 0)
    {
      player_status.next_level = 1;
    }

}

bool
Player::on_ground()
{
  return ( issolid(base.x + base.width / 2, base.y + base.height) ||
           issolid(base.x + 1, base.y + base.height) ||
           issolid(base.x + base.width - 1, base.y + base.height)  );
}

bool
Player::under_solid()
{
  return ( issolid(base.x + base.width / 2, base.y) ||
           issolid(base.x + 1, base.y) ||
           issolid(base.x + base.width - 1, base.y)  );
}

void
Player::handle_horizontal_input(int newdir)
{
  if(duck)
    return;

  float vx = physic.get_velocity_x();
  float vy = physic.get_velocity_y();
  dir = newdir;

  // skid if we're too fast
  if(dir != newdir && on_ground() && fabs(physic.get_velocity_x()) > SKID_XM 
          && !timer_started(&skidding_timer))
    {
      timer_start(&skidding_timer, SKID_TIME);
      play_sound(sounds[SND_SKID], SOUND_CENTER_SPEAKER);
      return;
    }

  if ((newdir ? (vx < 0) : (vx > 0)) && !isice(base.x, base.y + base.height) &&
      !timer_started(&skidding_timer))
    {
      //vx = 0;
    }

  /* Facing the direction we're jumping?  Go full-speed: */
  if (input.fire == UP)
    {
      if(vx >= MAX_WALK_XM) {
        vx = MAX_WALK_XM;
        physic.set_acceleration(0, 0); // enough speedup
      } else if(vx <= -MAX_WALK_XM) {
        vx = -MAX_WALK_XM;
        physic.set_acceleration(0, 0);
      }
      physic.set_acceleration(newdir ? 0.02 : -0.02, 0);
      if(fabs(vx) < 1) // set some basic run speed
        vx = newdir ? 1 : -1;
#if 0
      vx += ( newdir ? WALK_SPEED : -WALK_SPEED) * frame_ratio;

      if(newdir)
        {
          if (vx > MAX_WALK_XM)
            vx = MAX_WALK_XM;
        }
      else
        {
          if (vx < -MAX_WALK_XM)
            vx = -MAX_WALK_XM;
        }
#endif
    }
  else if ( input.fire == DOWN)
    {
      if(vx >= MAX_RUN_XM) {
        vx = MAX_RUN_XM;
        physic.set_acceleration(0, 0); // enough speedup      
      } else if(vx <= -MAX_RUN_XM) {
        vx = -MAX_RUN_XM;
        physic.set_acceleration(0, 0);
      }
      physic.set_acceleration(newdir ? 0.03 : -0.03, 0);
      if(fabs(vx) < 1) // set some basic run speed
        vx = newdir ? 1 : -1;

#if 0
      vx = vx + ( newdir ? RUN_SPEED : -RUN_SPEED) * frame_ratio;

      if(newdir)
        {
          if (vx > MAX_RUN_XM)
            vx = MAX_RUN_XM;
        }
      else
        {
          if (vx < -MAX_RUN_XM)
            vx = -MAX_RUN_XM;
        }
#endif
    }
  else
    {
#if 0
      /* Not facing the direction we're jumping?
         Go half-speed: */
      vx = vx + ( newdir ? (WALK_SPEED / 2) : -(WALK_SPEED / 2)) * frame_ratio;

      if(newdir)
        {
          if (vx > MAX_WALK_XM / 2)
            vx = MAX_WALK_XM / 2;
        }
      else
        {
          if (vx < -MAX_WALK_XM / 2)
            vx = -MAX_WALK_XM / 2;
        }
#endif
    }
  
  physic.set_velocity(vx, vy);
}

void
Player::handle_vertical_input()
{
  if(input.up == DOWN)
    {
      if (on_ground())
        {
          // jump
          physic.set_velocity(physic.get_velocity_x(), 5.5);
          --base.y;
          jumping = true;
          if (size == SMALL)
            play_sound(sounds[SND_JUMP], SOUND_CENTER_SPEAKER);
          else
            play_sound(sounds[SND_BIGJUMP], SOUND_CENTER_SPEAKER);
        }
    }
  else if(input.up == UP && jumping)
    {
      jumping = false;
      if(physic.get_velocity_y() > 0) {
        physic.set_velocity(physic.get_velocity_x(), 0);
      }
    }
}

void
Player::handle_input()
{
  /* Handle key and joystick state: */
  if(duck == false)
    {
      if (input.right == DOWN && input.left == UP)
        {
          handle_horizontal_input(RIGHT);
        }
      else if (input.left == DOWN && input.right == UP)
        {
          handle_horizontal_input(LEFT);
        }
      else
        {
          float vx = physic.get_velocity_x();
          if(fabs(vx) < 0.01) {
            physic.set_velocity(0, physic.get_velocity_y());
            physic.set_acceleration(0, 0);
          } else if(vx < 0) {
            physic.set_acceleration(0.1, 0);
          } else {
            physic.set_acceleration(-0.1, 0);
          }
        }
    }

  /* Jump/jumping? */

  if ( input.up == DOWN || (input.up == UP && jumping))
    {
      handle_vertical_input();
    }

  /* Shoot! */

  if (input.fire == DOWN && input.old_fire == UP && got_coffee)
    {
      World::current()->add_bullet(base.x, base.y, physic.get_velocity_x(), dir);
    }


  /* Duck! */

  if (input.down == DOWN)
    {
      if (size == BIG && duck != true)
        {
          duck = true;
          base.height = 32;
          base.y += 32;
        }
    }
  else
    {
      if (size == BIG && duck)
        {
          /* Make sure we're not standing back up into a solid! */
          base.height = 64;
          base.y -= 32;

          if (!collision_object_map(&base) /*issolid(base.x + 16, base.y - 16)*/)
            {
              duck = false;
              base.height = 64;
              old_base.y -= 32;
              old_base.height = 64;
            }
          else
            {
              base.height = 32;
              base.y += 32;
            }
        }
      else
        {
          duck = false;
        }
    }

  /* (Tux): */

  if(!timer_check(&frame_timer))
    {
      timer_start(&frame_timer,25);
      if (input.right == UP && input.left == UP)
        {
          frame_main = 1;
          frame_ = 1;
        }
      else
        {
          if ((input.fire == DOWN && (global_frame_counter % 2) == 0) ||
              (global_frame_counter % 4) == 0)
            frame_main = (frame_main + 1) % 4;

          frame_ = frame_main;

          if (frame_ == 3)
            frame_ = 1;
        }
    }

}

void
Player::grabdistros()
{
  /* Grab distros: */
  if (!dying)
    {
      World::current()->trygrabdistro(base.x, base.y, NO_BOUNCE);
      World::current()->trygrabdistro(base.x+ 31, base.y, NO_BOUNCE);

      World::current()->trygrabdistro(base.x, base.y + base.height, NO_BOUNCE);
      World::current()->trygrabdistro(base.x+ 31, base.y + base.height, NO_BOUNCE);

      if(size == BIG)
        {
          World::current()->trygrabdistro(base.x, base.y + base.height / 2, NO_BOUNCE);
          World::current()->trygrabdistro(base.x+ 31, base.y + base.height / 2, NO_BOUNCE);
        }

    }

  /* Enough distros for a One-up? */
  if (distros >= DISTROS_LIFEUP)
    {
      distros = distros - DISTROS_LIFEUP;
      if(lives < MAX_LIVES)
        lives++;
      /*We want to hear the sound even, if MAX_LIVES is reached*/
      play_sound(sounds[SND_LIFEUP], SOUND_CENTER_SPEAKER);
    }
}

void
Player::draw()
{
  if (!timer_started(&safe_timer) || (global_frame_counter % 2) == 0)
    {
      if (size == SMALL)
        {
          if (timer_started(&invincible_timer))
            {
              /* Draw cape: */

              if (dir == RIGHT)
                {
                  texture_draw(&cape_right[global_frame_counter % 2],
                               base.x- scroll_x, base.y);
                }
              else
                {
                  texture_draw(&cape_left[global_frame_counter % 2],
                               base.x- scroll_x, base.y);
                }
            }


          if (!got_coffee)
            {
              if (physic.get_velocity_y() != 0)
                {
                  if (dir == RIGHT)
                    texture_draw(&smalltux_jump_right, base.x - scroll_x, base.y - 10);
                  else
                    texture_draw(&smalltux_jump_left, base.x - scroll_x, base.y - 10);                   
                }
              else
                {
                  if (fabsf(physic.get_velocity_x()) < 1.0f) // standing
                    {
                      if (dir == RIGHT)
                        texture_draw(&smalltux_stand_right, base.x - scroll_x, base.y - 9);
                      else
                        texture_draw(&smalltux_stand_left, base.x - scroll_x, base.y - 9);
                    }
                  else // moving
                    {
                      if (dir == RIGHT)
                        texture_draw(&tux_right[(global_frame_counter/2) % tux_right.size()], 
                                     base.x - scroll_x, base.y - 9);
                      else
                        texture_draw(&tux_left[(global_frame_counter/2) % tux_left.size()], 
                                     base.x - scroll_x, base.y - 9);
                    }
                }
            }
          else
            {
              /* Tux got coffee! */

              if (dir == RIGHT)
                {
                  texture_draw(&firetux_right[frame_], base.x- scroll_x, base.y);
                }
              else
                {
                  texture_draw(&firetux_left[frame_], base.x- scroll_x, base.y);
                }
            }
        }
      else
        {
          if (timer_started(&invincible_timer))
            {
              /* Draw cape: */
              if (dir == RIGHT)
                {
                  texture_draw(&bigcape_right[global_frame_counter % 2],
                               base.x- scroll_x - 8, base.y);
                }
              else
                {
                  texture_draw(&bigcape_left[global_frame_counter % 2],
                               base.x-scroll_x - 8, base.y);
                }
            }

          if (!got_coffee)
            {
              if (!duck)
                {
                  if (!timer_started(&skidding_timer))
                    {
                      if (!jumping || physic.get_velocity_y() > 0)
                        {
                          if (dir == RIGHT)
                            {
                              texture_draw(&bigtux_right[frame_],
                                           base.x- scroll_x - 8, base.y);
                            }
                          else
                            {
                              texture_draw(&bigtux_left[frame_],
                                           base.x- scroll_x - 8, base.y);
                            }
                        }
                      else
                        {
                          if (dir == RIGHT)
                            {
                              texture_draw(&bigtux_right_jump,
                                           base.x- scroll_x - 8, base.y);
                            }
                          else
                            {
                              texture_draw(&bigtux_left_jump,
                                           base.x- scroll_x - 8, base.y);
                            }
                        }
                    }
                  else
                    {
                      if (dir == RIGHT)
                        {
                          texture_draw(&skidtux_right,
                                       base.x- scroll_x - 8, base.y);
                        }
                      else
                        {
                          texture_draw(&skidtux_left,
                                       base.x- scroll_x - 8, base.y);
                        }
                    }
                }
              else
                {
                  if (dir == RIGHT)
                    {
                      texture_draw(&ducktux_right, base.x- scroll_x - 8, base.y - 16);
                    }
                  else
                    {
                      texture_draw(&ducktux_left, base.x- scroll_x - 8, base.y - 16);
                    }
                }
            }
          else
            {
              /* Tux has coffee! */

              if (!duck)
                {
                  if (!timer_started(&skidding_timer))
                    {
                      if (!jumping || physic.get_velocity_y() > 0)
                        {
                          if (dir == RIGHT)
                            {
                              texture_draw(&bigfiretux_right[frame_],
                                           base.x- scroll_x - 8, base.y);
                            }
                          else
                            {
                              texture_draw(&bigfiretux_left[frame_],
                                           base.x- scroll_x - 8, base.y);
                            }
                        }
                      else
                        {
                          if (dir == RIGHT)
                            {
                              texture_draw(&bigfiretux_right_jump,
                                           base.x- scroll_x - 8, base.y);
                            }
                          else
                            {
                              texture_draw(&bigfiretux_left_jump,
                                           base.x- scroll_x - 8, base.y);
                            }
                        }
                    }
                  else
                    {
                      if (dir == RIGHT)
                        {
                          texture_draw(&skidfiretux_right,
                                       base.x- scroll_x - 8, base.y);
                        }
                      else
                        {
                          texture_draw(&skidfiretux_left,
                                       base.x- scroll_x - 8, base.y);
                        }
                    }
                }
              else
                {
                  if (dir == RIGHT)
                    {
                      texture_draw(&duckfiretux_right, base.x- scroll_x - 8, base.y - 16);
                    }
                  else
                    {
                      texture_draw(&duckfiretux_left, base.x- scroll_x - 8, base.y - 16);
                    }
                }
            }
        }
    }

  if(dying)
    text_drawf(&gold_text,"Penguins can fly !:",0,0,A_HMIDDLE,A_VMIDDLE,1);
}

void
Player::collision(void* p_c_object, int c_object)
{
  BadGuy* pbad_c = NULL;

  switch (c_object)
    {
    case CO_BADGUY:
      pbad_c = (BadGuy*) p_c_object;
      /* Hurt the player if he just touched it: */

      if (!pbad_c->dying && !dying &&
          !timer_started(&safe_timer) &&
          pbad_c->mode != HELD)
        {
          if (pbad_c->mode == FLAT && input.fire == DOWN)
            {
              pbad_c->mode = HELD;
              pbad_c->base.y-=8;
            }
          else if (pbad_c->mode == KICK)
            {
              if (base.y < pbad_c->base.y - 16)
                {
                  /* Step on (stop being kicked) */

                  pbad_c->mode = FLAT;
                  play_sound(sounds[SND_STOMP], SOUND_CENTER_SPEAKER);
                }
              else
                {
                  /* Hurt if you get hit by kicked laptop: */
                  if (!timer_started(&invincible_timer))
                    {
                      kill(SHRINK);
                    }
                  else
                    {
                      pbad_c->dying = DYING_FALLING;
                      play_sound(sounds[SND_FALL], SOUND_CENTER_SPEAKER);
                      World::current()->add_score(pbad_c->base.x - scroll_x,
                                                  pbad_c->base.y,
                                                  25 * player_status.score_multiplier);
                    }
                }
            }
          else
            {
              if (!timer_started(&invincible_timer ))
                {
                  kill(SHRINK);
                }
              else
                {
                  pbad_c->kill_me();
                }
            }
          player_status.score_multiplier++;
        }
      break;
    default:
      break;
    }

}

/* Kill Player! */

void
Player::kill(int mode)
{
  play_sound(sounds[SND_HURT], SOUND_CENTER_SPEAKER);

  physic.set_velocity(0, physic.get_velocity_y());

  if (mode == SHRINK && size == BIG)
    {
      if (got_coffee)
        got_coffee = false;

      size = SMALL;
      base.height = 32;

      timer_start(&safe_timer,TUX_SAFE_TIME);
    }
  else
    {
      if(size == BIG)
        duck = true;

      physic.enable_gravity(true);
      physic.set_acceleration(0, 0);
      physic.set_velocity(0, 7);
      dying = DYING_SQUISHED;
    }
}

void
Player::is_dying()
{
  /* He died :^( */

  --lives;
  remove_powerups();
  dying = DYING_NOT;
}

bool Player::is_dead()
{
  if(base.y > screen->h)
    return true;
  else
    return false;
}

/* Remove Tux's power ups */
void
Player::remove_powerups()
{
  got_coffee = false;
  size = SMALL;
  base.height = 32;
}

void
Player::keep_in_bounds()
{
  Level* plevel = World::current()->get_level();

  /* Keep tux in bounds: */
  if (base.x< 0)
    base.x= 0;
  else if(base.x< scroll_x)
    base.x= scroll_x;
  else if (base.x< 160 + scroll_x && scroll_x > 0 && debug_mode)
    {
      scroll_x = base.x- 160;
      /*base.x+= 160;*/

      if(scroll_x < 0)
        scroll_x = 0;

    }
  else if (base.x > screen->w / 2 + scroll_x
           && scroll_x < ((World::current()->get_level()->width * 32) - screen->w))
    {
      // FIXME: Scrolling needs to be handled by a seperate View
      // class, doing it as a player huck is ugly

      // Scroll the screen in past center:
      scroll_x = base.x - screen->w / 2;

      if (scroll_x > ((plevel->width * 32) - screen->w))
        scroll_x = ((plevel->width * 32) - screen->w);
    }
  else if (base.x> 608 + scroll_x)
    {
      /* ... unless there's no more to scroll! */

      /*base.x= 608 + scroll_x;*/
    }

  /* Keep in-bounds, vertically: */

  if (base.y > screen->h)
    {
      kill(KILL);
    }
}
