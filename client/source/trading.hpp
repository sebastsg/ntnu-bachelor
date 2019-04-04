#pragma once

#include "item.hpp"

class game_state;
class context_menu;

void start_trading(game_state& game, int trader_id);
bool is_trading();
void notify_trade_decision(bool accepted);
void add_trade_item(int which, item_instance item);
void remove_trade_item(int which, no::vector2i slot);
void add_trading_context_options(context_menu& context);
void update_trading_ui();
void draw_trading_ui();
void stop_trading();
