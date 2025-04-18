#include "fieldsize.h"
#include "ui_fieldsize.h"

#include <QGraphicsScene>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <opencv2/imgproc/imgproc.hpp>
#include <QImage>
#include <QMessageBox>



/* The FieldSize class is launched when the Field Size button is clicked in CVQA.
 * When a button is clicked in this child window is sends a signal to the parent window (CVQA) which tells it which test should be run.
 * The parent window writes back a value directly to a function in this child class which writes it to a lineEdit box.
 * The parent window sends back a picture which is displayed using the on_dialogSignalPicture function.
 */

FieldSize::FieldSize(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FieldSize)
{
    ui->setupUi(this);
    ui->graphicsView->setScene(&scene);
}

FieldSize::~FieldSize()
{
    delete ui;
}

void FieldSize::on_dialogSignalPicture(cv::Mat mat)
{
    scene.clear();
    ui->graphicsView->resetTransform();
    QPixmap pixmap=QPixmap::fromImage(QImage(mat.data,mat.cols,mat.rows,mat.step,QImage::Format::Format_RGB888));
    QPixmap pixmapscaled=pixmap.scaled(890,890,Qt::IgnoreAspectRatio,Qt::FastTransformation);
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmapscaled);
    scene.addItem(item);
    Graphics_view_zoom* z = new Graphics_view_zoom(ui->graphicsView);
    z->set_modifiers(Qt::NoModifier);


}

/* Symmetric 40 button. First on_pushButton_sym40 emits signal = 1 to CVQA.
 * Then CVQA wirtes the x field value to on_dialogSignalValuesym40X and if the value is within 2mm of 40cm then the boolean sym40XComplete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y field value to on_dialogSignalValuesym40Y and if the value is within 2mm of 40cm then the boolean sym40YComplete is set to true, otherwise the boolean is false.
 * If both the sym40XComplete and sym40YComplete are true then the 40x40 symmetric button turns green, otherwise it turns red.
 */
void FieldSize::on_pushButton_sym40_clicked()
{
    emit buttonPressed(1);

}

void FieldSize::on_dialogSignalValuesym40X(double d)
{
    ui->lineEdit_sym40X->setText(QString::number(d,'f',2));
    if(39.8<d && d<40.2)
    {
        fieldSizeFlags->sym40XComplete = true;
    } else
    {
        fieldSizeFlags->sym40XComplete = false;
    }
}

void FieldSize::on_dialogSignalValuesym40Y(double d)
{
    ui->lineEdit_sym40Y->setText(QString::number(d,'f',2));
    if(39.7<d&& d<40.3)
    {
        fieldSizeFlags->sym40YComplete = true;
    } else
    {
        fieldSizeFlags->sym40YComplete = false;
    }

    if(fieldSizeFlags->sym40XComplete&&fieldSizeFlags->sym40YComplete)
    {
        ui->pushButton_sym40->setStyleSheet("QPushButton { background-color: green; }");
    } else if(fieldSizeFlags->sym40XComplete==false&&fieldSizeFlags->sym40YComplete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X and Y field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->sym40XComplete==true&&fieldSizeFlags->sym40YComplete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->sym40XComplete==false&&fieldSizeFlags->sym40YComplete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym40->setStyleSheet("QPushButton { background-color: red; }");
    }
}

/* Symmetric 20 button. First on_pushButton_sym40 emits signal = 2 to CVQA.
 * Then CVQA wirtes the x field value to on_dialogSignalValuesym20X and if the value is within 2mm of 20cm then the boolean sym20XComplete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y field value to on_dialogSignalValuesym20Y and if the value is within 2mm of 20cm then the boolean sym20YComplete is set to true, otherwise the boolean is false.
 * If both the sym20XComplete and sym20YComplete are true then the 20x20 symmetric button turns green, otherwise it turns red.
 */
void FieldSize::on_pushButton_sym20_clicked()
{
    emit buttonPressed(2);
}

void FieldSize::on_dialogSignalValuesym20X(double d)
{
    ui->lineEdit_sym20X->setText(QString::number(d,'f',2));
    if(19.8<d && d<20.2)
    {
        fieldSizeFlags->sym20XComplete = true;
    } else
    {
        fieldSizeFlags->sym20XComplete = false;
    }

}

