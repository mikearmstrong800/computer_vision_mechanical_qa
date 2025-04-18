#include "tableodi.h"
#include "ui_tableodi.h"

#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <opencv2/imgproc/imgproc.hpp>
#include <QImage>
#include <QMessageBox>
#include "graphics_view_zoom.h"


/* The TableODI class is launched when the Table Travel/ODI button is clicked in CVQA.
 * When a button is clicked in this child window is sends a signal to the parent window (CVQA) which tells it which test should be run.
 * The parent window writes back a value directly to a function in this child class which writes it to a lineEdit box.
 * The parent window sends back a picture which is displayed using the on_dialogSignalPicture function.
 */

TableODI::TableODI(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TableODI)
{
    ui->setupUi(this);
    ui->graphicsView->setScene(&scene);
}

TableODI::~TableODI()
{
    delete ui;
}

void TableODI::on_dialogSignalPicture(cv::Mat mat)
{
    scene.clear();
    ui->graphicsView->resetTransform();
    QPixmap pixmap=QPixmap::fromImage(QImage(mat.data,mat.cols,mat.rows,mat.step,QImage::Format::Format_RGB888));
    QPixmap pixmapscaled=pixmap.scaled(890,890,Qt::KeepAspectRatio,Qt::FastTransformation);
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmapscaled);
    scene.addItem(item);
    Graphics_view_zoom* z = new Graphics_view_zoom(ui->graphicsView);
    z->set_modifiers(Qt::NoModifier);

}



/* Vrt 90 button. First on_pushButton_vrt90 emits signal = 1 to CVQA.
 * Then CVQA writes the SSD, the table travel relative to reference, and the ODI.
 * If the delta travel is within 1mm from 10cm then the boolean vrt90Complete is set to true, otherwise the boolean is false.
 * If the ODI is within 1cm from 90cm then the boolean ODI90Complete is set to true, otherwise the boolean is false.
 * If both vrt90Complete and ODI90Complete are true then the Vrt 90 button turns green, otherwise it turns red.
 */
void TableODI::on_pushButton_vrt90_clicked()
{
    emit buttonPressed(1);
}

void TableODI::on_dialogSignalValuevrt90(double d)
{

    ui->lineEdit_vrt90->setText(QString::number(d,'f',2));
}

void TableODI::on_dialogSignalValuedelta90(double d)
{
    ui->lineEdit_deltaVrt90->setText(QString::number(d,'f',2));
    if(d<-9.8&&d>-10.2)
    {
        tableODIFlags->vrt90Complete = true;
    } else
    {
        tableODIFlags->vrt90Complete = false;
    }
}

/* Vrt 110 button. First on_pushButton_vrt110 emits signal = 2 to CVQA.
 * Then CVQA writes the SSD, the table travel relative to reference, and the ODI.
 * If the delta travel is within 1mm from 10cm then the boolean vrt110Complete is set to true, otherwise the boolean is false.
 * If the ODI is within 1cm from 110cm then the boolean ODI110Complete is set to true, otherwise the boolean is false.
 * If both vrt110Complete and ODI110Complete are true then the Vrt 110 button turns green, otherwise it turns red.
 */
void TableODI::on_pushButton_vrt110_clicked()
{
    emit buttonPressed(2);
}

void TableODI::on_dialogSignalValuevrt110(double d)
{

    ui->lineEdit_vrt110->setText(QString::number(d,'f',2));
}

void TableODI::on_dialogSignalValuedelta110(double d)
{
    ui->lineEdit_deltaVrt110->setText(QString::number(d,'f',2));
    if(d>9.8&&d<10.2)
    {
        tableODIFlags->vrt110Complete = true;
    } else
    {
        tableODIFlags->vrt110Complete = false;
    }
}

/* Lng +10 button. First on_pushButton_lngpos emits signal = 3 to CVQA.
 * Then CVQA wirtes the table travel relative to reference.
 * If the value is within 1mm of 10cm then the button turns green, otherwise it turns red.
 */
void TableODI::on_pushButton_lngpos_clicked()
{
    emit buttonPressed(3);
}

void TableODI::on_dialogSignalValuelngpos(double d)
{

    ui->lineEdit_lngpos->setText(QString::number(d,'f',2));
    if(d>9.9&&d<10.1)
    {
        ui->pushButton_lngpos->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in positive longitudinal table shift.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_lngpos->setStyleSheet("QPushButton { background-color: red; }");
    }
}

/* Lng -10 button. First on_pushButton_lngneg emits signal = 4 to CVQA.
 * Then CVQA wirtes the table travel relative to reference.
 * If the value is within 1mm of 10cm then the button turns green, otherwise it turns red.
 */
