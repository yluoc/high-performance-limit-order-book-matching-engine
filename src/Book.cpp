#include "LOB/Book.h"

Trades Book::place_order(OrderPointer& order) {
    if (order->get_order_price() <= 0) return {};

    Trades trades;
    id_to_order[order->get_order_id()] = order;

    if (order->get_order_type() == BUY) {
        while (best_sell and order->get_order_price() >= best_sell and order->get_order_status() != FULFILLED) {
            Trades trades_at_limit = sell_side_limits[best_sell]->match_order(order);
            trades.insert(trades.end(), trades_at_limit.begin(), trades_at_limit.end());
            check_for_empty_sell_limit(best_sell);
        }
    } else {
        while (best_buy and order->get_order_price() <= best_buy and order->get_order_status() != FULFILLED) {
            Trades trades_at_limit = buy_side_limits[best_buy]->match_order(order);
            trades.insert(trades.end(), trades_at_limit.begin(), trades_at_limit.end());
            check_for_empty_buy_limit(best_buy);
        }
    }

    if (order->get_order_status() != FULFILLED) { insert_order(order); }

    return trades;
}

void Book::delete_order(ID id) {
    if (not id_to_order.contains(id)) return;

    OrderPointer order = id_to_order[id];
    if (order->get_order_status() == ACTIVE) {
        delete_order(order, order->get_order_type() == BUY);
        id_to_order.erase(id);
    }
}

bool Book::is_in_buy_limits(PRICE price) {
    return buy_side_limits.contains(price);
}

bool Book::is_in_sell_limits(PRICE price) {
    return sell_side_limits.contains(price);
}

void Book::update_best_buy() {
    if (not buy_side_tree.empty()) {
        best_buy = *buy_side_tree.rbegin();
    } else {
        best_buy = 0;
    }
}

void Book::update_best_sell() {
    if (not sell_side_tree.empty()) {
        best_sell = *sell_side_tree.begin();
    } else {
        best_sell = 0;
    }
}

void Book::check_for_empty_buy_limit(PRICE price) {
    if (is_in_buy_limits(price) and buy_side_limits[price]->is_empty()) {
        buy_side_limits.erase(price);
        buy_side_tree.erase(price);
        if (price == best_buy) {
            update_best_buy();
        }
    }
}

void Book::check_for_empty_sell_limit(PRICE price) {
    if (is_in_sell_limits(price) and sell_side_limits[price]->is_empty()) {
        sell_side_limits.erase(price);
        sell_side_tree.erase(price);
        if (price == best_sell) {
            update_best_sell();
        }
    }
}

void Book::insert_order(OrderPointer& order) {
    PRICE price = order->get_order_price();
    bool is_buy = order->get_order_type() == BUY;

    LimitPointer limit = get_or_create_limit(price, is_buy);

    if (is_buy and (not best_buy or price > best_buy)) {
        best_buy = price;
    } else if (not is_buy and (not best_sell or price < best_sell)) {
        best_sell = price;
    }

    limit->insert_order(order);
}

LimitPointer Book::get_or_create_limit(PRICE price, bool is_buy) {
    LimitPointer limit;
    if (is_buy) {
        if (is_in_buy_limits(price)) {
            limit = buy_side_limits[price];
        } else {
            limit = std::make_shared<Limit>(price);
            buy_side_tree.insert(price);
            buy_side_limits[price] = limit;
        }
    } else {
        if (is_in_sell_limits(price)) {
            limit = sell_side_limits[price];
        } else {
            limit = std::make_shared<Limit>(price);
            sell_side_tree.insert(price);
            sell_side_limits[price] = limit;
        }
    }
    return limit;
}

void Book::delete_order(OrderPointer& order, bool is_buy) {
    PRICE price = order->get_order_price();
    if (is_buy) {
        if (not is_in_buy_limits(price)) return;
        buy_side_limits[price]->delete_order(order);
        check_for_empty_buy_limit(price);
    } else {
        if (not is_in_sell_limits(price)) return;
        sell_side_limits[price]->delete_order(order);
        check_for_empty_sell_limit(price);
    }
}

void Book::print() {
    for (PRICE price : buy_side_tree) {
        buy_side_limits[price]->print();
    }
    std::cout << "==== BUY SIDE ====" << std::endl;
    std::cout << "Best Buy: " << best_buy << std::endl;
    std::cout << "==== SELL SIDE ====" << std::endl;
    std::cout << "Best Sell: " << best_sell << std::endl;
    for (PRICE price : sell_side_tree) {
        sell_side_limits[price]->print();
    }
}

PRICE Book::get_spread() { return best_sell - best_buy; }

double Book::get_mid_price() { return (best_sell + best_buy) / 2.; }

PriceTree& Book::get_buy_tree() { return buy_side_tree; }
PriceLimitMap& Book::get_buy_limits() { return buy_side_limits; }
PriceTree& Book::get_sell_tree() { return sell_side_tree; }
PriceLimitMap& Book::get_sell_limits() { return sell_side_limits; }
PRICE Book::get_best_buy(){ return best_buy; }
PRICE Book::get_best_sell() { return best_sell; }
Orders& Book::get_id_to_order() { return id_to_order; }
OrderStatus Book::get_order_status(ID id) {
	if (id_to_order.contains(id))
		return id_to_order[id]->get_order_status();
	return DELETED;
}