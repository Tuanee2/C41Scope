#include "fake_data_generator.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "../scope_controller.h"

namespace c41scope {

namespace {
constexpr double kTwoPi = 6.28318530717958647692;
}

FakeDataGenerator::FakeDataGenerator(QObject* parent)
    : QObject(parent)
    , m_running(false)
    , m_sampleRate(400)
    , m_channelCount(4)
    , m_batchSize(32)
{
}

FakeDataGenerator::~FakeDataGenerator()
{
    stop();
}

void FakeDataGenerator::setController(ScopeController* controller)
{
    m_controller = controller;
}

ScopeController* FakeDataGenerator::controller() const
{
    return m_controller;
}

void FakeDataGenerator::setSampleRate(int hz)
{
    m_sampleRate.store(std::max(1, hz), std::memory_order_release);
}

int FakeDataGenerator::sampleRate() const
{
    return m_sampleRate.load(std::memory_order_acquire);
}

void FakeDataGenerator::setChannelCount(int count)
{
    m_channelCount.store(std::max(1, count), std::memory_order_release);
}

int FakeDataGenerator::channelCount() const
{
    return m_channelCount.load(std::memory_order_acquire);
}

void FakeDataGenerator::setBatchSize(int samplesPerPush)
{
    m_batchSize.store(std::max(1, samplesPerPush), std::memory_order_release);
}

int FakeDataGenerator::batchSize() const
{
    return m_batchSize.load(std::memory_order_acquire);
}

void FakeDataGenerator::start()
{
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;
    }

    m_worker = std::thread([this]() {
        runLoop();
    });
}

void FakeDataGenerator::stop()
{
    if (!m_running.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    if (m_worker.joinable()) {
        m_worker.join();
    }
}

bool FakeDataGenerator::running() const
{
    return m_running.load(std::memory_order_acquire);
}

void FakeDataGenerator::runLoop()
{
    using Clock = std::chrono::steady_clock;
    auto nextWake = Clock::now();

    while (m_running.load(std::memory_order_acquire)) {
        const int sampleRateValue = m_sampleRate.load(std::memory_order_acquire);
        const int channelCountValue = m_channelCount.load(std::memory_order_acquire);
        const int batchSizeValue = m_batchSize.load(std::memory_order_acquire);

        const int sampleRate = std::max(1, sampleRateValue);
        const int channelCount = std::max(1, channelCountValue);
        const int batchSize = std::max(1, batchSizeValue);

        const double dt = 1.0 / static_cast<double>(sampleRate);
        const double t0 = m_sampleCursor;

        if (m_channelBuffer.size() < static_cast<std::size_t>(batchSize)) {
            m_channelBuffer.resize(static_cast<std::size_t>(batchSize));
        }

        ScopeController* controllerPtr = m_controller;
        if (controllerPtr != nullptr) {
            for (int channelId = 0; channelId < channelCount; ++channelId) {
                const double baseFreq = 0.8 + static_cast<double>(channelId) * 0.35;
                const double phase = static_cast<double>(channelId) * 0.55;
                const double amp = 0.6 + 0.15 * static_cast<double>(channelId % 3);

                for (int i = 0; i < batchSize; ++i) {
                    const double t = t0 + dt * static_cast<double>(i);
                    const double carrier = std::sin(kTwoPi * baseFreq * t + phase);
                    const double wobble = 0.1 * std::sin(kTwoPi * (baseFreq * 0.2) * t + phase * 0.5);
                    m_channelBuffer[static_cast<std::size_t>(i)] = static_cast<float>(amp * carrier + wobble);
                }

                controllerPtr->pushSamples(channelId, m_channelBuffer.data(), static_cast<std::size_t>(batchSize), t0, dt);
            }
        }

        m_sampleCursor += dt * static_cast<double>(batchSize);

        const double sleepMs = (1000.0 * static_cast<double>(batchSize)) / static_cast<double>(sampleRate);
        nextWake += std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double, std::milli>(sleepMs));

        std::this_thread::sleep_until(nextWake);

        const auto now = Clock::now();
        if (now > nextWake + std::chrono::milliseconds(50)) {
            nextWake = now;
        }
    }
}

} // namespace c41scope