void FieldSize::on_dialogSignalValuesym20Y(double d)
{
    ui->lineEdit_sym20Y->setText(QString::number(d,'f',2));
    if(19.7<d&& d<20.3)
    {
        fieldSizeFlags->sym20YComplete = true;
    } else
    {
        fieldSizeFlags->sym20YComplete = false;
    }

    if(fieldSizeFlags->sym20XComplete&&fieldSizeFlags->sym20YComplete)
    {
        ui->pushButton_sym20->setStyleSheet("QPushButton { background-color: green; }");
    } else if(fieldSizeFlags->sym20XComplete==false&&fieldSizeFlags->sym20YComplete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X and Y field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->sym20XComplete==true&&fieldSizeFlags->sym20YComplete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->sym20XComplete==false&&fieldSizeFlags->sym20YComplete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym20->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* Symmetric 10 button. First on_pushButton_sym40 emits signal = 3 to CVQA.
 * Then CVQA wirtes the x field value to on_dialogSignalValuesym10X and if the value is within 2mm of 10cm then the boolean sym10XComplete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y field value to on_dialogSignalValuesym10Y and if the value is within 2mm of 10cm then the boolean sym10YComplete is set to true, otherwise the boolean is false.
 * If both the sym10XComplete and sym10YComplete are true then the 10x10 symmetric button turns green, otherwise it turns red.
 */
void FieldSize::on_pushButton_sym10_clicked()
{
    emit buttonPressed(3);
}

void FieldSize::on_dialogSignalValuesym10X(double d)
{
    ui->lineEdit_sym10X->setText(QString::number(d,'f',2));
    if(9.8<d && d<10.2)
    {
        fieldSizeFlags->sym10XComplete = true;
    } else
    {
        fieldSizeFlags->sym10XComplete = false;
    }
}

void FieldSize::on_dialogSignalValuesym10Y(double d)
{
    ui->lineEdit_sym10Y->setText(QString::number(d,'f',2));
    if(9.8<d&& d<10.2)
    {
        fieldSizeFlags->sym10YComplete = true;
    } else
    {
        fieldSizeFlags->sym10YComplete = false;
    }

    if(fieldSizeFlags->sym10XComplete&&fieldSizeFlags->sym10YComplete)
    {
        ui->pushButton_sym10->setStyleSheet("QPushButton { background-color: green; }");
    } else if(fieldSizeFlags->sym10XComplete==false&&fieldSizeFlags->sym10YComplete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X and Y field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->sym10XComplete==true&&fieldSizeFlags->sym10YComplete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->sym10XComplete==false&&fieldSizeFlags->sym10YComplete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym10->setStyleSheet("QPushButton { background-color: red; }");
    }
}

/* Symmetric 5 button. First on_pushButton_sym40 emits signal = 4 to CVQA.
 * Then CVQA wirtes the x field value to on_dialogSignalValuesym5X and if the value is within 2mm of 5cm then the boolean sym5XComplete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y field value to on_dialogSignalValuesym5Y and if the value is within 2mm of 5cm then the boolean sym5YComplete is set to true, otherwise the boolean is false.
 * If both the sym5XComplete and sym5YComplete are true then the 5x5 symmetric button turns green, otherwise it turns red.
 */

void FieldSize::on_pushButton_sym5_clicked()
{
    emit buttonPressed(4);
}

void FieldSize::on_dialogSignalValuesym5X(double d)
{
    ui->lineEdit_sym5X->setText(QString::number(d,'f',2));
    if(4.8<d && d<5.2)
    {
        fieldSizeFlags->sym5XComplete = true;
    } else
    {
        fieldSizeFlags->sym5XComplete = false;
    }
}

void FieldSize::on_dialogSignalValuesym5Y(double d)
{
    ui->lineEdit_sym5Y->setText(QString::number(d,'f',2));
    if(4.8<d&& d<5.2)
    {
        fieldSizeFlags->sym5YComplete = true;
    } else
    {
        fieldSizeFlags->sym5YComplete = false;
    }

    if(fieldSizeFlags->sym5XComplete&&fieldSizeFlags->sym5YComplete)
    {
        ui->pushButton_sym5->setStyleSheet("QPushButton { background-color: green; }");
    } else if(fieldSizeFlags->sym5XComplete==false&&fieldSizeFlags->sym5YComplete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X and Y field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->sym5XComplete==true&&fieldSizeFlags->sym5YComplete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->sym5XComplete==false&&fieldSizeFlags->sym5YComplete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_sym5->setStyleSheet("QPushButton { background-color: red; }");
    }
}

/* Asymmetric 40 button. First on_pushButton_asym40 emits signal = 5 to CVQA.
 * Then CVQA wirtes the x1 field value to on_dialogSignalValueasym40X1 and if the value is within 2mm of 20cm then the boolean asym40X1Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the x2 field value to on_dialogSignalValueasym40X2 and if the value is within 2mm of 20cm then the boolean asym40X2Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y1 field value to on_dialogSignalValueasym40Y1 and if the value is within 2mm of 20cm then the boolean asym40Y1Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y2 field value to on_dialogSignalValueasym40Y2 and if the value is within 2mm of 20cm then the boolean asym40Y2Complete is set to true, otherwise the boolean is false.
 * If asym40X1Complete, asym40X2Complete, asym40Y1Complete, and asym40Y2Complete are true then the 40x40 asymmetric button turns green, otherwise it turns red.
 */
void FieldSize::on_pushButton_asym40_clicked()
{
    emit buttonPressed(5);
}

void FieldSize::on_dialogSignalValueasym40X1(double d)
{
    ui->lineEdit_asym40X1->setText(QString::number(d,'f',2));
    if(19.8<d && d<20.2)
    {
        fieldSizeFlags->asym40X1Complete = true;
    } else
    {
        fieldSizeFlags->asym40X1Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym40X2(double d)
{
    ui->lineEdit_asym40X2->setText(QString::number(d,'f',2));
    if(19.8<d && d<20.2)
    {
        fieldSizeFlags->asym40X2Complete = true;
    } else
    {
        fieldSizeFlags->asym40X2Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym40Y1(double d)
{
    ui->lineEdit_asym40Y1->setText(QString::number(d,'f',2));
    if(19.7<d && d<20.3)
    {
        fieldSizeFlags->asym40Y1Complete = true;
    } else
    {
        fieldSizeFlags->asym40Y1Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym40Y2(double d)
{
    ui->lineEdit_asym40Y2->setText(QString::number(d,'f',2));
    if(19.7<d&& d<20.3)
    {
        fieldSizeFlags->asym40Y2Complete = true;
    } else
    {
        fieldSizeFlags->asym40Y2Complete = false;
    }

    if(fieldSizeFlags->asym40X1Complete&&fieldSizeFlags->asym40X2Complete&&fieldSizeFlags->asym40Y1Complete&&fieldSizeFlags->asym40Y2Complete)
    {
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: green; }");
    } else if(fieldSizeFlags->asym40X1Complete==false&&fieldSizeFlags->asym40X2Complete==false&&fieldSizeFlags->asym40Y1Complete==false&&fieldSizeFlags->asym40Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==true&&fieldSizeFlags->asym40X2Complete==false&&fieldSizeFlags->asym40Y1Complete==false&&fieldSizeFlags->asym40Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==false&&fieldSizeFlags->asym40X2Complete==true&&fieldSizeFlags->asym40Y1Complete==false&&fieldSizeFlags->asym40Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==false&&fieldSizeFlags->asym40X2Complete==false&&fieldSizeFlags->asym40Y1Complete==true&&fieldSizeFlags->asym40Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==false&&fieldSizeFlags->asym40X2Complete==false&&fieldSizeFlags->asym40Y1Complete==false&&fieldSizeFlags->asym40Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, and Y1 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==false&&fieldSizeFlags->asym40X2Complete==false&&fieldSizeFlags->asym40Y1Complete==true&&fieldSizeFlags->asym40Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and X2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==false&&fieldSizeFlags->asym40X2Complete==true&&fieldSizeFlags->asym40Y1Complete==false&&fieldSizeFlags->asym40Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==false&&fieldSizeFlags->asym40X2Complete==true&&fieldSizeFlags->asym40Y1Complete==true&&fieldSizeFlags->asym40Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==true&&fieldSizeFlags->asym40X2Complete==false&&fieldSizeFlags->asym40Y1Complete==false&&fieldSizeFlags->asym40Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 and Y1 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==true&&fieldSizeFlags->asym40X2Complete==false&&fieldSizeFlags->asym40Y1Complete==true&&fieldSizeFlags->asym40Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==true&&fieldSizeFlags->asym40X2Complete==true&&fieldSizeFlags->asym40Y1Complete==false&&fieldSizeFlags->asym40Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==false&&fieldSizeFlags->asym40X2Complete==true&&fieldSizeFlags->asym40Y1Complete==true&&fieldSizeFlags->asym40Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==true&&fieldSizeFlags->asym40X2Complete==false&&fieldSizeFlags->asym40Y1Complete==true&&fieldSizeFlags->asym40Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==true&&fieldSizeFlags->asym40X2Complete==true&&fieldSizeFlags->asym40Y1Complete==false&&fieldSizeFlags->asym40Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y1 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym40X1Complete==true&&fieldSizeFlags->asym40X2Complete==true&&fieldSizeFlags->asym40Y1Complete==true&&fieldSizeFlags->asym40Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y2 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym40->setStyleSheet("QPushButton { background-color: red; }");
    }
}

/* Asymmetric 20 button. First on_pushButton_asym20 emits signal = 6 to CVQA.
 * Then CVQA wirtes the x1 field value to on_dialogSignalValueasym20X1 and if the value is within 2mm of 10cm then the boolean asym20X1Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the x2 field value to on_dialogSignalValueasym20X2 and if the value is within 2mm of 10cm then the boolean asym20X2Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y1 field value to on_dialogSignalValueasym20Y1 and if the value is within 2mm of 10cm then the boolean asym20Y1Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y2 field value to on_dialogSignalValueasym20Y2 and if the value is within 2mm of 10cm then the boolean asym20Y2Complete is set to true, otherwise the boolean is false.
 * If asym20X1Complete, asym20X2Complete, asym20Y1Complete, and asym20Y2Complete are true then the 20x20 asymmetric button turns green, otherwise it turns red.
 */
void FieldSize::on_pushButton_asym20_clicked()
{
    emit buttonPressed(6);
}

void FieldSize::on_dialogSignalValueasym20X1(double d)
{
    ui->lineEdit_asym20X1->setText(QString::number(d,'f',2));
    if(9.8<d && d<10.2)
    {
        fieldSizeFlags->asym20X1Complete = true;
    } else
    {
        fieldSizeFlags->asym20X1Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym20X2(double d)
{
    ui->lineEdit_asym20X2->setText(QString::number(d,'f',2));
    if(9.8<d && d<10.2)
    {
        fieldSizeFlags->asym20X2Complete = true;
    } else
    {
        fieldSizeFlags->asym20X2Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym20Y1(double d)
{
    ui->lineEdit_asym20Y1->setText(QString::number(d,'f',2));
    if(9.7<d && d<10.3)
    {
        fieldSizeFlags->asym20Y1Complete = true;
    } else
    {
        fieldSizeFlags->asym20Y1Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym20Y2(double d)
{
    ui->lineEdit_asym20Y2->setText(QString::number(d,'f',2));
    if(9.7<d&& d<10.3)
    {
        fieldSizeFlags->asym20Y2Complete = true;
    } else
    {
        fieldSizeFlags->asym20Y2Complete = false;
    }

    if(fieldSizeFlags->asym20X1Complete&&fieldSizeFlags->asym20X2Complete&&fieldSizeFlags->asym20Y1Complete&&fieldSizeFlags->asym20Y2Complete)
    {
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: green; }");
    } else if(fieldSizeFlags->asym20X1Complete==false&&fieldSizeFlags->asym20X2Complete==false&&fieldSizeFlags->asym20Y1Complete==false&&fieldSizeFlags->asym20Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==true&&fieldSizeFlags->asym20X2Complete==false&&fieldSizeFlags->asym20Y1Complete==false&&fieldSizeFlags->asym20Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==false&&fieldSizeFlags->asym20X2Complete==true&&fieldSizeFlags->asym20Y1Complete==false&&fieldSizeFlags->asym20Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==false&&fieldSizeFlags->asym20X2Complete==false&&fieldSizeFlags->asym20Y1Complete==true&&fieldSizeFlags->asym20Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==false&&fieldSizeFlags->asym20X2Complete==false&&fieldSizeFlags->asym20Y1Complete==false&&fieldSizeFlags->asym20Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, and Y1 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==false&&fieldSizeFlags->asym20X2Complete==false&&fieldSizeFlags->asym20Y1Complete==true&&fieldSizeFlags->asym20Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and X2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==false&&fieldSizeFlags->asym20X2Complete==true&&fieldSizeFlags->asym20Y1Complete==false&&fieldSizeFlags->asym20Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==false&&fieldSizeFlags->asym20X2Complete==true&&fieldSizeFlags->asym20Y1Complete==true&&fieldSizeFlags->asym20Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==true&&fieldSizeFlags->asym20X2Complete==false&&fieldSizeFlags->asym20Y1Complete==false&&fieldSizeFlags->asym20Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 and Y1 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==true&&fieldSizeFlags->asym20X2Complete==false&&fieldSizeFlags->asym20Y1Complete==true&&fieldSizeFlags->asym20Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==true&&fieldSizeFlags->asym20X2Complete==true&&fieldSizeFlags->asym20Y1Complete==false&&fieldSizeFlags->asym20Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==false&&fieldSizeFlags->asym20X2Complete==true&&fieldSizeFlags->asym20Y1Complete==true&&fieldSizeFlags->asym20Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==true&&fieldSizeFlags->asym20X2Complete==false&&fieldSizeFlags->asym20Y1Complete==true&&fieldSizeFlags->asym20Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==true&&fieldSizeFlags->asym20X2Complete==true&&fieldSizeFlags->asym20Y1Complete==false&&fieldSizeFlags->asym20Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y1 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym20X1Complete==true&&fieldSizeFlags->asym20X2Complete==true&&fieldSizeFlags->asym20Y1Complete==true&&fieldSizeFlags->asym20Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y2 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym20->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* Asymmetric 10 button. First on_pushButton_asym10 emits signal = 7 to CVQA.
 * Then CVQA wirtes the x1 field value to on_dialogSignalValueasym10X1 and if the value is within 2mm of 5cm then the boolean asym10X1Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the x2 field value to on_dialogSignalValueasym10X2 and if the value is within 2mm of 5cm then the boolean asym10X2Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y1 field value to on_dialogSignalValueasym10Y1 and if the value is within 2mm of 5cm then the boolean asym10Y1Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y2 field value to on_dialogSignalValueasym10Y2 and if the value is within 2mm of 5cm then the boolean asym10Y2Complete is set to true, otherwise the boolean is false.
 * If asym10X1Complete, asym10X2Complete, asym10Y1Complete, and asym10Y2Complete are true then the 40x40 asymmetric button turns green, otherwise it turns red.
 */
void FieldSize::on_pushButton_asym10_clicked()
{
    emit buttonPressed(7);
}

void FieldSize::on_dialogSignalValueasym10X1(double d)
{
    ui->lineEdit_asym10X1->setText(QString::number(d,'f',2));
    if(4.8<d && d<5.2)
    {
        fieldSizeFlags->asym10X1Complete = true;
    } else
    {
        fieldSizeFlags->asym10X1Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym10X2(double d)
{
    ui->lineEdit_asym10X2->setText(QString::number(d,'f',2));
    if(4.8<d && d<5.2)
    {
        fieldSizeFlags->asym10X2Complete = true;
    } else
    {
        fieldSizeFlags->asym10X2Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym10Y1(double d)
{
    ui->lineEdit_asym10Y1->setText(QString::number(d,'f',2));
    if(4.8<d && d<5.2)
    {
        fieldSizeFlags->asym10Y1Complete = true;
    } else
    {
        fieldSizeFlags->asym10Y1Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym10Y2(double d)
{
    ui->lineEdit_asym10Y2->setText(QString::number(d,'f',2));
    if(4.8<d&& d<5.2)
    {
        fieldSizeFlags->asym10Y2Complete = true;
    } else
    {
        fieldSizeFlags->asym10Y2Complete = false;
    }

    if(fieldSizeFlags->asym10X1Complete&&fieldSizeFlags->asym10X2Complete&&fieldSizeFlags->asym10Y1Complete&&fieldSizeFlags->asym10Y2Complete)
    {
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: green; }");
    } else if(fieldSizeFlags->asym10X1Complete==false&&fieldSizeFlags->asym10X2Complete==false&&fieldSizeFlags->asym10Y1Complete==false&&fieldSizeFlags->asym10Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==true&&fieldSizeFlags->asym10X2Complete==false&&fieldSizeFlags->asym10Y1Complete==false&&fieldSizeFlags->asym10Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==false&&fieldSizeFlags->asym10X2Complete==true&&fieldSizeFlags->asym10Y1Complete==false&&fieldSizeFlags->asym10Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==false&&fieldSizeFlags->asym10X2Complete==false&&fieldSizeFlags->asym10Y1Complete==true&&fieldSizeFlags->asym10Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==false&&fieldSizeFlags->asym10X2Complete==false&&fieldSizeFlags->asym10Y1Complete==false&&fieldSizeFlags->asym10Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, and Y1 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==false&&fieldSizeFlags->asym10X2Complete==false&&fieldSizeFlags->asym10Y1Complete==true&&fieldSizeFlags->asym10Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and X2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==false&&fieldSizeFlags->asym10X2Complete==true&&fieldSizeFlags->asym10Y1Complete==false&&fieldSizeFlags->asym10Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==false&&fieldSizeFlags->asym10X2Complete==true&&fieldSizeFlags->asym10Y1Complete==true&&fieldSizeFlags->asym10Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==true&&fieldSizeFlags->asym10X2Complete==false&&fieldSizeFlags->asym10Y1Complete==false&&fieldSizeFlags->asym10Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 and Y1 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==true&&fieldSizeFlags->asym10X2Complete==false&&fieldSizeFlags->asym10Y1Complete==true&&fieldSizeFlags->asym10Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==true&&fieldSizeFlags->asym10X2Complete==true&&fieldSizeFlags->asym10Y1Complete==false&&fieldSizeFlags->asym10Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==false&&fieldSizeFlags->asym10X2Complete==true&&fieldSizeFlags->asym10Y1Complete==true&&fieldSizeFlags->asym10Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==true&&fieldSizeFlags->asym10X2Complete==false&&fieldSizeFlags->asym10Y1Complete==true&&fieldSizeFlags->asym10Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==true&&fieldSizeFlags->asym10X2Complete==true&&fieldSizeFlags->asym10Y1Complete==false&&fieldSizeFlags->asym10Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y1 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym10X1Complete==true&&fieldSizeFlags->asym10X2Complete==true&&fieldSizeFlags->asym10Y1Complete==true&&fieldSizeFlags->asym10Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y2 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym10->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* Asymmetric 5 button. First on_pushButton_asym5 emits signal = 8 to CVQA.
 * Then CVQA wirtes the x1 field value to on_dialogSignalValueasym5X1 and if the value is within 2mm of 2.5cm then the boolean asym5X1Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the x2 field value to on_dialogSignalValueasym5X2 and if the value is within 2mm of 2.5cm then the boolean asym5X2Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y1 field value to on_dialogSignalValueasym5Y1 and if the value is within 2mm of 2.5cm then the boolean asym5Y1Complete is set to true, otherwise the boolean is false.
 * Then CVQA wirtes the y2 field value to on_dialogSignalValueasym5Y2 and if the value is within 2mm of 2.5cm then the boolean asym5Y2Complete is set to true, otherwise the boolean is false.
 * If asym5X1Complete, asym5X2Complete, asym5Y1Complete, and asym5Y2Complete are true then the 5x5 asymmetric button turns green, otherwise it turns red.
 */
void FieldSize::on_pushButton_asym5_clicked()
{
    emit buttonPressed(8);
}

void FieldSize::on_dialogSignalValueasym5X1(double d)
{
    ui->lineEdit_asym5X1->setText(QString::number(d,'f',2));
    if(2.3<d && d<2.7)
    {
        fieldSizeFlags->asym5X1Complete = true;
    } else
    {
        fieldSizeFlags->asym5X1Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym5X2(double d)
{
    ui->lineEdit_asym5X2->setText(QString::number(d,'f',2));
    if(2.3<d && d<2.7)
    {
        fieldSizeFlags->asym5X2Complete = true;
    } else
    {
        fieldSizeFlags->asym5X2Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym5Y1(double d)
{
    ui->lineEdit_asym5Y1->setText(QString::number(d,'f',2));
    if(2.3<d && d<2.7)
    {
        fieldSizeFlags->asym5Y1Complete = true;
    } else
    {
        fieldSizeFlags->asym5Y1Complete = false;
    }
}

void FieldSize::on_dialogSignalValueasym5Y2(double d)
{
    ui->lineEdit_asym5Y2->setText(QString::number(d,'f',2));
    if(2.3<d&& d<2.7)
    {
        fieldSizeFlags->asym5Y2Complete = true;
    } else
    {
        fieldSizeFlags->asym5Y2Complete = false;
    }

    if(fieldSizeFlags->asym5X1Complete&&fieldSizeFlags->asym5X2Complete&&fieldSizeFlags->asym5Y1Complete&&fieldSizeFlags->asym5Y2Complete)
    {
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: green; }");
    } else if(fieldSizeFlags->asym5X1Complete==false&&fieldSizeFlags->asym5X2Complete==false&&fieldSizeFlags->asym5Y1Complete==false&&fieldSizeFlags->asym5Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==true&&fieldSizeFlags->asym5X2Complete==false&&fieldSizeFlags->asym5Y1Complete==false&&fieldSizeFlags->asym5Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==false&&fieldSizeFlags->asym5X2Complete==true&&fieldSizeFlags->asym5Y1Complete==false&&fieldSizeFlags->asym5Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, Y1, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==false&&fieldSizeFlags->asym5X2Complete==false&&fieldSizeFlags->asym5Y1Complete==true&&fieldSizeFlags->asym5Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==false&&fieldSizeFlags->asym5X2Complete==false&&fieldSizeFlags->asym5Y1Complete==false&&fieldSizeFlags->asym5Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1, X2, and Y1 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==false&&fieldSizeFlags->asym5X2Complete==false&&fieldSizeFlags->asym5Y1Complete==true&&fieldSizeFlags->asym5Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and X2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==false&&fieldSizeFlags->asym5X2Complete==true&&fieldSizeFlags->asym5Y1Complete==false&&fieldSizeFlags->asym5Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==false&&fieldSizeFlags->asym5X2Complete==true&&fieldSizeFlags->asym5Y1Complete==true&&fieldSizeFlags->asym5Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==true&&fieldSizeFlags->asym5X2Complete==false&&fieldSizeFlags->asym5Y1Complete==false&&fieldSizeFlags->asym5Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 and Y1 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==true&&fieldSizeFlags->asym5X2Complete==false&&fieldSizeFlags->asym5Y1Complete==true&&fieldSizeFlags->asym5Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==true&&fieldSizeFlags->asym5X2Complete==true&&fieldSizeFlags->asym5Y1Complete==false&&fieldSizeFlags->asym5Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y1 and Y2 field sizes.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==false&&fieldSizeFlags->asym5X2Complete==true&&fieldSizeFlags->asym5Y1Complete==true&&fieldSizeFlags->asym5Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==true&&fieldSizeFlags->asym5X2Complete==false&&fieldSizeFlags->asym5Y1Complete==true&&fieldSizeFlags->asym5Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==true&&fieldSizeFlags->asym5X2Complete==true&&fieldSizeFlags->asym5Y1Complete==false&&fieldSizeFlags->asym5Y2Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y1 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    } else if(fieldSizeFlags->asym5X1Complete==true&&fieldSizeFlags->asym5X2Complete==true&&fieldSizeFlags->asym5Y1Complete==true&&fieldSizeFlags->asym5Y2Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y2 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_asym5->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* X1 over-travel button. First on_pushButton_overX1 emits signal = 9 to CVQA.
 * Then CVQA wirtes the x1 field value to on_dialogSignalValueoverX1 and if the value is within 2mm of 2cm then the boolean x1OverComplete is set to true, otherwise the boolean is false.
 * If x1OverComplete is true then the x1 over-travel button turns green, otherwise it turns red.
 */
void FieldSize::on_pushButton_overX1_clicked()
{
    emit buttonPressed(9);
}

void FieldSize::on_dialogSignalValueoverX1(double d)
{
    ui->lineEdit_overX1->setText(QString::number(d,'f',2));
    if(-1.8>d&& d>-2.2)
    {
        fieldSizeFlags->x1OverComplete = true;
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X1 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        fieldSizeFlags->x1OverComplete = false;
    }

    if(fieldSizeFlags->x1OverComplete)
    {
        ui->pushButton_overX1->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        ui->pushButton_overX1->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* X2 over-travel button. First on_pushButton_overX2 emits signal = 10 to CVQA.
 * Then CVQA wirtes the x2 field value to on_dialogSignalValueoverX2 and if the value is within 2mm of 2cm then the boolean x2OverComplete is set to true, otherwise the boolean is false.
 * If x2OverComplete is true then the x2 over-travel button turns green, otherwise it turns red.
 */
void FieldSize::on_pushButton_overX2_clicked()
{
    emit buttonPressed(10);
}

void FieldSize::on_dialogSignalValueoverX2(double d)
{
    ui->lineEdit_overX2->setText(QString::number(d,'f',2));
    if(-1.8>d&& d>-2.2)
    {
        fieldSizeFlags->x2OverComplete = true;
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in X2 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        fieldSizeFlags->x2OverComplete = false;
    }

    if(fieldSizeFlags->x2OverComplete)
    {
        ui->pushButton_overX2->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        ui->pushButton_overX2->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* Y1 over-travel button. First on_pushButton_overY1 emits signal = 11 to CVQA.
 * Then CVQA wirtes the y1 field value to on_dialogSignalValueoverY1 and if the value is within 2mm of 10cm then the boolean y1OverComplete is set to true, otherwise the boolean is false.
 * If y1OverComplete is true then the y1 over-travel button turns green, otherwise it turns red.
 */
void FieldSize::on_pushButton_overY1_clicked()
{
    emit buttonPressed(11);
}

void FieldSize::on_dialogSignalValueoverY1(double d)
{
    ui->lineEdit_overY1->setText(QString::number(d,'f',2));
    if(-9.8>d&& d>-10.2)
    {
        fieldSizeFlags->y1OverComplete = true;
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y1 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        fieldSizeFlags->y1OverComplete = false;
    }

    if(fieldSizeFlags->y1OverComplete)
    {
        ui->pushButton_overY1->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        ui->pushButton_overY1->setStyleSheet("QPushButton { background-color: red; }");
    }
}

/* Y2 over-travel button. First on_pushButton_overY2 emits signal = 12 to CVQA.
 * Then CVQA wirtes the y2 field value to on_dialogSignalValueoverY2 and if the value is within 2mm of 10cm then the boolean y2OverComplete is set to true, otherwise the boolean is false.
 * If y2OverComplete is true then the y2 over-travel button turns green, otherwise it turns red.
 */

void FieldSize::on_pushButton_overY2_clicked()
{
    emit buttonPressed(12);
}

void FieldSize::on_dialogSignalValueoverY2(double d)
{
    ui->lineEdit_overY2->setText(QString::number(d,'f',2));
    if(-9.8>d&& d>-10.2)
    {
        fieldSizeFlags->y2OverComplete = true;
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in Y2 field size.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        fieldSizeFlags->y2OverComplete = false;
    }

    if(fieldSizeFlags->y2OverComplete)
    {
        ui->pushButton_overY2->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        ui->pushButton_overY2->setStyleSheet("QPushButton { background-color: red; }");
    }
}
