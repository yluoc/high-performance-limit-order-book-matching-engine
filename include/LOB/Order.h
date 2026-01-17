#ifndef LOB_ORDER_H
#define LOB_ORDER_H

#include "Types.h"
#include <memory>

class Order {
    private:
        ID order_id; /**< Order id */
        ID agent_id; /**< id of the agent who placed the order */
        OrderType order_type; /**< Order type (buy/sell) */
        PRICE order_price; /**< Limit price of the order */
        Volume initial_volume; /**< Initial volume/number of shares in the order */
        Volume remaining_volume; /**< Volume/number of remaining shares in the order */
        OrderStatus order_status; /**< Current status of the order */
        
        /** As orders are stored in a doubly-linked list, each order stores its previous and next orders */
        std::shared_ptr<Order> prev_order; /**< Previous order in the list */
        std::shared_ptr<Order> next_order; /**< Next order in the list */
        
    public:
        Order(
            ID order_id,
            ID agent_id,
            OrderType order_type,
            PRICE order_price,
            Volume initial_volume,
            Volume remaining_volume,
            OrderStatus order_status)
            :
            order_id(order_id),
            agent_id(agent_id),
            order_type(order_type),
            order_price(order_price),
            initial_volume(initial_volume),
            remaining_volume(remaining_volume),
            order_status(order_status),
            prev_order(nullptr),
            next_order(nullptr) 
        {}
        
        /**
         * @brief fills the order with a given volume (quantity)
         * @param fill_volume: Volume to fill
         */
        void fill(Volume fill_volume);

        /**
         * @brief checks if the order is fulfilled
         * @return true if the order is fulfilled, false otherwise
         */
        bool is_fulfilled();

        /** Getters and setters */
        ID get_order_id() const;
        ID get_agent_id() const;
        OrderType get_order_type() const;
        PRICE get_order_price() const;
        Volume get_initial_volume() const;
        Volume get_remaining_volume() const;
        OrderStatus get_order_status() const;

        void set_order_status(OrderStatus order_status);
        std::shared_ptr<Order> &get_prev_order();
        void set_prev_order(std::shared_ptr<Order> &prev_order);
        std::shared_ptr<Order> &get_next_order();
        void set_next_order(std::shared_ptr<Order> &next_order);

        /** Print order details */
        void print();
};

using OrderPointer = std::shared_ptr<Order>;

#endif // LOB_ORDER_H