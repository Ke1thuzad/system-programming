#include "../task1/logger.h"
#include "generator.h"
#include "analyzer.h"
#include "thread_safe_queue.h"
#include "ip_stats.h"
#include "utils.h"

#include <iostream>
#include <vector>
#include <pthread.h>
#include <chrono>
#include <thread>
#include <map>
#include <iomanip>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <cstdlib>


IpStats queryIpStatistics(const std::string& ip_to_query, std::vector<AnalyzerData>& p_analyzer_data_store) {
    IpStats combined_stats;
    fprintf(stdout, "\nQuerying statistics for IP: %s...\n", ip_to_query.c_str()); fflush(stdout);
    for (AnalyzerData& analyzer_data : p_analyzer_data_store) {
        pthread_mutex_lock(&(analyzer_data.stats_mutex));
        try {
            auto it = analyzer_data.local_stats.find(ip_to_query);
            if (it != analyzer_data.local_stats.end()) combined_stats += it->second;
        } catch (...) {
            pthread_mutex_unlock(&(analyzer_data.stats_mutex));
            fprintf(stderr, "Error processing query for IP %s in one analyzer.\n", ip_to_query.c_str()); throw;
        }
        pthread_mutex_unlock(&(analyzer_data.stats_mutex));
    }
    return combined_stats;
}

std::map<std::string, IpStats> aggregateAllStatistics(std::vector<AnalyzerData>& p_analyzer_data_store) {
    std::map<std::string, IpStats> global_stats;
    fprintf(stdout, "\nAggregating final statistics from all analyzers...\n"); fflush(stdout);
    for (AnalyzerData& analyzer_data : p_analyzer_data_store) {
        pthread_mutex_lock(&(analyzer_data.stats_mutex));
        try {
            for (const auto& pair : analyzer_data.local_stats) global_stats[pair.first] += pair.second;
        } catch (...) {
            pthread_mutex_unlock(&(analyzer_data.stats_mutex));
            fprintf(stderr, "Error processing aggregation for one analyzer.\n"); throw;
        }
        pthread_mutex_unlock(&(analyzer_data.stats_mutex));
    }
    return global_stats;
}


