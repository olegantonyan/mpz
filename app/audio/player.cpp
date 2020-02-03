#include "player.h"

#include <QDebug>

namespace Audio {
  Player::Player() {
  }

  void Player::play() {
    qDebug() << "start play";
    Audio::Decoder dec;
    Audio::Output ao(&dec);
    ao.wait();
    qDebug() << "finish play";
  }
}
