#pragma once

#include "common/FixedList.h"
#include "controller/ControllerMapper.h"

#include <cstdint>

namespace midibrain {

enum class PadLearnResult : uint8_t {
    Ignored,
    Guarded,
    Duplicate,
    Captured,
    Complete
};

class PadLearner {
public:
    static constexpr std::size_t MappingCount = 9;

    void begin();
    void cancel() { active_ = false; }
    PadLearnResult capture(const MidiEvent& event);
    bool active() const;
    uint8_t step() const;
    SemanticAction currentAction() const;
    const FixedList<ControllerMapping, MappingCount>& mappings() const;

private:
    FixedList<ControllerMapping, MappingCount> mappings_{};
    uint64_t accept_after_us_{0};
    bool active_{false};
    bool awaiting_release_{false};
    SourceSelector held_{};
};

}