int main(int argc, char* argv[]) {

    int num_generator_threads = 2;
    int num_analyzer_threads = 3;
    int run_duration_seconds = 10;
    LogLevels generator_log_level = INFO;
    // (Parsing logic as before)
    if (argc > 1) try { num_generator_threads = std::stoi(argv[1]); } catch (...) {}
    if (argc > 2) try { num_analyzer_threads = std::stoi(argv[2]); } catch (...) {}
    if (argc > 3) try { run_duration_seconds = std::stoi(argv[3]); } catch (...) {}
    if (num_generator_threads <= 0) num_generator_threads = 1;
    if (num_analyzer_threads <= 0) num_analyzer_threads = 1;
    if (run_duration_seconds <= 0) run_duration_seconds = 5;

    printf("--- Log Generator & Analyzer ---\n");
    printf("Generators: %d\nAnalyzers:  %d\nRun Time:   %d seconds\n",
            num_generator_threads, num_analyzer_threads, run_duration_seconds);
    printf("---------------------------------\n");

    ThreadSafeQueue<std::string> log_queue;
    bool shutdown_requested = false;
    pthread_mutex_t shutdown_mutex;
    std::vector<AnalyzerData> analyzer_data_store;

    int main_ret_code = 0;
    int mutex_ret = pthread_mutex_init(&shutdown_mutex, nullptr);
    if (mutex_ret != 0) {
        return 1;
    }
    try {
        analyzer_data_store.resize(num_analyzer_threads);
    } catch (const std::exception& e) {
        fprintf(stderr, "FATAL: Failed to initialize analyzer data store: %s\n", e.what());
        pthread_mutex_destroy(&shutdown_mutex);
        return 1; // Exit early
    }

    std::vector<pthread_t> generator_pthreads(num_generator_threads, 0);
    std::vector<pthread_t> analyzer_pthreads(num_analyzer_threads, 0);

    std::vector<GeneratorThreadData*> generator_thread_args(num_generator_threads, nullptr);
    std::vector<AnalyzerThreadData*> analyzer_thread_args(num_analyzer_threads, nullptr);
    int pthread_ret;
    bool creation_failed = false;

    fprintf(stdout, "Starting analyzer threads...\n");
    for (int i = 0; i < num_analyzer_threads; ++i) {
        try {
            analyzer_thread_args[i] = new AnalyzerThreadData(&log_queue, &shutdown_requested, &shutdown_mutex, &analyzer_data_store[i], i);
            pthread_ret = pthread_create(&analyzer_pthreads[i], nullptr, analyzerThread, analyzer_thread_args[i]);
            if (pthread_ret != 0) {
                delete analyzer_thread_args[i]; analyzer_thread_args[i] = nullptr;
                fprintf(stderr, "ERROR: Analyzer pthread_create failed for thread %d: %s\n", i, strerror(pthread_ret));
                creation_failed = true;
                break;
            }
        } catch (const std::bad_alloc& ba) {
            fprintf(stderr, "ERROR: Failed to allocate memory for analyzer %d args: %s\n", i, ba.what());
            creation_failed = true;
            break;
        }
    }

    if (!creation_failed) {
        fprintf(stdout, "Starting generator threads...\n"); fflush(stdout);
        for (int i = 0; i < num_generator_threads; ++i) {
            try {
                generator_thread_args[i] = new GeneratorThreadData(&log_queue, &shutdown_requested, &shutdown_mutex, i, generator_log_level);
                pthread_ret = pthread_create(&generator_pthreads[i], nullptr, generatorThread, generator_thread_args[i]);
                if (pthread_ret != 0) {
                    delete generator_thread_args[i]; generator_thread_args[i] = nullptr;
                    fprintf(stderr, "ERROR: Generator pthread_create failed for thread %d: %s\n", i, strerror(pthread_ret));
                    creation_failed = true;
                    break;
                }
            } catch (const std::bad_alloc& ba) {
                fprintf(stderr, "ERROR: Failed to allocate memory for generator %d args: %s\n", i, ba.what());
                creation_failed = true;
                break;
            }
        }
    }

    if (creation_failed) {
        fprintf(stderr, "FATAL: Thread creation failed. Initiating immediate shutdown.\n");
        main_ret_code = 1;

        pthread_mutex_lock(&shutdown_mutex);
        shutdown_requested = true;
        pthread_mutex_unlock(&shutdown_mutex);

        log_queue.Shutdown();
    } else {
        fprintf(stdout, "Application running for %d seconds...\n", run_duration_seconds);

        auto start_time = std::chrono::steady_clock::now();
        while (true) {
            // Check for timeout
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
            if (elapsed.count() >= run_duration_seconds) {
                fprintf(stdout, "\nRun duration reached (%ld seconds). Initiating shutdown...\n", (long)run_duration_seconds); fflush(stdout);
                pthread_mutex_lock(&shutdown_mutex);
                shutdown_requested = true;
                pthread_mutex_unlock(&shutdown_mutex);
                log_queue.Shutdown();
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds((long) 200));
        }

    }

    for (int i = 0; i < num_generator_threads; ++i) {
        if (generator_pthreads[i] != 0) {
            pthread_join(generator_pthreads[i], nullptr);
        }

        delete generator_thread_args[i];
        generator_thread_args[i] = nullptr;
    }

    log_queue.Shutdown();
    for (int i = 0; i < num_analyzer_threads; ++i) {
        if (analyzer_pthreads[i] != 0) {
            pthread_join(analyzer_pthreads[i], nullptr);
        }
        delete analyzer_thread_args[i];
        analyzer_thread_args[i] = nullptr;
    }

    if (!creation_failed) {
        try {
            std::map<std::string, IpStats> final_stats = aggregateAllStatistics(analyzer_data_store);
            fprintf(stdout, "\n--- Final Aggregated Statistics ---\n");
            if (final_stats.empty()) { fprintf(stdout, "(No statistics collected)\n"); }
            else {
                for (const auto& pair : final_stats) {
                    fprintf(stdout, "IP Address: %s\n", pair.first.c_str());
                    std::cout << pair.second.toString();
                    fprintf(stdout, "---------------------------------\n");
                }
            }
            fflush(stdout);
        } catch (const std::exception& e) {
            main_ret_code = 1;
        }
    } else {
        fprintf(stdout, "\n--- Final Aggregated Statistics Skipped (due to thread creation failure) ---\n");
    }


    pthread_mutex_destroy(&shutdown_mutex);

    return main_ret_code;
}