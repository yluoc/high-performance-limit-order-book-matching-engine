#include <iostream>
#include "LOB/Limit.h"

void Limit::insert_order(OrderPointer order) {
    if (order_number == 0) {
        head = tail = order;
    } else {
        tail->set_next_order(order);
        order->set_prev_order(tail);
        tail = order;
    }
    total_volume += order->get_remaining_volume();
    order_number++;
}

void Limit::delete_order(OrderPointer order) {
    if (not order) return;

    if (order_number == 1) {
        head.reset();
        tail.reset();
    } else if (order == head) {
        head = order->get_next_order();
        if (head) {
            head->get_prev_order().reset();
        }
    } else if (order == tail) {
        tail = order->get_prev_order();
        if (tail) {
            tail->get_next_order().reset();
        }
    } else {
        order->get_prev_order()->set_next_order(order->get_next_order());
        order->get_next_order()->set_prev_order(order->get_prev_order());
    }

    order->get_prev_order().reset();
    order->get_next_order().reset();
    if (order->get_order_status() != FULFILLED) {
        order->set_order_status(DELETED);
    }
    total_volume -= order->get_remaining_volume();
    order_number--;
}

Trades Limit::match_order(OrderPointer& order) {
    Trades trades;

    while (order_number > 0 and head and not order->is_fulfilled()) {
        Volume fill_volume = std::min(head->get_remaining_volume(), order->get_remaining_volume());
        head->fill(fill_volume);
        order->fill(fill_volume);
        total_volume -= fill_volume;
        trades.emplace_back(
            order->get_order_id(),
            head->get_order_id(),
            limit_price,
            fill_volume
        );
        if (head->is_fulfilled()) {
            delete_order(head);
        }
    }
    return trades;
}

bool Limit::is_empty() {
    return order_number == 0;
}

PRICE Limit::get_price() const { return limit_price; }
Length Limit::get_order_number() { return order_number; }
Volume Limit::get_total_volume() { return total_volume; }

void Limit::print() {
    std::cout << "Limit Price: " << limit_price << std::endl;
    std::cout << "Number of Orders: " << order_number << std::endl;
    std::cout << "Total Volume: " << total_volume << std::endl;

    OrderPointer current = head;
    while (current) {
        std::cout << "\t";
        current->print();
        current = current->get_next_order();
    }
}

const std::function<bool(const LimitPointer&, const LimitPointer&)> cmp_limits = [](const LimitPointer& a, const LimitPointer& b) {
    return a->get_price() < b->get_price();
};