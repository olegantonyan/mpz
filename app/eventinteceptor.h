#ifndef EVENTINTECEPTOR_H
#define EVENTINTECEPTOR_H

#include <QObject>
#include <QEvent>

template <class T>
class EventInterceptor : public QObject {
public:
  explicit EventInterceptor(void (T::*cb)(QEvent *event), T *cbobj) :
    QObject(cbobj), callback_object(cbobj), callback(cb) {
  }

protected:
  bool eventFilter(QObject *obj, QEvent *event) {
    (callback_object->*callback)(event);
    return QObject::eventFilter(obj, event);
  }

private:
  T *callback_object;
  void (T::*callback)(QEvent *event);
};
#endif // EVENTINTECEPTOR_H
