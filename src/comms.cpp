module;

#include <string>

export module comms;

static int next_id = 1;

export namespace Comms {

enum Type {
  Group = 0,
  Float = 1,
  Log = 2,
  Control = 3,
};

class Item {
 public:
  Item(Item* parent, std::string name, Type type)
      : id(next_id++), parent(parent), name(name), type(type) {
  }

 private:
  int id;
  Item* parent;
  std::string name;
  Type type;
};

}
