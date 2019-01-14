#include "assets.hpp"

#include <unordered_map>

namespace no {

static std::string asset_directory_path = "assets";

static std::unordered_map<std::string, int> assets;

void set_asset_directory(const std::string& path) {
	asset_directory_path = path;
}

std::string asset_directory() {
	return asset_directory_path;
}

std::string asset_path(const std::string& path) {
	return asset_directory_path + "/" + path;
}

void register_asset(const std::string& name, int id) {
	assets[name] = id;
}

bool is_asset_loaded(const std::string& name) {
	return assets.find(name) != assets.end();
}

int find_asset(const std::string& name) {
	const auto& asset = assets.find(name);
	if (asset == assets.end()) {
		return -1;
	}
	return asset->second;
}

void unregister_asset(const std::string& name) {
	assets.erase(name);
}

}
