#pragma once

#include <string>

namespace no {

void set_asset_directory(const std::string& path);
std::string asset_directory();
std::string asset_path(const std::string& path);

void register_asset(const std::string& name, int id);
bool is_asset_loaded(const std::string& name);
int find_asset(const std::string& name);
void unregister_asset(const std::string& name);

}
