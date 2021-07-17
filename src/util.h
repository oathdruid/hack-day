#pragma once

#include <string_view>
#include <unistd.h>
#include <vector>

namespace baidu::minirec {

inline void split(::std::string_view line, char delim, ::std::vector<::std::string_view>& fields) {
    fields.clear();
    size_t begin = 0;
    do {
        auto end = line.find_first_of(delim, begin);
        fields.emplace_back(line.substr(begin, end - begin));
        begin = end;
    } while (begin++ != ::std::string_view::npos);
}

inline int64_t monotonic_time_us() {
    timespec now;
    ::clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000L + now.tv_nsec / 1000L;
}

class Timer {
public:
    inline void stop() {
        _end_us = monotonic_time_us();
    }

    inline int64_t elaspe_us() {
        return _end_us - _begin_us;
    }

private:
    int64_t _begin_us {monotonic_time_us()};
    int64_t _end_us {_begin_us};
};

inline int string_to_float(::std::string_view from, float& to) {
    char* end = nullptr;
    to = ::strtof(from.data(), &end);
    return end == &from[from.size()] ? 0 : -1;
}

inline int string_to_uint64(::std::string_view from, uint64_t& to) {
    char* end = nullptr;
    to = ::strtoull(from.data(), &end, 10);
    return end == &from[from.size()] ? 0 : -1;
}

}
