#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>

namespace omega {
namespace AI {

struct AIJob {
    juce::String name;
    std::function<void()> run;
    std::function<void()> onComplete;
};

class AIJobQueue {
public:
    explicit AIJobQueue(int workerThreads = 2) : pool(workerThreads) {}
    ~AIJobQueue() = default;

    void enqueue(const AIJob& job) {
        pool.addJob(new PoolJob(job, progress), true);
    }

    float getProgress() const { return progress.load(); }

private:
    class PoolJob : public juce::ThreadPoolJob {
    public:
        PoolJob(const AIJob& j, std::atomic<float>& p)
            : juce::ThreadPoolJob(j.name), job(j), progressRef(p) {}

        JobStatus runJob() override {
            progressRef.store(0.0f);
            if (job.run)
                job.run();
            progressRef.store(1.0f);
            if (job.onComplete)
                job.onComplete();
            return jobHasFinished;
        }

    private:
        AIJob job;
        std::atomic<float>& progressRef;
    };

    std::atomic<float> progress { 0.0f };
    juce::ThreadPool pool;
};

} // namespace AI
} // namespace omega
