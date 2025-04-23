#include "cvqa_cal.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CVQA_Cal w;
    w.show();
    return a.exec();
}
