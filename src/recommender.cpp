#include "recommender.h"
#include "util.h"

#include <fstream>
#include <iostream>

namespace baidu::minirec {

int Recommender::load_similarity_matrix(const ::std::string& file_name) {
    ::std::ifstream ifs(file_name);
    if (!ifs) {
        ::std::cerr << "open similarity matrix file " << file_name << " failed" << ::std::endl;
        return -1;
    }

    size_t num = 0;
    ::std::string line;
    ::std::vector<::std::string_view> fields;
    ::std::vector<::std::string_view> inner_fields;
    while (::std::getline(ifs, line)) {
        split(line, '\t', fields);
        if (fields.size() < 2) {
            ::std::cerr << "similarity file line fields error" << ::std::endl;
            return -1;
        }
        uint64_t source_id;
        if (0 != string_to_uint64(fields[0], source_id)) {
            ::std::cerr << "similarity field[0] not an integer" << ::std::endl;
            return -1;
        }
        split(fields[1], ',', inner_fields);
        ::std::vector<uint64_t> target_ids;
        target_ids.reserve(inner_fields.size());
        for (auto field : inner_fields) {
            uint64_t target_id;
            if (0 != string_to_uint64(field, target_id)) {
                ::std::cerr << "similarity field[1] not an integer list" << ::std::endl;
                return -1;
            }
            target_ids.push_back(target_id);
        }
        _similarity_matrix.emplace(source_id, ::std::move(target_ids));
        num++;
    }

    ::std::cerr << "load " << num << " similarity line" << ::std::endl;
    return 0;
}

void Recommender::set_index(Index& index) {
    _index = &index;
}

void Recommender::set_history(History& history) {
    _history = &history;
}

void Recommender::set_index_candidate_num(size_t num) {
    _index_candidate_num = num;
}

int Recommender::recommend(const ::std::string& user_id, ::std::string& item_ids_line) {
    item_ids_line.clear();

    Timer read_history_timer;
    // 读取用户历史
    ::std::string showed_items_line;
    if (0 != _history->read(user_id, showed_items_line)) {
        ::std::cerr << "read history failed" << ::std::endl;
        return -1;
    }
    read_history_timer.stop();

    Timer index_query_timer;
    // 检索推荐侯选集
    ::std::vector<::std::string> top_item_lines;
    if (0 != _index->query(user_id, showed_items_line, _index_candidate_num, top_item_lines)) {
        ::std::cerr << "query index failed" << ::std::endl;
        return -1;
    }
    index_query_timer.stop();

    Timer parse_result_timer;
    // 解析检索结果
    ::std::vector<::std::unique_ptr<Item>> top_items;
    top_items.reserve(top_item_lines.size());
    for (auto& top_item_line : top_item_lines) {
        ::std::unique_ptr<Item> item {new Item};
        if (0 != item->parse_from_line(top_item_line)) {
            ::std::cerr << "transfer from index to recommender error" << ::std::endl;
            return -1;
        }
        top_items.emplace_back(::std::move(item));
    }
    parse_result_timer.stop();

    Timer build_context_timer;
    // 构建去重上下文
    DedupContext dedup_context;
    if (0 != build_dedup_context(showed_items_line, dedup_context)) {
        ::std::cerr << "build dedup context failed" << ::std::endl;
        return -1;
    }
    build_context_timer.stop();

    Timer generate_list_timer;
    size_t num = 0;
    size_t pos = 0;
    size_t history_filter_count = 0;
    while (num < RECOMMEND_NUM) {
        // 连续跳过重复数据
        for (; pos < top_items.size(); ++pos) {
            if (!history_filter(top_items[pos], dedup_context)) {
                break;
            }
            history_filter_count++;
        }
        // 没有不重复的数据了，只能退出
        if (pos == top_items.size()) {
            break;
        }
        Item* best_item = top_items[pos].get();
        // 命中相似控制
        if (diversity_filter(best_item, dedup_context)) {
            // 尝试继续寻找一个不重复也不相似的
            for (size_t i = pos + 1; i < top_items.size(); ++i) {
                auto& item = top_items[i];
                if (history_filter(item, dedup_context)) {
                    continue;
                }
                if (diversity_filter(item.get(), dedup_context)) {
                    continue;
                }
                // 找到修改最佳指针
                best_item = item.get();
                break;
            }
            // 未找到保留之前的最好结果
        }
        num++;
        if (!item_ids_line.empty()) {
            item_ids_line.append(1, ',');
        }
        item_ids_line.append(::std::to_string(best_item->id));
        update_dedup_context(best_item, dedup_context);
    }
    generate_list_timer.stop();

    Timer append_history_timer;
    _history->append(user_id, item_ids_line);
    append_history_timer.stop();

    ::std::cerr << "user " << user_id << " query elaspe ("
        << read_history_timer.elaspe_us()
        << ", " << index_query_timer.elaspe_us()
        << ", " << parse_result_timer.elaspe_us()
        << ", " << build_context_timer.elaspe_us()
        << ", " << generate_list_timer.elaspe_us()
        << ", " << append_history_timer.elaspe_us()
        << ") history(" << dedup_context.history_items_num
        << ", " << dedup_context.dedup_items_num
        << ") funnel ("
        << top_items.size()
        << ", " << history_filter_count
        << ", " << num
        << ") get " << item_ids_line << ::std::endl;
    return 0;
}

int Recommender::build_dedup_context(::std::string_view showed_items_line,
        DedupContext& dedup_context) {
    ::std::vector<::std::string_view> fields;
    split(showed_items_line, ',', fields);
    dedup_context.history_items_num = fields.size();
    for (auto field : fields) {
        uint64_t id;
        if (0 != string_to_uint64(field, id)) {
            ::std::cerr << "show list format error" << ::std::endl;
            return -1;
        }
        dedup_context.dedup_items.insert(id);
        auto iter = _similarity_matrix.find(id);
        if (iter != _similarity_matrix.end()) {
            for (auto similar_id : iter->second) {
                dedup_context.dedup_items.insert(similar_id);
            }
        }
    }
    dedup_context.dedup_items_num = dedup_context.dedup_items.size();
    return 0;
}

void Recommender::update_dedup_context(Item* item, DedupContext& dedup_context) {
    dedup_context.dedup_items.insert(item->id);
    {
        auto iter = _similarity_matrix.find(item->id);
        if (iter != _similarity_matrix.end()) {
            for (auto similar_id : iter->second) {
                dedup_context.dedup_items.insert(similar_id);
            }
        }
    }
    dedup_context.recent_item = item;
}

bool Recommender::history_filter(::std::unique_ptr<Item>& item, DedupContext& dedup_context) {
    if (!item || item.get() == dedup_context.recent_item) {
        return true;
    }
    if (dedup_context.dedup_items.find(item->id) != dedup_context.dedup_items.end()) {
        item.reset();
        return true;
    }
    return false;
}

bool Recommender::diversity_filter(Item* item, DedupContext& dedup_context) {
    if (dedup_context.recent_item == nullptr) {
        return false;
    }
    if (item->category == dedup_context.recent_item->category) {
        return true;
    }
    for (auto& tag : item->tags) {
        if (0 != dedup_context.recent_item->tags.count(tag)) {
            return true;
        }
    }
    return false;
}

}
