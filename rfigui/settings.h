#ifndef SETTINGS_H
#define SETTINGS_H

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class SettingValue {
 public:
  virtual ~SettingValue() {}
  virtual std::string ValueToString() const = 0;
  virtual void SetFromString(const std::string& valueStr) = 0;
  virtual std::unique_ptr<SettingValue> Create() const = 0;
};

class StringSetting final : public SettingValue {
 public:
  StringSetting() {}
  StringSetting(const std::string& value) { _value = value; }
  std::string& Value() { return _value; }
  const std::string& Value() const { return _value; }
  std::string ValueToString() const override { return _value; }
  void SetFromString(const std::string& valueStr) override {
    _value = valueStr;
  }
  std::unique_ptr<SettingValue> Create() const override {
    return std::unique_ptr<SettingValue>(new StringSetting(_value));
  }

 private:
  std::string _value;
};

class SettingItem {
 public:
  SettingItem(std::unique_ptr<SettingValue> defaultValue)
      : _defaultValue(std::move(defaultValue)) {}
  bool HasValue() const { return _value != nullptr; }
  SettingValue& Value() { return *_value; }
  void SetValue(std::unique_ptr<SettingValue> value) {
    _value = std::move(value);
  }
  const SettingValue& DefaultValue() const { return *_defaultValue; }
  const SettingValue& ValueOrDefault() const {
    return _value == nullptr ? *_defaultValue : *_value;
  }
  void MakeDefault() { _value.reset(); }

 private:
  std::unique_ptr<SettingValue> _value, _defaultValue;
};

class Settings {
 public:
  Settings();

  static std::string GetConfigDir();

  std::string GetStrategyFilename() const;

  void Load();
  void Save() const;

  void InitializeWorkStrategy();

  std::vector<std::string> RecentFiles() const {
    return getStrArr("recent-files", 10);
  }
  void SetRecentFiles(const std::vector<std::string>& recentFiles) {
    return setStrArr("recent-files", recentFiles);
  }

  std::vector<std::string> RecentStrategies() const {
    return getStrArr("recent-strategies", 10);
  }
  void SetRecentStrategies(const std::vector<std::string>& recentStrategies) {
    return setStrArr("recent-strategies", recentStrategies);
  }

 private:
  static std::string getConfigFilename();

  void initStr(const std::string& key, const std::string& defaultStr) {
    _settings.emplace(
        key, std::unique_ptr<SettingValue>(new StringSetting(defaultStr)));
  }

  void initStrArray(const std::string& key,
                    const std::vector<std::string>& defaultVal) {
    for (size_t i = 0; i != defaultVal.size(); ++i)
      _settings.emplace(
          key + '[' + std::to_string(i) + ']',
          std::unique_ptr<SettingValue>(new StringSetting(defaultVal[i])));
  }

  void set(const std::string& key, const std::string& value) {
    auto iter = _settings.find(key);
    if (iter == _settings.end())
      throw std::runtime_error("Unknown setting in settings file: " + key);
    if (!iter->second.HasValue())
      iter->second.SetValue(iter->second.DefaultValue().Create());
    iter->second.Value().SetFromString(value);
  }

  std::string getStr(const std::string& key) const {
    return static_cast<const StringSetting&>(
               _settings.find(key)->second.ValueOrDefault())
        .Value();
  }

  void setStr(const std::string& key, const std::string& value) {
    SettingItem& item = _settings.find(key)->second;
    if (static_cast<const StringSetting&>(item.DefaultValue()).Value() == value)
      item.MakeDefault();
    else {
      if (!item.HasValue()) item.SetValue(item.DefaultValue().Create());
      static_cast<StringSetting&>(item.Value()).Value() = value;
    }
  }

  std::vector<std::string> getStrArr(const std::string& key, size_t n) const {
    std::vector<std::string> arr(n);
    for (size_t i = 0; i != n; ++i)
      arr[i] = getStr(key + '[' + std::to_string(i) + ']');
    return arr;
  }

  void setStrArr(const std::string& key,
                 const std::vector<std::string>& values) {
    for (size_t i = 0; i != values.size(); ++i)
      setStr(key + '[' + std::to_string(i) + ']', values[i]);
  }

  std::map<std::string, SettingItem> _settings;
};

#endif
