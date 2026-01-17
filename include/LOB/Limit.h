#ifndef LOB_LIMIT_H
#define LOB_LIMIT_H

#include <functional>
#include "Order.h"
#include "Trade.h"

class Limit {
    private:
        PRICE limit_price; /**< Limit price */
        Length order_number; /**< Number of orders at this limit */
        Volume total_volume; /**< Total volume at this limit */

        OrderPointer head; /**< First order in the list (oldest) */
        OrderPointer tail; /**< Last order in the list (most recent) */
    public:
        Limit(PRICE price):
            limit_price(price),
            order_number(0),
            total_volume(0),
            head(nullptr),
            tail(nullptr)
        {}

        /**
         * @brief Inserts an order at the end of the list
         * @param order pointer to the order to insert
         */
        void insert_order(OrderPointer order);
        /**
         * @brief Deletes an order from the limit order list. Assumes that the order is in this limit list.
         * @param order pointer to the order to delete
         */
        void delete_order(OrderPointer order);
        /**
         * @brief Matches an opposite OrderType order to the current orders in the list
         * @param order order to match
         * @return Trades an array of trades with the matched order
         */
        Trades match_order(OrderPointer& order);
        /**
         * @brief Checks if the limit is empty (i.e. no orders)
         * @return true if there are no order false otherwise
         */
        bool is_empty();

        /** Getters */
        PRICE get_price() const;
        Length get_order_number();
        Volume get_total_volume();

        /** Print method */
        void print();
};

using LimitPointer = std::shared_ptr<Limit>;
 
#endif //LOB_LIMIT_H