#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QWidget>
#include <memory>
#include <vector>

/**
 * @brief Memory-optimized manager class that uses RAII patterns
 * 
 * This class demonstrates proper memory management for Qt applications
 * using smart pointers and RAII principles instead of raw pointers.
 */
class MemoryOptimizedManager : public QObject {
    Q_OBJECT

public:
    explicit MemoryOptimizedManager(QObject *parent = nullptr);
    ~MemoryOptimizedManager() = default;

    // Use smart pointers for resource management
    template<typename T, typename... Args>
    std::unique_ptr<T> createResource(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // Pool for reusable objects to reduce allocations
    template<typename T>
    class ObjectPool {
    public:
        std::unique_ptr<T> acquire() {
            if (available_.empty()) {
                return std::make_unique<T>();
            }
            auto obj = std::move(available_.back());
            available_.pop_back();
            return obj;
        }

        void release(std::unique_ptr<T> obj) {
            if (obj && available_.size() < max_size_) {
                available_.push_back(std::move(obj));
            }
        }

    private:
        std::vector<std::unique_ptr<T>> available_;
        static constexpr size_t max_size_ = 10;
    };

    // Memory-efficient string handling
    class StringPool {
    public:
        const QString& intern(const QString& str);
        void clear();
        
    private:
        std::unordered_set<QString> strings_;
    };

    // Lazy initialization for expensive resources
    template<typename T>
    class LazyResource {
    public:
        template<typename... Args>
        explicit LazyResource(Args&&... args) 
            : args_(std::forward<Args>(args)...) {}

        T& get() {
            if (!resource_) {
                resource_ = std::apply([](auto&&... args) {
                    return std::make_unique<T>(std::forward<decltype(args)>(args)...);
                }, args_);
            }
            return *resource_;
        }

        void reset() {
            resource_.reset();
        }

    private:
        std::unique_ptr<T> resource_;
        std::tuple<Args...> args_;
    };

private:
    StringPool string_pool_;
};

// Utility macros for memory optimization
#define DECLARE_RAII_RESOURCE(type, name) \
    std::unique_ptr<type> name##_

#define INIT_RAII_RESOURCE(type, name, ...) \
    name##_ = std::make_unique<type>(__VA_ARGS__)

#define GET_RAII_RESOURCE(name) \
    name##_.get()

#define RESET_RAII_RESOURCE(name) \
    name##_.reset()