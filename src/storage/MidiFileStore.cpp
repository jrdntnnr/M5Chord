#include "storage/MidiFileStore.h"
#include <cstdio>
#ifdef ARDUINO
#include <SD.h>
#include "midi/CachedMidiReader.h"
#include <new>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
namespace midibrain {

struct MidiFileStore::Worker {
    class Reader final : public MidiFileReader {
    public:
        File input;
        uint32_t bytes{0};
        uint32_t size() const override { return bytes; }
        bool read(uint32_t offset, uint8_t* data, std::size_t count) override {
            return input && offset <= bytes && count <= bytes - offset && input.seek(offset) && input.read(data, count) == count;
        }
    } source;
    CachedMidiReader reader{source};
    MidiFileDecoder decoder;
    MidiFileStore& owner;
    uint32_t token{0};
    bool validating{false}, active{false}, load_job{false};
    explicit Worker(MidiFileStore& store) : owner(store) {}
    void finish(const char* error) {
        owner.stream_.finish(error);
        owner.stream_.publish(token);
        active = false;
        if (load_job) {
            owner.result_ = error;
            load_job = false;
            owner.complete_.store(true, std::memory_order_release);
        }
    }
    void run() {
        for (;;) {
            if (owner.load_pending_.exchange(false, std::memory_order_acquire)) {
                load_job = true; validating = true; active = true;
                token = owner.stream_.requested();
                owner.stream_.reset();
                source.input.close();
                char path[72]{};
                std::snprintf(path, sizeof(path), "/midi/%s", owner.pending_name_);
                source.input = SD.open(path, FILE_READ);
                source.bytes = source.input ? source.input.size() : 0;
                reader.clear();
                if (!source.input || source.input.isDirectory()) finish("MIDI FILE NOT FOUND");
                else if (!decoder.begin(reader)) finish(decoder.error());
            } else if (!load_job && token != owner.stream_.requested()) {
                token = owner.stream_.requested();
                owner.stream_.reset();
                validating = false; active = true;
                reader.clear();
                if (!source.input || !decoder.begin(reader)) finish(source.input ? decoder.error() : "LOAD A FILE FIRST");
            }
            unsigned budget = 64;
            while (active && budget-- && token == owner.stream_.requested()) {
                if (!validating && owner.stream_.size() == MidiFileStream::Capacity) break;
                MidiFileEvent event;
                const auto result = decoder.next(reader, event);
                if (result == MidiFileDecoder::Result::Error) { finish(decoder.error()); break; }
                if (validating) {
                    owner.progress_.store(decoder.progress(), std::memory_order_relaxed);
                    if (result == MidiFileDecoder::Result::End) {
                        owner.duration_ = decoder.duration(); owner.channels_ = decoder.channels();
                        owner.tracks_ = decoder.tracks(); owner.skipped_sysex_ = decoder.skippedSysex();
                        validating = false;
                        reader.clear();
                        if (!decoder.begin(reader)) finish(decoder.error());
                    }
                } else {
                    if (result == MidiFileDecoder::Result::Event) owner.stream_.push(event);
                    else if (result == MidiFileDecoder::Result::End) { finish(nullptr); break; }
                }
            }
            if (active && !validating && owner.stream_.size() == MidiFileStream::Capacity) {
                owner.stream_.publish(token);
                if (load_job) {
                    owner.result_ = nullptr; load_job = false;
                    owner.complete_.store(true, std::memory_order_release);
                }
            }
            vTaskDelay(1);
        }
    }
};
bool MidiFileStore::beginLoad(const char* name) {
    if (loading_) { status_ = "LOADING: PLEASE WAIT"; return false; }
    MidiFileCatalog validation;
    if (!validation.add(name)) { status_ = "INVALID FILE NAME"; return false; }
    if (!worker_) {
        worker_ = new (std::nothrow) Worker(*this);
        if (!worker_) { status_ = "NO MEMORY FOR SD READER"; return false; }
        if (xTaskCreatePinnedToCore([](void* context) { static_cast<Worker*>(context)->run(); }, "midi-sd", 6144, worker_, 1, nullptr, 0) != pdPASS) {
            delete worker_; worker_ = nullptr; status_ = "NO MEMORY FOR SD TASK"; return false;
        }
    }
    std::snprintf(pending_name_, sizeof(pending_name_), "%s", name);
    loading_ = true; status_ = "LOADING"; progress_.store(0, std::memory_order_relaxed);
    load_pending_.store(true, std::memory_order_release);
    return true;
}

bool MidiFileStore::scan(MidiFileCatalog& catalog) {
    catalog.names.clear(); catalog.truncated = false;
    if (SD.cardType() == CARD_NONE) { status_ = "NO SD CARD"; return false; }
    if (!SD.exists("/midi") && !SD.mkdir("/midi")) { status_ = "CANNOT CREATE /midi"; return false; }
    File directory = SD.open("/midi");
    if (!directory || !directory.isDirectory()) { status_ = "/midi IS NOT A FOLDER"; return false; }
    unsigned examined = 0;
    for (;;) {
        File entry = directory.openNextFile();
        if (!entry) break;
        if (++examined > 512) { catalog.truncated = true; break; }
        if (!entry.isDirectory()) {
            const char* name = entry.name();
            const char* slash = std::strrchr(name, '/');
            catalog.add(slash ? slash + 1 : name);
        }
        entry.close();
    }
    status_ = catalog.names.empty() ? "PUT .MID FILES IN /midi" : catalog.truncated ? "LIST LIMITED: 32 FILES" : "SELECT FILE; ENTER LOADS";
    return true;
}
}
#else
namespace midibrain {
bool MidiFileStore::beginLoad(const char*) { return false; }
bool MidiFileStore::scan(MidiFileCatalog&) { return false; }
}
#endif

namespace midibrain {
bool MidiFileStore::complete(StandardMidiFile& file) {
    if (!complete_.exchange(false, std::memory_order_acquire)) return false;
    loading_ = false;
    file.events.clear(); file.duration_us = result_ ? 0 : duration_;
    file.channels = result_ ? 0 : channels_; file.tracks = result_ ? 0 : tracks_;
    file.skipped_sysex = result_ ? 0 : skipped_sysex_; file.error = result_;
    status_ = result_ ? result_ : skipped_sysex_ ? "READY; SYSEX SKIPPED" : "READY";
    return true;
}
}
