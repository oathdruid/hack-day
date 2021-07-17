#pragma once

#include <string>
#include <unordered_set>
#include <vector>

namespace baidu::minirec {

struct Item {
    uint64_t id;
    ::std::vector<float> embedding;
    ::std::string category;
    ::std::unordered_set<::std::string> tags;

    int parse_from_line(::std::string_view line);
    void serialize_to_line(::std::string& line);
};

}
