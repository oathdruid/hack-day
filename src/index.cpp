#include "index.h"
#include "util.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_set>

namespace baidu::minirec {

int Index::load_user_embeddings(const ::std::string& file_name) {
    ::std::ifstream ifs(file_name);
    if (!ifs) {
        ::std::cerr << "open user embedding file " << file_name << " failed" << ::std::endl;
        return -1;
    }

    size_t num = 0;
    ::std::string line;
    ::std::vector<::std::string_view> fields;
    ::std::vector<::std::string_view> inner_fields;
    while (::std::getline(ifs, line)) {
        split(line, '\t', fields);
        if (fields.size() < 2) {
            ::std::cerr << "illegal user file" << ::std::endl;
            return -1;
        }
        split(fields[1], ',', inner_fields);
        ::std::vector<float> embedding;
        for (auto field : inner_fields) {
            float weight;
            if (0 != string_to_float(field, weight)) {
                ::std::cerr << "illegal user embedding field" << ::std::endl;
                return -1;
            }
            embedding.push_back(weight);
        }
        _user_embeddings.emplace(fields[0], ::std::move(embedding));
        num++;
    }
    ::std::cerr << "load " << num << " user embeddings" << ::std::endl;
    return 0;
}

int Index::load_items(const ::std::string& file_name) {
    ::std::ifstream ifs(file_name);
    if (!ifs) {
        ::std::cerr << "open user embedding file " << file_name << " failed" << ::std::endl;
        return -1;
    }

    size_t num = 0;
    ::std::string line;
    while (::std::getline(ifs, line)) {
        ::std::unique_ptr<Item> item {new Item};
        if (0 != item->parse_from_line(line)) {
            ::std::cerr << "parse line failed" << std::endl;
            return -1;
        }
        _items.emplace(item->id, ::std::move(item));
        num++;
    }
    ::std::cerr << "load " << num << " items " << std::endl;
    return 0;
}

int Index::add_item(::std::string_view line) {
    ::std::unique_ptr<Item> item {new Item};
    if (0 != item->parse_from_line(line)) {
        ::std::cerr << "parse item line failed" << std::endl;
        return -1;
    }
    {
        ::std::unique_lock lock(_mutex);
        _items.emplace(item->id, ::std::move(item));
    }
    return 0;
}

int Index::query(const ::std::string& user_id, ::std::string_view showed_items_line,
        size_t topk, ::std::vector<::std::string>& top_item_lines) {
    top_item_lines.clear();
    top_item_lines.reserve(topk);

    // 解析并构建去重表
    ::std::vector<::std::string_view> fields;
    split(showed_items_line, ',', fields);
    ::std::unordered_set<uint64_t> showed_item_ids(fields.size());
    for (auto field : fields) {
        uint64_t id;
        if (0 != string_to_uint64(field, id)) {
            ::std::cerr << "corrupted show list" << ::std::endl;
            return -1;
        }
        showed_item_ids.insert(id);
    }
    
    // 取得用户向量
    auto iter = _user_embeddings.find(user_id);
    if (iter == _user_embeddings.end()) {
        ::std::cerr << "get user embedding failed" << ::std::endl;
        return -1;
    }
    auto& user_embedding = iter->second;

    {
        // 遍历打分
        ::std::vector<::std::pair<float, Item*>> result;
        result.reserve(_items.size());
        ::std::shared_lock lock(_mutex);
        for (auto& pair : _items) {
            auto& item = pair.second;
            if (0 != showed_item_ids.count(item->id)) {
                continue;
            }
            float score = 0.0f;
            for (size_t i = 0; i < user_embedding.size(); ++i) {
                score += user_embedding[i] * item->embedding[i];
            }
            result.push_back(::std::make_pair(score, item.get()));
        }
        // 排序取topk
        ::std::sort(result.begin(), result.end(), ::std::greater<::std::pair<float, Item*>>());
        for (auto& pair : result) {
            if (top_item_lines.size() >= topk) {
                break;
            }
            ::std::string top_item_line;
            pair.second->serialize_to_line(top_item_line);
            top_item_lines.emplace_back(::std::move(top_item_line));
        }
    }
    return 0;
}

}
