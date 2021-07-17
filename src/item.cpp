#include "item.h"
#include "util.h"

#include <iostream>

namespace baidu::minirec {

int Item::parse_from_line(::std::string_view line) {
    ::std::vector<::std::string_view> fields;
    split(line, '\t', fields);
    if (fields.size() < 4) {
        ::std::cerr << "line fields < 4" << ::std::endl;
        return -1;
    }
    {
        if (0 != string_to_uint64(fields[0], id)) {
            ::std::cerr << "illegal id field" << ::std::endl;
            return -1;
        }
    }
    {
        ::std::vector<::std::string_view> inner_fields;
        split(fields[1], ',', inner_fields);
        for (auto field : inner_fields) {
            float weight;
            if (0 != string_to_float(field, weight)) {
                ::std::cerr << "illegal embedding field" << ::std::endl;
                return -1;
            }
            embedding.push_back(weight);
        }
    }
    category = fields[2];
    {
        ::std::vector<::std::string_view> inner_fields;
        split(fields[3], ',', inner_fields);
        for (auto field : inner_fields) {
            tags.emplace(field);
        }
    }
    return 0;
}

void Item::serialize_to_line(::std::string& line) {
    line.clear();
    line.append(::std::to_string(id));
    line.append(1, '\t');
    for (auto weight : embedding) {
        line.append(::std::to_string(weight));
        line.append(1, ',');
    }
    line.resize(line.size() - 1);
    line.append(1, '\t');
    line.append(category);
    line.append(1, '\t');
    for (auto& tag : tags) {
        line.append(tag);
        line.append(1, ',');
    }
    line.resize(line.size() - 1);
}

}
