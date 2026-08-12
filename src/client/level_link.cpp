#include "level_link.hpp"

#include "constants.hpp"

level_link::level_link(const og::shared::level_link &link)
    : new_level_(link.destination),
      new_x_(link.destination_x),
      new_y_(link.destination_y),
      rectangle_{
          .x = static_cast<float>(link.x * tile_size),
          .y = static_cast<float>(link.y * tile_size),
          .width = static_cast<float>(link.width * tile_size),
          .height = static_cast<float>(link.height * tile_size),
      } {
}
