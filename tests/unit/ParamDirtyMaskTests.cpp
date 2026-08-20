#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <thread>
#include <vector>

#include "state/ParamChangeQueue.hpp"

// ParamDirtyMask is the Phase 2 replacement for the old lossy SPSC ring-buffer
// ParamChangeQueue (see ParamChangeQueue.hpp's doc comment for why). It has no
// JUCE dependency -- pure <atomic> -- so it's directly unit-testable without a
// plugin/processor harness.

using namespace pw8::plugin;

TEST_CASE("ParamDirtyMask: a freshly constructed mask starts fully dirty", "[state][dirtymask]")
{
    ParamDirtyMask mask;
    const std::uint64_t dirty = mask.consume();
    for (std::uint16_t i = 0; i < static_cast<std::uint16_t>(ParamGroup::Count); ++i)
        CHECK(((dirty >> i) & 1) == 1);
}

TEST_CASE("ParamDirtyMask: consume() clears the mask", "[state][dirtymask]")
{
    ParamDirtyMask mask;
    (void)mask.consume(); // clear the initial all-dirty state
    REQUIRE(mask.consume() == 0);
}

TEST_CASE("ParamDirtyMask: markDirty sets exactly the requested bit", "[state][dirtymask]")
{
    ParamDirtyMask mask;
    (void)mask.consume();

    mask.markDirty(ParamGroup::Op3);
    const std::uint64_t dirty = mask.consume();

    CHECK(((dirty >> static_cast<unsigned>(ParamGroup::Op3)) & 1) == 1);
    for (std::uint16_t i = 0; i < static_cast<std::uint16_t>(ParamGroup::Count); ++i)
        if (i != static_cast<std::uint16_t>(ParamGroup::Op3))
            CHECK(((dirty >> i) & 1) == 0);
}

TEST_CASE("ParamDirtyMask: marking the same group twice before a drain coalesces to one bit, not two "
          "signals -- proves coalescing drops redundant *signals*, never *updates*",
          "[state][dirtymask]")
{
    ParamDirtyMask mask;
    (void)mask.consume();

    mask.markDirty(ParamGroup::Filter);
    mask.markDirty(ParamGroup::Filter); // rapid back-to-back changes, e.g. a fast knob drag
    mask.markDirty(ParamGroup::Filter);

    const std::uint64_t dirty = mask.consume();
    CHECK(((dirty >> static_cast<unsigned>(ParamGroup::Filter)) & 1) == 1);
    // A second consume immediately after must see nothing -- the mask doesn't
    // remember "how many times", only "at least once since the last drain".
    CHECK(mask.consume() == 0);
}

TEST_CASE("ParamDirtyMask: markAllDirty sets every real group bit and nothing above it", "[state][dirtymask]")
{
    ParamDirtyMask mask;
    (void)mask.consume();

    mask.markAllDirty();
    const std::uint64_t dirty = mask.consume();

    for (std::uint16_t i = 0; i < static_cast<std::uint16_t>(ParamGroup::Count); ++i)
        CHECK(((dirty >> i) & 1) == 1);
    // Nothing set above the real group range.
    CHECK((dirty >> static_cast<unsigned>(ParamGroup::Count)) == 0);
}

TEST_CASE("ParamDirtyMask: marking every distinct group produces the full mask", "[state][dirtymask]")
{
    ParamDirtyMask mask;
    (void)mask.consume();

    for (std::uint16_t i = 0; i < static_cast<std::uint16_t>(ParamGroup::Count); ++i)
        mask.markDirty(static_cast<ParamGroup>(i));

    const std::uint64_t dirty = mask.consume();
    const std::uint64_t expected = (std::uint64_t{1} << static_cast<unsigned>(ParamGroup::Count)) - 1;
    CHECK(dirty == expected);
}

// Real-world scenario this class exists to fix: multiple concurrent producers
// (a host applying automation from the audio thread + a UI drag from the
// message thread) racing markDirty() against a consumer repeatedly draining --
// no group's dirty signal may ever be silently lost across the whole run,
// which is exactly the correctness bug the old lossy ring buffer had.
TEST_CASE("ParamDirtyMask: no group is ever silently lost under concurrent multi-producer stress",
          "[state][dirtymask]")
{
    ParamDirtyMask mask;
    (void)mask.consume();

    constexpr int kProducers = 4;
    constexpr int kMarksPerProducer = 20000;
    const auto numGroups = static_cast<std::uint16_t>(ParamGroup::Count);

    // Side-channel: independently counts how many times each group was really
    // marked dirty by any producer, so we can cross-check against what the
    // consumer actually observed.
    std::vector<std::atomic<int>> reallyMarked(numGroups);
    for (auto& c : reallyMarked)
        c.store(0, std::memory_order_relaxed);

    std::atomic<bool> stop{false};
    std::vector<std::atomic<int>> observedDirty(numGroups); // consumer's tally, incremented each time a bit is seen
    for (auto& c : observedDirty)
        c.store(0, std::memory_order_relaxed);

    std::thread consumer([&]() {
        while (!stop.load(std::memory_order_relaxed))
        {
            const std::uint64_t dirty = mask.consume();
            for (std::uint16_t i = 0; i < numGroups; ++i)
                if ((dirty >> i) & 1)
                    observedDirty[i].fetch_add(1, std::memory_order_relaxed);
        }
        // Final drain after producers have stopped, to catch anything marked
        // just before they finished.
        const std::uint64_t dirty = mask.consume();
        for (std::uint16_t i = 0; i < numGroups; ++i)
            if ((dirty >> i) & 1)
                observedDirty[i].fetch_add(1, std::memory_order_relaxed);
    });

    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p)
    {
        producers.emplace_back([&, p]() {
            for (int i = 0; i < kMarksPerProducer; ++i)
            {
                const auto group = static_cast<ParamGroup>((p * 7 + i) % numGroups);
                mask.markDirty(group);
                reallyMarked[static_cast<unsigned>(group)].fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : producers)
        t.join();
    stop.store(true, std::memory_order_relaxed);
    consumer.join();

    // The coalescing property means observedDirty[i] can legitimately be far
    // less than reallyMarked[i] (many marks between two drains collapse to
    // one observed bit) -- that's correct behavior, not loss. What must never
    // happen: a group that was really marked at least once ends up observed
    // zero times.
    for (std::uint16_t i = 0; i < numGroups; ++i)
    {
        INFO("group index " << i);
        if (reallyMarked[i].load() > 0)
            CHECK(observedDirty[i].load() > 0);
    }
}
