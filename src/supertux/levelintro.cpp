//  SuperTux -- LevelIntro screen
//  Copyright (C) 2008 Christoph Sommer <christoph.sommer@2008.expires.deltadevelopment.de>
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
#include <config.h>

#include "supertux/levelintro.hpp"

#include <stdexcept>
#include "util/log.hpp"
#include "supertux/mainloop.hpp"
#include "util/gettext.hpp"
#include "supertux/resources.hpp"
#include "video/drawing_context.hpp"
#include "gui/menu.hpp"
#include "supertux/main.hpp"
#include "supertux/fadeout.hpp"
#include "control/joystickkeyboardcontroller.hpp"
#include "sprite/sprite_manager.hpp"
#include "math/random_generator.hpp"

LevelIntro::LevelIntro(const Level* level, const Statistics* best_level_statistics)
        : level(level), best_level_statistics(best_level_statistics), player_sprite_py(0), player_sprite_vy(0)
{
  player_sprite.reset(sprite_manager->create("images/creatures/tux/tux.sprite"));
  player_sprite->set_action("small-walk-right");
  player_sprite_jump_timer.start(systemRandom.randf(5,10));
}

LevelIntro::~LevelIntro()
{
}

void
LevelIntro::setup()
{
}

void
LevelIntro::update(float elapsed_time)
{

  // Check if it's time to exit the screen
  if(main_controller->pressed(Controller::JUMP)
      || main_controller->pressed(Controller::ACTION)
      || main_controller->pressed(Controller::MENU_SELECT)
      || main_controller->pressed(Controller::PAUSE_MENU)) {
    main_loop->exit_screen(new FadeOut(0.1));
  }

  player_sprite_py += player_sprite_vy * elapsed_time;
  player_sprite_vy += 1000 * elapsed_time;
  if (player_sprite_py >= 0) {
    player_sprite_py = 0;
    player_sprite_vy = 0;
  }
  if (player_sprite_jump_timer.check()) {
    player_sprite_vy = -300;
    player_sprite_jump_timer.start(systemRandom.randf(2,3));
  }
  
}

void
LevelIntro::draw(DrawingContext& context)
{
  const Statistics& stats = level->stats;
  int py = static_cast<int>(SCREEN_HEIGHT / 2 - normal_font->get_height() / 2);

  context.draw_filled_rect(Vector(0, 0), Vector(SCREEN_WIDTH, SCREEN_HEIGHT), Color(0.0f, 0.0f, 0.0f, 1.0f), 0);

  {
    context.draw_center_text(normal_font, level->get_name(), Vector(0, py), LAYER_FOREGROUND1, LevelIntro::header_color);
    py += static_cast<int>(normal_font->get_height());
  }

  std::string author = level->get_author();
  if ((author != "") && (author != "SuperTux Team")) {
    std::string author_text = std::string(_("contributed by ")) + author;
    context.draw_center_text(small_font, author_text, Vector(0, py), LAYER_FOREGROUND1, LevelIntro::author_color);
    py += static_cast<int>(small_font->get_height());
  }

  py += 32;

  {
    player_sprite->draw(context, Vector((SCREEN_WIDTH - player_sprite->get_current_hitbox_width()) / 2, py + player_sprite_py), LAYER_FOREGROUND1);
    py += static_cast<int>(player_sprite->get_current_hitbox_height());
  }

  py += 32;

  {
    context.draw_center_text(normal_font, std::string("- ") + _("Best Level Statistics") + std::string(" -"), Vector(0, py), LAYER_FOREGROUND1, LevelIntro::stat_hdr_color);
    py += static_cast<int>(normal_font->get_height());
  }

  {
    std::stringstream ss;
    ss << _("Coins") << ": " << Statistics::coins_to_string((best_level_statistics && (best_level_statistics->coins >= 0)) ? best_level_statistics->coins : 0, stats.total_coins);
    context.draw_center_text(normal_font, ss.str(), Vector(0, py), LAYER_FOREGROUND1, LevelIntro::stat_color);
    py += static_cast<int>(normal_font->get_height());
  }
  
  {
    std::stringstream ss;
    ss << _("Secrets") << ": " << Statistics::secrets_to_string((best_level_statistics && (best_level_statistics->coins >= 0)) ? best_level_statistics->secrets : 0, stats.total_secrets);
    context.draw_center_text(normal_font, ss.str(), Vector(0, py), LAYER_FOREGROUND1,LevelIntro::stat_color);
    py += static_cast<int>(normal_font->get_height());
  }

  {
    std::stringstream ss;
    ss << _("Time") << ": " << Statistics::time_to_string((best_level_statistics && (best_level_statistics->coins >= 0)) ? best_level_statistics->time : 0);
    context.draw_center_text(normal_font, ss.str(), Vector(0, py), LAYER_FOREGROUND1,LevelIntro::stat_color);
    py += static_cast<int>(normal_font->get_height());
  }

}
