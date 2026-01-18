#include "LOB/Book.h"
#include <algorithm>
#include <iostream>
#include <limits>

Book::Book(size_t initial_capacity)
    : best_bid(nullptr),
      best_ask(nullptr),
      order_pool(initial_capacity),
      level_pool(initial_capacity / 16) { // Fewer levels than orders
    trade_buffer.reserve(TRADE_BUFFER_SIZE);
    
    // Reserve hash table buckets for better performance
    buy_side_limits.reserve(256);
    sell_side_limits.reserve(256);
    id_to_order.reserve(initial_capacity);
}

const Trades& Book::place_order(
    ID order_id,
    ID agent_id,
    OrderType order_type,
    PRICE price,
    Volume volume
) {
    // Clear trade buffer
    trade_buffer.clear();
    
    // Validate price
    if (price <= 0 || volume == 0) {
        return trade_buffer;
    }
    
    // Allocate order from pool
    Order* order = order_pool.allocate(
        order_id, agent_id, order_type, price, volume, volume, ACTIVE
    );
    
    // Match against opposite side
    if (order_type == BUY) {
        // Match against best ask
        while (best_ask && price >= best_ask->get_price() && !order->is_fulfilled()) {
            bool level_empty = match_against_level(order, best_ask);
            if (level_empty) {
                PRICE empty_price = best_ask->get_price();
                level_pool.deallocate(best_ask);
                sell_side_limits.erase(empty_price);
                sell_prices.erase(empty_price);
                update_best_ask();
            }
        }
    } else {
        // Match against best bid
        while (best_bid && price <= best_bid->get_price() && !order->is_fulfilled()) {
            bool level_empty = match_against_level(order, best_bid);
            if (level_empty) {
                PRICE empty_price = best_bid->get_price();
                level_pool.deallocate(best_bid);
                buy_side_limits.erase(empty_price);
                buy_prices.erase(empty_price);
                update_best_bid();
            }
        }
    }
    
    // Insert as resting order if not fulfilled
    if (!order->is_fulfilled()) {
        insert_resting_order(order);
    } else {
        // Order was fully matched, return to pool
        order_pool.deallocate(order);
    }
    
    return trade_buffer;
}

bool Book::match_against_level(Order* incoming_order, Level* level) {
    if (!incoming_order || !level || level->is_empty()) {
        return false;
    }
    
    // Match in FIFO order (head to tail)
    while (level->get_head() && !incoming_order->is_fulfilled()) {
        Order* resting_order = level->get_head();
        Volume fill_volume = (resting_order->get_remaining_volume() < incoming_order->get_remaining_volume())
                            ? resting_order->get_remaining_volume()
                            : incoming_order->get_remaining_volume();
        
        // Fill both orders
        resting_order->fill(fill_volume);
        incoming_order->fill(fill_volume);
        level->decrease_volume(fill_volume);
        
        // Create trade record
        trade_buffer.emplace_back(
            incoming_order->get_order_id(),
            resting_order->get_order_id(),
            level->get_price(),
            fill_volume
        );
        
        // Handle fulfilled resting order
        if (resting_order->is_fulfilled()) {
            // Remove from level
            level->pop_front();
            
            // Remove from id_to_order and return to pool
            auto it = id_to_order.find(resting_order->get_order_id());
            if (it != id_to_order.end()) {
                id_to_order.erase(it);
            }
            order_pool.deallocate(resting_order);
        }
    }
    
    return level->is_empty();
}

void Book::delete_order(ID id) {
    auto it = id_to_order.find(id);
    if (it == id_to_order.end()) {
        return; // Order not found
    }
    
    Order* order = it->second;
    if (order->get_order_status() == ACTIVE) {
        bool is_buy = (order->get_order_type() == BUY);
        remove_order_from_level(order, is_buy);
        id_to_order.erase(it);
        order_pool.deallocate(order);
    }
}

void Book::insert_resting_order(Order* order) {
    PRICE price = order->get_order_price();
    bool is_buy = (order->get_order_type() == BUY);
    
    Level* level = get_or_create_level(price, is_buy);
    level->push_back(order);
    
    // Update price sets
    if (is_buy) {
        buy_prices.insert(price);
    } else {
        sell_prices.insert(price);
    }
    
    // Update best prices
    if (is_buy) {
        if (!best_bid || price > best_bid->get_price()) {
            best_bid = level;
        }
    } else {
        if (!best_ask || price < best_ask->get_price()) {
            best_ask = level;
        }
    }
    
    // Store in id_to_order for cancellation
    id_to_order[order->get_order_id()] = order;
}

