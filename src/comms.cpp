module;

import logger;

#include <string>
#include <vector>

export module comms;

export namespace Comms {

enum Type {
  Group = 0,
  Float = 1,
  Log = 2,
  Control = 3,
};

class Item {
 public:
  Item(int parent, std::string name, Type type)
      : parent(parent), name(name), type(type) {
  }

  virtual int definition_length() = 0;
  virtual int write_definition(char* buffer) = 0;

  virtual int value_length() = 0;
  virtual int write_value(char* buffer) = 0;

  virtual void on_received(char* data, int len) {}

 private:
  int parent;
  std::string name;
  Type type;
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
