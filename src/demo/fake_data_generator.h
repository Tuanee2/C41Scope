#pragma once

#include <QObject>

#include <atomic>
#include <thread>
#include <vector>

namespace c41scope {

class ScopeController;

class FakeDataGenerator : public QObject {
    Q_OBJECT

public:
    explicit FakeDataGenerator(QObject* parent = nullptr);
    ~FakeDataGenerator() override;

    void setController(ScopeController* controller);
    ScopeController* controller() const;

    void setSampleRate(int hz);
    int sampleRate() const;

    void setChannelCount(int count);
    int channelCount() const;

    void setBatchSize(int samplesPerPush);
    int batchSize() const;

    void start();
    void stop();
    bool running() const;

private:
    void runLoop();

    ScopeController* m_controller = nullptr;
    std::thread m_worker;

    std::atomic<bool> m_running;
    std::atomic<int> m_sampleRate;
    std::atomic<int> m_channelCount;
    std::atomic<int> m_batchSize;

    double m_sampleCursor = 0.0;
    std::vector<float> m_channelBuffer;
};

} // namespace c41scope

