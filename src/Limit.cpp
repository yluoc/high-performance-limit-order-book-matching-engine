#include <iostream>
#include "Limit.h"

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

    if (total_volume == 1) {
        head.reset();
        tail.reset();
    } else if (order == head) {
        head = order->get_next_order();
        order->get_next_order()->get_prev_order().reset();
    } else if (order == tail) {
        tail = order->get_prev_order();
        order->get_prev_order()->get_next_order().reset();
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