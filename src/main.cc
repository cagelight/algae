#include <QApplication>

#include "algae.hh"

int main(int argc, char * * argv) {	
	QApplication app {argc, argv};
	AlgaeCore * core = new AlgaeCore {};
	core->show();
	return app.exec();
}

