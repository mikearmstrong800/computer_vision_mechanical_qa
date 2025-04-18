#include "cvqa.h"
#include "ui_cvqa.h"

#include "fielddetect.h"
#include <QMessageBox>
#include <QString>
#include <QTextStream>
#include <QTemporaryFile>
#include <QFile>
#include <QFileDialog>
#include <QDate>
#include <QTime>



/* CVQA utilizes functions from FieldDetect to calculate mechanical QA tests.
 * The tests that are calculated are: ODI, table travel (vrt, lat, lng), field size, collimator and table angle, and collimator and table walkout.
 * All calculations are performed here and relayed to child windows for display.
 * For more information about the functions used see the documentation on the FieldDetect class.
 */

CVQA::CVQA(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CVQA)
{
    ui->setupUi(this);
    ui->pushButton_refimage->setDisabled(true);
    ui->pushButton_TableODI->setDisabled(true);
    ui->pushButton_fieldSize->setDisabled(true);
    ui->pushButton_rotation->setDisabled(true);
    setAttribute(Qt::WA_DeleteOnClose);

    QObject::connect(rotations,&Rotations::buttonPressed, this, &CVQA::on_rotations_button_pressed);
    QObject::connect(tableODI,&TableODI::buttonPressed, this, &CVQA::on_tableodi_button_pressed);
    QObject::connect(fieldSize,&FieldSize::buttonPressed, this, &CVQA::on_fieldsize_button_pressed);
}

CVQA::~CVQA()
{
    delete ui;
    QObject::destroyed();

}

FieldDetect *fieldDetect = new(FieldDetect);



/* The initialization button first loads a calibration file. If this is missing then a warning is displayed.
 * If the calibration file is found then initializeCamera from FieldDetect is run.
 * If the error code is 0 then the reference button is enabled and the initialization button is turned green.
 * If the error code is <0  then the reference button is not enabled, the initialization button is turned red, and a warning message is displayed.
 * See FieldDetect documentation for specifics on the function and error code meanings.
 */

void CVQA::on_pushButton_initialize_clicked()
{
   ui->pushButton_initialize->setEnabled(false);
   QApplication::processEvents();
   int err=0;
   double l;
   err=fieldDetect->loadConfigFromFile("..//..//config//cvqa_config.xml");
   if(err<0)
   {
       QMessageBox msgBox;
       msgBox.setInformativeText("Calibration file cvqa_config.xml not found. Cannot proceed.");
       msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
       msgBox.exec();
       ui->pushButton_initialize->setStyleSheet("QPushButton { background-color: red; }");
   } else
   {
       err=0;
       err=fieldDetect->initializeCamera();
       if(err==-1)
       {
           QMessageBox msgBox;
           msgBox.setInformativeText("Camera initialization failed. Check if camera is connected properly.");
           msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
           int ret=msgBox.exec();
           ui->pushButton_initialize->setStyleSheet("QPushButton { background-color: red; }");

       } else if(err==-2)
       {
           QMessageBox msgBox;
           msgBox.setInformativeText("Camera calibration file could not be loaded.");
           msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
           int ret=msgBox.exec();
           ui->pushButton_initialize->setStyleSheet("QPushButton { background-color: red; }");
       } else
       {
           l=fieldDetect->warmUpCamera();
           ui->labelLightLevel->setText(QString::number(l, 'f', 1));
           //if(l<85) original - change - Preprocessing before Otsu should be robust to intensity variance
           if(l<fieldDetect->lightLevelLow)
           {
               QMessageBox msgBox;
               msgBox.setInformativeText("Check light conditions and re-perform camera initialization. Room lights are too dark or lens cap still on.");
               msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
               int ret=msgBox.exec();
               ui->pushButton_initialize->setStyleSheet("QPushButton { background-color: red; }");
           } else if(l>fieldDetect->lightLevelHigh)
           //else if(l>120) original - change - Preprocessing before Otsu should be robust to intensity variance
           {
               QMessageBox msgBox;
               msgBox.setInformativeText("Check light conditions and re-perform camera initialization. Room lights are too bright.");
               msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
               int ret=msgBox.exec();
               ui->pushButton_initialize->setStyleSheet("QPushButton { background-color: red; }");
           } else
           {
               ui->pushButton_initialize->setStyleSheet("QPushButton { background-color: green; }");
               ui->pushButton_refimage->setEnabled(true);

           }

       }
   }

    ui->pushButton_initialize->setEnabled(true);

}

