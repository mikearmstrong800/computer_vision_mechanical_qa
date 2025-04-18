#include "rotations.h"
#include "ui_rotations.h"

#include <QDialog>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <opencv2/imgproc/imgproc.hpp>
#include <QImage>
#include <QtCharts/QChartView>
#include <QtCharts/QPolarChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QScatterSeries>


/* The Rotations class is launched when the Collimator/Table Rotation button is clicked in CVQA.
 * When a button is clicked in this child window is sends a signal to the parent window (CVQA) which tells it which test should be run.
 * The parent window writes back a value directly to a function in this child class which writes it to a lineEdit box.
 * The parent window sends back a picture which is displayed using the on_dialogSignalPicture function.
 */

Rotations::Rotations(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Rotations)
{
    ui->setupUi(this);
    ui->label_acquireDataColl->setVisible(false);
    ui->label_acquireDataTable->setVisible(false);
    ui->graphicsView->setScene(&scene);
}

Rotations::~Rotations()
{
    delete ui;
}

void Rotations::on_dialogSignalPicture(cv::Mat mat)
{
    scene.clear();
    QPixmap pixmap=QPixmap::fromImage(QImage(mat.data,mat.cols,mat.rows,mat.step,QImage::Format::Format_Grayscale8));
    QPixmap pixmapscaled=pixmap.scaled(890,890,Qt::IgnoreAspectRatio,Qt::FastTransformation);
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmapscaled);
    scene.addItem(item);

}

void Rotations::on_dialogSignalPolar(std::vector <double> angles,std::vector <double> vector)
{

    scene.clear();
    std::vector<std::pair<double, double>> seriesPairVector;

    for(size_t p=0; p<angles.size(); p++)
      seriesPairVector.push_back(std::make_pair(angles[p], vector[p]));

    //std::sort(seriesPairVector.begin(), seriesPairVector.end());

    for(size_t i=0; i<seriesPairVector.size(); i++)
      qDebug()<<seriesPairVector[i].first<<","<<seriesPairVector[i].second;


    QScatterSeries *series = new QScatterSeries();
    series->setName("Walkout");
    for (size_t i = 0; i < seriesPairVector.size(); i++)
        series->append(seriesPairVector[i].first, seriesPairVector[i].second);

    QPolarChart *chart = new QPolarChart();
    chart->addSeries(series);

    QValueAxis *angularAxis = new QValueAxis();
    angularAxis->setTickCount(9); // First and last ticks are co-located on 0/360 angle.
    angularAxis->setLabelFormat("%.1f");
    angularAxis->setShadesVisible(true);
    angularAxis->setShadesBrush(QBrush(QColor(249, 249, 255)));
    chart->addAxis(angularAxis, QPolarChart::PolarOrientationAngular);

    QValueAxis *radialAxis = new QValueAxis();
    radialAxis->setTickCount(9);
    radialAxis->setLabelFormat("%.2f");
    chart->addAxis(radialAxis, QPolarChart::PolarOrientationRadial);

    series->attachAxis(radialAxis);
    series->attachAxis(angularAxis);

    radialAxis->setRange(0, 1);
    angularAxis->setRange(0, 360);

    QChartView *chartView = new QChartView();
    chartView->setChart(chart);

    chart->resize(900,900);

    scene.addItem(chart);


}


/* Collimator 90 button. First on_pushButton_coll90 emits signal = 1 to CVQA.
 * Then CVQA wirtes the collimator angle value to on_dialogSignalValuecoll90.
 * If the value is within 1 degree of 90 degrees then the button turns green, otherwise it turns red.
 */
void Rotations::on_pushButton_coll90_clicked()
{
    emit buttonPressed(1);
}

