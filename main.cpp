#include "EasyFile.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    EasyFile w;
    w.show();
    return a.exec();
}