/* The reference image button runs acquireReferenceImage from FieldDetect.
 * If the error code is 0 then the button is turned green and all buttons to determine mechanical QA values are enabled.
 * If the error code is <0 then the button is turned red, all buttons to determine mechanical QA values are left disabled, and a warning message is displayed.
 * See FieldDetect documentation for specifics on the function and error code meanings.
 */

void CVQA::on_pushButton_refimage_clicked()
{
    ui->pushButton_refimage->setStyleSheet("QPushButton { background-color: gray; }");
    Sleep(1000);
    QApplication::processEvents();
    int err=0;
    err=fieldDetect->acquireReferenceImage();
    if(err==-1)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Camera image was empty.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        int ret=msgBox.exec();
        ui->pushButton_refimage->setStyleSheet("QPushButton { background-color: red; }");

    } else if(err==-2)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Failed at aruco perspective.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        int ret=msgBox.exec();
        ui->pushButton_refimage->setStyleSheet("QPushButton { background-color: red; }");
    } else if(err==-3)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Failed at find marker points.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        int ret=msgBox.exec();
        ui->pushButton_refimage->setStyleSheet("QPushButton { background-color: red; }");
    } else if(err==-4)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Failed at find light field dimensions.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        int ret=msgBox.exec();
        ui->pushButton_refimage->setStyleSheet("QPushButton { background-color: red; }");
    } else if(err==-5)
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Camera was not initialized.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        int ret=msgBox.exec();
        ui->pushButton_refimage->setStyleSheet("QPushButton { background-color: red; }");
    } else
    {
        ui->pushButton_refimage->setStyleSheet("QPushButton { background-color: green; }");
        ui->pushButton_TableODI->setEnabled(true);
        ui->pushButton_fieldSize->setEnabled(true);
        ui->pushButton_rotation->setEnabled(true);
    }

}

/* The Table Travel/ODI button launches a child window and class called TableODI.
 * Each of the buttons in the child window sends a signal back to the parent window with a different integer.
 * The integer specifies which test is run and where to send the data back to in the child window.
 * Signal = 1 table travel and ODI vrt 90
 * Signal = 2 table travel and ODI vrt 110
 * Signal = 3 table travel lng +10
 * Signal = 4 table travel lng -10
 * Signal = 5 table travel lat -10
 * Signal = 6 table travel lat +10
 * Signal = 7 ODI vrt 100
 * findArucoDisplacement from FieldDetect is used to determine the vrt, lat, and lng change from the reference position.
 * findODIDistance from FieldDetect is used to determine the ODI.
 * See FieldDetect documentation for specifics on the functions and error code meanings.
 */

void CVQA::on_pushButton_TableODI_clicked()
{

    //QObject::connect(tableODI,&TableODI::buttonPressed, this, &CVQA::on_tableodi_button_pressed);
    int ret = tableODI->exec();

}


/*!
 * \brief CVQA::on_tableodi_button_pressed
 */
