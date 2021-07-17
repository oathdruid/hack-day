#include "history.h"
#include "util.h"

namespace baidu::minirec {

void History::set_capacity(size_t bytes) {
    _capacity_bytes = bytes;
}

int History::read(const ::std::string& user_id, ::std::string& showed_items_line) {
    ::std::shared_lock lock(_mutex);
    showed_items_line = _showed_items_per_user[user_id];
    return 0;
}

int History::append(const ::std::string& user_id, ::std::string_view new_showed_items_line) {
    if (new_showed_items_line.empty()) {
        return 0;
    }

    ::std::unique_lock lock(_mutex);
    auto& base_showed_items_line = _showed_items_per_user[user_id];

    // 如果超长则先尝试截断
    if (base_showed_items_line.size() + new_showed_items_line.size() > _capacity_bytes) {
        size_t erase_size = new_showed_items_line.size() + base_showed_items_line.size() - _capacity_bytes;
        // 对齐到下一个分割点
        size_t next_comma_pos = base_showed_items_line.find_first_of(',', erase_size);
        if (next_comma_pos != ::std::string::npos) {
            base_showed_items_line.erase(0, next_comma_pos + 1);
        } else {
            base_showed_items_line.clear();
        }
    }

    // 尾部追加
    if (!base_showed_items_line.empty()) {
        base_showed_items_line.append(1, ',');
    }
    base_showed_items_line.append(new_showed_items_line);
    return 0;
}

}
