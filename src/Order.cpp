#include <iostream>
#include "LOB/Order.h"

void Order::fill(Volume fill_volume) {
    if (fill_volume > remaining_volume) throw std::logic_error("Fill volume exceeds remaining volume");
    remaining_volume -= fill_volume;
    if (remaining_volume == 0) order_status = FULFILLED;
}

bool Order::is_fulfilled() {
    return remaining_volume == 0;
}

ID Order::get_order_id() const { return order_id; }
ID Order::get_agent_id() const { return agent_id; }
OrderType Order::get_order_type() const { return order_type; }
PRICE Order::get_order_price() const { return order_price; }
Volume Order::get_initial_volume() const { return initial_volume; }
Volume Order::get_remaining_volume() const { return remaining_volume; }
OrderStatus Order::get_order_status() const { return order_status; }

void Order::set_order_status(OrderStatus status) { this->order_status = status; }
OrderPointer &Order::get_prev_order() { return prev_order; }
void Order::set_prev_order(OrderPointer &prev) { this->prev_order = prev; }
OrderPointer &Order::get_next_order() { return next_order; }
void Order::set_next_order(OrderPointer &next) { this->next_order = next; }

void Order::print() {
    std::cout << "Order Details:" << std::endl;
    std::cout << "Order ID: " << order_id << std::endl;
    std::cout << "Agent ID: " << agent_id << std::endl;
    std::cout << "Order Type: " << (order_type == BUY ? "BUY" : "SELL") << std::endl;
    std::cout << "Order Price: " << order_price << std::endl;
    std::cout << "Initial Volume: " << initial_volume << std::endl;
    std::cout << "Remaining Volume: " << remaining_volume << std::endl;
    std::cout << "Order Status: ";
    switch (order_status) {
        case ACTIVE:
            std::cout << "ACTIVE" << std::endl;
            break;
        case FULFILLED:
            std::cout << "FULFILLED" << std::endl;
            break;
        case DELETED:
            std::cout << "DELETED" << std::endl;
            break;
    }
}   