Level* Book::get_or_create_level(PRICE price, bool is_buy) {
    PriceLevelMap& limits = is_buy ? buy_side_limits : sell_side_limits;
    auto it = limits.find(price);
    
    if (it != limits.end()) {
        return it->second;
    }
    
    // Create new level
    Level* level = level_pool.allocate(price);
    limits[price] = level;
    return level;
}

void Book::remove_order_from_level(Order* order, bool is_buy) {
    PRICE price = order->get_order_price();
    PriceLevelMap& limits = is_buy ? buy_side_limits : sell_side_limits;
    PriceSet& prices = is_buy ? buy_prices : sell_prices;
    
    auto it = limits.find(price);
    if (it == limits.end()) {
        return;
    }
    
    Level* level = it->second;
    level->erase(order);
    order->set_order_status(DELETED);
    
    // Check if level is now empty
    if (level->is_empty()) {
        level_pool.deallocate(level);
        limits.erase(it);
        prices.erase(price);
        
        // Update best prices
        if (is_buy) {
            if (best_bid == level) {
                update_best_bid();
            }
        } else {
            if (best_ask == level) {
                update_best_ask();
            }
        }
    }
}

void Book::update_best_bid() {
    // Use price set for O(log n) lookup
    if (buy_prices.empty()) {
        best_bid = nullptr;
        return;
    }
    
    // Get highest price (last element in set)
    PRICE best_price = *buy_prices.rbegin();
    auto it = buy_side_limits.find(best_price);
    if (it != buy_side_limits.end() && !it->second->is_empty()) {
        best_bid = it->second;
    } else {
        best_bid = nullptr;
    }
}

void Book::update_best_ask() {
    // Use price set for O(log n) lookup
    if (sell_prices.empty()) {
        best_ask = nullptr;
        return;
    }
    
    // Get lowest price (first element in set)
    PRICE best_price = *sell_prices.begin();
    auto it = sell_side_limits.find(best_price);
    if (it != sell_side_limits.end() && !it->second->is_empty()) {
        best_ask = it->second;
    } else {
        best_ask = nullptr;
    }
}

PRICE Book::get_best_buy() const {
    return best_bid ? best_bid->get_price() : 0;
}

PRICE Book::get_best_sell() const {
    return best_ask ? best_ask->get_price() : 0;
}

PRICE Book::get_spread() const {
    PRICE bid = get_best_buy();
    PRICE ask = get_best_sell();
    if (bid == 0 || ask == 0) return 0;
    return ask - bid;
}

double Book::get_mid_price() const {
    PRICE bid = get_best_buy();
    PRICE ask = get_best_sell();
    if (bid == 0 || ask == 0) return 0.0;
    return (bid + ask) / 2.0;
}

std::vector<PRICE> Book::get_buy_prices() const {
    std::vector<PRICE> result;
    result.reserve(buy_prices.size());
    for (auto it = buy_prices.rbegin(); it != buy_prices.rend(); ++it) {
        PRICE price = *it;
        auto level_it = buy_side_limits.find(price);
        if (level_it != buy_side_limits.end() && !level_it->second->is_empty()) {
            result.push_back(price);
        }
    }
    return result;
}

std::vector<PRICE> Book::get_sell_prices() const {
    std::vector<PRICE> result;
    result.reserve(sell_prices.size());
    for (PRICE price : sell_prices) {
        auto level_it = sell_side_limits.find(price);
        if (level_it != sell_side_limits.end() && !level_it->second->is_empty()) {
            result.push_back(price);
        }
    }
    return result;
}

OrderStatus Book::get_order_status(ID id) const {
    auto it = id_to_order.find(id);
    if (it != id_to_order.end()) {
        return it->second->get_order_status();
    }
    return DELETED;
}

void Book::print() const {
    std::cout << "==== BUY SIDE ====" << std::endl;
    std::cout << "Best Buy: " << get_best_buy() << std::endl;
    for (const auto& [price, level] : buy_side_limits) {
        level->print();
    }
    std::cout << "==== SELL SIDE ====" << std::endl;
    std::cout << "Best Sell: " << get_best_sell() << std::endl;
    for (const auto& [price, level] : sell_side_limits) {
        level->print();
    }
}
