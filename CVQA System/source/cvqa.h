#ifndef CVQA_H
#define CVQA_H

#include <QMainWindow>
#include "tableodi.h"
#include "fieldsize.h"
#include "rotations.h"



QT_BEGIN_NAMESPACE
namespace Ui { class CVQA; }
QT_END_NAMESPACE

class CVQA : public QMainWindow
{
    Q_OBJECT

public:
    CVQA(QWidget *parent = nullptr);
    ~CVQA();
    TableODI *tableODI = new(TableODI);
    FieldSize *fieldSize = new(FieldSize);
    Rotations *rotations = new(Rotations);

    struct CompletedFlags{
        bool sym40Complete{};
        bool sym20Complete{};
        bool sym10Complete{};
        bool sym5Complete{};
        bool asym40Complete{};
        bool asym20Complete{};
        bool asym10Complete{};
        bool asym5Complete{};
        bool x1OverComplete{};
        bool x2OverComplete{};
        bool y1OverComplete{};
        bool y2OverComplete{};
        bool vrt90Complete{};
        bool vrt110Complete{};
        bool odi90Complete{};
        bool odi100Complete{};
        bool odi110Complete{};
        bool lngPosComplete{};
        bool lngNegComplete{};
        bool latPosComplete{};
        bool latNegComplete{};
    };

    CompletedFlags *completedFlags=new(CompletedFlags);

    struct OutputReturnStruct{
        QString ODI100;
        QString ODI90;
        QString ODI110;
        QString tablevrtpos;
        QString tablevrtneg;
        QString tablelatpos;
        QString tablelatneg;
        QString tablelngpos;
        QString tablelngneg;
        QString sym40X;
        QString sym40Y;
        QString sym20X;
        QString sym20Y;
        QString sym10X;
        QString sym10Y;
        QString sym5X;
        QString sym5Y;
        QString asym40X1;
        QString asym40X2;
        QString asym40Y1;
        QString asym40Y2;
        QString asym20X1;
        QString asym20X2;
        QString asym20Y1;
        QString asym20Y2;
        QString asym10X1;
        QString asym10X2;
        QString asym10Y1;
        QString asym10Y2;
        QString asym5X1;
        QString asym5X2;
        QString asym5Y1;
        QString asym5Y2;
        QString overX1;
        QString overX2;
        QString overY1;
        QString overY2;
        QString coll90;
        QString coll270;
        QString collWalk;
        QString table90;
        QString table270;
        QString tableWalk;
    };

    OutputReturnStruct *ORS=new(OutputReturnStruct);

signals:
    void dialogSignal(double d);

private slots:
    void on_pushButton_initialize_clicked();

    void on_pushButton_refimage_clicked();

    void on_pushButton_TableODI_clicked();

    void on_pushButton_rotation_clicked();

    void on_pushButton_fieldSize_clicked();

    void on_pushButton_writeOutput_clicked();


public slots:
    void on_tableodi_button_pressed(int i);
    void on_fieldsize_button_pressed(int i);
    void on_rotations_button_pressed(int i);

private:
    Ui::CVQA *ui;
    QGraphicsScene scene;
};
#endif // CVQA_H
