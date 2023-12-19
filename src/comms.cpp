module;

import logger;

#include <cstring>
#include <string>
#include <vector>

export module comms;

export namespace Comms {

enum Type {
  Group = 0,
  Log = 1,
  Control = 2,
  String = 3,
  Float = 4,
};

class Item {
 public:
  Item(int parent, std::string name, Type type, bool writable = false)
      : parent(parent), name(name), type(type), writable(writable) {
  }

  virtual int definition_length() {
    return 0;
  }

  virtual int write_definition(char* buffer) {
  }

  virtual int value_length() {
    return 0;
  }

  virtual int write_value(char* buffer) {
  }

  virtual void on_received(char* data, int len) {
  }

 protected:
  int parent;
  std::string name;
  Type type;
  bool writable;
};

class ItemGroup: public Item {
 public:
  ItemGroup(int parent, std::string name) : Item(parent, name, Group) {
  }
};

class ItemControl: public Item {
 public:
  ItemControl(int parent, std::string name) : Item(parent, name, Control) {
    // TODO: add callback parameter
  }
};

class ItemString: public Item {
 public:
  ItemString(int parent, std::string name, std::string value, bool writable = false)
    : Item(parent, name, String, writable), value(value) {
  }

  int definition_length() {
    return value.size();
  }

  int write_definition(char* buffer) {
    memcpy(buffer, value.c_str(), definition_length());
  }

  int value_length() {
    return value.size();
  }

  int write_value(char* buffer) {
    memcpy(buffer, value.c_str(), value_length());
  }

 private:
  std::string value;
};

class ItemFloat: public Item {
 public:
  ItemFloat(int parent, std::string name, double value, bool writable = false)
    : Item(parent, name, String, writable), value(value) {
  }

  int definition_length() {
    return sizeof(value);
  }

  int write_definition(char* buffer) {
    memcpy(buffer, &value, definition_length());
  }

  int value_length() {
    return sizeof(value);
  }

  int write_value(char* buffer) {
    memcpy(buffer, &value, value_length());
  }

 private:
  double value;
};

class Items {
 public:
  int add(Item* item) {
    items.push_back(item);
    return items.size();
  }

  void on_item_received(int id, char* data, int len) {
    logger::info("Received item %d with payload size %d", id, len);
    if (id > items.size()) {
      logger::warn("Received item with id %d but max id is %d", id, items.size());
    } else {
      items[id - 1]->on_received(data, len);
    }
  }

 private:
  std::vector<Item*> items;
};

}
