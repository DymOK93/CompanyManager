#include "company_manager_ui.h"
#include <QtWidgets/QApplication>


int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	CompanyManagerUI w;
	w.show();
	return a.exec();
	return 0;
} 
