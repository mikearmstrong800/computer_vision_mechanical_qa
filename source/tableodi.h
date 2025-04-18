#ifndef TABLEODI_H
#define TABLEODI_H

#include <QDialog>
#include <QGraphicsScene>
#include <opencv2/opencv.hpp>


namespace Ui {
class TableODI;
}

class TableODI : public QDialog
{
    Q_OBJECT

public:
    explicit TableODI(QWidget *parent = nullptr);
    ~TableODI();

private slots:
    void on_pushButton_vrt90_clicked();

    void on_pushButton_vrt110_clicked();

    void on_pushButton_lngpos_clicked();

    void on_pushButton_lngneg_clicked();

    void on_pushButton_latpos_clicked();

    void on_pushButton_latneg_clicked();

    void on_pushButton_ODI100_clicked();


public slots:
    void on_dialogSignalValuevrt90(double d);
    void on_dialogSignalValueODI90(double d);
    void on_dialogSignalValuevrt110(double d);
    void on_dialogSignalValueODI110(double d);
    void on_dialogSignalValuelngpos(double d);
    void on_dialogSignalValuelngneg(double d);
    void on_dialogSignalValuelatpos(double d);
    void on_dialogSignalValuelatneg(double d);
    void on_dialogSignalValueODI100(double d);
    void on_dialogSignalPicture(cv::Mat mat);
    void on_dialogSignalValuedelta90(double d);
    void on_dialogSignalValuedelta110(double d);


signals:
    void buttonPressed(int i);

public:
    Ui::TableODI *ui;
    QGraphicsScene scene;
    struct TableODIFlags{
        bool vrt90Complete{};
        bool ODI90Complete{};
        bool vrt110Complete{};
        bool ODI110Complete{};
    };

    TableODIFlags *tableODIFlags=new(TableODIFlags);
};

#endif // TABLEODI_H
