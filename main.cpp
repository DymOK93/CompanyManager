#include <QtWidgets/QApplication>
#include "company_manager_ui.h"

int main(int argc, char* argv[]) {
  QApplication a(argc, argv);
  CompanyManagerUI w;
  w.show();
  return a.exec();
}
