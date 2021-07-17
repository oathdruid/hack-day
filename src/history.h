#pragma once

#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace baidu::minirec {

class History {
public:
    // 设置单用户最大长度限制
    void set_capacity(size_t bytes);

    int read(const ::std::string& user_id, ::std::string& showed_items_line);
    int append(const ::std::string& user_id, ::std::string_view new_showed_items_line);

private:
    ::std::shared_mutex _mutex;
    size_t _capacity_bytes {1024};
    ::std::unordered_map<::std::string, ::std::string> _showed_items_per_user;
};

}