void CVQA::on_tableodi_button_pressed(int i)
{
    FieldDetect::t_findArucoDisplacementReturnStruct FADRS;
    FieldDetect::t_findODIDistanceReturnStruct FODRS;
    double zref=fieldDetect->referenceStruct.referencePosition.z;
    if(i==1)
    {

        FADRS = fieldDetect->findArucoDisplacement();
        if(FADRS.error==0){
            tableODI->on_dialogSignalValuevrt90((FADRS.disp.z/10)+100);
            tableODI->on_dialogSignalPicture(FADRS.imageDisplay);
            tableODI->on_dialogSignalValuedelta90(FADRS.disp.z/10);
            ORS->tablevrtneg = QString::number(FADRS.disp.z/10,'f',2);
            if(91>(FADRS.disp.z)&&(FADRS.disp.z)>89)
            {
                completedFlags->vrt90Complete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }

        FODRS=fieldDetect->findODI();
        if(FODRS.error==0)
        {
             tableODI->on_dialogSignalValueODI90(FODRS.distance);
             tableODI->on_dialogSignalPicture(FODRS.imageDisplay);
             ORS->ODI90 = QString::number(FODRS.distance,'f',2);
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
            tableODI->on_dialogSignalPicture(FODRS.imageDisplay);
        }


    } else if(i==2)
    {
        FADRS=fieldDetect->findArucoDisplacement();
        if(FADRS.error==0){
            tableODI->on_dialogSignalValuevrt110((FADRS.disp.z/10)+100);
            tableODI->on_dialogSignalValuedelta110(FADRS.disp.z/10);
            ORS->tablevrtpos = QString::number(FADRS.disp.z/10,'f',2);
            if(111>(FADRS.disp.z)&&(FADRS.disp.z)>109)
            {
                completedFlags->vrt110Complete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
        FODRS=fieldDetect->findODI();
        if(FODRS.error==0)
        {
             tableODI->on_dialogSignalValueODI110(FODRS.distance);
             tableODI->on_dialogSignalPicture(FODRS.imageDisplay);
             ORS->ODI110 = QString::number(FODRS.distance,'f',2);
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
            tableODI->on_dialogSignalPicture(FODRS.imageDisplay);
        }
    } else if(i==3)
    {
        FADRS=fieldDetect->findArucoDisplacement();
        if(FADRS.error==0)
        {
            tableODI->on_dialogSignalValuelngpos((FADRS.disp.y)/10);
            tableODI->on_dialogSignalPicture(FADRS.imageDisplay);
            ORS->tablelngpos = QString::number((FADRS.disp.y)/10,'f',2);
            if(10.1>(FADRS.disp.y)&&(FADRS.disp.y)>9.9)
            {
                completedFlags->lngPosComplete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==4)
    {
        FADRS=fieldDetect->findArucoDisplacement();
        if (FADRS.error==0)
        {
            tableODI->on_dialogSignalValuelngneg((FADRS.disp.y)/10);
            tableODI->on_dialogSignalPicture(FADRS.imageDisplay);
            ORS->tablelngneg = QString::number((FADRS.disp.y)/10,'f',2);
            if(-9.9>(FADRS.disp.y)&&(FADRS.disp.y)>-10.1)
            {
                completedFlags->lngNegComplete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==5)
    {
        FADRS=fieldDetect->findArucoDisplacement();
        if(FADRS.error==0)
        {
            tableODI->on_dialogSignalValuelatpos((FADRS.disp.x)/10);
            tableODI->on_dialogSignalPicture(FADRS.imageDisplay);
            ORS->tablelatpos = QString::number((FADRS.disp.x)/10,'f',2);
            if(10.1>(FADRS.disp.x)&&(FADRS.disp.x)>9.9)
            {
                completedFlags->latPosComplete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==6)
    {
        FADRS=fieldDetect->findArucoDisplacement();
        if (FADRS.error==0)
        {
            tableODI->on_dialogSignalValuelatneg((FADRS.disp.x)/10);
            tableODI->on_dialogSignalPicture(FADRS.imageDisplay);
            ORS->tablelatneg = QString::number((FADRS.disp.x)/10,'f',2);
            if(-9.9>(FADRS.disp.y)&&(FADRS.disp.y)>-10.1)
            {
                completedFlags->latNegComplete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==7)
    {
       FODRS=fieldDetect->findODI();
       if(FODRS.error==0)
       {
            tableODI->on_dialogSignalValueODI100(FODRS.distance);
            tableODI->on_dialogSignalPicture(FODRS.imageDisplay);
            ORS->ODI100 = QString::number(FODRS.distance,'f',2);
       } else
       {
           QMessageBox msgBox;
           msgBox.setInformativeText("Error in acquiring image.");
           msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
           msgBox.exec();
           tableODI->on_dialogSignalPicture(FODRS.imageDisplay);
       }
    }
}


/* The Field Size button launches a child window and class called FieldSize.
 * Each of the buttons in the child window sends a signal back to the parent window with a different integer.
 * The integer specifies which test is run and where to send the data back to in the child window.
 * Signal = 1 40x40 symmetric
 * Signal = 2 20x20 symmetric
 * Signal = 3 10x10 symmetric
 * Signal = 4 5x5 symmetric
 * Signal = 5 40x40 asymmetric
 * Signal = 6 20x20 asymmetric
 * Signal = 7 10x10 asymmetric
 * Signal = 8 5x5 asymmetric
 * Signal = 9 X1 over-travel
 * Signal = 10 X2 over-travel
 * Signal = 11 Y1 over-travel
 * Signal = 12 Y2 over-travel
 * findLightFieldDimension from FieldDetect is used to determine the field size. The following math is used to compute each of the types of field sizes.
 * Symmetric: X fields: the 2 height values are added together and divided by 80 (2x10x4, 2 for the 2 values, 10 for conversion from mm to cm, 4 for the 4 pixels per mm)
 *            Y fields: the 2 width values are added together and divided by 80
 * Asymmetric: X fields: the distance from one edge point relative to the reference x center position. The two values of this along that edge are averaged.
 *             Y fields: the distance from one edge point relative to the reference y center position. The two values of this along that edge are averaged.
 * Over-travel: X fields: the distance from one edge point of the specific jaw that over-travelled relative to the reference x center position. The two values of this along that edge are averaged.
 *              Y fields: the distance from one edge point of the specific jaw that over-travelled relative to the reference y center position. The two values of this along that edge are averaged.
 * See FieldDetect documentation for specifics on the function and error code meanings.
 */

void CVQA::on_pushButton_fieldSize_clicked()
{
    //QObject::connect(fieldSize,&FieldSize::buttonPressed, this, &CVQA::on_fieldsize_button_pressed);
    int ret=fieldSize->exec();

}

void CVQA::on_fieldsize_button_pressed(int i)
{
    FieldDetect::t_FindLightFieldReturnStruct FLFRS;
    double xRef=fieldDetect->referenceStruct.crosshairCenterReference.x;
    double yRef=fieldDetect->referenceStruct.crosshairCenterReference.y;
    if(i==1)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            fieldSize->on_dialogSignalValuesym40X((FLFRS.h1Len + FLFRS.h2Len)/80);
            fieldSize->on_dialogSignalValuesym40Y((FLFRS.v1Len + FLFRS.v2Len)/80);
            ORS->sym40X = QString::number((FLFRS.h1Len + FLFRS.h2Len)/80,'f',2);
            ORS->sym40Y = QString::number((FLFRS.v1Len + FLFRS.v2Len)/80,'f',2);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            if(40.2>((FLFRS.h1Len + FLFRS.h2Len)/80)&&((FLFRS.h1Len + FLFRS.h2Len)/80)>39.8&&40.2>((FLFRS.v1Len + FLFRS.v2Len)/80)&&((FLFRS.v1Len + FLFRS.v2Len)/80)>39.8)
            {
                completedFlags->sym40Complete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==2)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            fieldSize->on_dialogSignalValuesym20X((FLFRS.h1Len + FLFRS.h2Len)/80);
            fieldSize->on_dialogSignalValuesym20Y((FLFRS.v1Len + FLFRS.v2Len)/80);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            ORS->sym20X = QString::number((FLFRS.h1Len + FLFRS.h2Len)/80,'f',2);
            ORS->sym20Y = QString::number((FLFRS.v1Len + FLFRS.v2Len)/80,'f',2);
            if(20.2>((FLFRS.h1Len + FLFRS.h2Len)/80)&&((FLFRS.h1Len + FLFRS.h2Len)/80)>19.8&&20.2>((FLFRS.v1Len + FLFRS.v2Len)/80)&&((FLFRS.v1Len + FLFRS.v2Len)/80)>19.8)
            {
                completedFlags->sym20Complete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==3)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            fieldSize->on_dialogSignalValuesym10X((FLFRS.h1Len + FLFRS.h2Len)/80);
            fieldSize->on_dialogSignalValuesym10Y((FLFRS.v1Len + FLFRS.v2Len)/80);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            ORS->sym10X = QString::number((FLFRS.h1Len + FLFRS.h2Len)/80,'f',2);
            ORS->sym10Y = QString::number((FLFRS.v1Len + FLFRS.v2Len)/80,'f',2);
            if(10.2>((FLFRS.h1Len + FLFRS.h2Len)/80)&&((FLFRS.h1Len + FLFRS.h2Len)/80)>9.8&&10.2>((FLFRS.v1Len + FLFRS.v2Len)/80)&&((FLFRS.v1Len + FLFRS.v2Len)/80)>9.8)
            {
                completedFlags->sym10Complete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==4)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            fieldSize->on_dialogSignalValuesym5X((FLFRS.h1Len + FLFRS.h2Len)/80);
            fieldSize->on_dialogSignalValuesym5Y((FLFRS.v1Len + FLFRS.v2Len)/80);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            ORS->sym5X = QString::number((FLFRS.h1Len + FLFRS.h2Len)/80,'f',2);
            ORS->sym5Y = QString::number((FLFRS.v1Len + FLFRS.v2Len)/80,'f',2);
            if(5.2>((FLFRS.h1Len + FLFRS.h2Len)/80)&&((FLFRS.h1Len + FLFRS.h2Len)/80)>4.8&&5.2>((FLFRS.v1Len + FLFRS.v2Len)/80)&&((FLFRS.v1Len + FLFRS.v2Len)/80)>4.8)
            {
                completedFlags->sym5Complete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==5)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            double x1=(((FLFRS.edgePoints[2].x - xRef)/4)+((FLFRS.edgePoints[3].x - xRef)/4))/2;
            double x2=(((FLFRS.edgePoints[6].x - xRef)/4)+((FLFRS.edgePoints[7].x - xRef)/4))/2;
            double y1=(((FLFRS.edgePoints[4].y - yRef)/4)+((FLFRS.edgePoints[5].y - yRef)/4))/2;
            double y2=(((FLFRS.edgePoints[0].y - yRef)/4)+((FLFRS.edgePoints[1].y - yRef)/4))/2;
            fieldSize->on_dialogSignalValueasym40X1(-1*x1/10);
            fieldSize->on_dialogSignalValueasym40X2(x2/10);
            fieldSize->on_dialogSignalValueasym40Y1(y1/10);
            fieldSize->on_dialogSignalValueasym40Y2(-1*y2/10);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            ORS->asym40X1 = QString::number(-1*x1/10,'f',2);
            ORS->asym40X2 = QString::number(x2/10,'f',2);
            ORS->asym40Y1 = QString::number(y1/10,'f',2);
            ORS->asym40Y2 = QString::number(-1*y2/10,'f',2);
            if(40.2>((FLFRS.h1Len + FLFRS.h2Len)/80)&&((FLFRS.h1Len + FLFRS.h2Len)/80)>39.8&&40.2>((FLFRS.v1Len + FLFRS.v2Len)/80)&&((FLFRS.v1Len + FLFRS.v2Len)/80)>39.8)
            {
                completedFlags->asym40Complete = true;

            }

        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==6)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            double x1=(((FLFRS.edgePoints[2].x - xRef)/4)+((FLFRS.edgePoints[3].x - xRef)/4))/2;
            double x2=(((FLFRS.edgePoints[6].x - xRef)/4)+((FLFRS.edgePoints[7].x - xRef)/4))/2;
            double y1=(((FLFRS.edgePoints[4].y - yRef)/4)+((FLFRS.edgePoints[5].y - yRef)/4))/2;
            double y2=(((FLFRS.edgePoints[0].y - yRef)/4)+((FLFRS.edgePoints[1].y - yRef)/4))/2;
            fieldSize->on_dialogSignalValueasym20X1(-1*x1/10);
            fieldSize->on_dialogSignalValueasym20X2(x2/10);
            fieldSize->on_dialogSignalValueasym20Y1(y1/10);
            fieldSize->on_dialogSignalValueasym20Y2(-1*y2/10);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            ORS->asym20X1 = QString::number(-1*x1/10,'f',2);
            ORS->asym20X2 = QString::number(x2/10,'f',2);
            ORS->asym20Y1 = QString::number(y1/10,'f',2);
            ORS->asym20Y2 = QString::number(-1*y2/10,'f',2);
            if(20.2>((FLFRS.h1Len + FLFRS.h2Len)/80)&&((FLFRS.h1Len + FLFRS.h2Len)/80)>19.8&&20.2>((FLFRS.v1Len + FLFRS.v2Len)/80)&&((FLFRS.v1Len + FLFRS.v2Len)/80)>19.8)
            {
                completedFlags->asym20Complete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==7)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            double x1=(((FLFRS.edgePoints[2].x - xRef)/4)+((FLFRS.edgePoints[3].x - xRef)/4))/2;
            double x2=(((FLFRS.edgePoints[6].x - xRef)/4)+((FLFRS.edgePoints[7].x - xRef)/4))/2;
            double y1=(((FLFRS.edgePoints[4].y - yRef)/4)+((FLFRS.edgePoints[5].y - yRef)/4))/2;
            double y2=(((FLFRS.edgePoints[0].y - yRef)/4)+((FLFRS.edgePoints[1].y - yRef)/4))/2;
            fieldSize->on_dialogSignalValueasym10X1(-1*x1/10);
            fieldSize->on_dialogSignalValueasym10X2(x2/10);
            fieldSize->on_dialogSignalValueasym10Y1(y1/10);
            fieldSize->on_dialogSignalValueasym10Y2(-1*y2/10);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            ORS->asym10X1 = QString::number(-1*x1/10,'f',2);
            ORS->asym10X2 = QString::number(x2/10,'f',2);
            ORS->asym10Y1 = QString::number(y1/10,'f',2);
            ORS->asym10Y2 = QString::number(-1*y2/10,'f',2);
            if(10.2>((FLFRS.h1Len + FLFRS.h2Len)/80)&&((FLFRS.h1Len + FLFRS.h2Len)/80)>9.8&&10.2>((FLFRS.v1Len + FLFRS.v2Len)/80)&&((FLFRS.v1Len + FLFRS.v2Len)/80)>9.8)
            {
                completedFlags->asym10Complete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==8)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            double x1=(((FLFRS.edgePoints[2].x - xRef)/4)+((FLFRS.edgePoints[3].x - xRef)/4))/2;
            double x2=(((FLFRS.edgePoints[6].x - xRef)/4)+((FLFRS.edgePoints[7].x - xRef)/4))/2;
            double y1=(((FLFRS.edgePoints[4].y - yRef)/4)+((FLFRS.edgePoints[5].y - yRef)/4))/2;
            double y2=(((FLFRS.edgePoints[0].y - yRef)/4)+((FLFRS.edgePoints[1].y - yRef)/4))/2;
            fieldSize->on_dialogSignalValueasym5X1(-1*x1/10);
            fieldSize->on_dialogSignalValueasym5X2(x2/10);
            fieldSize->on_dialogSignalValueasym5Y1(y1/10);
            fieldSize->on_dialogSignalValueasym5Y2(-1*y2/10);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            ORS->asym5X1 = QString::number(-1*x1/10,'f',2);
            ORS->asym5X2 = QString::number(x2/10,'f',2);
            ORS->asym5Y1 = QString::number(y1/10,'f',2);
            ORS->asym5Y2 = QString::number(-1*y2/10,'f',2);
            if(5.2>((FLFRS.h1Len + FLFRS.h2Len)/80)&&((FLFRS.h1Len + FLFRS.h2Len)/80)>4.8&&5.2>((FLFRS.v1Len + FLFRS.v2Len)/80)&&((FLFRS.v1Len + FLFRS.v2Len)/80)>4.8)
            {
                completedFlags->asym5Complete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==9)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            double x1=(((FLFRS.edgePoints[2].x - xRef)/4)+((FLFRS.edgePoints[3].x - xRef)/4))/2;
            fieldSize->on_dialogSignalValueoverX1(-1*x1/10);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            ORS->overX1 = QString::number(-1*x1/10,'f',2);
            if(2.2>(x1/10)&&(x1/10)>1.8)
            {
                completedFlags->x1OverComplete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==10)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            double x2=(((FLFRS.edgePoints[6].x - xRef)/4)+((FLFRS.edgePoints[7].x - xRef)/4))/2;
            fieldSize->on_dialogSignalValueoverX2(x2/10);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            ORS->overX2 = QString::number(x2/10,'f',2);
            if(-2.2<(x2/10)&&(x2/10)<-1.8)
            {
                completedFlags->x2OverComplete = true;

            }
        } else {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==11)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            double y1=(((FLFRS.edgePoints[4].y - yRef)/4)+((FLFRS.edgePoints[5].y - yRef)/4))/2;
            fieldSize->on_dialogSignalValueoverY1(y1/10);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            ORS->overY1 = QString::number(y1/10,'f',2);
            if(-10.2<(y1/10)&&(y1/10)<-9.8)
            {
                completedFlags->y1OverComplete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==12)
    {
        FLFRS=fieldDetect->findLightFieldDimensions("");
        if(FLFRS.error==0)
        {
            double y2=(((FLFRS.edgePoints[0].y - yRef)/4)+((FLFRS.edgePoints[1].y - yRef)/4))/2;
            fieldSize->on_dialogSignalValueoverY2(-1*y2/10);
            fieldSize->on_dialogSignalPicture(FLFRS.imageCorrected);
            ORS->overY2 = QString::number(-1*y2/10,'f',2);
            if(10.2>(y2/10)&&(y2/10)>9.8)
            {
                completedFlags->y2OverComplete = true;

            }
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    }

}

/* The Collimator/Table Rotation button launches a child window and class called Rotations.
 * Each of the buttons in the child window sends a signal back to the parent window with a different integer.
 * The integer specifies which test is run and where to send the data back to in the child window.
 * Signal = 1 collimator 90
 * Signal = 2 collimator walkout
 * Signal = 3 collimator 270
 * Signal = 4 table 90
 * Signal = 5 table walkout
 * Signal = 6 table 270
 * Signal = 7 ODI vrt 100
 * findBoardAngle from FieldDetect is used to determine collimator or table angle.
 * findWalkout from FieldDetect is used to determine the collimator or table walkout.
 * See FieldDetect documentation for specifics on the functions and error code meanings.
 */

void CVQA::on_pushButton_rotation_clicked()
{
    //QObject::connect(rotations,&Rotations::buttonPressed, this, &CVQA::on_rotations_button_pressed);
    int ret=rotations->exec();
}

void CVQA::on_rotations_button_pressed(int i)
{
    //FieldDetect::t_findWalkoutRetStruct FWRS; //changed by mike for walkout problem
    FieldDetect::t_findBoardAngleReturnStructure FBARS;
    if(i==1)
    {
        FBARS=fieldDetect->findBoardAngle();
        if(FBARS.error==0)
        {
            rotations->on_dialogSignalValuecoll90(FBARS.angle);
            rotations->on_dialogSignalPicture(FBARS.imageDisplay);
            ORS->coll90 = QString::number(FBARS.angle,'f',2);
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==2)
    {
        FieldDetect::t_findWalkoutRetStruct FWRS=fieldDetect->findWalkout(false);
        if(FWRS.error==0)
        {
            std::vector <double> angles, vectorLength;

            rotations->on_dialogSignalValuecollWalk((FWRS.maxWOVectorLength));
            ORS->collWalk = QString::number(FWRS.maxWOVectorLength,'f',2);

            qDebug()<<"--------------------------------------------------------------------------------------------------maxWOVectorLength "<<FWRS.maxWOVectorLength;
            for(size_t i=0; i<FWRS.walkoutStruct.size(); i++)
            {
              qDebug()<<"vector lengths " << FWRS.walkoutStruct[i].vecLen;
              vectorLength.push_back(FWRS.walkoutStruct[i].vecLen);

              qDebug()<<"angles " << FWRS.walkoutStruct[i].angle;
              angles.push_back(FWRS.walkoutStruct[i].angle);

            }

            rotations->on_dialogSignalPolar(angles,vectorLength);




        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==3)
    {
        FBARS=fieldDetect->findBoardAngle();
        if(FBARS.error==0)
        {
            rotations->on_dialogSignalValuecoll270(FBARS.angle);
            rotations->on_dialogSignalPicture(FBARS.imageDisplay);
            ORS->coll270 = QString::number(FBARS.angle,'f',2);
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }

    } else if(i==4)
    {
        FBARS=fieldDetect->findBoardAngle();
        if(FBARS.error==0)
        {
            rotations->on_dialogSignalValuetable90(FBARS.angle);
            rotations->on_dialogSignalPicture(FBARS.imageDisplay);
            ORS->table90 = QString::number(FBARS.angle,'f',2);
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==5)
    {
        FieldDetect::t_findWalkoutRetStruct FWRS=fieldDetect->findWalkout(true);
        if(FWRS.error==0)
        {
            std::vector <double> angles, vectorLength;
            rotations->on_dialogSignalValuetableWalk((FWRS.maxWOVectorLength));
            ORS->tableWalk = QString::number(FWRS.maxWOVectorLength,'f',2);
            for(size_t i=0; i<FWRS.walkoutStruct.size(); i++)
            {
              qDebug()<<"vector lengths " << FWRS.walkoutStruct[i].vecLen;
              vectorLength.push_back(FWRS.walkoutStruct[i].vecLen);

              qDebug()<<"angles " << FWRS.walkoutStruct[i].angle;
              angles.push_back(FWRS.walkoutStruct[i].angle);
            }

            rotations->on_dialogSignalPolar(angles,vectorLength);
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    } else if(i==6)
    {
        FBARS=fieldDetect->findBoardAngle();
        if(FBARS.error==0)
        {
            rotations->on_dialogSignalValuetable270(FBARS.angle);
            rotations->on_dialogSignalPicture(FBARS.imageDisplay);
            ORS->table270 = QString::number(FBARS.angle,'f',2);
        } else
        {
            QMessageBox msgBox;
            msgBox.setInformativeText("Error in acquiring image.");
            msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
            msgBox.exec();
        }
    }
}

/* The Print Results button writes a text document with values from each of the tests.
 * If any tests were not performed the values are left blank in the text document.
 * A pop-up window has the user select the location for the text document and the name of it.
 */

void CVQA::on_pushButton_writeOutput_clicked()
{
    QDate cdate = QDate::currentDate();
    QTime ctime = QTime::currentTime();
    QString timeString = ctime.toString();
    QString reportFileNameTime = timeString.replace(":","-");
    QString reportFileName = "MechanicalQA_" + reportFileNameTime;
    QString fileNameEntered = QFileDialog::getSaveFileName(this,tr("Save File"), reportFileName,tr("Text files (*txt"));
    QString fileName = fileNameEntered + ".txt";
    QFile data(fileName);
    if (data.open(QFile::WriteOnly | QFile::Truncate)){
        QTextStream out(&data);
        out << "Date: "<<cdate.toString()<<'\n';
        out << "Time: "<<ctime.toString()<<'\n'<<'\n';
        out << "ODI Values"<<'\n';
        out << "ODI 90: "<<ORS->ODI90<<'\n';
        out << "ODI 100: "<<ORS->ODI100<<'\n';
        out << "ODI 110: "<<ORS->ODI110<<'\n'<<'\n';
        out << "Table Travel"<<'\n';
        out << "Table Vrt Neg: "<<ORS->tablevrtneg<<'\n';
        out << "Table Vrt Pos: "<<ORS->tablevrtpos<<'\n';
        out << "Table Lat Neg: "<<ORS->tablelatneg<<'\n';
        out << "Table Lat Pos: "<<ORS->tablelatpos<<'\n';
        out << "Table Lng Neg: "<<ORS->tablelngneg<<'\n';
        out << "Table Lng Pos: "<<ORS->tablelngpos<<'\n'<<'\n';
        out << "Field Sizes"<<'\n';
        out << "40 Symmetric: X: "<<ORS->sym40X<<"  Y: "<<ORS->sym40Y<<'\n';
        out << "20 Symmetric: X: "<<ORS->sym20X<<"  Y: "<<ORS->sym20Y<<'\n';
        out << "10 Symmetric: X: "<<ORS->sym10X<<"  Y: "<<ORS->sym10Y<<'\n';
        out << "5 Symmetric: X: "<<ORS->sym5X<<"  Y: "<<ORS->sym5Y<<'\n';
        out << "40 Asymmetric: X1: "<<ORS->asym40X1<<"  X2: "<<ORS->asym40X2<<"  Y1: "<<ORS->asym40Y1<<"  Y2: "<<ORS->asym40Y2<<'\n';
        out << "20 Asymmetric: X1: "<<ORS->asym20X1<<"  X2: "<<ORS->asym20X2<<"  Y1: "<<ORS->asym20Y1<<"  Y2: "<<ORS->asym20Y2<<'\n';
        out << "10 Asymmetric: X1: "<<ORS->asym10X1<<"  X2: "<<ORS->asym10X2<<"  Y1: "<<ORS->asym10Y1<<"  Y2: "<<ORS->asym10Y2<<'\n';
        out << "5 Asymmetric: X1: "<<ORS->asym5X1<<"  X2: "<<ORS->asym5X2<<"  Y1: "<<ORS->asym5Y1<<"  Y2: "<<ORS->asym5Y2<<'\n';
        out << "X1 Over-travel: "<<ORS->overX1<<'\n';
        out << "X2 Over-travel: "<<ORS->overX2<<'\n';
        out << "Y1 Over-travel: "<<ORS->overY1<<'\n';
        out << "Y2 Over-travel: "<<ORS->overY2<<'\n'<<'\n';
        out << "Collimator Rotations"<<'\n';
        out << "90deg: "<<ORS->coll90<<'\n';
        out << "270deg: "<<ORS->coll270<<'\n';
        out << "Walkout: "<<ORS->collWalk<<'\n'<<'\n';
        out << "Table Rotations"<<'\n';
        out << "90deg: "<<ORS->table90<<'\n';
        out << "270deg: "<<ORS->table270<<'\n';
        out << "Walkout: "<<ORS->tableWalk<<'\n'<<'\n';
    }

}




