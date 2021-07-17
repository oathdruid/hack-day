#pragma once

#include "index.h"
#include "history.h"

#include <unordered_set>

namespace baidu::minirec {

struct DedupContext {
    ::std::unordered_set<uint64_t> dedup_items;
    const Item* recent_item {nullptr};
    size_t history_items_num {0};
    size_t dedup_items_num {0};
};

class Recommender {
public:
    // 初始化操作
    int load_similarity_matrix(const ::std::string& file_name);
    void set_index(Index& index);
    void set_history(History& history);
    void set_index_candidate_num(size_t num);

    // 运行时操作
    int recommend(const ::std::string& user_id, ::std::string& item_ids_line);

private:
    static constexpr size_t RECOMMEND_NUM = 4;

    int build_dedup_context(::std::string_view showed_items_line, DedupContext& dedup_context);
    void update_dedup_context(Item* item, DedupContext& dedup_context);
    bool history_filter(::std::unique_ptr<Item>& item, DedupContext& dedup_context);
    bool diversity_filter(Item* item, DedupContext& dedup_context);

    Index* _index {nullptr};
    History* _history {nullptr};
    ::std::unordered_map<uint64_t, ::std::vector<uint64_t>> _similarity_matrix;
    size_t _index_candidate_num {256};
};

}
