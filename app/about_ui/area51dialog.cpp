#include "about_ui/area51dialog.h"

#include <QPushButton>
#include <QVBoxLayout>

// Hidden developer panel. Strings here are intentionally not tr()-translated.
Area51Dialog::Area51Dialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle("Area 51");

  auto *layout = new QVBoxLayout(this);

  auto *crash_btn = new QPushButton("Crash", this);
  connect(crash_btn, &QPushButton::clicked, this, []() {
    volatile int *p = nullptr;
    *p = 0xDEAD;
  });
  layout->addWidget(crash_btn);
}
