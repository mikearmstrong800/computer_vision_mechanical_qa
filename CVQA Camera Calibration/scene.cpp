#include "scene.h"




Scene::Scene()
{



}



/*!-------------------------------------------------------------------------------------------
 *
 *
 * -------------------------------------------------------------------------------------------*/
void Scene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
      lastPoint = event->scenePos().toPoint();
      drawing = true;
    }
}



/*!-------------------------------------------------------------------------------------------
 *
 *
 * -------------------------------------------------------------------------------------------*/
void Scene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && drawing)
    {
      drawRectangle(event->scenePos().toPoint());
      drawing = false;

      cv::Rect cvRect;
      cvRect.x = lastPoint.x();
      cvRect.y = lastPoint.y();
      cvRect.width = abs(event->scenePos().x()-lastPoint.x());
      cvRect.height = abs(event->scenePos().y()-lastPoint.y());

      roiVect.push_back(cvRect);
      emit ROIWasDrawn(roiVect.size());
    }
}




/*!-------------------------------------------------------------------------------------------
 *
 *
 * -------------------------------------------------------------------------------------------*/
void Scene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && drawing)
    {
      removeItem(roi);
      drawRectangle(event->scenePos().toPoint());
    }
}



/*!-------------------------------------------------------------------------------------------
 *
 *
 * -------------------------------------------------------------------------------------------*/
void Scene::drawRectangle(const QPoint &endPoint)
{
    roi = addRect(lastPoint.x(), lastPoint.y(), endPoint.x()-lastPoint.x(), endPoint.y()-lastPoint.y());

}



/*!-------------------------------------------------------------------------------------------
 *
 *
 * -------------------------------------------------------------------------------------------*/
void Scene::clearROIRects()
{
  foreach(QGraphicsItem *item, items())
  {
    int i = item->type();
    if(i==3)                //3 is a rectangle
      removeItem(item);
  }
  roiVect.clear();
}


/*!-------------------------------------------------------------------------------------------
 *
 *
 * -------------------------------------------------------------------------------------------*/
void Scene::clearTextItems()
{
  foreach(QGraphicsItem *item, items())
  {
    int i = item->type();
    if(i==8)                //8 is a text item
      removeItem(item);
  }
}






