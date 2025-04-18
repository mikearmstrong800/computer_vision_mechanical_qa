#ifndef FIELDSIZE_H
#define FIELDSIZE_H

#include <QDialog>
#include <opencv2/opencv.hpp>
#include "graphics_view_zoom.h"     // IWYU pragma: keep

namespace Ui {
class FieldSize;
}

class FieldSize : public QDialog
{
    Q_OBJECT

public:
    explicit FieldSize(QWidget *parent = nullptr);
    ~FieldSize();



private slots:
    void on_pushButton_sym40_clicked();

    void on_pushButton_sym20_clicked();

    void on_pushButton_sym10_clicked();

    void on_pushButton_sym5_clicked();

    void on_pushButton_asym40_clicked();

    void on_pushButton_asym20_clicked();

    void on_pushButton_asym10_clicked();

    void on_pushButton_asym5_clicked();

    void on_pushButton_overX1_clicked();

    void on_pushButton_overX2_clicked();

    void on_pushButton_overY1_clicked();

    void on_pushButton_overY2_clicked();

public slots:
    void on_dialogSignalValuesym40X(double d);
    void on_dialogSignalValuesym40Y(double d);
    void on_dialogSignalValuesym20X(double d);
    void on_dialogSignalValuesym20Y(double d);
    void on_dialogSignalValuesym10X(double d);
    void on_dialogSignalValuesym10Y(double d);
    void on_dialogSignalValuesym5X(double d);
    void on_dialogSignalValuesym5Y(double d);
    void on_dialogSignalValueasym40X1(double d);
    void on_dialogSignalValueasym40X2(double d);
    void on_dialogSignalValueasym40Y1(double d);
    void on_dialogSignalValueasym40Y2(double d);
    void on_dialogSignalValueasym20X1(double d);
    void on_dialogSignalValueasym20X2(double d);
    void on_dialogSignalValueasym20Y1(double d);
    void on_dialogSignalValueasym20Y2(double d);
    void on_dialogSignalValueasym10X1(double d);
    void on_dialogSignalValueasym10X2(double d);
    void on_dialogSignalValueasym10Y1(double d);
    void on_dialogSignalValueasym10Y2(double d);
    void on_dialogSignalValueasym5X1(double d);
    void on_dialogSignalValueasym5X2(double d);
    void on_dialogSignalValueasym5Y1(double d);
    void on_dialogSignalValueasym5Y2(double d);
    void on_dialogSignalValueoverX1(double d);
    void on_dialogSignalValueoverX2(double d);
    void on_dialogSignalValueoverY1(double d);
    void on_dialogSignalValueoverY2(double d);
    void on_dialogSignalPicture(cv::Mat mat);


signals:
    void buttonPressed(int i);

public:
    Ui::FieldSize *ui;
    QGraphicsScene scene;
    struct FieldSizeFlags{
        bool sym40XComplete{};
        bool sym40YComplete{};
        bool sym20XComplete{};
        bool sym20YComplete{};
        bool sym10XComplete{};
        bool sym10YComplete{};
        bool sym5XComplete{};
        bool sym5YComplete{};
        bool asym40X1Complete{};
        bool asym40X2Complete{};
        bool asym40Y1Complete{};
        bool asym40Y2Complete{};
        bool asym20X1Complete{};
        bool asym20X2Complete{};
        bool asym20Y1Complete{};
        bool asym20Y2Complete{};
        bool asym10X1Complete{};
        bool asym10X2Complete{};
        bool asym10Y1Complete{};
        bool asym10Y2Complete{};
        bool asym5X1Complete{};
        bool asym5X2Complete{};
        bool asym5Y1Complete{};
        bool asym5Y2Complete{};
        bool x1OverComplete{};
        bool x2OverComplete{};
        bool y1OverComplete{};
        bool y2OverComplete{};
    };

    FieldSizeFlags *fieldSizeFlags=new(FieldSizeFlags);
};

#endif // FIELDSIZE_H
