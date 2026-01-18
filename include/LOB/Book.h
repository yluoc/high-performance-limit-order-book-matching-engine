#ifndef LOB_BOOK_H
#define LOB_BOOK_H

#include <unordered_map>
#include <vector>
#include <set>
#include "Level.h"
#include "SlabPool.h"

using PriceLevelMap = std::unordered_map<PRICE, Level*>;
using Orders = std::unordered_map<ID, Order*>;
using PriceSet = std::set<PRICE>;

/**
 * Book: High-performance limit order book matching engine.
 * 
 * Design:
 * - Uses SlabPool for zero-allocation hot path (after warmup)
 * - Raw pointers internally (no shared_ptr overhead)
 * - Cached best bid/ask pointers (no hash lookups in matching loop)
 * - Intrusive FIFO lists at each price level
 * 
 * Invariants:
 * - best_bid points to highest buy price level (or nullptr)
 * - best_ask points to lowest sell price level (or nullptr)
 * - All orders/levels owned by internal pools
 * - id_to_order only contains resting orders
 */
class Book {
    private:
        // Price level maps (price -> Level*)
        PriceLevelMap buy_side_limits; /**< Maps buy limit prices to level objects */
        PriceLevelMap sell_side_limits; /**< Maps sell limit prices to level objects */

        // Price sets for O(log n) best price updates
        PriceSet buy_prices;  /**< Set of buy prices (sorted descending) */
        PriceSet sell_prices; /**< Set of sell prices (sorted ascending) */

        // Cached best prices (no lookups needed in hot path)
        Level* best_bid; /**< Pointer to best (highest) buy level */
        Level* best_ask; /**< Pointer to best (lowest) sell level */

        // Order lookup (only for resting orders)
        Orders id_to_order; /**< Maps order IDs to order pointers */

        // Memory pools (own all orders and levels)
        SlabPool<Order> order_pool;
        SlabPool<Level> level_pool;
        
        // Trade output buffer (reserved to avoid allocations)
        static constexpr size_t TRADE_BUFFER_SIZE = 16;
        mutable std::vector<Trade> trade_buffer;

        /**
         * @brief Gets or creates a level at the given price
         * @param price limit price
         * @param is_buy true if buy side, false if sell side
         * @return pointer to the level
         */
        Level* get_or_create_level(PRICE price, bool is_buy);
        
        /**
         * @brief Updates best_bid using price set (O(log n))
         */
        void update_best_bid();
        
        /**
         * @brief Updates best_ask using price set (O(log n))
         */
        void update_best_ask();
        
        /**
         * @brief Matches incoming order against a level, handling fulfilled resting orders
         * @param incoming_order order to match
         * @param level level to match against
         * @return true if level became empty
         */
        bool match_against_level(Order* incoming_order, Level* level);
        
        /**
         * @brief Checks if a buy level is empty and removes it if so
         * @param price price level to check
         */
        void check_for_empty_buy_level(PRICE price);
        
        /**
         * @brief Checks if a sell level is empty and removes it if so
         * @param price price level to check
         */
        void check_for_empty_sell_level(PRICE price);
        
        /**
         * @brief Inserts a resting order into the book
         * @param order order to insert (must be non-null, owned by pool)
         */
        void insert_resting_order(Order* order);
        
        /**
         * @brief Deletes an order from a level
         * @param order order to delete
         * @param is_buy true if buy side
         */
        void remove_order_from_level(Order* order, bool is_buy);
        
    public:
        /**
         * @brief Constructor
         * @param initial_capacity initial capacity for pools
         */
        explicit Book(size_t initial_capacity = 1024);
        
        /**
         * @brief Destructor - pools handle cleanup
         */
        ~Book() = default;
        
        // Non-copyable, non-movable
        Book(const Book&) = delete;
        Book& operator=(const Book&) = delete;
        
        /**
         * @brief Places an order, tries to match it, and inserts if not fulfilled
         * @param order_id order ID
         * @param agent_id agent ID
         * @param order_type BUY or SELL
         * @param price limit price
         * @param volume order volume
         * @return vector of trades (uses internal buffer, valid until next call)
         */
        const Trades& place_order(
            ID order_id,
            ID agent_id,
            OrderType order_type,
            PRICE price,
            Volume volume
        );
        
        /**
         * @brief Deletes an order by ID
         * @param id order ID to delete
         */
        void delete_order(ID id);

        /** Getters */
        PRICE get_spread() const;
        double get_mid_price() const;
        PRICE get_best_buy() const;
        PRICE get_best_sell() const;
        
        // Accessors for testing (return sizes, not full containers)
        size_t get_buy_levels_count() const { return buy_side_limits.size(); }
        size_t get_sell_levels_count() const { return sell_side_limits.size(); }
        size_t get_resting_orders_count() const { return id_to_order.size(); }
        
        PriceLevelMap& get_buy_limits() { return buy_side_limits; }
        PriceLevelMap& get_sell_limits() { return sell_side_limits; }
        Orders& get_id_to_order() { return id_to_order; }
        
        std::vector<PRICE> get_buy_prices() const;
        std::vector<PRICE> get_sell_prices() const;

        /** Print method */
        void print() const;

        OrderStatus get_order_status(ID id) const;
};

#endif // LOB_BOOK_H
