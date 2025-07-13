#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QDebug>
#include <chrono>
#include <unordered_map>
#include <memory>

/**
 * @brief Performance monitoring utility for tracking bottlenecks
 * 
 * This class provides tools to measure and optimize performance
 * in Qt applications with minimal overhead.
 */
class PerformanceMonitor : public QObject {
    Q_OBJECT

public:
    static PerformanceMonitor& instance();

    // RAII-based timer for automatic measurement
    class ScopedTimer {
    public:
        explicit ScopedTimer(const QString& name);
        ~ScopedTimer();

    private:
        QString name_;
        QElapsedTimer timer_;
    };

    // Memory usage tracking
    struct MemoryInfo {
        size_t peak_memory_mb = 0;
        size_t current_memory_mb = 0;
        size_t allocated_objects = 0;
    };

    // Performance metrics
    struct PerformanceMetrics {
        double average_time_ms = 0.0;
        double max_time_ms = 0.0;
        double min_time_ms = std::numeric_limits<double>::max();
        size_t call_count = 0;
        double total_time_ms = 0.0;
    };

public slots:
    void recordTiming(const QString& operation, qint64 elapsed_ms);
    void recordMemoryUsage();
    void printStatistics();
    void reset();

public:
    // Memory optimization settings
    void enableMemoryOptimizations(bool enable = true);
    void setMaxCacheSize(size_t max_size_mb);
    
    // Performance thresholds
    void setSlowOperationThreshold(double threshold_ms);
    void enableAutoOptimization(bool enable = true);

    // Get current metrics
    MemoryInfo getMemoryInfo() const;
    PerformanceMetrics getMetrics(const QString& operation) const;
    
    // Resource management
    void compactMemory();
    void prefetchResources();

signals:
    void slowOperationDetected(const QString& operation, double time_ms);
    void memoryThresholdExceeded(size_t current_mb, size_t threshold_mb);
    void optimizationSuggestion(const QString& suggestion);

private:
    explicit PerformanceMonitor(QObject* parent = nullptr);
    ~PerformanceMonitor() = default;

    void updateMetrics(const QString& operation, qint64 elapsed_ms);
    void checkThresholds(const QString& operation, qint64 elapsed_ms);
    void suggestOptimizations();

private:
    std::unordered_map<QString, PerformanceMetrics> metrics_;
    MemoryInfo memory_info_;
    
    double slow_threshold_ms_ = 100.0;
    size_t max_cache_size_mb_ = 100;
    bool auto_optimization_enabled_ = false;
    bool memory_optimizations_enabled_ = true;
    
    QTimer* monitoring_timer_;
    static std::unique_ptr<PerformanceMonitor> instance_;
};

// Convenience macros for performance monitoring
#define PERF_SCOPE(name) \
    PerformanceMonitor::ScopedTimer _perf_timer(name)

#define PERF_FUNCTION() \
    PERF_SCOPE(__FUNCTION__)

#define PERF_MONITOR() \
    PerformanceMonitor::instance()

// Memory optimization hints
#define HINT_CACHE_SIZE(size) \
    PERF_MONITOR().setMaxCacheSize(size)

#define HINT_PREFETCH() \
    PERF_MONITOR().prefetchResources()

#define HINT_COMPACT() \
    PERF_MONITOR().compactMemory()