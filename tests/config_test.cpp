#include "hyprbar/core/config_manager.h"
#include "test_utils.h"
#include <fstream>

using namespace hyprbar;

void test_config_value_integer() {
  ConfigValue val(static_cast<int64_t>(42));
  test::assert(val.type == ConfigValue::Type::Integer, "Integer type");
  test::assert(val.int_value == 42, "Integer value");
}

void test_config_value_double() {
  ConfigValue val(3.14);
  test::assert(val.type == ConfigValue::Type::Double, "Double type");
  test::assert(val.double_value == 3.14, "Double value");
}

void test_config_value_string() {
  ConfigValue val("test");
  test::assert(val.type == ConfigValue::Type::String, "String type");
  test::assert(val.string_value == "test", "String value");
}

void test_config_value_boolean() {
  ConfigValue val_true(true);
  ConfigValue val_false(false);
  test::assert(val_true.type == ConfigValue::Type::Boolean, "Boolean type");
  test::assert(val_true.bool_value == true, "Boolean true");
  test::assert(val_false.bool_value == false, "Boolean false");
}

void test_config_value_array() {
  std::vector<ConfigValue> arr = {ConfigValue(static_cast<int64_t>(1)),
                                  ConfigValue(static_cast<int64_t>(2)),
                                  ConfigValue(static_cast<int64_t>(3))};
  ConfigValue val(arr);
  test::assert(val.type == ConfigValue::Type::Array, "Array type");
  test::assert(val.array_value.size() == 3, "Array size");
  test::assert(val.array_value[0].int_value == 1, "Array element 0");
  test::assert(val.array_value[2].int_value == 3, "Array element 2");
}

void test_config_value_object() {
  std::map<std::string, ConfigValue> obj;
  obj["name"] = ConfigValue("hyprbar");
  obj["version"] = ConfigValue(static_cast<int64_t>(1));

  ConfigValue val(obj);
  test::assert(val.type == ConfigValue::Type::Object, "Object type");
  test::assert(val.object_value.size() == 2, "Object size");
  test::assert(val.object_value["name"].string_value == "hyprbar",
               "Object string field");
  test::assert(val.object_value["version"].int_value == 1, "Object int field");
}

void test_parse_simple_json() {
  ConfigManager mgr;
  std::string json = R"({"key": "value", "number": 42})";

  // Write temp file
  std::ofstream tmp("/tmp/test_config.json");
  tmp << json;
  tmp.close();

  bool loaded = mgr.load("/tmp/test_config.json");
  test::assert(loaded, "Parse simple JSON");
}

void test_default_config() {
  Config cfg;
  test::assert(cfg.bar.height == 30, "Default bar height");
  test::assert(cfg.bar.background == "#1e1e2e", "Default background");
  test::assert(cfg.bar.color == "#cdd6f4", "Default color");
}

void run_config_tests() {
  std::cout << "\n--- Config Tests ---" << std::endl;
  test_config_value_integer();
  test_config_value_double();
  test_config_value_string();
  test_config_value_boolean();
  test_config_value_array();
  test_config_value_object();
  test_parse_simple_json();
  test_default_config();
}
