#pragma once
#include "transport/BleMidiTrace.h"
#include <algorithm>
#include <cstdio>

namespace midibrain {

class BleTraceConsole {
public:
    void rewind(const BleMidiTrace& trace) { next_ = trace.sequence() - trace.size(); }
    bool next(const BleMidiTrace& trace, char* output, std::size_t size) {
        if (!output || !size) return false;
        const uint32_t oldest = trace.sequence() - trace.size();
        if (next_ < oldest) {
            std::snprintf(output, size, "TRACE_GAP lost=%lu\n", static_cast<unsigned long>(oldest - next_));
            next_ = oldest;
            return true;
        }
        if (next_ >= trace.sequence()) return false;
        const auto& entry = trace.at(next_++ - oldest);
        char bytes[33]{};
        for (std::size_t i = 0; i < std::min<std::size_t>(entry.length, sizeof(entry.data)); ++i)
            std::snprintf(bytes + i * 2, sizeof(bytes) - i * 2, "%02X", entry.data[i]);
        std::snprintf(output, size, "BLE us=%llu event=%s status=%ld handle=%u len=%u data=%s\n",
            static_cast<unsigned long long>(entry.at_us), bleTraceName(entry.event), static_cast<long>(entry.status), entry.handle, entry.length, bytes);
        return true;
    }
private:
    uint32_t next_{0};
};

}
