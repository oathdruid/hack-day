#pragma once

#include <unordered_map>
#include <memory>
#include <shared_mutex>

#include "item.h"

namespace baidu::minirec {

class Index {
public:
    // 初始化阶段
    int load_user_embeddings(const ::std::string& file_name);
    int load_items(const ::std::string& file_name);

    // 运行阶段
    // 使用string模拟服务间序列化
    int add_item(::std::string_view item_line);
    int query(const ::std::string& user_id, ::std::string_view showed_items_line,
            size_t topk, ::std::vector<::std::string>& top_item_lines);

private:
    ::std::shared_mutex _mutex;
    ::std::unordered_map<::std::string, ::std::vector<float>> _user_embeddings;
    ::std::unordered_map<uint64_t, ::std::unique_ptr<Item>> _items;
};

}