void Rotations::on_dialogSignalValuecoll90(double d)
{
    ui->lineEdit_coll90->setText(QString::number((d+(collOffset*collDir)),'f',2));
    if((d+(collOffset*collDir))<91&&(d+(collOffset*collDir))>89)
    {
        ui->pushButton_coll90->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in collimator angle.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_coll90->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* Collimator Walkout button. First on_pushButton_collWalk emits signal = 2 to CVQA.
 * Then CVQA wirtes the collimator angle value to on_dialogSignalValuecollWalk.
 * If the value is less than 1 then the button turns green, otherwise it turns red.
 */
void Rotations::on_pushButton_collWalk_clicked()
{
    ui->pushButton_collWalk->setDisabled(true);
    ui->label_acquireDataColl->setVisible(true);
    ui->label_acquireDataColl->setStyleSheet("QLabel { background-color: green; }");
    emit buttonPressed(2);

}

void Rotations::on_dialogSignalValuecollWalk(double d)
{
    ui->lineEdit_collWalk->setText(QString::number(d,'f',2));
    if(d<1)
    {
        ui->pushButton_collWalk->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in collimator walkout.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_collWalk->setStyleSheet("QPushButton { background-color: red; }");
    }
    ui->pushButton_collWalk->setEnabled(true);
    ui->label_acquireDataColl->setVisible(false);
}


/* Collimator 270 button. First on_pushButton_coll270 emits signal = 3 to CVQA.
 * Then CVQA wirtes the collimator angle value to on_dialogSignalValuecoll270.
 * If the value is within 1 degree of 270 degrees then the button turns green, otherwise it turns red.
 */
void Rotations::on_pushButton_coll270_clicked()
{
    emit buttonPressed(3);
}

void Rotations::on_dialogSignalValuecoll270(double d)
{
    ui->lineEdit_coll270->setText(QString::number((d-(collOffset*collDir)),'f',2));
    if((d-(collOffset*collDir))<271&&(d-(collOffset*collDir))>269)
    {
        ui->pushButton_coll270->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in collimator angle.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_coll270->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* Table 90 button. First on_pushButton_table90 emits signal = 4 to CVQA.
 * Then CVQA wirtes the table angle value to on_dialogSignalValuetable90.
 * If the value is within 1 degree of 90 degrees then the button turns green, otherwise it turns red.
 */
void Rotations::on_pushButton_table90_clicked()
{
    emit buttonPressed(4);
}

void Rotations::on_dialogSignalValuetable90(double d)
{
    ui->lineEdit_table90->setText(QString::number(d,'f',2));
    if(d<91&&d>89)
    {
        ui->pushButton_table90->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in table angle.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_table90->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* Table Walkout button. First on_pushButton_tableWalk emits signal = 5 to CVQA.
 * Then CVQA wirtes the table angle value to on_dialogSignalValuetableWalk.
 * If the value is less than 1 then the button turns green, otherwise it turns red.
 */
void Rotations::on_pushButton_tableWalk_clicked()
{
    ui->pushButton_tableWalk->setDisabled(true);
    ui->label_acquireDataTable->setVisible(true);
    ui->label_acquireDataTable->setStyleSheet("QLabel { background-color: green; }");
    emit buttonPressed(5);
}

void Rotations::on_dialogSignalValuetableWalk(double d)
{
    ui->lineEdit_tableWalk->setText(QString::number(d,'f',2));
    if(d<1)
    {
        ui->pushButton_tableWalk->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in table walkout.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_tableWalk->setStyleSheet("QPushButton { background-color: red; }");
    }
    ui->pushButton_tableWalk->setEnabled(true);
    ui->label_acquireDataTable->setVisible(false);
}


/* Table 270 button. First on_pushButton_table270 emits signal = 6 to CVQA.
 * Then CVQA wirtes the table angle value to on_dialogSignalValuetable270.
 * If the value is within 1 degree of 270 degrees then the button turns green, otherwise it turns red.
 */
void Rotations::on_pushButton_table270_clicked()
{
    emit buttonPressed(6);
}

void Rotations::on_dialogSignalValuetable270(double d)
{
    ui->lineEdit_table270->setText(QString::number(d,'f',2));
    if(d<271&&d>269)
    {
        ui->pushButton_table270->setStyleSheet("QPushButton { background-color: green; }");
    } else
    {
        QMessageBox msgBox;
        msgBox.setInformativeText("Error in table angle.");
        msgBox.setStyleSheet("QLabel{min-width:500 px;font-size: 24px;} QPushButton{ width:250px; font-size: 18px;}");
        msgBox.exec();
        ui->pushButton_table270->setStyleSheet("QPushButton { background-color: red; }");
    }
}


/* To account for the fact that the plan uses specified values for the collimator settings which assumes that
 * for the reference image collimator 0 truly is 0 degrees.
 * There is a text box to enter what the true collimator rotation is when the machine specifies 0 degrees.
 * Then there is a push button to select which direction this offset is in.
 * This allows for having all equations the same to display collimator angle with no if statement if the value is >0 or <360.
 * If the direction is to the left then the collimator offset from the textbox is multiplied by -1.
 */
void Rotations::on_lineEdit_collOffset_editingFinished()
{
    QString n = ui->lineEdit_collOffset->text();
    collOffset = n.toDouble();
}

void Rotations::on_pushButton_collLeft_clicked()
{
    ui->pushButton_collLeft->setStyleSheet("QPushButton { background-color: cyan; }");
    ui->pushButton_collRight->setStyleSheet("QPushButton { background-color: darkGray; }");
    collDir = -1;
}

void Rotations::on_pushButton_collRight_clicked()
{
    ui->pushButton_collRight->setStyleSheet("QPushButton { background-color: cyan; }");
    ui->pushButton_collLeft->setStyleSheet("QPushButton { background-color: darkGray; }");
    collDir = 1;

}
