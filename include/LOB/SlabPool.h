#ifndef LOB_SLAB_POOL_H
#define LOB_SLAB_POOL_H

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include <cassert>
#include <new>

/**
 * SlabPool: O(1) free-list based object pool allocator for hot-path allocations.
 * 
 * Provides:
 * - O(1) allocation/deallocation via free list
 * - Cache-friendly contiguous memory blocks
 * - Proper alignment for all types
 * - No heap allocations after initial setup
 * 
 * Design:
 * - Uses free list (linked list of free slots) for O(1) allocation
 * - Stores free list pointer in object memory (union with object)
 * - No scanning of used[] arrays or slabs
 * 
 * Invariants:
 * - All objects are owned by the pool
 * - Objects are never moved (pointers remain valid)
 * - Alignment is guaranteed to be at least alignof(T)
 */
template<typename T>
class SlabPool {
private:
    static constexpr size_t ALIGNMENT = alignof(T) > alignof(std::max_align_t) 
                                        ? alignof(T) 
                                        : alignof(std::max_align_t);
    static constexpr size_t OBJECT_SIZE = sizeof(T);
    static constexpr size_t SLAB_SIZE = 1024; // Objects per slab
    
    // Free list node - stored in unused object memory
    union FreeNode {
        char storage[OBJECT_SIZE];
        FreeNode* next;
        
        FreeNode() : next(nullptr) {}
    };
    
    struct Slab {
        alignas(ALIGNMENT) char storage[SLAB_SIZE * OBJECT_SIZE];
        bool initialized;
        
        Slab() : initialized(false) {}
        
        ~Slab() {
            // Note: We don't destruct objects here because free list
            // doesn't track which are used. Objects are destructed on deallocate.
        }
        
        // Non-copyable, non-movable
        Slab(const Slab&) = delete;
        Slab& operator=(const Slab&) = delete;
    };
    
    std::vector<std::unique_ptr<Slab>> slabs_;
    FreeNode* free_list_;  // Head of free list
    size_t total_capacity_;
    size_t allocated_count_;
    
    // Get FreeNode* from object pointer
    static FreeNode* to_free_node(T* obj) {
        return reinterpret_cast<FreeNode*>(obj);
    }
    
    // Get T* from FreeNode*
    static T* to_object(FreeNode* node) {
        return reinterpret_cast<T*>(node);
    }
    
    // Find which slab contains this pointer (for deallocate validation)
    Slab* find_slab(T* obj) {
        char* obj_ptr = reinterpret_cast<char*>(obj);
        for (auto& slab : slabs_) {
            char* slab_start = slab->storage;
            char* slab_end = slab->storage + SLAB_SIZE * OBJECT_SIZE;
            if (obj_ptr >= slab_start && obj_ptr < slab_end) {
                return slab.get();
            }
        }
        return nullptr;
    }
    
public:
    SlabPool(size_t initial_capacity = SLAB_SIZE) 
        : free_list_(nullptr),
          total_capacity_(0),
          allocated_count_(0) {
        // Pre-allocate at least one slab
        size_t initial_slabs = (initial_capacity + SLAB_SIZE - 1) / SLAB_SIZE;
        for (size_t i = 0; i < initial_slabs; ++i) {
            add_slab();
        }
    }
    
    ~SlabPool() {
        // Destruct all allocated objects
        // We need to track which objects are allocated - for simplicity,
        // we'll scan slabs and check if pointer is in free list
        // In practice, this is only called on Book destruction
        for (auto& slab : slabs_) {
            for (size_t i = 0; i < SLAB_SIZE; ++i) {
                T* obj = reinterpret_cast<T*>(slab->storage + i * OBJECT_SIZE);
                // Check if this object is in free list
                bool in_free_list = false;
                for (FreeNode* node = free_list_; node; node = node->next) {
                    if (to_free_node(obj) == node) {
                        in_free_list = true;
                        break;
                    }
                }
                if (!in_free_list) {
                    obj->~T();
                }
            }
        }
    }
    
    // Non-copyable, non-movable
    SlabPool(const SlabPool&) = delete;
    SlabPool& operator=(const SlabPool&) = delete;
    
    void add_slab() {
        slabs_.emplace_back(std::make_unique<Slab>());
        Slab* slab = slabs_.back().get();
        total_capacity_ += SLAB_SIZE;
        
        // Add all slots in new slab to free list
        for (size_t i = 0; i < SLAB_SIZE; ++i) {
            FreeNode* node = reinterpret_cast<FreeNode*>(slab->storage + i * OBJECT_SIZE);
            node->next = free_list_;
            free_list_ = node;
        }
    }
    
    /**
     * Allocate a new object using placement new.
     * O(1) - pops from free list
     */
    template<typename... Args>
    T* allocate(Args&&... args) {
        if (!free_list_) {
            // No free slots, allocate new slab
            add_slab();
        }
        
        // Pop from free list
        FreeNode* node = free_list_;
        free_list_ = node->next;
        
        // Construct object in place
        T* obj = to_object(node);
        new (obj) T(std::forward<Args>(args)...);
        ++allocated_count_;
        
        return obj;
    }
    
    /**
     * Deallocate an object. O(1) - pushes to free list.
     * The object must have been allocated by this pool.
     */
    void deallocate(T* obj) {
        if (!obj) return;
        
        // Validate pointer is in one of our slabs
        assert(find_slab(obj) != nullptr && "Object not allocated from this pool");
        
        // Destruct object
        obj->~T();
        
        // Push to free list
        FreeNode* node = to_free_node(obj);
        node->next = free_list_;
        free_list_ = node;
        --allocated_count_;
    }
    
    size_t capacity() const { return total_capacity_; }
    size_t size() const { return allocated_count_; }
};

#endif // LOB_SLAB_POOL_H
