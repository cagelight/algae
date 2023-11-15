#include <QApplication>
#include <QTextCodec>

#include "algae.hh"

int main(int argc, char * * argv) {	
	QApplication app {argc, argv};
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
	AlgaeCore * core = new AlgaeCore {};
	core->show();
	return app.exec();
}

