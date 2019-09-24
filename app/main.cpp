#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

#include <yaml-cpp/yaml.h>
#include <iostream>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  QCoreApplication::setOrganizationName("mpz");
  QCoreApplication::setOrganizationDomain("github.com/olegantonyan/mpz");
  QCoreApplication::setApplicationName("mpz");

  YAML::Node config = YAML::LoadFile("/home/oleg/Desktop/config.yaml");
  if (config["hello"]) {
    qDebug() << "hello " << QString(config["hello"].as<std::string>().c_str()) << "\n";
  } else {
    qDebug() << "nope";
  }

  MainWindow w;
  w.show();
  return a.exec();
}
