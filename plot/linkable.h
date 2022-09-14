#ifndef LINKABLE_H
#define LINKABLE_H

#include <algorithm>
#include <memory>
#include <vector>

struct LinkableDefaultData {};

template <typename Implementation, typename SharedData = LinkableDefaultData>
class Linkable {
 public:
  using iterator = typename std::vector<Implementation*>::iterator;
  using const_iterator = typename std::vector<Implementation*>::const_iterator;

  Linkable() : _group(new Group()) {
    _group->members.emplace_back(static_cast<Implementation*>(this));
  }

  ~Linkable() { Unlink(); }

  void Link(Implementation& other) {
    // Are they not linked yet?
    if (other._group != _group) {
      std::shared_ptr<Group> otherGroup = other._group;
      // Combine the two groups
      for (auto& scalePtr : otherGroup->members) {
        scalePtr->_group = _group;
        _group->members.emplace_back(scalePtr);
      }
    }
  }

  void Unlink() {
    auto& members = _group->members;
    members.erase(std::find(members.begin(), members.end(), this));
    _group = std::make_shared<Group>();
    _group->members.emplace_back(static_cast<Implementation*>(this));
  }

  bool IsLinked() const { return _group->members.size() > 1; }

  SharedData& Data() { return _group->shared; }
  const SharedData& Data() const { return _group->shared; }

  iterator begin() { return _group->members.begin(); }
  const_iterator begin() const { return _group->members.begin(); }
  iterator end() { return _group->members.end(); }
  const_iterator end() const { return _group->members.end(); }
  size_t MemberCount() const { return _group->members.size(); }

 private:
  struct Group {
    SharedData shared;
    std::vector<Implementation*> members;
  };
  std::shared_ptr<Group> _group;
};

#endif
