#ifndef SCENE_H
#define SCENE_H

#include "qgraphicsitem.h"
#include <QGraphicsScene>
#include <opencv2/opencv.hpp>
#include <QGraphicsItem>
#include <QPoint>
#include <QGraphicsSceneMouseEvent>
#include <qdebug.h>






class Scene : public QGraphicsScene
{
    Q_OBJECT

public:
    Scene();

    QGraphicsRectItem* roi = new(QGraphicsRectItem);


    bool drawing = false;
    QPoint lastPoint;
    std::vector<cv::Rect> roiVect;

    void clearROIRects();
    void clearTextItems();

protected:

private:

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void drawRectangle(const QPoint &endPoint);

signals:

    void ROIWasDrawn(uint roiIndex);

};

#endif // SCENE_H
