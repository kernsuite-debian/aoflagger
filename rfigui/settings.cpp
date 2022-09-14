#include "settings.h"

#include <glibmm/miscutils.h>

#include <boost/algorithm/string.hpp>

#include <filesystem>
#include <fstream>

#include "../lua/default-strategy.h"

Settings::Settings() {
  initStrArray("recent-files", std::vector<std::string>(10));
  initStrArray("recent-strategies", std::vector<std::string>(10));
}

std::string Settings::getConfigFilename() {
  return (std::filesystem::path(GetConfigDir()) / "settings").string();
}

std::string Settings::GetStrategyFilename() const {
  return (std::filesystem::path(GetConfigDir()) / "strategy.lua").string();
}

std::string Settings::GetConfigDir() {
  std::filesystem::path configPath =
      std::filesystem::path(Glib::get_user_config_dir()) / "aoflagger";
  if (!std::filesystem::is_directory(configPath))
    std::filesystem::create_directory(configPath);
  return configPath.string();
}

void Settings::InitializeWorkStrategy() {
  std::string filename = GetStrategyFilename();
  // if(!std::filesystem::exists(filename))
  //{
  std::ofstream str(filename);
  str.write(reinterpret_cast<const char*>(data_strategies_generic_default_lua),
            data_strategies_generic_default_lua_len);
  if (!str)
    throw std::runtime_error(
        "Failed to write working file for Lua strategy: " + filename +
        ", size " + std::to_string(data_strategies_generic_default_lua_len));
  //}
}

void Settings::Load() {
  std::string configFilename = getConfigFilename();
  if (std::filesystem::exists(configFilename)) {
    std::ifstream file(configFilename);
    std::string line;
    std::getline(file, line);
    while (file) {
      boost::algorithm::trim(line);
      if (!line.empty() && line[0] != '#') {
        size_t sep = line.find('=');
        if (sep == std::string::npos)
          throw std::runtime_error("Invalid key-value pair in config file " +
                                   configFilename);
        std::string key = boost::algorithm::trim_copy(line.substr(0, sep));
        std::string value = boost::algorithm::trim_copy(line.substr(sep + 1));
        if (key.empty() || value.empty())
          throw std::runtime_error("Empty key or value in config file " +
                                   configFilename);
        set(key, value);
      }

      std::getline(file, line);
    }
  }
}

void Settings::Save() const {
  std::string configFilename = getConfigFilename();
  std::ofstream file(configFilename);
  if (!file)
    throw std::runtime_error("Error opening config file: " + configFilename);
  file
      << "# This is the user settings file for the AOFlagger software package\n"
         "# Any unchanged settings will be preceded by a hash symbol (#)\n"
         "# Some of these settings can be found in the 'rfigui' application "
         "under\n"
         "# menu 'edit', option 'preferences'.\n"
         "\n";
  for (const auto& item : _settings) {
    if (!item.second.HasValue()) file << "# ";
    file << item.first << '=' << item.second.ValueOrDefault().ValueToString()
         << '\n';
  }
}
