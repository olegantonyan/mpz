#ifndef SINGLEINSTANCEGUARD_H
#define SINGLEINSTANCEGUARD_H

#include <QtGlobal>

template <typename Derived>
class SingleInstanceGuard {
protected:
  explicit SingleInstanceGuard(const char *type_name) {
    if (alive) {
      qFatal("%s must be instantiated only once", type_name);
    }
    alive = true;
  }
  ~SingleInstanceGuard() { alive = false; }

  SingleInstanceGuard(const SingleInstanceGuard &) = delete;
  SingleInstanceGuard &operator=(const SingleInstanceGuard &) = delete;
  SingleInstanceGuard(SingleInstanceGuard &&) = delete;
  SingleInstanceGuard &operator=(SingleInstanceGuard &&) = delete;

private:
  static inline bool alive = false;
};

#endif // SINGLEINSTANCEGUARD_H
