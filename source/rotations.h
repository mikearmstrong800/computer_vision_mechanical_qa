#ifndef ROTATIONS_H
#define ROTATIONS_H

#include <QMessageBox>
#include <QGraphicsScene>
#include <opencv2/opencv.hpp>


namespace Ui {
class Rotations;
}

class Rotations : public QDialog
{
    Q_OBJECT

public:
    explicit Rotations(QWidget *parent = nullptr);
    ~Rotations();

private slots:
    void on_pushButton_coll90_clicked();

    void on_pushButton_collWalk_clicked();

    void on_pushButton_coll270_clicked();

    void on_pushButton_table90_clicked();

    void on_pushButton_tableWalk_clicked();

    void on_pushButton_table270_clicked();

    void on_lineEdit_collOffset_editingFinished();

    void on_pushButton_collLeft_clicked();

    void on_pushButton_collRight_clicked();

public slots:
    void on_dialogSignalValuecoll90(double d);
    void on_dialogSignalValuecollWalk(double d);
    void on_dialogSignalValuecoll270(double d);
    void on_dialogSignalValuetable90(double d);
    void on_dialogSignalValuetableWalk(double d);
    void on_dialogSignalValuetable270(double d);
    void on_dialogSignalPicture(cv::Mat mat);
    void on_dialogSignalPolar(std::vector <double> angles,std::vector <double> vector);

signals:
    void buttonPressed(int i);

public:
    Ui::Rotations *ui;
    QGraphicsScene scene;
    double collOffset=0;
    double collDir=0;
};

#endif // ROTATIONS_H