void TableODI::on_pushButton_lngneg_clicked()
{
    emit buttonPressed(4);
}

void TableODI::on_dialogSignalValuelngneg(double d)
{

    ui->lineEdit_lngneg->setText(QString::number(d,'f',2));
    if(d<-9.9&&d>-10.1)
    {
        ui->pushButton_lngneg->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in negative longitudinal table shift.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_lngneg->setStyleSheet("QPushButton { background-color: red; }");
    }
}



/* Lat +10 button. First on_pushButton_latpos emits signal = 5 to CVQA.
 * Then CVQA wirtes the table travel relative to reference.
 * If the value is within 1mm of 10cm then the button turns green, otherwise it turns red.
 */
void TableODI::on_pushButton_latpos_clicked()
{
    emit buttonPressed(5);
}

void TableODI::on_dialogSignalValuelatpos(double d)
{

    ui->lineEdit_latpos->setText(QString::number(d,'f',2));
    if(d>9.9&&d<10.1)
    {
        ui->pushButton_latpos->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in positive lateral table shift.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_latpos->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* Lat -10 button. First on_pushButton_latneg emits signal = 6 to CVQA.
 * Then CVQA wirtes the table travel relative to reference.
 * If the value is within 1mm of 10cm then the button turns green, otherwise it turns red.
 */
void TableODI::on_pushButton_latneg_clicked()
{
    emit buttonPressed(6);
}

void TableODI::on_dialogSignalValuelatneg(double d)
{

    ui->lineEdit_latneg->setText(QString::number(d,'f',2));
    if(d<-9.9&&d>-10.1)
    {
        ui->pushButton_latneg->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in negative lateral table shift.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_latneg->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* ODI 100 button. First on_pushButton_ODI100 emits signal = 7 to CVQA.
 * Then CVQA wirtes the ODI.
 * If the value is within 1cm of 100cm then the button turns green, otherwise it turns red.
 */
void TableODI::on_pushButton_ODI100_clicked()
{
    emit buttonPressed(7);
}

void TableODI::on_dialogSignalValueODI100(double d)
{

    ui->lineEdit_ODI100->setText(QString::number(d,'f',2));
    if(d<100.2&&d>99.8)
    {
        ui->pushButton_ODI100->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in ODI 100.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_ODI100->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* ODI portion for Vrt 90
 */
void TableODI::on_dialogSignalValueODI90(double d)
{

    ui->lineEdit_ODI90->setText(QString::number(d,'f',2));
    if(d<90.2&&d>89.8)
    {
        tableODIFlags->ODI90Complete = true;

    } else
    {
        tableODIFlags->ODI90Complete = false;
    }
    if(tableODIFlags->vrt90Complete&&tableODIFlags->ODI90Complete)
    {
        ui->pushButton_vrt90->setStyleSheet("QPushButton { background-color: green; }");
    } else if(tableODIFlags->vrt90Complete==false&&tableODIFlags->ODI90Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in positive vertical table travel and ODI.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_vrt90->setStyleSheet("QPushButton { background-color: red; }");
    } else if(tableODIFlags->vrt90Complete==true&&tableODIFlags->ODI90Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in ODI.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_vrt90->setStyleSheet("QPushButton { background-color: red; }");
    } else if(tableODIFlags->vrt90Complete==false&&tableODIFlags->ODI90Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in positive vertical table travel.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_vrt90->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* ODI portion for Vrt 110
 */
void TableODI::on_dialogSignalValueODI110(double d)
{

    ui->lineEdit_ODI110->setText(QString::number(d,'f',2));
    if(d<110.2&&d>109.8)
    {
        tableODIFlags->ODI110Complete = true;

    } else
    {
        tableODIFlags->ODI110Complete = false;
    }

    if(tableODIFlags->vrt110Complete&&tableODIFlags->ODI110Complete)
    {
        ui->pushButton_vrt110->setStyleSheet("QPushButton { background-color: green; }");
    } else if(tableODIFlags->vrt110Complete==false&&tableODIFlags->ODI110Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in positive vertical table travel and ODI.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_vrt110->setStyleSheet("QPushButton { background-color: red; }");
    } else if(tableODIFlags->vrt110Complete==true&&tableODIFlags->ODI110Complete==false)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in ODI.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_vrt110->setStyleSheet("QPushButton { background-color: red; }");
    } else if(tableODIFlags->vrt110Complete==false&&tableODIFlags->ODI110Complete==true)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in positive vertical table travel.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_vrt110->setStyleSheet("QPushButton { background-color: red; }");
    }
}
