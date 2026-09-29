/*! \mainpage
 *
 * \section intro_sec Introduction
 *
 * The FieldDetect class is used to find the location and dimensions of the light field and crosshairs, find three dimensional displacement between two fields, find crosshair and table walkout, and find collimator and couch angle.
 *
 *
 * \section workflow_sec Workflow and Examples
 *
 * \n <b>All measurement functions need the camera to be initialized, warmed up and a reference image taken:</b>
 *
 * \n initializeCamera()
 * \n warmUpCamera()
 * \n acquireReferenceImage()
 * \n
 * \n <b>After everything is set up, the field dimensions can be found:</b>
 * \n
 * \n findLightFieldDimensions()
 * \n
 * \n <b>The y2 field over-travel, for example, can be found by subtracting the y2 field edge from the reference crosshair position:</b>
 * \n
   \n FieldDetect::FindLightFieldReturnStruct flfrs;                            //declare the return structure;
   \n double yRef = fieldDetect->referenceStruct.crosshairCenterReference.y;    //get the reference crosshair y value
   \n flfrs = fieldDetect->findLightFieldDimensions("");                        //find the field dimensions of the live image
   \n if(flfrs.error == 0)
   \n {
   \n   double y2=flfrs.edgePoints[0].y;                                        //edgePoints[0] is the rop right field edge point
   \n   double overtravel=(y2-yRef)/4;                                          //difference between field and crosshair (over-travel)
   \n }

 *
 */


#include "fielddetect.h"

#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/objdetect/aruco_board.hpp>
#include <opencv2/objdetect/aruco_dictionary.hpp>
#include <opencv2/geometry.hpp>

#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/videostab/deblurring.hpp>
#include <opencv2/core/persistence.hpp>
#include <algorithm>
#include <vector>
#include <fstream>
#include <QDebug>
#include <QDateTime>
#include <PeakFinder.h>
#include <findODI.h>



/*!The FieldDetect class finds the light field, crosshair and Aruco markers in an image (cv::Mat) and reports:
 * 1. Two points on each side of the field that define the 50% penumbra point.
 * 2. The distance between each of those points to the other side of the field (h1, h2, v1, v2).
 * 3. The center point of the light field.
 * 4. The two points each on the vertical and horizontal lines of the crosshair.
 * 5. The crosshair center.
 * 6. The undistorted, unwarped (transformed) image of the field.
 * 7. The four corner points of each of the four Aruco markers (sixteen points).
 *
 */
FieldDetect::FieldDetect()
{
  referenceStruct.cameraInitialized = false;
  referenceStruct.cameraCalLoaded = false;
  referenceStruct.referenceSet = false;

}

FieldDetect::~FieldDetect()
{
}



 /*!
 * Finds the location and dimensions of the light field and crosshair.
 * A reference image must have been acquired before the first call to this function.
 * Eight points are detected around the light field, two on each side.
 * Those two points are located at a distance 30% of the length of the side from each end point of the side.
 * The eight field edge points are the highest gradient across the field edge.
 * To acquire the image from the camera empty quotes are passed "".
 * To acquire the image from a file, a file name is passed.
 * \return t_FindLightFieldReturnStruct that contains the results
 * \n error code  0 = no error
 * \n error code -1 = reference image is not set
 * \n error code -2 = error finding refined corners - could not find a square or octogon in the image
 */
FieldDetect::t_FindLightFieldReturnStruct FieldDetect::findLightFieldDimensions(std::string imageFileName)
{

    t_findLightFieldRefinedCornersReturnStruct FLFRCRS;
    roiStruct_t roiStruct;
    std::vector<roiStruct_t> roiStructVec;
    FindLightFieldReturnStruct findLightFieldStruct;
    double pctOfSide = 0.30;   //this is the percentage of the field side length to place roi's

    findLightFieldStruct.error = 0;

    if(!referenceStruct.referenceSet)
    {
      findLightFieldStruct.error = -1;
      return findLightFieldStruct;
    }


    FLFRCRS = findLightFieldRefinedCorners(imageFileName);
    if(FLFRCRS.error<0)
    {
      findLightFieldStruct.error = -2;
      return findLightFieldStruct;
    }


    //get two roi points per side. point sequence for square -> lines (0,1), (1,2), (2,3), (3,0)
    std::vector<std::vector<cv::Point2d>> roiPoints;
    for(int i=1; i<5; i++)
    {
       roiPoints.push_back(calcROIPoints(FLFRCRS.corners[i-1],FLFRCRS.corners[i%4], pctOfSide));
    }

    //package the four roi rectangles and roi mats into a vector of roiStructVec's
    //the element order of the ROI's is ccw with the first point being the top right corner.
    for(size_t i=0; i<roiPoints.size(); i++)
    {
      cv::Rect2d ROIRect0(roiPoints[i][0].x-(lfSquare/2), roiPoints[i][0].y-(lfSquare/2) , lfSquare, lfSquare);
      cv::Rect2d ROIRect1(roiPoints[i][1].x-(lfSquare/2), roiPoints[i][1].y-(lfSquare/2) , lfSquare, lfSquare);

      cv::Mat topROI = FLFRCRS.imageFlat(ROIRect0);
      cv::Mat botROI = FLFRCRS.imageFlat(ROIRect1);

      cv::Mat topROIcpy;
      cv::Mat botROIcpy;

      topROI.copyTo(topROIcpy);
      botROI.copyTo(botROIcpy);

      roiStruct.roiRect0 = ROIRect0;
      roiStruct.roiRect1 = ROIRect1;
      roiStruct.roiMat0 = topROIcpy;    //copy the roi into a new mat.  Otherwise the roi box is rotated in the original
      roiStruct.roiMat1 = botROIcpy;
      roiStructVec.push_back(roiStruct);
    }


    //Add the value calculated from the roi to the roi top left corner to get the precise field edge
    //and put the field edges in fieldEdges vector.  There are two points per side.  p1 (index 0) is the top left.
    //and rotatin is ccw.  So, the ccw order is p1, p2, p3, ...
    std::vector<cv::Point2d> fieldEdges;
    int fileCount=0;
    for(size_t i=0; i<roiStructVec.size(); i++)
    {
        //Rotate the roi. The rotateAndFindMedian function processes the roi by row, so two sides of the light field need to be rotated 90deg
        double angle;
        if(i==0) angle = 0;   //
        if(i==1) angle = -90;  //
        if(i==2) angle = 0;   //
        if(i==3) angle = -90;  //

        std::vector<cv::Point2d> posROI0 = rotateAndFindMedian(roiStructVec[i].roiMat0, angle, fileCount);
        std::vector<cv::Point2d> posROI1 = rotateAndFindMedian(roiStructVec[i].roiMat1, angle, fileCount+1);

        fileCount=fileCount+2;

        cv::Point2d p0, p1;
        p0.x=roiStructVec[i].roiRect0.x+posROI0[0].x;
        p0.y=roiStructVec[i].roiRect0.y+posROI0[0].y;
        p1.x=roiStructVec[i].roiRect1.x+posROI1[0].x;
        p1.y=roiStructVec[i].roiRect1.y+posROI1[0].y;

        fieldEdges.push_back(p0);
        fieldEdges.push_back(p1);

        findLightFieldStruct.edgePoints.push_back(p0);
        findLightFieldStruct.edgePoints.push_back(p1);
    }

    findLightFieldStruct.lfCenterPoint = cv::Point2d(FLFRCRS.corners[1].x + ((FLFRCRS.corners[0].x - FLFRCRS.corners[1].x)/2),
                                                                                ((FLFRCRS.corners[1].y+(FLFRCRS.corners[2].y - FLFRCRS.corners[1].y)/2)));

    //calculate the distance between field light edges - two vertical and two horizontal
    double vertLenX, vertLenY;
    vertLenX = abs(fieldEdges[1].x - fieldEdges[4].x);
    vertLenY = abs(fieldEdges[1].y - fieldEdges[4].y);
    double v1 = sqrt((vertLenX*vertLenX)+(vertLenY*vertLenY));

    vertLenX = abs(fieldEdges[0].x - fieldEdges[5].x);
    vertLenY = abs(fieldEdges[0].y - fieldEdges[5].y);
    double v2 = sqrt((vertLenX*vertLenX)+(vertLenY*vertLenY));

    double horLenX, horLenY;
    horLenX = abs(fieldEdges[2].x - fieldEdges[7].x);
    horLenY = abs(fieldEdges[2].y - fieldEdges[7].y);
    double h1 = sqrt((horLenX*horLenX)+(horLenY*horLenY));

    horLenX = abs(fieldEdges[3].x - fieldEdges[6].x);
    horLenY = abs(fieldEdges[3].y - fieldEdges[6].y);
    double h2 = sqrt((horLenX*horLenX)+(horLenY*horLenY));

    findLightFieldStruct.h1Len = h1;
    findLightFieldStruct.h2Len = h2;
    findLightFieldStruct.v1Len = v1;
    findLightFieldStruct.v2Len = v2;

    //Handle crosshairs
    //First, calculate the center point of the field light sides and
    //put these four center points in a vector of points (chROIPts).
    //These points are the endpoints of the crosshairs.
    double x,y;
    std::vector<cv::Point2d> chROIPoint;
    for(size_t i=1; i<5; i++)
    {
      if ( i % 2 == 0)  //if it's the vertical crosshair
      {
        //the field light side plus the centerpoint of the crosshair
        x = (abs(FLFRCRS.corners[i-1].x + (FLFRCRS.corners[i-1].x - FLFRCRS.corners[i%4].x)/2));
        y = (abs(FLFRCRS.corners[i-1].y - (FLFRCRS.corners[i-1].y - FLFRCRS.corners[i%4].y)/2));


      }
      else              //if it's the horizontal crosshair
      {
        //the field light side plus the centerpoint of the crosshair
        x = abs(FLFRCRS.corners[i-1].x - (FLFRCRS.corners[i-1].x - FLFRCRS.corners[i%4].x)/2);
        y = abs(FLFRCRS.corners[i-1].y + (FLFRCRS.corners[i-1].y - FLFRCRS.corners[i%4].y)/2);

      }
      chROIPoint.push_back(cv::Point2d(x,y));
    }

    //calculate the two roi points 20% from crosshair ends
    std::vector<cv::Point2d> chCalcPtsHorz = calcROIPoints(chROIPoint[0], chROIPoint[2], 0.35);
    std::vector<cv::Point2d> chCalcPtsVert = calcROIPoints(chROIPoint[1], chROIPoint[3], 0.35);

    //make the crosshair roi rectangles
    cv::Rect2d chROIVertRect0(chCalcPtsVert[0].x-(chSquare/2), chCalcPtsVert[0].y-(chSquare/2),chSquare,chSquare);
    cv::Rect2d chROIVertRect1(chCalcPtsVert[1].x-(chSquare/2), chCalcPtsVert[1].y-(chSquare/2),chSquare,chSquare);
    cv::Rect2d chROIHorzRect0(chCalcPtsHorz[0].x-(chSquare/2), chCalcPtsHorz[0].y-(chSquare/2),chSquare,chSquare);
    cv::Rect2d chROIHorzRect1(chCalcPtsHorz[1].x-(chSquare/2), chCalcPtsHorz[1].y-(chSquare/2),chSquare,chSquare);
    //make the crosshair roi Mats
    cv::Mat chROIVertMat0;
    cv::Mat chROIVertMat1;
    cv::Mat chROIHorzMat0;
    cv::Mat chROIHorzMat1;


    chROIVertMat0 = FLFRCRS.imageFlat(chROIVertRect0);
    std::vector<cv::Point2d> chVertCalcPt0 = processCrosshairMat(chROIVertMat0, 0);
    cv::Point2d chVertPt0;
    chVertPt0.x = chVertCalcPt0[1].x + chROIVertRect0.tl().x;
    chVertPt0.y = chVertCalcPt0[1].y + chROIVertRect0.tl().y;

    chROIVertMat1 = FLFRCRS.imageFlat(chROIVertRect1);
    std::vector<cv::Point2d> chVertCalcPt1 = processCrosshairMat(chROIVertMat1, 0);
    cv::Point2d chVertPt1;
    chVertPt1.x = chVertCalcPt1[1].x + chROIVertRect1.tl().x;
    chVertPt1.y = chVertCalcPt1[1].y + chROIVertRect1.tl().y;

    chROIHorzMat0 = FLFRCRS.imageFlat(chROIHorzRect0);
    std::vector<cv::Point2d> chHorzCalcPt0 = processCrosshairMat(chROIHorzMat0, -90);
    cv::Point2d chHorzPt0;
    chHorzPt0.x = chHorzCalcPt0[1].x + chROIHorzRect0.tl().x;
    chHorzPt0.y = chHorzCalcPt0[1].y + chROIHorzRect0.tl().y;

    chROIHorzMat1 = FLFRCRS.imageFlat(chROIHorzRect1);
    std::vector<cv::Point2d> chHorzCalcPt1 = processCrosshairMat(chROIHorzMat1, -90);
    cv::Point2d chHorzPt1;
    chHorzPt1.x = chHorzCalcPt1[1].x + chROIHorzRect1.tl().x;
    chHorzPt1.y = chHorzCalcPt1[1].y + chROIHorzRect1.tl().y;

    findLightFieldStruct.chV0 = chVertPt0;
    findLightFieldStruct.chV1 = chVertPt1;
    findLightFieldStruct.chH0 = chHorzPt0;
    findLightFieldStruct.chH1 = chHorzPt1;

    //crosshair center point
    cv::Point2d center = lineLineIntersection(chVertPt0, chVertPt1, chHorzPt0, chHorzPt1);
    findLightFieldStruct.chCenterPoint.x = center.x;
    findLightFieldStruct.chCenterPoint.y = center.y;


    //  ALL DIAG BELOW

    cvtColor(FLFRCRS.imageFlat, FLFRCRS.imageFlat, COLOR_GRAY2RGB);


    for(size_t i=0; i<roiStructVec.size(); i++)                                                   //diagnostic
    {                                                                                             //diagnostic
      //cv::circle(aprs.imageFlat,roiPoints[i][0],2,cv::Scalar(25,255,255),1,cv::LINE_AA);  //diagnostic
      //cv::circle(aprs.imageFlat,roiPoints[i][1],2,cv::Scalar(25,255,255),1,cv::LINE_AA);  //diagnostic
      //cv::circle(aprs.imageFlat,rotPoints[i],2,cv::Scalar(25,255,255),1,cv::LINE_AA);  //diagnostic
      cv::rectangle(FLFRCRS.imageFlat, roiStructVec[i].roiRect0,cv::Scalar(0,255,0),3,cv::LINE_AA);
      cv::rectangle(FLFRCRS.imageFlat, roiStructVec[i].roiRect1,cv::Scalar(0,255,0),3,cv::LINE_AA);
      //cv::circle(aprs.imageFlat, chCalcPtsVert[0],2,cv::Scalar(0,255,0),1,cv::LINE_AA);
      //cv::circle(aprs.imageFlat, chCalcPtsVert[1],2,cv::Scalar(0,255,0),1,cv::LINE_AA);
      //cv::circle(aprs.imageFlat, chCalcPtsHorz[0],2,cv::Scalar(0,255,0),1,cv::LINE_AA);
      //cv::circle(aprs.imageFlat, chCalcPtsHorz[1],2,cv::Scalar(0,255,0),1,cv::LINE_AA);
    }                                                                                           //diagnostic


    cv::rectangle(FLFRCRS.imageFlat, chROIHorzRect0,cv::Scalar(0,255,0),3,cv::LINE_AA);
    cv::rectangle(FLFRCRS.imageFlat, chROIHorzRect1,cv::Scalar(0,255,0),3,cv::LINE_AA);
    cv::rectangle(FLFRCRS.imageFlat, chROIVertRect0,cv::Scalar(0,255,0),3,cv::LINE_AA);
    cv::rectangle(FLFRCRS.imageFlat, chROIVertRect1,cv::Scalar(0,255,0),3,cv::LINE_AA);

    cv::line(FLFRCRS.imageFlat,cv::Point2d(fieldEdges[0].x,fieldEdges[0].y), cv::Point2d(fieldEdges[5].x,fieldEdges[5].y),cv::Scalar(0,255,255,0),3,cv::LINE_AA);
    cv::line(FLFRCRS.imageFlat,cv::Point2d(fieldEdges[1].x,fieldEdges[1].y), cv::Point2d(fieldEdges[4].x,fieldEdges[4].y),cv::Scalar(0,255,255,0),3,cv::LINE_AA);
    cv::line(FLFRCRS.imageFlat,cv::Point2d(fieldEdges[2].x,fieldEdges[2].y), cv::Point2d(fieldEdges[7].x,fieldEdges[7].y),cv::Scalar(0,255,255,0),3,cv::LINE_AA);
    cv::line(FLFRCRS.imageFlat,cv::Point2d(fieldEdges[3].x,fieldEdges[3].y), cv::Point2d(fieldEdges[6].x,fieldEdges[6].y),cv::Scalar(0,255,255,0),3,cv::LINE_AA);


    cv::line(FLFRCRS.imageFlat,cv::Point2d(chVertPt0.x,chVertPt0.y), cv::Point2d(chVertPt1.x,chVertPt1.y),cv::Scalar(0,255,255,0),3,cv::LINE_AA);
    cv::line(FLFRCRS.imageFlat,cv::Point2d(chHorzPt0.x,chHorzPt0.y), cv::Point2d(chHorzPt1.x,chHorzPt1.y),cv::Scalar(0,255,255,0),3,cv::LINE_AA);


    cv::circle(FLFRCRS.imageFlat,FLFRCRS.corners[0],3,cv::Scalar(255,255,255),3,cv::LINE_AA);
    cv::circle(FLFRCRS.imageFlat,FLFRCRS.corners[1],3,cv::Scalar(255,255,255),3,cv::LINE_AA);
    cv::circle(FLFRCRS.imageFlat,FLFRCRS.corners[2],3,cv::Scalar(255,255,255),3,cv::LINE_AA);
    cv::circle(FLFRCRS.imageFlat,FLFRCRS.corners[3],3,cv::Scalar(255,255,255),3,cv::LINE_AA);

    cv::circle(FLFRCRS.imageFlat,findLightFieldStruct.lfCenterPoint,5,cv::Scalar(255,255,255),3,cv::LINE_AA);



    //cv::circle(aprs.imageFlat,chVertPt0,3,cv::Scalar(255,255,255),1,cv::LINE_AA);
    //cv::circle(aprs.imageFlat,chVertPt1,3,cv::Scalar(255,255,255),1,cv::LINE_AA);
    //cv::circle(aprs.imageFlat,chHorzPt0,3,cv::Scalar(255,255,255),1,cv::LINE_AA);
    //cv::circle(aprs.imageFlat,chHorzPt1,3,cv::Scalar(255,255,255),1,cv::LINE_AA);

    cv::circle(FLFRCRS.imageFlat,findLightFieldStruct.chCenterPoint,10,cv::Scalar(255,255,255),3,cv::LINE_AA);

    cv::putText(FLFRCRS.imageFlat,"v1.1",cv::Point(30,1930),cv::FONT_HERSHEY_DUPLEX,1,cv::Scalar(255,255,255),2,false);

    //cv::imshow("roi", aprs.imageFlat);
    //cv::waitKey(0);
    //cv::imwrite("C:\\Users\\mikea\\OneDrive\\Documents\\fieldcheckqt\\top_roi.bmp", rotMat);



  //return the field dimensions and unwarped and rotated image
  findLightFieldStruct.imageCorrected = FLFRCRS.imageFlat;
  return findLightFieldStruct;
}






/*!
* Finds the location and dimensions of the light field and crosshair.
* Overloaded function that takes a cv::Mat as the input
* A reference image must have been acquired before the first call to this function.
* Eight points are detected around the light field, two on each side.
* Those two points are located at a distance 30% of the length of the side from each end point of the side.
* The eight field edge points are the highest gradient across the field edge.
* To acquire the image from the camera empty quotes are passed "".
* To acquire the image from a file, a file name is passed.
* \return t_FindLightFieldReturnStruct that contains the results
* \n error code  0 = no error
* \n error code -1 = reference image is not set
* \n error code -2 = error finding refined corners - could not find a square or octogon in the image
*/
FieldDetect::t_FindLightFieldReturnStruct FieldDetect::findLightFieldDimensions(cv::Mat* image)
{

   t_findLightFieldRefinedCornersReturnStruct FLFRCRS;
   roiStruct_t roiStruct;
   std::vector<roiStruct_t> roiStructVec;
   FindLightFieldReturnStruct findLightFieldStruct;
   double pctOfSide = 0.30;   //this is the percentage of the field side length to place roi's

   findLightFieldStruct.error = 0;

   qDebug()<<"entering findLightFieldDimensions(cv::Mat image)";
   FLFRCRS = findLightFieldRefinedCorners(image);
   qDebug()<<"findLightFieldDimensions(cv::Mat image) FLFRCRS.error: "<<FLFRCRS.error;

   if(FLFRCRS.error <0)
   {
     findLightFieldStruct.error = -1;
     return findLightFieldStruct;
   }


   //get two roi points per side. point sequence for square -> lines (0,1), (1,2), (2,3), (3,0)
   std::vector<std::vector<cv::Point2d>> roiPoints;
   for(int i=1; i<5; i++)
   {
      roiPoints.push_back(calcROIPoints(FLFRCRS.corners[i-1],FLFRCRS.corners[i%4], pctOfSide));
   }

   //package the four roi rectangles and roi mats into a vector of roiStructVec's
   //the element order of the ROI's is ccw with the first point being the top right corner.
   for(size_t i=0; i<roiPoints.size(); i++)
   {
     cv::Rect2d ROIRect0(roiPoints[i][0].x-(lfSquare/2), roiPoints[i][0].y-(lfSquare/2) , lfSquare, lfSquare);
     cv::Rect2d ROIRect1(roiPoints[i][1].x-(lfSquare/2), roiPoints[i][1].y-(lfSquare/2) , lfSquare, lfSquare);

     cv::Mat topROI = FLFRCRS.imageFlat(ROIRect0);
     cv::Mat botROI = FLFRCRS.imageFlat(ROIRect1);

     cv::Mat topROIcpy;
     cv::Mat botROIcpy;

     topROI.copyTo(topROIcpy);
     botROI.copyTo(botROIcpy);

     roiStruct.roiRect0 = ROIRect0;
     roiStruct.roiRect1 = ROIRect1;
     roiStruct.roiMat0 = topROIcpy;    //copy the roi into a new mat.  Otherwise the roi box is rotated in the original
     roiStruct.roiMat1 = botROIcpy;
     roiStructVec.push_back(roiStruct);
   }


   //Add the value calculated from the roi to the roi top left corner to get the precise field edge
   //and put the field edges in fieldEdges vector.  There are two points per side.  p1 (index 0) is the top left.
   //and rotatin is ccw.  So, the ccw order is p1, p2, p3, ...
   std::vector<cv::Point2d> fieldEdges;
   int fileCount=0;
   for(size_t i=0; i<roiStructVec.size(); i++)
   {
       //Rotate the roi. The rotateAndFindMedian function processes the roi by row, so two sides of the light field need to be rotated 90deg
       double angle;
       if(i==0) angle = 0;   //
       if(i==1) angle = -90;  //
       if(i==2) angle = 0;   //
       if(i==3) angle = -90;  //

       std::vector<cv::Point2d> posROI0 = rotateAndFindMedian(roiStructVec[i].roiMat0, angle, fileCount);
       std::vector<cv::Point2d> posROI1 = rotateAndFindMedian(roiStructVec[i].roiMat1, angle, fileCount+1);

       fileCount=fileCount+2;

       cv::Point2d p0, p1;
       p0.x=roiStructVec[i].roiRect0.x+posROI0[0].x;
       p0.y=roiStructVec[i].roiRect0.y+posROI0[0].y;
       p1.x=roiStructVec[i].roiRect1.x+posROI1[0].x;
       p1.y=roiStructVec[i].roiRect1.y+posROI1[0].y;

       fieldEdges.push_back(p0);
       fieldEdges.push_back(p1);

       findLightFieldStruct.edgePoints.push_back(p0);
       findLightFieldStruct.edgePoints.push_back(p1);
   }

   findLightFieldStruct.lfCenterPoint = cv::Point2d(FLFRCRS.corners[1].x + ((FLFRCRS.corners[0].x - FLFRCRS.corners[1].x)/2),
                                                                               ((FLFRCRS.corners[1].y+(FLFRCRS.corners[2].y - FLFRCRS.corners[1].y)/2)));

   //calculate the distance between field light edges - two vertical and two horizontal
   double vertLenX, vertLenY;
   vertLenX = abs(fieldEdges[1].x - fieldEdges[4].x);
   vertLenY = abs(fieldEdges[1].y - fieldEdges[4].y);
   double v1 = sqrt((vertLenX*vertLenX)+(vertLenY*vertLenY));

   vertLenX = abs(fieldEdges[0].x - fieldEdges[5].x);
   vertLenY = abs(fieldEdges[0].y - fieldEdges[5].y);
   double v2 = sqrt((vertLenX*vertLenX)+(vertLenY*vertLenY));

   double horLenX, horLenY;
   horLenX = abs(fieldEdges[2].x - fieldEdges[7].x);
   horLenY = abs(fieldEdges[2].y - fieldEdges[7].y);
   double h1 = sqrt((horLenX*horLenX)+(horLenY*horLenY));

   horLenX = abs(fieldEdges[3].x - fieldEdges[6].x);
   horLenY = abs(fieldEdges[3].y - fieldEdges[6].y);
   double h2 = sqrt((horLenX*horLenX)+(horLenY*horLenY));

   findLightFieldStruct.h1Len = h1;
   findLightFieldStruct.h2Len = h2;
   findLightFieldStruct.v1Len = v1;
   findLightFieldStruct.v2Len = v2;

   //Handle crosshairs
   //First, calculate the center point of the field light sides and
   //put these four center points in a vector of points (chROIPts).
   //These points are the endpoints of the crosshairs.
   double x,y;
   std::vector<cv::Point2d> chROIPoint;
   for(size_t i=1; i<5; i++)
   {
     if ( i % 2 == 0)  //if it's the vertical crosshair
     {
       //the field light side plus the centerpoint of the crosshair
       x = (abs(FLFRCRS.corners[i-1].x + (FLFRCRS.corners[i-1].x - FLFRCRS.corners[i%4].x)/2));
       y = (abs(FLFRCRS.corners[i-1].y - (FLFRCRS.corners[i-1].y - FLFRCRS.corners[i%4].y)/2));


     }
     else              //if it's the horizontal crosshair
     {
       //the field light side plus the centerpoint of the crosshair
       x = abs(FLFRCRS.corners[i-1].x - (FLFRCRS.corners[i-1].x - FLFRCRS.corners[i%4].x)/2);
       y = abs(FLFRCRS.corners[i-1].y + (FLFRCRS.corners[i-1].y - FLFRCRS.corners[i%4].y)/2);

     }
     chROIPoint.push_back(cv::Point2d(x,y));
   }

   //calculate the two roi points 20% from crosshair ends
   std::vector<cv::Point2d> chCalcPtsHorz = calcROIPoints(chROIPoint[0], chROIPoint[2], 0.35);
   std::vector<cv::Point2d> chCalcPtsVert = calcROIPoints(chROIPoint[1], chROIPoint[3], 0.35);

   //make the crosshair roi rectangles
   cv::Rect2d chROIVertRect0(chCalcPtsVert[0].x-(chSquare/2), chCalcPtsVert[0].y-(chSquare/2),chSquare,chSquare);
   cv::Rect2d chROIVertRect1(chCalcPtsVert[1].x-(chSquare/2), chCalcPtsVert[1].y-(chSquare/2),chSquare,chSquare);
   cv::Rect2d chROIHorzRect0(chCalcPtsHorz[0].x-(chSquare/2), chCalcPtsHorz[0].y-(chSquare/2),chSquare,chSquare);
   cv::Rect2d chROIHorzRect1(chCalcPtsHorz[1].x-(chSquare/2), chCalcPtsHorz[1].y-(chSquare/2),chSquare,chSquare);
   //make the crosshair roi Mats
   cv::Mat chROIVertMat0;
   cv::Mat chROIVertMat1;
   cv::Mat chROIHorzMat0;
   cv::Mat chROIHorzMat1;


   chROIVertMat0 = FLFRCRS.imageFlat(chROIVertRect0);
   std::vector<cv::Point2d> chVertCalcPt0 = processCrosshairMat(chROIVertMat0, 0);
   cv::Point2d chVertPt0;
   chVertPt0.x = chVertCalcPt0[1].x + chROIVertRect0.tl().x;
   chVertPt0.y = chVertCalcPt0[1].y + chROIVertRect0.tl().y;

   chROIVertMat1 = FLFRCRS.imageFlat(chROIVertRect1);
   std::vector<cv::Point2d> chVertCalcPt1 = processCrosshairMat(chROIVertMat1, 0);
   cv::Point2d chVertPt1;
   chVertPt1.x = chVertCalcPt1[1].x + chROIVertRect1.tl().x;
   chVertPt1.y = chVertCalcPt1[1].y + chROIVertRect1.tl().y;

   chROIHorzMat0 = FLFRCRS.imageFlat(chROIHorzRect0);
   std::vector<cv::Point2d> chHorzCalcPt0 = processCrosshairMat(chROIHorzMat0, -90);
   cv::Point2d chHorzPt0;
   chHorzPt0.x = chHorzCalcPt0[1].x + chROIHorzRect0.tl().x;
   chHorzPt0.y = chHorzCalcPt0[1].y + chROIHorzRect0.tl().y;

   chROIHorzMat1 = FLFRCRS.imageFlat(chROIHorzRect1);
   std::vector<cv::Point2d> chHorzCalcPt1 = processCrosshairMat(chROIHorzMat1, -90);
   cv::Point2d chHorzPt1;
   chHorzPt1.x = chHorzCalcPt1[1].x + chROIHorzRect1.tl().x;
   chHorzPt1.y = chHorzCalcPt1[1].y + chROIHorzRect1.tl().y;

   findLightFieldStruct.chV0 = chVertPt0;
   findLightFieldStruct.chV1 = chVertPt1;
   findLightFieldStruct.chH0 = chHorzPt0;
   findLightFieldStruct.chH1 = chHorzPt1;

   //crosshair center point
   cv::Point2d center = lineLineIntersection(chVertPt0, chVertPt1, chHorzPt0, chHorzPt1);
   findLightFieldStruct.chCenterPoint.x = center.x;
   findLightFieldStruct.chCenterPoint.y = center.y;


   //  ALL DIAG BELOW

   for(size_t i=0; i<roiStructVec.size(); i++)                                                   //diagnostic
   {                                                                                             //diagnostic
     //cv::circle(aprs.imageFlat,roiPoints[i][0],2,cv::Scalar(25,255,255),1,cv::LINE_AA);  //diagnostic
     //cv::circle(aprs.imageFlat,roiPoints[i][1],2,cv::Scalar(25,255,255),1,cv::LINE_AA);  //diagnostic
     //cv::circle(aprs.imageFlat,rotPoints[i],2,cv::Scalar(25,255,255),1,cv::LINE_AA);  //diagnostic
     cv::rectangle(FLFRCRS.imageFlat, roiStructVec[i].roiRect0,cv::Scalar(0,255,0),1,cv::LINE_AA);
     cv::rectangle(FLFRCRS.imageFlat, roiStructVec[i].roiRect1,cv::Scalar(0,255,0),1,cv::LINE_AA);
     //cv::circle(aprs.imageFlat, chCalcPtsVert[0],2,cv::Scalar(0,255,0),1,cv::LINE_AA);
     //cv::circle(aprs.imageFlat, chCalcPtsVert[1],2,cv::Scalar(0,255,0),1,cv::LINE_AA);
     //cv::circle(aprs.imageFlat, chCalcPtsHorz[0],2,cv::Scalar(0,255,0),1,cv::LINE_AA);
     //cv::circle(aprs.imageFlat, chCalcPtsHorz[1],2,cv::Scalar(0,255,0),1,cv::LINE_AA);
   }                                                                                           //diagnostic


   cv::rectangle(FLFRCRS.imageFlat, chROIHorzRect0,cv::Scalar(0,255,0),1,cv::LINE_AA);
   cv::rectangle(FLFRCRS.imageFlat, chROIHorzRect1,cv::Scalar(0,255,0),1,cv::LINE_AA);
   cv::rectangle(FLFRCRS.imageFlat, chROIVertRect0,cv::Scalar(0,255,0),1,cv::LINE_AA);
   cv::rectangle(FLFRCRS.imageFlat, chROIVertRect1,cv::Scalar(0,255,0),1,cv::LINE_AA);

   cv::line(FLFRCRS.imageFlat,cv::Point2d(fieldEdges[0].x,fieldEdges[0].y), cv::Point2d(fieldEdges[5].x,fieldEdges[5].y),cv::Scalar(0,255,255,0),1,cv::LINE_AA);
   cv::line(FLFRCRS.imageFlat,cv::Point2d(fieldEdges[1].x,fieldEdges[1].y), cv::Point2d(fieldEdges[4].x,fieldEdges[4].y),cv::Scalar(0,255,255,0),1,cv::LINE_AA);
   cv::line(FLFRCRS.imageFlat,cv::Point2d(fieldEdges[2].x,fieldEdges[2].y), cv::Point2d(fieldEdges[7].x,fieldEdges[7].y),cv::Scalar(0,255,255,0),1,cv::LINE_AA);
   cv::line(FLFRCRS.imageFlat,cv::Point2d(fieldEdges[3].x,fieldEdges[3].y), cv::Point2d(fieldEdges[6].x,fieldEdges[6].y),cv::Scalar(0,255,255,0),1,cv::LINE_AA);


   cv::line(FLFRCRS.imageFlat,cv::Point2d(chVertPt0.x,chVertPt0.y), cv::Point2d(chVertPt1.x,chVertPt1.y),cv::Scalar(0,255,255,0),1,cv::LINE_AA);
   cv::line(FLFRCRS.imageFlat,cv::Point2d(chHorzPt0.x,chHorzPt0.y), cv::Point2d(chHorzPt1.x,chHorzPt1.y),cv::Scalar(0,255,255,0),1,cv::LINE_AA);


   cv::circle(FLFRCRS.imageFlat,FLFRCRS.corners[0],3,cv::Scalar(255,255,255),1,cv::LINE_AA);
   cv::circle(FLFRCRS.imageFlat,FLFRCRS.corners[1],3,cv::Scalar(255,255,255),1,cv::LINE_AA);
   cv::circle(FLFRCRS.imageFlat,FLFRCRS.corners[2],3,cv::Scalar(255,255,255),1,cv::LINE_AA);
   cv::circle(FLFRCRS.imageFlat,FLFRCRS.corners[3],3,cv::Scalar(255,255,255),1,cv::LINE_AA);

   cv::circle(FLFRCRS.imageFlat,findLightFieldStruct.lfCenterPoint,5,cv::Scalar(255,255,255),1,cv::LINE_AA);



   //cv::circle(aprs.imageFlat,chVertPt0,3,cv::Scalar(255,255,255),1,cv::LINE_AA);
   //cv::circle(aprs.imageFlat,chVertPt1,3,cv::Scalar(255,255,255),1,cv::LINE_AA);
   //cv::circle(aprs.imageFlat,chHorzPt0,3,cv::Scalar(255,255,255),1,cv::LINE_AA);
   //cv::circle(aprs.imageFlat,chHorzPt1,3,cv::Scalar(255,255,255),1,cv::LINE_AA);

   cv::circle(FLFRCRS.imageFlat,findLightFieldStruct.chCenterPoint,10,cv::Scalar(255,255,255),1,cv::LINE_4);

   cv::putText(FLFRCRS.imageFlat,"v1.1",cv::Point(30,1930),cv::FONT_HERSHEY_DUPLEX,1,cv::Scalar(255,255,255),2,false);

   //cv::imshow("roi", aprs.imageFlat);
   //cv::waitKey(0);
   //cv::imwrite("C:\\Users\\mikea\\OneDrive\\Documents\\fieldcheckqt\\top_roi.bmp", rotMat);



 //return the field dimensions and unwarped and rotated image
 findLightFieldStruct.imageCorrected = FLFRCRS.imageFlat;
 return findLightFieldStruct;
}



/*!
 * Find the refined light field corners
 * \param imageFileName image from a file or if imageFileName = "" the image is acquired from the camera.
 * \return t_findLightFieldRefinedCornersReturnStruct
 */
FieldDetect::t_findLightFieldRefinedCornersReturnStruct FieldDetect::findLightFieldRefinedCorners(std::string imageFileName)
{

    cv::Mat *imageXfmd;
    APRetStrut aprs;
    roiStruct_t roiStruct;
    std::vector<roiStruct_t> roiStructVec;
    t_findLightFieldRefinedCornersReturnStruct FLFRCRS;
    double pctOfSide = 0.30;   //this is the percentage of the field side length to place roi's

    FLFRCRS.error = 0;

    //if the image is from the camera
    if(imageFileName.size()==0 && referenceStruct.cameraInitialized && referenceStruct.cameraCalLoaded)
    {
      imageXfmd = getImage();
      if(imageXfmd->empty())
      {
        FLFRCRS.error = -1;
        return FLFRCRS;
      }
    }
    else
    //if the image is from a file
    if(imageFileName.size()>0 && referenceStruct.cameraCalLoaded)
    {
      imageXfmd = getImage(imageFileName);
      if(imageXfmd->empty())
      {
        FLFRCRS.error = -2;
        return FLFRCRS;
      }
    }
    else
    {
      FLFRCRS.error = -3;
      return FLFRCRS;
    }

    aprs = *arucoPerspective(imageXfmd, false);


    PolyReturnStruct field = findLightField(&aprs.imageFlat);

    if(!field.validField || field.pts.size() == 0)
    {
      FLFRCRS.error = -4;
      return FLFRCRS;
    }


    //find field center by putting the field square in an upright bounding box
    cv::Rect rect = cv::boundingRect(field.pts);
    cv::Point2d fieldCenter((rect.width/2)+rect.x,(rect.height/2)+rect.y);

    //get two roi points per side. point sequence for square -> lines (0,1), (1,2), (2,3), (3,0)
    std::vector<std::vector<cv::Point2d>> roiPoints;
    for(int i=1; i<5; i++)
    {
       roiPoints.push_back(calcROIPoints(field.pts[i-1],field.pts[i%4], pctOfSide));
    }

    //package the four roi rectangles and roi mats into a vector of roiStructVec's
    //the element order of the ROI's is ccw with the first point being the top right corner.
    try
    {


      for(size_t i=0; i<roiPoints.size(); i++)
      {
        cv::Rect2d ROIRect0(roiPoints[i][0].x-(lfSquare/2), roiPoints[i][0].y-(lfSquare/2) , lfSquare, lfSquare);
        cv::Rect2d ROIRect1(roiPoints[i][1].x-(lfSquare/2), roiPoints[i][1].y-(lfSquare/2) , lfSquare, lfSquare);

        cv::Mat topROI = aprs.imageFlat(ROIRect0);
        cv::Mat botROI = aprs.imageFlat(ROIRect1);

        cv::Mat topROIcpy;
        cv::Mat botROIcpy;

        topROI.copyTo(topROIcpy);
        botROI.copyTo(botROIcpy);

        roiStruct.roiRect0 = ROIRect0;
        roiStruct.roiRect1 = ROIRect1;
        roiStruct.roiMat0 = topROIcpy;    //copy the roi into a new mat.  Otherwise the roi box is rotated in the original
        roiStruct.roiMat1 = botROIcpy;
        roiStructVec.push_back(roiStruct);
      }

    }
    catch( cv::Exception& e )
    {
      FLFRCRS.error = -99;
      return FLFRCRS;
    }

    //Add the value calculated from the roi to the roi top left corner to get the precise field edge
    //and put the field edges in fieldEdges vector.  There are two points per side.  p1 (index 0) is the top left.
    //and rotatin is ccw.  So, the ccw order is p1, p2, p3, ...
    std::vector<cv::Point2d> fieldEdges;
    int fileCount=0;
    for(size_t i=0; i<roiStructVec.size(); i++)
    {
        //Rotate the roi. The rotateAndFindMedian function processes the roi by row, so two sides of the light field need to be rotated 90deg
        double angle;
        if(i==0) angle = 0;   //
        if(i==1) angle = -90;  //
        if(i==2) angle = 0;   //
        if(i==3) angle = -90;  //

        std::vector<cv::Point2d> posROI0 = rotateAndFindMedian(roiStructVec[i].roiMat0, angle, fileCount);
        std::vector<cv::Point2d> posROI1 = rotateAndFindMedian(roiStructVec[i].roiMat1, angle, fileCount+1);

        fileCount=fileCount+2;

        cv::Point2d p0, p1;
        p0.x=roiStructVec[i].roiRect0.x+posROI0[0].x;
        p0.y=roiStructVec[i].roiRect0.y+posROI0[0].y;
        p1.x=roiStructVec[i].roiRect1.x+posROI1[0].x;
        p1.y=roiStructVec[i].roiRect1.y+posROI1[0].y;

        fieldEdges.push_back(p0);
        fieldEdges.push_back(p1);

    }

    FLFRCRS.corners.push_back(lineLineIntersection(fieldEdges[0], fieldEdges[1], fieldEdges[6], fieldEdges[7]));
    FLFRCRS.corners.push_back(lineLineIntersection(fieldEdges[0], fieldEdges[1], fieldEdges[2], fieldEdges[3]));
    FLFRCRS.corners.push_back(lineLineIntersection(fieldEdges[2], fieldEdges[3], fieldEdges[4], fieldEdges[5]));
    FLFRCRS.corners.push_back(lineLineIntersection(fieldEdges[4], fieldEdges[5], fieldEdges[6], fieldEdges[7]));

    FLFRCRS.imageFlat = aprs.imageFlat;

    return FLFRCRS;

}




/*!
 * Find the refined light field corners
 * \n Overload function to get the image from a cv::Mat
 * \n The image must be raw (not undistorted or flat)
 * \param cv::Mat*
 * \return t_findLightFieldRefinedCornersReturnStruct
 */
FieldDetect::t_findLightFieldRefinedCornersReturnStruct FieldDetect::findLightFieldRefinedCorners(cv::Mat* imageUndistorted)
{

    APRetStrut aprs;
    roiStruct_t roiStruct;
    std::vector<roiStruct_t> roiStructVec;
    t_findLightFieldRefinedCornersReturnStruct FLFRCRS;
    double pctOfSide = 0.30;   //this is the percentage of the field side length to place roi's

    qDebug()<<"entering findLightFieldRefinedCorners(cv::Mat*)";
    aprs = *arucoPerspective(imageUndistorted, false);
    qDebug()<<"findLightFieldRefinedCorners(cv::Mat*) arucoPerspective error "<<aprs.error;

    PolyReturnStruct field = findLightField(&aprs.imageFlat);
    if(!field.validField || field.pts.size() == 0)

    {
        FLFRCRS.error = -1;
        return FLFRCRS;
    }


    //find field center by putting the field square in an upright bounding box
    cv::Rect rect = cv::boundingRect(field.pts);
    cv::Point2d fieldCenter((rect.width/2)+rect.x,(rect.height/2)+rect.y);

    //get two roi points per side. point sequence for square -> lines (0,1), (1,2), (2,3), (3,0)
    std::vector<std::vector<cv::Point2d>> roiPoints;
    for(int i=1; i<5; i++)
    {
       roiPoints.push_back(calcROIPoints(field.pts[i-1],field.pts[i%4], pctOfSide));
    }

    //package the four roi rectangles and roi mats into a vector of roiStructVec's
    //the element order of the ROI's is ccw with the first point being the top right corner.
    try
    {

      for(size_t i=0; i<roiPoints.size(); i++)
      {
        cv::Rect2d ROIRect0(roiPoints[i][0].x-(lfSquare/2), roiPoints[i][0].y-(lfSquare/2) , lfSquare, lfSquare);
        cv::Rect2d ROIRect1(roiPoints[i][1].x-(lfSquare/2), roiPoints[i][1].y-(lfSquare/2) , lfSquare, lfSquare);

        cv::Mat topROI = aprs.imageFlat(ROIRect0);
        cv::Mat botROI = aprs.imageFlat(ROIRect1);

        cv::Mat topROIcpy;
        cv::Mat botROIcpy;

        topROI.copyTo(topROIcpy);
        botROI.copyTo(botROIcpy);

        roiStruct.roiRect0 = ROIRect0;
        roiStruct.roiRect1 = ROIRect1;
        roiStruct.roiMat0 = topROIcpy;    //copy the roi into a new mat.  Otherwise the roi box is rotated in the original
        roiStruct.roiMat1 = botROIcpy;
        roiStructVec.push_back(roiStruct);
      }
    }
    catch( cv::Exception& e )
    {
      FLFRCRS.error = -99;
      return FLFRCRS;
    }

    //Add the value calculated from the roi to the roi top left corner to get the precise field edge
    //and put the field edges in fieldEdges vector.  There are two points per side.  p1 (index 0) is the top left.
    //and rotatin is ccw.  So, the ccw order is p1, p2, p3, ...
    std::vector<cv::Point2d> fieldEdges;
    int fileCount=0;
    for(size_t i=0; i<roiStructVec.size(); i++)
    {
        //Rotate the roi. The rotateAndFindMedian function processes the roi by row, so two sides of the light field need to be rotated 90deg
        double angle;
        if(i==0) angle = 0;   //
        if(i==1) angle = -90;  //
        if(i==2) angle = 0;   //
        if(i==3) angle = -90;  //

        std::vector<cv::Point2d> posROI0 = rotateAndFindMedian(roiStructVec[i].roiMat0, angle, fileCount);
        std::vector<cv::Point2d> posROI1 = rotateAndFindMedian(roiStructVec[i].roiMat1, angle, fileCount+1);

        fileCount=fileCount+2;

        cv::Point2d p0, p1;
        p0.x=roiStructVec[i].roiRect0.x+posROI0[0].x;
        p0.y=roiStructVec[i].roiRect0.y+posROI0[0].y;
        p1.x=roiStructVec[i].roiRect1.x+posROI1[0].x;
        p1.y=roiStructVec[i].roiRect1.y+posROI1[0].y;

        fieldEdges.push_back(p0);
        fieldEdges.push_back(p1);

    }

    FLFRCRS.corners.push_back(lineLineIntersection(fieldEdges[0], fieldEdges[1], fieldEdges[6], fieldEdges[7]));
    FLFRCRS.corners.push_back(lineLineIntersection(fieldEdges[0], fieldEdges[1], fieldEdges[2], fieldEdges[3]));
    FLFRCRS.corners.push_back(lineLineIntersection(fieldEdges[2], fieldEdges[3], fieldEdges[4], fieldEdges[5]));
    FLFRCRS.corners.push_back(lineLineIntersection(fieldEdges[4], fieldEdges[5], fieldEdges[6], fieldEdges[7]));

    FLFRCRS.imageFlat = aprs.imageFlat;

    FLFRCRS.error = 0;

    return FLFRCRS;

}







/*!
 * Finds the lateral and longitudinal displacement between the reference image acquired by acquireReferenceImage() and the undistorted image from the camera.
 * Also finds the absolute vertical value based on difference between reference pose and current pose.  Vertical is accurate ONLY if vertical ONLY changed.
 * \return t_findArucoDisplacementReturnStruct that contains an error code and the lat and long displacement and absolute vert position.
 * \n error code  0 = no error
 * \n error code -1 = camera not initialized or reference image not set
 * \n error code -2 = no markers were found in the reference image
 * \n error code -3 = no live image was acquired from camera
 * \n error code -4 = no markers were found in the live image
 * \n error code -5 = width error (not between 28 and 32 mm)  check SSD = 100
 * \n error code -6 = height error (not between 28 and 32 mm) check SSD = 100
 */
FieldDetect::t_findArucoDisplacementReturnStruct FieldDetect::findArucoDisplacement()
{

 std::vector<std::vector<cv::Point2f>> cornersA, cornersR, rejectedCandidatesA, rejectedCandidatesR;
 std::vector<int> idsA, idsR;


 t_apRetStruct aprs;
 std::vector<cv::Point2d> imagePoints, referencePoints;
 cv::Mat image1, imageRaw;
 FieldDetect::t_findArucoDisplacementReturnStruct fadrs;

 if(!referenceStruct.cameraInitialized || !referenceStruct.referenceSet)
 {
   fadrs.error = -1;
   return fadrs;
 }

 referenceStruct.imageReferenceFlat.copyTo(image1);

 params.cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
 cv::aruco::ArucoDetector detector(dictionary, params);
 detector.detectMarkers(referenceStruct.imageReferenceFlat, cornersR, idsR, rejectedCandidatesR);


 if(idsR.size() < 1)
 {
   fadrs.error = -2;
   return fadrs;
 }

 cv::Mat* imageUndistorted = getImage();
 if(imageUndistorted->empty())
 {
   fadrs.error = -3;
   return fadrs;
 }

 imageUndistorted->copyTo(imageRaw);

 aprs = *arucoPerspective(imageUndistorted,false);

 detector.detectMarkers(aprs.imageFlat, cornersA, idsA, rejectedCandidatesA);

 if(idsA.size() < 1)
 {
   fadrs.error = -4;
   return fadrs;
 }


 //cv::imshow("findDisp getImage", *image);     //diagnostic
 //cv::waitKey(0);                              //diagnostic


 //use marker #3
 for(size_t i=0; i<idsA.size(); i++)
 {
   if(idsA[i] == 3)
   {
     imagePoints.push_back(cornersA[i][0]);
     imagePoints.push_back(cornersA[i][1]);
     imagePoints.push_back(cornersA[i][2]);
     imagePoints.push_back(cornersA[i][3]);
   }
 }

 for(size_t i=0; i<idsA.size(); i++)
 {
   if(idsR[i] == 3)
   {
       referencePoints.push_back(cornersR[i][0]);
       referencePoints.push_back(cornersR[i][1]);
       referencePoints.push_back(cornersR[i][2]);
       referencePoints.push_back(cornersR[i][3]);
   }
 }





 long double width  = fabs(imagePoints[0].x - imagePoints[1].x);
 long double height = fabs(imagePoints[0].y - imagePoints[2].y);



 //x and y travel is the distance between the top left marker point on the first live marker found and the same point on the reference marker
 long double x = (imagePoints[0].x - referencePoints[0].x)/4;
 long double y = (referencePoints[0].y - imagePoints[0].y)/4;


 if(width > 112 && width < 128 && height > 112 && height < 128)
 {
   fadrs.disp.x = x;
   fadrs.disp.y = y;
 }
 else
 {
   fadrs.disp.x = 0;
   fadrs.disp.y = 0;
 }



 t_boardPose boardPose;

 //find the pose of the current marker board.  The pose is of a marker board that consists of aruco markers #3 and #4
 boardPose = findBoardPose(&imageRaw);
 //subtract the current pose from the reference image pose.  The total vector length is the vertical distance in mm.
 double z = boardPose.translationLength - referenceStruct.boardPose.translationLength;

 fadrs.disp.z = z;

 Mat displayImage;
 aprs.imageFlat.copyTo(displayImage);
 cvtColor(displayImage, displayImage, COLOR_GRAY2RGB);

 fadrs.imageDisplay = displayImage;
 fadrs.error = 0;
 return fadrs;

}




/*!
 * Finds the lateral and longitudinal displacement between the reference image acquired by acquireReferenceImage() and  imageUndistorted.
 * Also finds the absolute vertical value based on difference between reference pose and current pose.  Vertical is accurate ONLY if vertical ONLY changed.
 * Overloaded function that takes a cv::Mat undistorted image as the input.
 * \return t_findArucoDisplacementReturnStruct that contains an error code and the lat and long displacement and absolute vert position.
 * \n error code  0 = no error
 * \n error code -1 = camera not initialized or reference image not set
 * \n error code -2 = no markers were found in the reference image
 * \n error code -4 = no markers were found in the image
 * \n error code -5 = width error (not between 28 and 32 mm)  check SSD = 100
 * \n error code -6 = height error (not between 28 and 32 mm) check SSD = 100
*/

FieldDetect::t_findArucoDisplacementReturnStruct FieldDetect::findArucoDisplacement(cv::Mat* imageUndistorted)
{

 std::vector<std::vector<cv::Point2f>> cornersA, cornersR, rejectedCandidatesA, rejectedCandidatesR;
 std::vector<int> idsA, idsR;


 t_apRetStruct aprs;
 std::vector<cv::Point2d> imagePoints, referencePoints;
 cv::Mat image1, imageRaw;
 FieldDetect::t_findArucoDisplacementReturnStruct fadrs;

 if(!referenceStruct.cameraInitialized || !referenceStruct.referenceSet)
 {
   fadrs.error = -1;
   return fadrs;
 }

 referenceStruct.imageReferenceFlat.copyTo(image1);

 params.cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
 cv::aruco::ArucoDetector detector(dictionary, params);
 detector.detectMarkers(referenceStruct.imageReferenceFlat, cornersR, idsR, rejectedCandidatesR);

 if(idsR.size() < 1)
 {
   fadrs.error = -2;
   return fadrs;
 }

 imageUndistorted->copyTo(imageRaw);

 aprs = *arucoPerspective(imageUndistorted,false);


 params.cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
 detector.detectMarkers(aprs.imageFlat, cornersA, idsA, rejectedCandidatesA);

 if(idsA.size() < 1)
 {
   fadrs.error = -4;
   return fadrs;
 }


 //cv::imshow("findDisp getImage", *image);     //diagnostic
 //cv::waitKey(0);                              //diagnostic


 //match each marker found in the image with its corresponding reference marker
 for(size_t i=0; i<idsA.size(); i++)
 {

   imagePoints.push_back(cornersA[i][0]);
   imagePoints.push_back(cornersA[i][1]);
   imagePoints.push_back(cornersA[i][2]);
   imagePoints.push_back(cornersA[i][3]);

   for(size_t j=0; j<idsR.size(); j++)
   {
     if(idsR[j] == idsA[i])
     {
       referencePoints.push_back(cornersR[j][0]);
       referencePoints.push_back(cornersR[j][1]);
       referencePoints.push_back(cornersR[j][2]);
       referencePoints.push_back(cornersR[j][3]);
     }
   }
 }


 long double width  = fabs(imagePoints[0].x - imagePoints[1].x);
 long double height = fabs(imagePoints[0].y - imagePoints[2].y);


/*
 if(z>96 && z<104)                  //if the marker is at isocenter (100cm) the marker should be square 120 (120/4 = 30mm)
 {
   if(width < 112 || width >128)    //if the width isn't 28mm - 32mm return with error
   {
     fadrs.error = -5;
     return fadrs;
   }
   if(height < 112 || width >128)    //if the height isn't 28mm - 32mm return with error
   {
     fadrs.error = -6;
     return fadrs;
   }

 }
*/

 //x and y travel is the distance between the top left marker point on the first live marker found and the same point on the reference marker
 long double x = (imagePoints[0].x - referencePoints[0].x)/4;
 long double y = (referencePoints[0].y - imagePoints[0].y)/4;


 if(width > 112 && width < 128 && height > 112 && height < 128)
 {
   fadrs.disp.x = x;
   fadrs.disp.y = y;
 }
 else
 {
   fadrs.disp.x = 0;
   fadrs.disp.y = 0;
 }



 t_boardPose boardPose;

 //find the pose of the current marker board.  The pose is of a marker board that consists of aruco markers #3 and #4
 boardPose = findBoardPose(&imageRaw);
 //subtract the current pose from the reference image pose.  The total vector length is the vertical distance in mm.
 double z = boardPose.translationLength - referenceStruct.boardPose.translationLength;

 fadrs.disp.z = z;

 fadrs.imageDisplay = aprs.imageFlat;
 fadrs.error = 0;
 return fadrs;

}





/*!Parameters:
 * imageCrosshair (the ROI Mat),
 * angle (the angle to rotate the image for processing (should always be 0 or +/-90 deg)
 * Return: The center point of the crosshair and the points of the crosshair edge on both sides
 * If a crosshair wasn't found, a three points of value (28,28) is returned.
 * In this case, the "crosshair" represents the center of the field.
 *
 */
std::vector<cv::Point2d> FieldDetect::processCrosshairMat(cv::Mat imageCrosshair, double angle)
{
  std::vector<cv::Point2d> crosshairCenters;
  std::vector<double> rowVect;
  cv::Mat imageSmoothed;
  cv::Mat imageProcess;
  cv::Mat imageEdges;

  //cv::imshow("raw",imageCrosshair);    //diagnostic
  //cv::waitKey(0);                      //diagnostic

  //smooth the image
  cv::GaussianBlur(imageCrosshair,imageSmoothed,cv::Size( 5, 5 ), 0, 0 );

  //cv::imshow("blurred",imageSmoothed); //diagnostic
  //cv::waitKey(0);                      //diagnostic

  //find the center of the ROI to use as the center of rotation
  cv::Point matCenter = cv::Point(imageSmoothed.cols/2, imageSmoothed.rows/2);
  cv::Mat rotMat = cv::getRotationMatrix2D(matCenter, angle, 1);
  //rotate it because processing is row oriented
  cv::warpAffine(imageSmoothed, imageProcess, rotMat, imageSmoothed.size(), cv::INTER_CUBIC);

  //cv::imshow("process",imageProcess);   //diagnostic
  //cv::waitKey(0);                       //diagnostic

  //Canny(imageProcess, imageEdges, 5, 5*2.5f, 3);

  //find the edges
  imageEdges = *cannyThreshold(&imageProcess);

  //cv::imshow("edges",imageEdges);   //diagnostic
  //cv::waitKey(0);                       //diagnostic

  //make a vectors of the index of the rows that have a part of the crosshair
  int i=0,j=0;
  std::vector<std::vector<double>> chData;
  std::vector<double> row;
  //average each row of the roi mat and put each row average in avgRowVect

  row.clear();
  for (i = 0; i < imageEdges.rows; ++i)
  {
      //uchar* row_ptr = processMat.ptr<uchar>(i);                //diagnostic
      for (j = 0; j < imageEdges.cols; ++j)
      {
          uchar v = imageEdges.at<uchar>(i,j);
          if(v>0) rowVect.push_back(i);
      }
          if(rowVect.size()>0) chData.push_back(rowVect);
          rowVect.clear();
  }


  //now we have a vector that contains an index to the ROI row that has the crosshair in it.
  //the first element is the starting row of the crosshair and the last element is the ending row

  if(chData.size() == 0)
  {
    crosshairCenters.push_back(cv::Point2d(28,28));
    crosshairCenters.push_back(cv::Point2d(28,28));
    crosshairCenters.push_back(cv::Point2d(28,28));
    return crosshairCenters;
  };

  double edge1 = chData[0][0];
  double edge2 = chData[chData.size()-1][0];
  double chCenter = edge1+(edge2-edge1)/2.0;

  //swap axes if it had to be rotated
  if(angle==0)
  {
    crosshairCenters.push_back(cv::Point2d(chSquare/2, edge1));
    crosshairCenters.push_back(cv::Point2d(chSquare/2, chCenter));
    crosshairCenters.push_back(cv::Point2d(chSquare/2, edge2));
  }
    else

  {
    crosshairCenters.push_back(cv::Point2d(edge1,    chSquare/2));
    crosshairCenters.push_back(cv::Point2d(chCenter, chSquare/2));
    crosshairCenters.push_back(cv::Point2d(edge2,    chSquare/2));
  }

  return crosshairCenters;
}


/*!Finds the 50% penumbra point of the field edge of roi.
*
*roi is lfSquare x lfSquare pixels and is symmetric about a point on the field edge.
*Since the roi is processed by row, it is necessary to rotate it by -90 degrees for two of the sides (angle paramerer).
*If it was rotated, the x and y axes are swapped.
*First, a gaussian filter is applied to the roi mat.
*Finds the median of each row (filtering) and then the max gradient of those medians.  This is the 50% penumbra point.
*Returns the point in the roi that location was found.  The returned point can be added to the origin of the roiRect to get the actual field edge.
*/
std::vector<cv::Point2d> FieldDetect::rotateAndFindMedian(cv::Mat roi, double angle, int index)
{
    //average each row of the ROI and put the  averages into a vector of doubles
    std::vector<double> avgRowVect;
    double avg = 0;
    double acc = 0;
    int i=0,j=0;
    cv::Mat processMat;
    cv::Mat roiTransposed;
    cv::Mat imageSmoothed;

    //cv::imshow("",roi);                  //diagnostic
    //cv::waitKey(0);                      //diagnostic

    cv::GaussianBlur(roi,imageSmoothed,cv::Size( 5, 5 ), 0, 0 );

    //cv::imshow("",imageSmoothed);        //diagnostic
    //cv::waitKey(0);                      //diagnostic

    cv::Point matCenter = cv::Point(imageSmoothed.cols/2, roi.rows/2);
    cv::Mat rotMat = cv::getRotationMatrix2D(matCenter, angle, 1);
    cv::warpAffine(imageSmoothed, processMat, rotMat, processMat.size(), cv::INTER_CUBIC);

    //cv::imshow("",processMat);         //diagnostic
    //cv::waitKey(0);                    //diagnostic


    avgRowVect.clear();

    /*
    //Save 8 roi's for the edges and 4 for the crosshairs in csv format for testing in Excel
    std::string filename, indexStr;                             //diagnostic
    filename = "roi_data";                                      //diagnostic
    indexStr = std::to_string(index);                           //diagnostic
    writeCSV(filename + "_" + indexStr + ".csv", processMat);   //diagnostic
    */

    //average each row of the roi mat and put each row average in avgRowVect
    for (i = 0; i < processMat.rows; ++i)
    {
        //uchar* row_ptr = processMat.ptr<uchar>(i);                //diagnostic
        avg=0;
        acc=0;
        for (j = 0; j < processMat.cols; ++j)
        {
            uchar v = processMat.at<uchar>(i,j);
            acc += v;
        }
            avg=acc/j;
            avgRowVect.push_back(avg);
    }


    //calculate the gradients of the values of the column
    //(y2-y1)/(x2-x1)
    std::vector<double> grad;
    for(size_t i=0; i<avgRowVect.size()-1; i++)
    {

      grad.push_back((avgRowVect[i+1]-avgRowVect[i])/(i+1-i));

    }

    //pair the gradient with the row index
    //we need these pairs for the sort
    std::vector<std::pair<double,int>> gradWithIndex;

    for(size_t i=0; i<grad.size(); i++)
    {
      gradWithIndex.push_back(std::make_pair(abs(grad[i]),i));

    }

    //sort the gradient-index pair in ascending order
    //the last index position of gradWithIndex has an index to the 50% penumbra point
    std::sort(gradWithIndex.begin(),gradWithIndex.end());

   //the middle of the roi is chosen as the x value
   //the y value with the highest gradient is the 50% penumbra point of the ROI

    cv::Point2d ptEdge;

    //if the mat was rotated for processing, swap the x and y axes
    if(angle == 0)
    {
      ptEdge = cv::Point2d(lfSquare/2, gradWithIndex[gradWithIndex.size()-1].second);
    }
      else
    {
      ptEdge = cv::Point2d(gradWithIndex[gradWithIndex.size()-1].second,lfSquare/2);
    }

    std::vector<cv::Point2d> retPts;

    retPts.push_back(ptEdge);
    return retPts;
}



/*!Calculates two ROI points at (percentFromEnd) percent of the field length from each end of the field light side
*
* Parameters: Two points that define a line that forms a side of the field light square.
* Return: top ROI point x, top ROI point y, bottom ROI point x, bottom ROI point y
*/
std::vector<cv::Point2d> FieldDetect::calcROIPoints(cv::Point2d p1, cv::Point2d p2, double percentFromEnd)
{
  std::vector<cv::Point2d> points;

  //make the vector
  double Vp1p2X = p2.x-p1.x;
  double Vp1p2Y = p2.y-p1.y;

  //length of vector
  double LV = sqrt((Vp1p2X*Vp1p2X)+(Vp1p2Y*Vp1p2Y));

  //unit vector
  double UVp1p2X = Vp1p2X/LV;
  double UVp1p2Y = Vp1p2Y/LV;

  //target vector length is 10% of the length of the field side
  double TVx = UVp1p2X*(LV*percentFromEnd);
  double TVy = UVp1p2Y*(LV*percentFromEnd);

  //add the vector to p1.  The new length is the center point of the ROI
  //i.e. a field size of 100mm has two points. One at 10mm down from the top and one at 10mm up from the bottom
  double topROIx = p1.x+TVx;
  double topROIy = p1.y+TVy;

  double botROIx = p2.x-TVx;
  double botROIy = p2.y-TVy;


  cv::Point2d topROIPt(topROIx,topROIy);
  cv::Point2d botROIPt(botROIx,botROIy);

  points.push_back(topROIPt);
  points.push_back(botROIPt);

  return  points;

}



/*!Finds the four corner points of the detected light field.
 * If the light field is >35cm the field is octagonal.  In this case, the points returned are of the complete square
 * without the corners clipped.
*/
FieldDetect::t_polyReturnStruct FieldDetect::findLightField(cv::Mat *imageIn)
{

  //cv::imshow("findLightField() in", *imageIn);    //diagnostic
  //cv::waitKey(0);             //diagnostic
  qDebug()<<"enter findLightField()";

  cv::Mat *imageProcess;
  std::vector<std::vector<cv::Point>> contours;
  std::vector<cv::Vec4i> hierarchy;
  std::vector<std::vector<cv::Point>> polygon;
  t_polyReturnStruct polygonRetStruct;

  qDebug()<<"enter findLightField() cannyThreshold()";



  //TODO ---> make bi-modal image for thresholding


   cv::Mat imagePreProcessed(imageIn->rows, imageIn->cols, CV_8UC1);




   imageIn->copyTo(imagePreProcessed);




   //imageProcess = cannyThreshold(&imagePreProcessed);
   imageProcess = cannyThreshold(&imagePreProcessed);

   //cv::imshow("Edges", *imageProcess);                                                           //diagnostic
   //cv::waitKey(0);                                                                               //diagnostic

   cv::findContours(*imageProcess, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);  //retrieval modes -> cv::RETR_EXTERNAL cv::RETR_CCOMP

/*
  //show contour
  cv::Mat imageContours = cv::Mat::zeros(imageIn->rows, imageIn->cols, CV_8UC1);                //diagnostic
  cv::drawContours(imageContours, contours, -1, cv::Scalar(255,255,255), 1, 8, hierarchy, 2);   //diagnostic
  cv::imshow("Edges", imageContours);                                                           //diagnostic
  cv::waitKey(0);                                                                               //diagnostic
*/

  for( size_t i = 0; i < contours.size(); i++ )
  {

      std::vector<cv::Point> approx;
      double cosine;
      double maxCosine = 0;

      cv::approxPolyDP(contours[i], approx, arcLength(contours[i], true)*.02, true);

      //double area = contourArea(approx);                          //Diagnostic
      //double arcLen = cv::arcLength(contours[i], true)*.02;       //Diagnostic

      if(approx.size() == 8 && fabs(contourArea(approx)) > 2e+6 && isContourConvex(approx))
      {

        //sortPoints(contours[i]);    //use this to find corner points???

        for(int i=0; i<6; i++)
        {
          cosine = fabs(angle(approx[i+2], approx[i], approx[i+1]));
          maxCosine = MAX(maxCosine, cosine);
        }
        if(maxCosine > 0.60 && maxCosine < 0.80)
        {
          polygon.push_back(approx);
          qDebug()<<"findLightField() found octogon";
        }
        else
        {
          polygonRetStruct.validField = false;
          qDebug()<<"findLightField() the polygon sides were not in the range of cos(45)";
          return polygonRetStruct;
        }

        //cv::imshow("test", imageIn);                                              //diagnostic
        //cv::waitKey(0);                                                           //diagnostic
        //sort the points of the octogon
        std::vector<cv::Point> sortedPoints = sortPointsAngle(polygon[0]);


        //There are the eight points of the light field for a field size larger than 35cm because
        //the corners are clipped (it's an octogon).
        //We're returning four points that are a square (not octagonal) without the corners clipped
        //p0 is the top right corner wrt Aruco marker #1.  The order is ccw.
        //See the comments for the sortPoints() function below for point order

        cv::Point2d p0,p1,p2,p3;
        p0.x = sortedPoints[1].x;
        p0.y = sortedPoints[0].y;
        p1.x = sortedPoints[5].x;
        p1.y = sortedPoints[2].y;
        p2.x = sortedPoints[7].x;
        p2.y = sortedPoints[6].y;
        p3.x = sortedPoints[3].x;
        p3.y = sortedPoints[4].y;

        polygonRetStruct.pts.push_back(p0);
        polygonRetStruct.pts.push_back(p1);
        polygonRetStruct.pts.push_back(p2);
        polygonRetStruct.pts.push_back(p3);


      }

      //this is a square
      //the smallest square we're measuring is 5cmx5cm. The scale is 1/4mm, so the min area should be (50mmx4)squared = 40000
      //we'll call it 38000
      if( approx.size() == 4 && fabs(contourArea(approx)) > 38000 && isContourConvex(approx) )
      {
        //double area = fabs(contourArea(approx));    //diagnostic
        maxCosine = 0;
        for( int j = 2; j < 5; j++ )
        {
          cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
          maxCosine = MAX(maxCosine, cosine);
        }
        if( maxCosine < 0.3 )
        {
          polygon.push_back(approx);
          qDebug()<<"findLightField() found square";
        }
        else
        {
          polygonRetStruct.validField = false;
          qDebug()<<"findLightField() the polygon sides were not in the range of cos(90)";
          return polygonRetStruct;
        }

/*
        for( size_t i = 0; i < polygon.size(); i++ )                                      //diagnostic
        {
          const cv::Point* p = &polygon[i][0];                                            //diagnostic
          int n = (int)polygon[i].size();                                                 //diagnostic
          polylines(imageIn, &p, &n, 1, true, cv::Scalar(0,255,0), 1, cv::LINE_AA);   //diagnostic
        }
*/

        //cv::imshow("test", imageIn);                                                    //diagnostic
        //cv::waitKey(0);                                                                 //diagnostic

        //these are the four points of the square
        for(int i=0; i<4; i++)
          polygonRetStruct.pts.push_back(polygon[0][i]);


        //the points must be returned in the order of tr, tl, bl, br
        //so, we creat an upright bounding rectangle and take the points from it
        cv::Rect rect;
        rect = cv::boundingRect(polygon[0]);
        polygonRetStruct.pts[0] = cv::Point(rect.x+rect.width, rect.y); //top right
        polygonRetStruct.pts[1] = rect.tl();                            //top left
        polygonRetStruct.pts[2] = cv::Point(rect.x, rect.y+rect.height);//bottom left
        polygonRetStruct.pts[3] = rect.br();                            //bottom right


        //sortPoints(contours[i], polygonRetStruct.pts);
/*
        //The points returned from approxPolyDP don't necessarily have the top left point as the origin,
        //so we sort the corner points to ensure it is.

        std::vector<cv::Point> sortedPoints = sortPointsAngle(polygon[0]);

        polygonRetStruct.pts[0] = sortedPoints[0];
        polygonRetStruct.pts[1] = sortedPoints[1];
        polygonRetStruct.pts[2] = sortedPoints[3];
        polygonRetStruct.pts[3] = sortedPoints[2];


        polygonRetStruct.pts[0] = polygon[0][2];
        polygonRetStruct.pts[1] = polygon[0][1];
        polygonRetStruct.pts[2] = polygon[0][0];
        polygonRetStruct.pts[3] = polygon[0][3];
*/

      }
  }

  //return has the invalid flag set if four or eight points were not found
  if(polygonRetStruct.pts.size() == 4) polygonRetStruct.validField = true;
  else polygonRetStruct.validField = false;
  qDebug()<<"findLightField() polygonReturnStruct.pts.size() "<<polygonRetStruct.pts.size();
  return polygonRetStruct;

}

/*!Applies warpPerspective() transform to the input undistorted Mat and returns a pointer to the transformed Mat
 *
 * Set the calcTransform flag to true if you want to calculate the transform
 * from the markers or false if a transform already exists and you just want to transform the image.
 * Four markers must be found to find the transform, so this function would have first been called for a reference image.
 *
 * The input Mat assumes that there are four 30mm 6x6 aruco markers.  The markers are at each corner of the inside perimiter of a 190mm
 * square grid.  The four corners of each marker make up 16 points that are input to the perspective transform.  The aruco markers
 * are in a clockwise pattern with the origin at the upper left of the 190mm square.  Aruco 6x6 #1 is the origin marker.
 *
 */
FieldDetect::t_apRetStruct *FieldDetect::arucoPerspective(cv::Mat *img, bool calcTransform)
{
  //cv::imshow("before perspective transform", *img);      //diagnostic
  //cv::waitKey(0);           //diagnostic

  static t_apRetStruct apRetStruct;

  params.cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
  cv::aruco::ArucoDetector detector(dictionary, params);
  detector.detectMarkers(*img, apRetStruct.arucoCorners, apRetStruct.arucoIds);

  std::vector<int> markerNum;
  std::vector<cv::Point2d> imagePoints;
  std::vector<cv::Point3d> referencePoints;

  const double s = 4;       //scale 4 = 0.25mm/pixel
  const double fw = 500;    //field width
  const double aw = 180;    //aruco marker grid width
  const double mw = 30;     //aruco marker width

  const double FW = fw*s;
  const double AW = aw*s;
  const double MW = mw*s;

  imagePoints.clear();
  referencePoints.clear();

  //Make a pair with the marker id to use as an index and sort it in marker order.
  //This is important because the marker numbers are not necessaily in order in ids,
  //but they have to be to be in order to match the corner order to the world points order
  //for the findHomography function
  std::vector<std::pair<int, int>> markerIDCornerIndex;
  for(uint i=0; i<apRetStruct.arucoCorners.size(); i++)
    markerIDCornerIndex.push_back(std::make_pair(apRetStruct.arucoIds.at(i),i));
  std::sort(markerIDCornerIndex.begin(), markerIDCornerIndex.end());

  if(apRetStruct.arucoIds.size() ==4)
  {

    //These are the image points of the four corners of the four markers.
    //The order is, top left is marker 1, top right is marker 2, bottom right is marker 3, bottom left is marker 4
    //The order of the points on the marker are top left is corner one.  2-4 are clockwise from 1.
    //Corners is a vector of points so there are two indices. They are markerIDCornerIndex[i].second] and [0]
    for(uint i=0; i<markerIDCornerIndex.size(); i++)
    {
      imagePoints.push_back(apRetStruct.arucoCorners[markerIDCornerIndex[i].second][0]); //see comment above about indices
      imagePoints.push_back(apRetStruct.arucoCorners[markerIDCornerIndex[i].second][1]);
      imagePoints.push_back(apRetStruct.arucoCorners[markerIDCornerIndex[i].second][2]);
      imagePoints.push_back(apRetStruct.arucoCorners[markerIDCornerIndex[i].second][3]);

    }

     //the world points in the same order as the aruco marker points
     //top left marker top left corner is 0, top left marker top right corner is 1
     //the marker order is clockwise
     referencePoints.push_back(cv::Point3d( ((FW-AW)/2),       ((FW-AW)/2),             0.0f)); //TL
     referencePoints.push_back(cv::Point3d( ((FW-AW)/2)+MW,    ((FW-AW)/2),             0.0f)); //TL
     referencePoints.push_back(cv::Point3d( ((FW-AW)/2)+MW,    ((FW-AW)/2)+MW,          0.0f)); //TL
     referencePoints.push_back(cv::Point3d( ((FW-AW)/2),       ((FW-AW)/2)+MW,          0.0f)); //TL

     referencePoints.push_back(cv::Point3d( FW-((FW-AW)/2)-MW, ((FW-AW)/2),             0.0f)); //TR
     referencePoints.push_back(cv::Point3d( FW-((FW-AW)/2),    ((FW-AW)/2),             0.0f)); //TR
     referencePoints.push_back(cv::Point3d( FW-((FW-AW)/2),    ((FW-AW)/2)+MW,          0.0f)); //TR
     referencePoints.push_back(cv::Point3d( FW-((FW-AW)/2)-MW, ((FW-AW)/2)+MW,          0.0f)); //TR

     referencePoints.push_back(cv::Point3d( FW-((FW-AW)/2)-MW, FW-((FW-AW)/2)-MW,       0.0f)); //BR
     referencePoints.push_back(cv::Point3d( FW-((FW-AW)/2),    FW-((FW-AW)/2)-MW,       0.0f)); //BR
     referencePoints.push_back(cv::Point3d( FW-((FW-AW)/2),    FW-((FW-AW)/2),          0.0f)); //BR
     referencePoints.push_back(cv::Point3d( FW-((FW-AW)/2)-MW, FW-((FW-AW)/2),          0.0f)); //BR

     referencePoints.push_back(cv::Point3d( ((FW-AW)/2),       FW-((FW-AW)/2)-MW,       0.0f)); //BL
     referencePoints.push_back(cv::Point3d( ((FW-AW)/2)+MW,    FW-((FW-AW)/2)-MW,       0.0f)); //BL
     referencePoints.push_back(cv::Point3d( ((FW-AW)/2)+MW,    FW-((FW-AW)/2),          0.0f)); //BL
     referencePoints.push_back(cv::Point3d( ((FW-AW)/2),       FW-((FW-AW)/2),          0.0f)); //BL
  }

  cv::Mat imageOut;
  cv::Mat imageCropped;


  //findHomography needs all four markers
  //H is declared in the class and must be set by calling this function once.
  //The calling function must set the class variable transformSet to true.
  if(apRetStruct.arucoIds.size() == 4 && calcTransform)
      referenceStruct.H = findHomography(imagePoints, referencePoints);

  /*
  //write transform matrix file - diagnostic
  std::string filename;                             //diagnostic
  filename = "transform";                           //diagnostic
  writeCSV(filename + ".csv", H);                   //diagnostic
  */

  if(!referenceStruct.H.empty() && !img->empty())
  {

    double d = cv::determinant(referenceStruct.H);

    qDebug()<<"arucoPerspective() determinant " << d;
    qDebug()<<"arucoPerspective() entering warpPerspective ";

    cv::warpPerspective(*img, imageOut, referenceStruct.H, imageOut.size());
  }
  else
  {
    apRetStruct.error = -1;
    return &apRetStruct;
  }

  qDebug()<<"arucoPerspective() leaving warpPerspective ";

  //cv::imshow("before median filtered", imageOut);     //diagnostic
  //cv::waitKey(0);                                     //diagnostic

  //Low pass filtering the image before edge detection helps the approxPolyDP() algorithm find the right corner points.
  //The corner points being at the wrong spot negatively impacts finding the field width because the point on the other side
  //of the field may be at different y point, for example, we would be measuring at a diagonal instead of across the field.
  //It also gives a cleaner line for angle detection.

  //cv::Mat imageMedianBlurred;
  //cv::medianBlur(imageOut, imageMedianBlurred, 11);

  qDebug()<<"arucoPerspective() entering crop";

  //cv::Rect ROI(0, 0, FW, FW);
  cv::Rect ROI(0, 0, 2000, 2000);
  //imageCropped = imageMedianBlurred(ROI);

  imageCropped = imageOut(ROI);

  //cv::rectangle (imageCropped, ROI, cv::Scalar(255,255,255));

  qDebug()<<"arucoPerspective() leaving crop";
  //cv::imshow("arucoPerspective() output", imageCropped);  //diagnostic
  //cv::waitKey(0);                                         //diagnostic

  apRetStruct.imageFlat = imageCropped;
  apRetStruct.error = false;

  return &apRetStruct;

}



/*!
 * \param imageFlat is a transformed (flat) field image
 * \return std::vector<cv:point2d> of the four outer marker points
 * \n This function is used to find the angle of the field.
 */
std::vector<cv::Point2d> FieldDetect::findMarkerPoints(cv::Mat *imageFlat)
{

  //cv::imshow("imageFlat in", *imageFlat);         //diagnostic
  //cv::waitKey(0);                                 //diagnostic

  std::vector<std::vector<cv::Point2f>> corners;
  std::vector<int> ids;

  std::vector<cv::Point2d> mkPoints;

  params.cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
  cv::aruco::ArucoDetector detector(dictionary, params);
  detector.detectMarkers(*imageFlat, corners, ids);
  qDebug()<<"findMarkerPoints corners size " << corners.size();

  //Make a pair with the marker id to use as an index and sort it in marker order.
  //This is important because the marker numbers are not necessaily in order in ids,
  //but they have to be to be in order to match the corner order to the world points order
  //for the findHomography function
  std::vector<std::pair<int, int>> markerIDCornerIndex;
  for(uint i=0; i<corners.size(); i++)
    markerIDCornerIndex.push_back(std::make_pair(ids.at(i),i));
  std::sort(markerIDCornerIndex.begin(), markerIDCornerIndex.end());



  if(ids.size() ==4)
  {

    //These are the image points of the four corners of the four markers.
    //The order is, top left is marker 1, top right is marker 2, bottom right is marker 3, bottom left is marker 4
    //The order of the points on the marker are top left is corner one.  2-4 are clockwise from 1.
    //Corners is a vector of points so there are two indices. They are markerIDCornerIndex[i].second] and [0]
    for(uint i=0; i<markerIDCornerIndex.size(); i++)
    {
      mkPoints.push_back(corners[markerIDCornerIndex[i].second][0]); //see comment above about indices
      mkPoints.push_back(corners[markerIDCornerIndex[i].second][1]);
      mkPoints.push_back(corners[markerIDCornerIndex[i].second][2]);
      mkPoints.push_back(corners[markerIDCornerIndex[i].second][3]);

    }

  }



  return mkPoints;
}

/*!Finds edges of the image using these steps:
 *
 * 1. The image is pyramid filtered
 * 2. Canny edge detection
 * 3. dilated
 *
*/
cv::Mat* FieldDetect::cannyThreshold(cv::Mat *imageRaw)
{
  cv::Mat imageGray;
  cv::Mat imagePyr;
  cv::Mat imageTemp;
  cv::Mat imageCanny;
  static cv::Mat imageEdge;
  //int threshold = 8;    //8
  //int kernel_size = 3;  //3

  //cv::imshow("canny input raw", *imageRaw);
  //cv::waitKey(0);

  qDebug()<<"enter cannyThreshold()";
  imageRaw->copyTo(imageGray);


  pyrDown(imageGray, imagePyr, cv::Size(imageGray.cols/2, imageGray.rows/2));
  pyrUp(imagePyr, imageTemp, imageGray.size());

  //Canny(imageTemp, imageCanny, threshold, threshold*2.5f, kernel_size);
  double threshOtsu = cv::threshold(imageTemp, imageCanny, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
  //qDebug()<<"cannyThreshold() after pyr and Canny()";
  qDebug() << "Otsu Threshold() ---------------------------------------------> " << threshOtsu;
  int dilation_size = 5;
  //cv::MORPH_RECT cv::MORPH_CROSS cv::MORPH_ELLIPSE

  //set dilation parameters
  cv::Mat element = getStructuringElement(cv::MORPH_RECT,
                                           cv::Size(2*dilation_size + 1, 2*dilation_size+1),
                                           cv::Point(dilation_size, dilation_size));

  cv::dilate(imageCanny, imageEdge, element);

  //cv::imshow("otsu", imageCanny);
  //cv::waitKey(0);


  return &imageEdge;
}



//--------------------------------------------------------------------------------------------------------------------------
//Finds the cosine of the angle between pt0->pt1 and pt0->pt2
//--------------------------------------------------------------------------------------------------------------------------
double FieldDetect::angle( cv::Point pt1, cv::Point pt2, cv::Point pt0 )
{
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}



//--------------------------------------------------------------------------------------------------------------------------
//Rotates pt around the point by the angle
//input, output angles in radians
// x' = xcos(alpha) - ysin(alpha)
// y' = xsin(alpha) + ycos(alpha)
//--------------------------------------------------------------------------------------------------------------------------
cv::Point2d FieldDetect::rotatePoint(cv::Point2d pt, cv::Point2d around, double angle)
{
    double xNegTrans = pt.x - around.x;
    double yNegTrans = pt.y - around.y;
    double xTrans = xNegTrans*cos(angle)-yNegTrans*sin(angle);
    double yTrans = yNegTrans*cos(angle)+xNegTrans*sin(angle);
    double xPosTrans = xTrans + around.x;
    double yPosTrans = yTrans + around.y;

    cv::Point2d T(xPosTrans,yPosTrans);

    return T;
}



//--------------------------------------------------------------------------------------------------------------------------
//Sort the points of the square or octogon - see below for the order of the returned points
//This is necessary because cv::approxPolyDP does not necessarily return the points in the same order.
//
//--------------------------------------------------------------------------------------------------------------------------
std::vector<cv::Point> FieldDetect::sortPointsAngle(std::vector<cv::Point> points)
{


  //find the center point by getting the bounding rectancle of the points
  cv::Rect bb;
  cv::Point center;
  std::vector<std::pair<double, int>> sortedPointPair;
  std::vector<cv::Point> sortedPoints;

  bb = cv::boundingRect(points);
  center.x = (bb.width/2)+bb.tl().x;
  center.y = (bb.height/2)+bb.tl().y;

  //find the angle of each point wrt the center point
  for(size_t i=0; i<points.size();i++)
  {

    double dx = (center.x * points[i].x);
    double dy = abs(center.y * points[i].y);
    double angle = atan2(dy,dx);

    //sort the points - the order for the octogon, with top right vertex being point 1 and the point order ccw is, 0,2,5,7,6,4,3,1
    //the order for the square with top right vertex being point 1 and the points numbered ccw is, 2,3,1,0.

    sortedPointPair.push_back(std::make_pair(angle*(180/CV_PI), i));
  }

    std::sort(sortedPointPair.begin(), sortedPointPair.end());

    for(size_t i=0; i<sortedPointPair.size(); i++)
    {

      sortedPoints.push_back(points.at(sortedPointPair[i].second));

    }


  return sortedPoints ;

}

//--------------------------------------------------------------------------------------------------------------------------
//
//
//
//--------------------------------------------------------------------------------------------------------------------------
void FieldDetect::writeCSV(std::string filename, cv::Mat m)
{
  std::ofstream myfile;
  myfile.open(filename.c_str());
  myfile<< cv::format(m, cv::Formatter::FMT_CSV) << std::endl;
  myfile.close();
}


//--------------------------------------------------------------------------------------------------------------------------
//Returns the intersection point of two lines, (A1,B1) (C1,D1)
//
//
//--------------------------------------------------------------------------------------------------------------------------
//#define pdd std::pair<double, double>
cv::Point2d FieldDetect::lineLineIntersection(cv::Point2d A1, cv::Point2d B1, cv::Point2d C1, cv::Point2d D1)
{

    cv::Point2d ptIntersection;
    pdd A, B, C, D;

    A.first =  A1.x;
    A.second = A1.y;
    B.first =  B1.x;
    B.second = B1.y;
    C.first =  C1.x;
    C.second = C1.y;
    D.first =  D1.x;
    D.second = D1.y;



    // Line AB represented as a1x + b1y = c1
    double a1 = B.second - A.second;
    double b1 = A.first - B.first;
    double c1 = a1*(A.first) + b1*(A.second);

    // Line CD represented as a2x + b2y = c2
    double a2 = D.second - C.second;
    double b2 = C.first - D.first;
    double c2 = a2*(C.first)+ b2*(C.second);

    double determinant = a1*b2 - a2*b1;

    if (determinant == 0)
    {
        ptIntersection.x = FLT_MAX;
        ptIntersection.y = FLT_MAX;
        return ptIntersection;
    }
    else
    {
        double x = (b2*c1 - b1*c2)/determinant;
        double y = (a1*c2 - a2*c1)/determinant;
        ptIntersection.x = x;
        ptIntersection.y = y;
        return ptIntersection;
    }
}


//--------------------------------------------------------------------------------------------------------------------------
//
//
//
//--------------------------------------------------------------------------------------------------------------------------
std::vector<cv::Point> FieldDetect::sortPoints(std::vector<cv::Point> points, std::vector<cv::Point> corners)
{

    std::vector<cv::Point> xSorted;
    std::vector<cv::Point> YSorted;

    struct SortY {
        bool operator() (cv::Point pt1, cv::Point pt2) { return (pt1.y < pt2.y);}
    } sortY;
    struct SortX {
        bool operator() (cv::Point pt1, cv::Point pt2) { return (pt1.x < pt2.x);}
    } sortX;

        std::sort(points.begin(),points.end(),sortX);
  return  points;
}




/*!
 * Initializes the attached Point Grey camera, loads the camera specific calibration file and loads the aruco parameters file
 * \param calFileName
 * \return error code that indicates the results
 * \n error code  0 = The camera was successfully initialized and the camera cal file and aruco parameter files were successfully loaded
 * \n error code -1 = Camera initialization failed
 * \n error code -2 = The cal file could not be loaded
 * \n error code -3 = The aruco parameter file could not be loaded. << removed - no longer used
 * \n error code -4 = The config file could not be loaded.
 * \n sets cameraCalLoaded true if the camera calibration was loaded from file.
 * \n sets arucoParametersLoaded if the parameters were loaded from file.
 * \n The camera calibration contains the camera and distortion matrices and are private

*/
int FieldDetect::initializeCamera()
{

  int err = 0;
  referenceStruct.cameraInitialized = false;
  referenceStruct.cameraCalLoaded = false;

  if(spin.cameraMatrix.empty() || spin.distortionCoefficients.empty())
  {
    referenceStruct.cameraCalLoaded = false;
    return -2;
  }
  else referenceStruct.cameraCalLoaded = true;


  //comment out the two below lines for diagnostics
  err = spin.initPGRCamera();
  if(err<0)return -1;

  referenceStruct.cameraInitialized = true;
  referenceStruct.cameraCalLoaded = true;

  return 0;
}




/*!
 * \brief FieldDetect::setCameraFrameRate
 * \param frameRate
 * \return
 */
int FieldDetect::setCameraFrameRate(int frameRate)
{
  int fr = spin.setPGRCameraThroughputLimit(frameRate); //25120000 = 5 fps
  return fr;
}



/*!
 * Gets an image from the camera and applies the camera calibration values to the image.
 * Camera calibration has to be loaded (cameraCalLoaded flag set true by initializeCamera function)
 * \return cv::Mat undistorted image
 * \n The user application should check to make sure an image was captured with myMat.empty()
 */
cv::Mat* FieldDetect::getImage()
{

  SpinInterface::acquireImageReturnStruct* AIRS;
  static cv::Mat imageUndistorted;
  cv::Mat newCameraMatrix;

  AIRS = spin.acquireImage();

  if(!AIRS->error && referenceStruct.cameraCalLoaded)
  {
    //cv::medianBlur(AIRS->imageRaw, AIRS->imageRaw, 3);
    newCameraMatrix = cv::getOptimalNewCameraMatrix(spin.cameraMatrix, spin.distortionCoefficients,AIRS->imageRaw.size(), 1, AIRS->imageRaw.size());
    cv::undistort(AIRS->imageRaw, imageUndistorted, spin.cameraMatrix, spin.distortionCoefficients, newCameraMatrix);
    return &imageUndistorted;
  }
  else return &imageUndistorted;
}



/*!
 * Overloaded function
 * Gets an image from a file and applies the camera calibration values to the image.
 * Camera calibration has to be loaded (cameraCalLoaded flag set true by initializeCamera function)
 * \return cv::Mat undistorted image
 * \n The user application should check to make sure an image was captured with myMat.empty()
 */
cv::Mat* FieldDetect::getImage(std::string imageFileName)

{
  cv::Mat *imageRaw;
  static cv::Mat imageUndistorted;
  cv::Mat newCameraMatrix;

  imageRaw = spin.acquireImage(imageFileName);
  if(!imageRaw->empty() && referenceStruct.cameraCalLoaded)
  {
    cv::medianBlur(*imageRaw, *imageRaw, 3);
    newCameraMatrix = cv::getOptimalNewCameraMatrix(spin.cameraMatrix, spin.distortionCoefficients,imageRaw->size(), 1, imageRaw->size());
    cv::undistort(*imageRaw, imageUndistorted, spin.cameraMatrix, spin.distortionCoefficients, newCameraMatrix);
    return &imageUndistorted;
  }
  else
  {
    imageUndistorted.release();
    return &imageUndistorted;
  }
}

/*!
 * Overloaded function
 * Applies the camera calibration values to the raw image supplied.
 * Camera calibration has to be loaded (cameraCalLoaded flag set true by initializeCamera function)
 * \return cv::Mat undistorted image
 * \n The user application should check to make sure an image was captured with myMat.empty()
 */
cv::Mat* FieldDetect::getImage(cv::Mat* image)

{
  static cv::Mat imageUndistorted;
  cv::Mat newCameraMatrix;

  if(!image->empty() && referenceStruct.cameraCalLoaded)
  {
    cv::medianBlur(*image, *image, 3);
    newCameraMatrix = cv::getOptimalNewCameraMatrix(spin.cameraMatrix, spin.distortionCoefficients,image->size(), 1, image->size());
    cv::undistort(*image, imageUndistorted, spin.cameraMatrix, spin.distortionCoefficients, newCameraMatrix);
    return &imageUndistorted;
  }

  else return &imageUndistorted;
}


/*!
 * Acquires a reference image from the camera that must contain four detectable aruco markers.  The transform matrix is calculated
 * from the marker corner points and stored in class member referenceStruct.H.  Class member referenceStruct.referenceSet is set to true;
 * Four outer corner points from the aruco markers are stored in class member markerPoints.
 * The center point of the aruco grid is stored in class member arucoCenterReference.
 * Crosshair points, two horizontal and two vertical are stored in crosshairVertPointsReference and crosshairHorzPointsReference.
 * The reference image is stored in referenceStruct.imageReferenceFlat.
 * \return integer error code
 * \n error code 0  = there were no errors.
 * \n error code -1 = the camera image was empty.
 * \n error code -2 = failed at arucoPerspective().
 * \n error code -3 = failed at findMarkerPoints().
 * \n error code -4 = failed at findLightFieldDimensions();
 * \n error code -5 = the camera was not initialized.
 * \n error code -6 = failed at findArucoDisplecement.
 */
int FieldDetect::acquireReferenceImage()
{

  //clear the reference structure vectors
  referenceStruct.referenceSet = false;
  referenceStruct.markerPoints.clear();
  referenceStruct.crosshairVertPointsReference.clear();
  referenceStruct.crosshairHorzPointsReference.clear();
  referenceStruct.imageReferenceFlat.release();


  if(referenceStruct.cameraInitialized)
  {
    cv::Mat* undistortedImg, undistortedImgCopy;
    APRetStrut aprs;
    FindLightFieldReturnStruct flfrs;

    //get undistorted from camera
    undistortedImg = getImage();
    undistortedImg->copyTo(undistortedImgCopy);

    cv::Mat imageCopy;
    undistortedImg->copyTo(imageCopy);

    if(undistortedImg->empty())
    {
      referenceStruct.referenceSet = false;
      return -1;
    }

    referenceStruct.imageReferenceUndistorted = *undistortedImg;

    qDebug()<<"at acquireReferenceImage() line before arucoPerspective";
    aprs = *arucoPerspective(undistortedImg, true);
    if(aprs.error <0)
    {
      referenceStruct.referenceSet = false;
      return -2;
    }
    qDebug()<<"acquireReferenceImage() arucoPerspective aprs.error "<<aprs.error;


    qDebug()<< "acquireReferenceImage() line before findMarkerPoints()";
    std::vector<cv::Point2d> mkpts = findMarkerPoints(&aprs.imageFlat);
    qDebug()<< "acquireReferenceImage() findMarkerPoints() mkpts size " << mkpts.size();
    if(mkpts.size()==0)
    {
      referenceStruct.referenceSet = false;
      return -3;
    }

    referenceStruct.imageReferenceFlat = aprs.imageFlat;

    for(size_t i=0; i<mkpts.size(); i++)
      referenceStruct.markerPoints.push_back(mkpts[i]);

    qDebug()<<"acquireReferenceImage() entering lineLineIntersection";

    //find the center of the aruco grid by finding the intersection of the diagonal lines
    referenceStruct.arucoCenterReference = lineLineIntersection(mkpts[0], mkpts[10], mkpts[5], mkpts[15]);

    cv::circle(aprs.imageFlat, mkpts[0], 3, cv::Scalar(255,255,255));
    cv::circle(aprs.imageFlat, mkpts[5], 3, cv::Scalar(255,255,255));
    cv::circle(aprs.imageFlat, mkpts[10], 3, cv::Scalar(255,255,255));
    cv::circle(aprs.imageFlat, mkpts[15], 3, cv::Scalar(255,255,255));

    qDebug()<<"acquireReferenceImage() leaving lineLineIntersection";


    qDebug()<< "acquireReferenceImage() line before findLightFieldDimensions()";
    //get the crosshair center and vert and horz points

    //cv:imshow("acquireRefImage", aprs.imageFlat);     //diagnostic
    //cv::waitKey(0);                                   //diganostic

    flfrs = findLightFieldDimensions(&undistortedImgCopy);
    if(flfrs.error < 0)
    {
      referenceStruct.referenceSet = false;
      return -4;
    }

    referenceStruct.crosshairCenterReference = flfrs.chCenterPoint;
    referenceStruct.crosshairHorzPointsReference.push_back(flfrs.chH0);
    referenceStruct.crosshairHorzPointsReference.push_back(flfrs.chH1);
    referenceStruct.crosshairVertPointsReference.push_back(flfrs.chV0);
    referenceStruct.crosshairVertPointsReference.push_back(flfrs.chV1);


    referenceStruct.referenceSet = true;


    t_boardPose boardPose;
    boardPose = findBoardPose(&imageCopy);
    referenceStruct.boardPose = boardPose;
    referenceStruct.boardPose.translationLength = boardPose.translationLength;


    t_findArucoDisplacementReturnStruct fadrs = findArucoDisplacement(&undistortedImgCopy);
    if(fadrs.error <0)
    {
      return -6;    //error finding aruco distance
    }

    referenceStruct.referencePosition = fadrs.disp;



    return 0;
  }
  else
  {
    referenceStruct.referenceSet = false;
    return -5;
  }
}


/*!
 * Overload function to get the image from a file.  Intended for development.
 * Acquires a reference image that is guaranteed to have four detectable aruco markers.  The transform matrix is calculated
 * from the marker corner points and stored in class member H.  Class member referenceSet is set to true.
 * Four outer corner points from the aruco markers are stored in class member markerPoints.
 * The center point of the aruco grid is stored in class member arucoCenterReference.
 * Crosshair points, two horizontal and two vertical are stored in crosshairVertPointsReference and crosshairHorzPointsReference.
 * \return
 * \n error code  0 = There were no errors
 * \n error code -1 = There were errors
 * \n error come -6 = error at findArucoDisplacement
 */
int FieldDetect::acquireReferenceImage(std::string imageFileName)
{
  cv::Mat *img;
  APRetStrut aprs;
  FindLightFieldReturnStruct flfrs;

  //clear the reference structure vectors
  referenceStruct.referenceSet = false;
  referenceStruct.markerPoints.clear();
  referenceStruct.crosshairVertPointsReference.clear();
  referenceStruct.crosshairHorzPointsReference.clear();
  referenceStruct.imageReferenceFlat.release();

  img = getImage(imageFileName);
  if(img->empty())
  {
      return -1;
  }
  cv::Mat imgCopy;
  img->copyTo(imgCopy);

  referenceStruct.imageReferenceUndistorted = *img;

  //cv::imshow("ref", referenceStruct.imageReferenceUndistorted);
  //cv::waitKey(0);

  aprs = *arucoPerspective(img, true);
  if(aprs.error <0)  return -1;

  std::vector<cv::Point2d> mkpts = findMarkerPoints(&aprs.imageFlat);
  referenceStruct.imageReferenceFlat =  aprs.imageFlat;

  for(size_t i=0; i<mkpts.size(); i++)
    referenceStruct.markerPoints.push_back(mkpts[i]);

  //find the center of the aruco grid by finding the intersection of the diagonal lines
  referenceStruct.arucoCenterReference = lineLineIntersection(mkpts[0], mkpts[10], mkpts[5], mkpts[15]);

  referenceStruct.referenceSet = true;


  //get the crosshair center and vert and horz points
  flfrs = findLightFieldDimensions(imageFileName);
  referenceStruct.crosshairCenterReference = flfrs.chCenterPoint;
  referenceStruct.crosshairHorzPointsReference.push_back(flfrs.chH0);
  referenceStruct.crosshairHorzPointsReference.push_back(flfrs.chH1);
  referenceStruct.crosshairVertPointsReference.push_back(flfrs.chV0);
  referenceStruct.crosshairVertPointsReference.push_back(flfrs.chV1);


  referenceStruct.cameraInitialized = true;


  t_boardPose boardPose;
  boardPose = findBoardPose(&imgCopy);
  referenceStruct.boardPose = boardPose;
  referenceStruct.boardPose.translationLength = boardPose.translationLength;

  t_findArucoDisplacementReturnStruct fadrs = findArucoDisplacement(img);
  if(fadrs.error <0)
  {
    return -6;    //error finding aruco distance
  }

  referenceStruct.referencePosition = fadrs.disp;


  return 0;
}





/*!
 * Finds the angle of the markers in the live camera image with respect to the reference image.
 * The scene must contain four detectable Aruco markers for the angle to be measured.
 * The angle is obtained by calculating the inverse tangent of the difference in slopes of the reference image
 * and rotated image.
 * \return angle in degress of the marker board with respect to a previously taken reference image.
 * \n error code -1 = no image from camera.
 * \n error code -2 = no reference image.
 * \n error code -3 = failed at arucoPerspective.
 * \n error code -4 = did not find 16 marker points.
 */
 FieldDetect::t_findBoardAngleReturnStructure FieldDetect::findBoardAngle()
{

     cv::Mat *img;
     APRetStrut aprs;
     double theta = -1;
     t_findBoardAngleReturnStructure fbars;

     fbars.error = 0;

     std::vector<cv::Point2d> acqPoints;

     if(!referenceStruct.referenceSet)
     {
         fbars.error = -1;
         return fbars;
     }

     img = getImage();
     if(img->empty())
     {
         fbars.error = -2;
         return fbars;
     }

     aprs = *arucoPerspective(img, false);
     if(aprs.error < 0)
     {
         fbars.error = -3;
         return fbars;
     }

     acqPoints = findMarkerPoints(&aprs.imageFlat);

     cv::Point2d p1, p2, p3;

     p1 =   referenceStruct.arucoCenterReference;
     p2.x = (referenceStruct.markerPoints[10].x + referenceStruct.markerPoints[15].x)/2;
     p2.y = (referenceStruct.markerPoints[10].y + referenceStruct.markerPoints[15].y)/2;

     double L12x = p1.x - p2.x;
     double L12y = p1.y - p2.y;

     //find the field angle wrt the reference angle set by acquireReferenceImage()
     if(acqPoints.size()==16 && referenceStruct.markerPoints.size()==16) //if all markers are there
     {

       p3.x = (acqPoints[10].x + acqPoints[15].x)/2;  //midpoint between marker points 10 and 15
       p3.y = (acqPoints[10].y + acqPoints[15].y)/2;  //midpoint between marker points 10 and 15

       double L13x = p1.x - p3.x;
       double L13y = p1.y - p3.y;

       double theta = atan2(L13y, L13x) - atan2(L12y, L12x);
       theta = theta * 360 / (2*CV_PI);
       if(theta < 0) theta += 360;

       fbars.imageDisplay = aprs.imageFlat;
       fbars.angle = theta;

       return fbars;
     }

     fbars.error = -4;
     return fbars;
}





 /*!
 * \return angle in degress of the marker board with respect to a previously taken reference image.
 * \n error code -1 = failed at arucoPerspective.
 * \n error code -2 = did not find 16 marker points.
 * \n overload function to supply the image from file imageFileName
 * \n The scene must contain four detectable Aruco markers for the angle to be measured.
 * \n The angle is obtained by calculating the inverse tangent of the difference in slopes of the reference image
 * \n and rotated image.
  */
  double FieldDetect::findBoardAngle(std::string imageFileName)
 {

      cv::Mat *img;
      APRetStrut aprs;
      double theta;

      std::vector<cv::Point2d> acqPoints;

      img = getImage(imageFileName);
      aprs = *arucoPerspective(img, false);

      acqPoints = findMarkerPoints(&aprs.imageFlat);

      cv::Point2d p1, p2, p3;

      p1 =   referenceStruct.arucoCenterReference;
      p2.x = (referenceStruct.markerPoints[10].x + referenceStruct.markerPoints[15].x)/2;
      p2.y = (referenceStruct.markerPoints[10].y + referenceStruct.markerPoints[15].y)/2;

      double L12x = p1.x - p2.x;
      double L12y = p1.y - p2.y;

      //find the field angle wrt the reference angle set by acquireReferenceImage()
      if(acqPoints.size()==16 && referenceStruct.markerPoints.size()==16) //if all markers are there
      {

        p3.x = (acqPoints[10].x + acqPoints[15].x)/2;  //midpoint between marker points 10 and 15
        p3.y = (acqPoints[10].y + acqPoints[15].y)/2;  //midpoint between marker points 10 and 15

        double L13x = p1.x - p3.x;
        double L13y = p1.y - p3.y;

        double theta = atan2(L13y, L13x) - atan2(L12y, L12x);
        theta = theta * 360 / (2*CV_PI);
        if(theta < 0) theta += 360;

        return theta;

     }

      return -2;  //error - didn't find 16 marker points
 }


  /*!
  * \return Angle in degress of the marker board with respect to a previously taken reference image.
  * \n error code -1 = failed at arucoPerspective.
  * \n error code -2 = did not find 16 marker points.
  * \n overload function to supply the image from cv::Mat image
  * \n The scene must contain four detectable Aruco markers for the angle to be measured.
  * \n The angle is obtained by calculating the inverse tangent of the difference in slopes of the reference image
  * \n and rotated image.
  */
  double FieldDetect::findBoardAngle(cv::Mat *image)
  {

       APRetStrut aprs;
       double theta;
       std::vector<cv::Point2d> acqPoints;

       aprs = *arucoPerspective(image, false);
       if(aprs.error < 0)
       return -1;

       acqPoints = findMarkerPoints(&aprs.imageFlat);

       cv::Point2d p1, p2, p3;

       //construct a line from thr center of the aruco grid to the mid-point of the bottom two markers
       p1 =   referenceStruct.arucoCenterReference;
       p2.x = (referenceStruct.markerPoints[10].x + referenceStruct.markerPoints[15].x)/2;
       p2.y = (referenceStruct.markerPoints[10].y + referenceStruct.markerPoints[15].y)/2;

       double L12x = p1.x - p2.x;
       double L12y = p1.y - p2.y;

       //find the field angle wrt the reference angle set by acquireReferenceImage()
       if(acqPoints.size()==16 && referenceStruct.markerPoints.size()==16) //if all markers are there
       {

         p3.x = (acqPoints[10].x + acqPoints[15].x)/2;  //midpoint between marker points 10 and 15
         p3.y = (acqPoints[10].y + acqPoints[15].y)/2;  //midpoint between marker points 10 and 15

         double L13x = p1.x - p3.x;
         double L13y = p1.y - p3.y;

         double theta = atan2(L13y, L13x) - atan2(L12y, L12x);
         theta = theta * 360 / (2*CV_PI);
         if(theta < 0) theta += 360;

         return theta;

      }

       return -2;  //error - didn't find 16 marker points


  }





 /*!
 * Finds the collimator or couch walkout using the camera.
 * True needs to be passed if table walkout is being measured.
 * False needs to be passed if collimator walkout is being measured.
 * \return t_findWalkoutRetStruct
 * \n error code 0 = no error
 * \n error code -1 = the camera is not initialized or the reference is not set
 * \n error code -2 = no image was acquired from the camera
 * \n error code =3 = failure at arucoPerspective()
 * \n These values are obtained by taking a series of live video images during single direction collimator or couch rotation and first finding
 * the intersection point of the aruco marker diagonals.  The chrosshair center point was already established
 * from the reference image.  The distance between these two points is the walkout for that capture point.
 * \n The maximum vector length of all of the points is the diameter of walkout.
 * \n The function keeps the Windows message loop running during processing so there is no
 * need to run it in another thread.
 * \n The function uses Signals and Slots and emits a t_WOStruct than can be used in a calling function for real-time feedback of rotation angle, walkout distance and the point coordinate.
 * \n
 * \n <b>Below is an example of using signals and slots to get real time data into your application.</b>
 * \n
 * \n The slot function would be declared in the application header like this:
 * \n
 * \n public slots:
 * \n void on_woDataEmit(FieldDetect::WOStruct* woData);
 * \n
 * \n The signal and slot would be connected in the calling function like this:
 * \n
 * \n QObject::connect(fieldDetect, &FieldDetect::walkoutDataEmitter, this, &MainWindow::on_woDataEmit);
 * \n
 * \n The function body would look like this:
 * \n
 * \n void MainWindow::on_woDataEmit(FieldDetect::WOStruct *woData)
 * \n {
 * \n   do something with the data...
 * \n }
 */
FieldDetect::t_findWalkoutRetStruct FieldDetect::findWalkout(bool isTableWalkOut)
{

  WOStruct woStruct;
  t_findWalkoutRetStruct wors;
  cv::Mat* imageUndistorted;
  APRetStrut aprs;
  std::vector<WOStruct> woStructVect;
  bool inAcqAngleRange = false;
  bool acquiring = true;

  //if it's table wo, lower the camera frame rate to 5fps
  int fr;
  if(isTableWalkOut) fr = setCameraFrameRate(25120000);
  qDebug()<<"frame rate = "<<fr;

  if(!referenceStruct.cameraInitialized || !referenceStruct.referenceSet)
  {                                           //production
    wors.error = -1;                          //production
    return wors;                              //production
  }                                           //production

  while(acquiring)
  {
    //cv::Mat frame;                          //development
    //vidCap >> frame;                        //development
    //if(frame.empty()) break;                //development
    //imageUndistorted = getImage(&frame);    //development


    imageUndistorted = getImage();            //production
    if(imageUndistorted->empty())             //production
    {                                         //production
      wors.error = -2;                        //production
      return wors;                            //production
    }                                         //production

    aprs = *arucoPerspective(imageUndistorted, false);
    if(aprs.error < 0)
    {
      wors.error = -3;
      return wors;
    }


    std::vector<cv::Point2d> acqPoints;
    acqPoints = findMarkerPoints(&aprs.imageFlat);

    cv::Point2d p1, p2, p3;

    p1 =   referenceStruct.arucoCenterReference;
    p2.x = (referenceStruct.markerPoints[10].x + referenceStruct.markerPoints[15].x)/2;
    p2.y = (referenceStruct.markerPoints[10].y + referenceStruct.markerPoints[15].y)/2;

    double L12x = p1.x - p2.x;
    double L12y = p1.y - p2.y;

    //find the field angle wrt the reference angle set by acquireReferenceImage()
    if(acqPoints.size()==16 && referenceStruct.markerPoints.size()==16) //if all markers are there
    {

      p3.x = (acqPoints[10].x + acqPoints[15].x)/2;  //midpoint between marker points 10 and 15
      p3.y = (acqPoints[10].y + acqPoints[15].y)/2;  //midpoint between marker points 10 and 15

      double L13x = p1.x - p3.x;
      double L13y = p1.y - p3.y;

      double theta = atan2(L13y, L13x) - atan2(L12y, L12x);
      theta = theta * 360 / (2*CV_PI);
      if(theta < 0) theta += 360;

      std::string s = std::to_string(theta);

      /*DIAGNOSTIC
      cv::line(aprs.imageFlat, p1, p2, cv::Scalar(0,0,255), 1, cv::LINE_AA);
      cv::line(aprs.imageFlat, p1, p3, cv::Scalar(0,255,0), 1, cv::LINE_AA);
      cv::line(aprs.imageFlat, p2, p3, cv::Scalar(255,0,0), 1, cv::LINE_AA);
      cv::circle(aprs.imageFlat, acqPoints[15], 4, cv::Scalar(0,255,0), 1, cv::LINE_AA);
      cv::circle(aprs.imageFlat, acqPoints[10], 4, cv::Scalar(0,255,0), 1, cv::LINE_AA);
      cv::circle(aprs.imageFlat, p3, 4, cv::Scalar(0,255,0), 1, cv::LINE_AA);
      cv::putText(aprs.imageFlat, s, cv::Point2d(1200,900), 0, 1.5, cv::Scalar(255,0,0), 3, cv::LINE_8);
      */

      qDebug()<<"findWalkout() WOStruct.size() "<<woStructVect.size();

      //determine if the angle is within the 90deg -> 270deg acquisition range
      //dir is the difference between two consecutive angle measurements - it peaks at 180 at coll angles 90 and 270
      if(theta > 90 && theta <= 270 && inAcqAngleRange && woStructVect.size()>0)
      {
        inAcqAngleRange = false;
        acquiring = false;
        qDebug()<<"NOT IN RANGE";
        break;

      }

      qDebug()<<" findWalkout() angle "<<theta;

      if( (theta >= 0 && theta <= 90) || (theta >= 270 && theta <= 360))
      {
        inAcqAngleRange = true;
        qDebug()<<"IN RANGE";
      }

      //find the intersection of the diaganols
      cv::Point2d arucoWalkoutPoint = lineLineIntersection(acqPoints[0], acqPoints[10],acqPoints[5],acqPoints[15]);


      /*DIAGNOSTIC
      cv::circle(aprs.imageFlat, arucoWalkoutPoint, 4, cv::Scalar(255,0,0), 1, cv::LINE_AA);
      cv::circle(aprs.imageFlat, referenceStruct.crosshairCenterReference, 4, cv::Scalar(0,0,255), 1, cv::LINE_AA);
      cv::line(aprs.imageFlat, acqPoints[0], acqPoints[10], cv::Scalar(255,255,255), 1, cv::LINE_AA);
      cv::line(aprs.imageFlat, acqPoints[5], acqPoints[15], cv::Scalar(255,255,255), 1, cv::LINE_AA);
      cv::imshow("vidcap", aprs.imageFlat);
      char ch = cv::waitKey(1);
      if(ch==27) break;
      */

      double dx = abs(referenceStruct.arucoCenterReference.x - arucoWalkoutPoint.x);
      double dy = abs(referenceStruct.arucoCenterReference.y - arucoWalkoutPoint.y);

      double wo = sqrt((dx*dx)+(dy*dy));

      woStruct.sampleNumber = woStructVect.size();
      woStruct.angle = theta;
      woStruct.vecLen = wo/4;

      qDebug()<<"--------------vec len--------------------> "<<wo;

      woStruct.woPt = arucoWalkoutPoint;

      if(inAcqAngleRange) woStructVect.push_back(woStruct);

      //emit the woStruct data
      emit walkoutDataEmitter(&woStruct);

      //keep the msg loop running during loop
      QCoreApplication::processEvents();

    }
  }


  std::vector<std::pair<double,int>> card90Vect;
  std::vector<std::pair<double,int>> card0Vect;

  //pair the angle with an integer to use as an index for sorting
  //subtract two consecutive angle samples to find the quadrant boundary.
  for(size_t i=0; i<woStructVect.size()-1;i++)
  {
    card90Vect.push_back(std::make_pair(woStructVect[i+1].angle - woStructVect[i].angle, i));
    card0Vect.push_back(std::make_pair(abs(woStructVect[i].angle),i));
  }

  //sort the angles and use the second value as an index to the point

  //the first two values in sorted card90Vect are the 90 and 180 cardinal point values
  std::sort(card90Vect.begin(), card90Vect.end());
  //the lowest absolute value is the 0 cardinal point
  std::sort(card0Vect.begin(), card0Vect.end());

  std::vector<double> minMaxWO;
  for(size_t i=0;i<woStructVect.size()-1;i++)
    minMaxWO.push_back(woStructVect[i].vecLen);
  std::sort(minMaxWO.begin(), minMaxWO.end());

  wors.minWOVectorLength = minMaxWO[0];
  wors.maxWOVectorLength = minMaxWO[minMaxWO.size()-1];

  qDebug()<<"minWO "<<minMaxWO[0]<<"maxWO "<<minMaxWO[minMaxWO.size()-1];

  wors.cardinalPointsVect.push_back(woStructVect[card90Vect[0].second].woPt);   //90 degree point
  wors.cardinalPointsVect.push_back(woStructVect[card0Vect[0].second].woPt);    //0 degree point
  wors.cardinalPointsVect.push_back(woStructVect[card90Vect[1].second].woPt);   //180 degree point

  wors.walkoutStruct = woStructVect;

  //if it's table wo, set the frame rate back to 15fps
  if(isTableWalkOut) fr = setCameraFrameRate(75360000);
  qDebug()<<"frame rate = "<<fr;


  wors.error = 0;
  return wors;
}


/*!
* Overloaded function used for development - Finds the collimator or couch walkout from a video file
* \return t_findWalkoutRetStruct
* \n error code 0 = no error
* \n error code -1 = the camera is not initialized or the reference is not set
* \n error code -2 = no image was acquired from the camera
* \n error code =3 = failure at arucoPerspective()
* \n These values are obtained by taking a series of live video images during single direction collimator or couch rotation and first finding
* the intersection point of the aruco marker diagonals.  The chrosshair center point was already established
* from the reference image.  The distance between these two points is the walkout for that capture point.
* \n The maximum vector length of all of the points is the diameter of walkout.
* \n The function keeps the Windows message loop running during processing so there is no
* need to run it in another thread.
* \n The function uses Signals and Slots and emits a t_WOStruct than can be used in a calling function for real-time feedback of rotation angle, walkout distance and the point coordinate.
* \n
* \n <b>Below is an example of using signals and slots to get real time data into your application.</b>
* \n
* \n The slot function would be declared in the application header like this:
* \n
* \n public slots:
* \n void on_woDataEmit(FieldDetect::WOStruct* woData);
* \n
* \n The signal and slot would be connected in the calling function like this:
* \n
* \n QObject::connect(fieldDetect, &FieldDetect::walkoutDataEmitter, this, &MainWindow::on_woDataEmit);
* \n
* \n The function body would look like this:
* \n
* \n void MainWindow::on_woDataEmit(FieldDetect::WOStruct *woData)
* \n {
* \n   do something with the data...
* \n }
*/
FieldDetect::t_findWalkoutRetStruct FieldDetect::findWalkout(std::string videoFileName)
{

 WOStruct woStruct;
 t_findWalkoutRetStruct wors;
 cv::Mat* imageUndistorted;
 APRetStrut aprs;
 std::vector<WOStruct> woStructVect;
 bool inAcqAngleRange = false;
 bool acquiring = true;

 cv::VideoCapture vidCap(videoFileName);    //development

 if(!referenceStruct.cameraInitialized || !referenceStruct.referenceSet)
 {
   wors.error = -1;
   return wors;
 }

 while(acquiring)
 {
   cv::Mat frame;                          //development
   vidCap >> frame;                        //development
   if(frame.empty()) break;                //development
   imageUndistorted = getImage(&frame);    //development

   if(imageUndistorted->empty())
   {
     wors.error = -2;
     return wors;
   }

   aprs = *arucoPerspective(imageUndistorted, false);
   if(aprs.error < 0)
   {
     wors.error = -3;
     return wors;
   }

   std::vector<cv::Point2d> acqPoints;
   acqPoints = findMarkerPoints(&aprs.imageFlat);

   cv::Point2d p1, p2, p3;

   p1 =   referenceStruct.arucoCenterReference;
   p2.x = (referenceStruct.markerPoints[10].x + referenceStruct.markerPoints[15].x)/2;
   p2.y = (referenceStruct.markerPoints[10].y + referenceStruct.markerPoints[15].y)/2;

   double L12x = p1.x - p2.x;
   double L12y = p1.y - p2.y;

   //find the field angle wrt the reference angle set by acquireReferenceImage()
   if(acqPoints.size()==16 && referenceStruct.markerPoints.size()==16) //if all markers are there
   {

     p3.x = (acqPoints[10].x + acqPoints[15].x)/2;  //midpoint between marker points 10 and 15
     p3.y = (acqPoints[10].y + acqPoints[15].y)/2;  //midpoint between marker points 10 and 15

     double L13x = p1.x - p3.x;
     double L13y = p1.y - p3.y;

     double theta = atan2(L13y, L13x) - atan2(L12y, L12x);
     theta = theta * 360 / (2*CV_PI);
     if(theta < 0) theta += 360;

     std::string s = std::to_string(theta);

     //DIAGNOSTIC

     cv::line(aprs.imageFlat, p1, p2, cv::Scalar(0,0,255), 1, cv::LINE_AA);
     cv::line(aprs.imageFlat, p1, p3, cv::Scalar(0,255,0), 1, cv::LINE_AA);
     cv::line(aprs.imageFlat, p2, p3, cv::Scalar(255,0,0), 1, cv::LINE_AA);
     cv::circle(aprs.imageFlat, acqPoints[15], 4, cv::Scalar(0,255,0), 1, cv::LINE_AA);
     cv::circle(aprs.imageFlat, acqPoints[10], 4, cv::Scalar(0,255,0), 1, cv::LINE_AA);
     cv::circle(aprs.imageFlat, p3, 4, cv::Scalar(0,255,0), 1, cv::LINE_AA);
     cv::putText(aprs.imageFlat, s, cv::Point2d(1200,900), 0, 1.5, cv::Scalar(255,0,0), 3, cv::LINE_8);


     qDebug()<<"findWalkout() WOStruct.size() "<<woStructVect.size();

     //determine if the angle is within the 90deg -> 270deg acquisition range
     //dir is the difference between two consecutive angle measurements - it peaks at 180 at coll angles 90 and 270
     if(theta > 90 && theta <= 270 && inAcqAngleRange && woStructVect.size()>0)
     {
       inAcqAngleRange = false;
       acquiring = false;
       qDebug()<<"NOT IN RANGE";
       break;

     }

     qDebug()<<" findWalkout() angle "<<theta;

     if( (theta >= 0 && theta <= 90) || (theta >= 270 && theta <= 360))
     {
       inAcqAngleRange = true;
       qDebug()<<"IN RANGE";
     }

     //find the intersection of the diaganols
     cv::Point2d arucoWalkoutPoint = lineLineIntersection(acqPoints[0], acqPoints[10],acqPoints[5],acqPoints[15]);


     //DIAGNOSTIC
     cv::circle(aprs.imageFlat, arucoWalkoutPoint, 4, cv::Scalar(255,0,0), 1, cv::LINE_AA);
     cv::circle(aprs.imageFlat, referenceStruct.crosshairCenterReference, 4, cv::Scalar(0,0,255), 1, cv::LINE_AA);
     cv::line(aprs.imageFlat, acqPoints[0], acqPoints[10], cv::Scalar(255,255,255), 1, cv::LINE_AA);
     cv::line(aprs.imageFlat, acqPoints[5], acqPoints[15], cv::Scalar(255,255,255), 1, cv::LINE_AA);
     cv::imshow("vidcap", aprs.imageFlat);
     char ch = cv::waitKey(1);
     if(ch==27) break;


     double dx = abs(referenceStruct.arucoCenterReference.x - arucoWalkoutPoint.x);
     double dy = abs(referenceStruct.arucoCenterReference.y - arucoWalkoutPoint.y);

     double wo = sqrt((dx*dx)+(dy*dy));

     woStruct.sampleNumber = woStructVect.size();
     woStruct.angle = theta;
     woStruct.vecLen = wo/4;

     qDebug()<<"--------------vec len--------------------> "<<wo;

     woStruct.woPt = arucoWalkoutPoint;

     if(inAcqAngleRange) woStructVect.push_back(woStruct);

     //emit the woStruct data
     emit walkoutDataEmitter(&woStruct);

     //keep the msg loop running during loop
     QCoreApplication::processEvents();

   }
 }


 std::vector<std::pair<double,int>> card90Vect;
 std::vector<std::pair<double,int>> card0Vect;

 //pair the angle with an integer to use as an index for sorting
 //subtract two consecutive angle samples to find the quadrant boundary.
 for(size_t i=0; i<woStructVect.size()-1;i++)
 {
   card90Vect.push_back(std::make_pair(woStructVect[i+1].angle - woStructVect[i].angle, i));
   card0Vect.push_back(std::make_pair(abs(woStructVect[i].angle),i));
 }

 //sort the angles and use the second value as an index to the point

 //the first two values in sorted card90Vect are the 90 and 180 cardinal point values
 std::sort(card90Vect.begin(), card90Vect.end());
 //the lowest absolute value is the 0 cardinal point
 std::sort(card0Vect.begin(), card0Vect.end());

 std::vector<double> minMaxWO;
 for(size_t i=0;i<woStructVect.size()-1;i++)
   minMaxWO.push_back(woStructVect[i].vecLen);
 std::sort(minMaxWO.begin(), minMaxWO.end());

 wors.minWOVectorLength = minMaxWO[0];
 wors.maxWOVectorLength = minMaxWO[minMaxWO.size()-1];

 qDebug()<<"minWO "<<minMaxWO[0]<<"maxWO "<<minMaxWO[minMaxWO.size()-1];

 wors.cardinalPointsVect.push_back(woStructVect[card90Vect[0].second].woPt);   //90 degree point
 wors.cardinalPointsVect.push_back(woStructVect[card0Vect[0].second].woPt);    //0 degree point
 wors.cardinalPointsVect.push_back(woStructVect[card90Vect[1].second].woPt);   //180 degree point

 wors.walkoutStruct = woStructVect;
 wors.error = 0;

 return wors;
}





/*!
 * Makes twenty exposures to warm up the camera.
 * \return if there is no error, a double that contains the mean value of the raw image pixels of the last (20th) warmup image.
 * \n error code = -1 = error acquiring an image from the camera.
 * \n error code = -2 = camera was not initialized.
 */
double FieldDetect::warmUpCamera()
{
    SpinInterface::t_acquireImageReturnStruct* AIRS;
    cv::Scalar m;
    double deltaBrightness = 1;
    int timeOut = 0;
    double previousBrightness;

    m[0] = 255;
    timeOut = 20;

  if(referenceStruct.cameraInitialized)
  {
    while(timeOut != 0)
    {
      previousBrightness = m[0];
      AIRS = spin.acquireImage();
      if(AIRS->error < 0) return -1;

      m = cv::mean(AIRS->imageRaw);

      deltaBrightness = abs(m[0] - previousBrightness);
      timeOut--;
      if(timeOut > 30) break;

      qDebug()<<"warmUpCamera() current brightness " << m[0];
      qDebug()<<"warmUpCamera() previousBrightness " << previousBrightness;
      qDebug()<<"warmUpCamera() deltaBrightness " << deltaBrightness;
    }
  }
  else
    return -2;

return m[0];

}



/*!
 *  The PolyFit() function uses Gaussian elimination with backward substitution.
 *  Code from Army Research Labs
 */
template<class T>void FieldDetect::PolyFit(//<=========FITS A POLYNOMIAL TO A SET OF POINTS
const T*X,//<-------INDEPENDENT-VARIABLE VALUES (EACH X[i] MUST BE UNIQUE)
const T*Y,//<-------------------DEPENDENT-VARIABLE VALUES (SAME SIZE AS X)
int n,//<-------------------------------------NUMBER OF ELEMENTS IN X OR Y
int m,//<----------------NUMBER OF COEFFICIENTS IN THE BEST-FIT POLYNOMIAL
T*C)
{//<------STORAGE FOR COEFFICIENTS (SIZE=m) y=C[0]+C[1]*x+C[2]*x^2+...
T**A=new T*[m];/*<-*/
for(int i=0,j,k;i<m;++i)
  {//............augmented matrix
  for(A[i]=new T[m+1],j=0;j<m+1;++j)A[i][j]=0;
    for(k=0;k<n;++k)
      for(A[i][m]+=pow(X[k],i)*Y[k],j=0;j<m;++j)
          A[i][j]+=pow(X[k],i+j);
  }
  for(int i=1,j,k;i<m;++i)for(k=0;k<i;++k)for(j=m;j>=0;--j)//.........Gaussian
    A[i][j]-=A[i][k]*A[k][j]/A[k][k];// elimination
    for(int i=m-1,j;i>=0;--i)for(C[i]=A[i][m]/A[i][i],j=i+1;j<m;++j)//..backward
      C[i]-=C[j]*A[i][j]/A[i][i];// substitution
      for(int i=0;i<m;++i)delete[]A[i];/*&*/delete[]A;
}//~~~~YAGENAUT@GMAIL.COM~~~~~~~~~~~~~~~~~~~~~~~~~LAST~UPDATED~21JUL2014~~~~~~



/*!
 * \brief FieldDetect::correlationCoefficient
 * \param X
 * \param Y
 * \param n
 * \return
 */
double FieldDetect::correlationCoefficient(double X[], double Y[], int n)
{

    double sum_X = 0, sum_Y = 0, sum_XY = 0;
    double squareSum_X = 0, squareSum_Y = 0;

    for (int i = 0; i < n; i++)
    {
        // sum of elements of array X.
        sum_X = sum_X + X[i];

        // sum of elements of array Y.
        sum_Y = sum_Y + Y[i];

        // sum of X[i] * Y[i].
        sum_XY = sum_XY + X[i] * Y[i];

        // sum of square of array elements.
        squareSum_X = squareSum_X + X[i] * X[i];
        squareSum_Y = squareSum_Y + Y[i] * Y[i];
    }

    // use formula for calculating correlation coefficient.
    double corr = (n * sum_XY - sum_X * sum_Y)
                  / sqrt((n * squareSum_X - sum_X * sum_X)
                      * (n * squareSum_Y - sum_Y * sum_Y));

    return corr;
}


/*EXPERIMENTAL
float FieldDetect::checkBlurriness()
{

  SpinInterface::t_acquireImageReturnStruct* tairs;

  while(1)
  {
  tairs = spin.acquireImage();
  if(tairs->error < 0) return -1;           //error acquiring raw image from camera
  if(tairs->imageRaw.empty()) return -2;    //error - the acquired image was empty

  cv::Mat imageWorking;
  tairs->imageRaw.copyTo(imageWorking);

  float blurriness = cv::videostab::calcBlurriness(imageWorking);
  qDebug()<<"blurriness: " << blurriness;

  cv::putText(imageWorking, std::to_string(blurriness) , cv::Point2f(50,150), 0, 2, cv::Scalar(0,0,0),5);


  cv::imshow("blurriness", imageWorking);          //diagnostic
  cv::waitKey(1);                           //diagnostic
  //QCoreApplication::processEvents();

  }
  return 0;//blurriness;
}


EXPERIMENTAL
 * Checks camera calibration by performing a camera calibration using one image.  The one image is the raw (distorted) reference image with the board set up and aligned.  Calibration is then compared to the calibration from the normal calibration in the calibration file.
 * \n error code = 0 = calibration matched the cal file.
 * \n error code = -1 = error acquiring raw image from camera.
 * \n error code = -2 = the acquired image was empty
 *
int FieldDetect::checkCameraCalibration()
{

  SpinInterface::t_acquireImageReturnStruct* tairs;

  tairs = spin.acquireImage();
  if(tairs->error < 0) return -1;           //error acquiring raw image from camera
  if(tairs->imageRaw.empty()) return -2;    //error - the acquired image was empty
  //cv::imshow("", tairs->imageRaw);          //diagnostic
  //cv::waitKey(0);                           //diagnostic

  cv::Mat imageWorking;
  tairs->imageRaw.copyTo(imageWorking);

  std::vector<int> ids;
  std::vector<std::vector<cv::Point2f> > corners;
  cv::aruco::detectMarkers(imageWorking, dictionary, corners, ids);
  // if at least one marker detected
  if (ids.size() > 0) cv::aruco::drawDetectedMarkers(imageWorking, corners, ids);

  //cv::imshow("markers", imageWorking);          //diagnostic
  //cv::waitKey(0);                               //diagnostic

  std::vector<cv::Point2d> imagePts;
  std::vector<cv::Point3d> worldPts;
  double s=1;
  double os=0;

  //match the image points with known marker points
  for(size_t i=0; i<ids.size(); i++)
  {

    if(ids[i] == 1) imagePts.push_back(corners[i][0]);
    if(ids[i] == 1) imagePts.push_back(corners[i][1]);
    if(ids[i] == 1) imagePts.push_back(corners[i][2]);
    if(ids[i] == 1) imagePts.push_back(corners[i][3]);

    if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+0*s,os+0*s,0));
    if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+30*s,os+0*s,0));
    if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+30*s,os+30*s,0));
    if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+0*s,os+30*s,0));

    if(ids[i] == 2) imagePts.push_back(corners[i][0]);
    if(ids[i] == 2) imagePts.push_back(corners[i][1]);
    if(ids[i] == 2) imagePts.push_back(corners[i][2]);
    if(ids[i] == 2) imagePts.push_back(corners[i][3]);

    if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+150*s,os+0*s,0));
    if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+180*s,os+0*s,0));
    if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+180*s,os+30*s,0));
    if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+150*s,os+30*s,0));

    if(ids[i] == 3) imagePts.push_back(corners[i][0]);
    if(ids[i] == 3) imagePts.push_back(corners[i][1]);
    if(ids[i] == 3) imagePts.push_back(corners[i][2]);
    if(ids[i] == 3) imagePts.push_back(corners[i][3]);

    if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+150*s,os+150*s,0));
    if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+180*s,os+150*s,0));
    if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+180*s,os+180*s,0));
    if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+150*s,os+180*s,0));


    if(ids[i] == 4) imagePts.push_back(corners[i][0]);
    if(ids[i] == 4) imagePts.push_back(corners[i][1]);
    if(ids[i] == 4) imagePts.push_back(corners[i][2]);
    if(ids[i] == 4) imagePts.push_back(corners[i][3]);

    if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+0*s,os+150*s,0));
    if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+30*s,os+150*s,0));
    if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+30*s,os+180*s,0));
    if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+0*s,os+180*s,0));


      if(ids[i] == 1) imagePts.push_back(corners[i][0]);
      if(ids[i] == 1) imagePts.push_back(corners[i][1]);
      if(ids[i] == 1) imagePts.push_back(corners[i][2]);
      if(ids[i] == 1) imagePts.push_back(corners[i][3]);

      if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+0*s,os+0*s,0));
      if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+37*s,os+0*s,0));
      if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+37*s,os+37*s,0));
      if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+0*s,os+37*s,0));

      if(ids[i] == 2) imagePts.push_back(corners[i][0]);
      if(ids[i] == 2) imagePts.push_back(corners[i][1]);
      if(ids[i] == 2) imagePts.push_back(corners[i][2]);
      if(ids[i] == 2) imagePts.push_back(corners[i][3]);

      if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+135*s,os+0*s,0));
      if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+172*s,os+0*s,0));
      if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+172*s,os+37*s,0));
      if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+135*s,os+37*s,0));

      if(ids[i] == 3) imagePts.push_back(corners[i][0]);
      if(ids[i] == 3) imagePts.push_back(corners[i][1]);
      if(ids[i] == 3) imagePts.push_back(corners[i][2]);
      if(ids[i] == 3) imagePts.push_back(corners[i][3]);

      if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+135*s,os+135*s,0));
      if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+172*s,os+135*s,0));
      if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+172*s,os+172*s,0));
      if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+135*s,os+172*s,0));


      if(ids[i] == 4) imagePts.push_back(corners[i][0]);
      if(ids[i] == 4) imagePts.push_back(corners[i][1]);
      if(ids[i] == 4) imagePts.push_back(corners[i][2]);
      if(ids[i] == 4) imagePts.push_back(corners[i][3]);

      if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+0*s,os+135*s,0));
      if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+37*s,os+135*s,0));
      if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+37*s,os+172*s,0));
      if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+0*s,os+172*s,0));

  }


  cv::Vec2f imgPtsTemp;
  std::vector<cv::Vec2f> imgPtsVec;
  std::vector<std::vector<cv::Vec2f>> imgPtsCal;

  for(size_t i=0; i<imagePts.size(); i++)
  {
    imgPtsTemp[0] = imagePts[i].x;
    imgPtsTemp[1] = imagePts[i].y;
    imgPtsVec.push_back(imgPtsTemp);
  }

  imgPtsCal.push_back(imgPtsVec);

  cv::Vec3f objPtsTemp;
  std::vector<cv::Vec3f> objPtsVec;
  std::vector<std::vector<cv::Vec3f>> objPtsCal;

  for(size_t i=0; i<imagePts.size(); i++)
  {
    objPtsTemp[0] = worldPts[i].x;
    objPtsTemp[1] = worldPts[i].y;
    objPtsTemp[2] = worldPts[i].z;
    objPtsVec.push_back(objPtsTemp);
  }

  objPtsCal.push_back(objPtsVec);


  for(size_t i=0; i<imagePts.size(); i++)
    cv::circle(imageWorking, imagePts[i], 5, cv::Scalar(255,255,255),2);

  for(size_t i=0; i<imagePts.size(); i++)
    cv::circle(imageWorking, cv::Point2d(worldPts[i].x,worldPts[i].y) , 10, cv::Scalar(255,255,255),2);

  cv::Mat tvecs, rvecs;
  cv::Mat distCoeffs;
  cv::Mat camMatrix;
  camMatrix = cv::Mat::eye(3, 3, CV_64F);
  camMatrix.at<double>(0,0) = 1.f;    //were using fixed aspect ratio in calibrateCameraRO so we need to initialize fx

  std::vector<cv::Point3f> newObjPoints;

  cv::imshow("markers", imageWorking);          //diagnostic
  cv::waitKey(0);                               //diagnostic

  qDebug()<<"world points: "<< worldPts.size();
  qDebug()<<"image points: "<< imagePts.size();


  if(imagePts.size() == 16 && worldPts.size() == 16)
  {
    double rms = cv::calibrateCameraRO(objPtsCal, imgPtsCal, imageWorking.size(), -1,
                              camMatrix, distCoeffs, rvecs, tvecs, newObjPoints,
                              cv::CALIB_USE_LU+cv::CALIB_FIX_ASPECT_RATIO+cv::CALIB_FIX_PRINCIPAL_POINT+
                              cv::CALIB_ZERO_TANGENT_DIST);

    qDebug()<<"calibration RMS: "<<rms;
  }

  writeCSV("_cam.txt", camMatrix);
  writeCSV("_dst.txt", distCoeffs);

  cv::Mat imageUndistorted;
  cv::undistort(imageWorking, imageUndistorted, camMatrix, distCoeffs);

  cv::imshow("undistorted", imageUndistorted);          //diagnostic
  cv::waitKey(0);                               //diagnostic



  return 0;
}
*/



/*!
 * Loads the configuration file
 *
 *
 *
 */
int FieldDetect::loadConfigFromFile(std::string configFileName)
{


    int err = readConfigXML(configFileName);
    if(err <  0) return -1;



    return 0;
}





/*!
 * Finds the pose of the camera wrt the marker board coordinate system.  Also returns the translation vector length (used in findArucoDisplacement).
 * \return t_boardPose structure - see docs for this structure.
 */
FieldDetect::t_boardPose FieldDetect::findBoardPose(cv::Mat *imageUndistorted)
{

    //Only CVQA marker board Aruco markers 4 and 3 are used
    std::vector<int> cvqaIDs = {4, 3};

    t_boardPose boardPose;

    //Create a gridboard with two horizontal aruco markers, with the left marker being id=4 and the right being id =3.
    //The two markers will be spaced 120mm from left edge to right edge and the markers will be 30mm square.
    aruco::GridBoard board(Size(2, 1), 0.030f, 0.120f, dictionary, cvqaIDs);


    //cv::imshow("board pose", *imageUndistorted);
    //cv::waitKey(0);

    std::vector<std::vector<cv::Point2f>> corners, rejectedCandidates;
    std::vector<int> ids;
    cv::Mat imageCopy;

    imageUndistorted->copyTo(imageCopy);
    params.cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
    params.cornerRefinementWinSize = 10;   //5
    params.cornerRefinementMaxIterations = 100; //30
    params.cornerRefinementMinAccuracy = 0.001; //0.1

    cv::aruco::ArucoDetector detector(dictionary, params);
    detector.detectMarkers(*imageUndistorted, corners, ids, rejectedCandidates);


    //cv::imshow("points", imageCopy);
    //cv::waitKey(1);

    cv::Vec3d rvec, tvec;
    if (ids.size() > 0)
    {
        cv::aruco::drawDetectedMarkers(imageCopy, corners, ids);

        cv::Mat objPoints, imgPoints;
        board.matchImagePoints(corners, ids, objPoints, imgPoints);

        cv::solvePnP(objPoints, imgPoints, spin.cameraMatrix, spin.distortionCoefficients, rvec, tvec);

        //int valid = estimatePoseBoard(corners, ids, board, spin.cameraMatrix, spin.distortionCoefficients, rvec, tvec);


        //if(valid) cv::drawFrameAxes(imageCopy, spin.cameraMatrix, spin.distortionCoefficients, rvec, tvec, 0.1);
    }

    double z = sqrt( /* (tvec[0]*tvec[0]) + */ (tvec[1]*tvec[1]) + (tvec[2]*tvec[2]) )*1000;

/*
    qDebug()<<"------------------------------------------------------------------------trans vector length "<<z;
    qDebug()<<"-------------------------------------------------------------------------------rvecs x "<<rvec[0]*(180/CV_PI);
    qDebug()<<"-------------------------------------------------------------------------------rvecs y "<<rvec[1]*(180/CV_PI);
    qDebug()<<"-------------------------------------------------------------------------------rvecs z "<<rvec[2]*(180/CV_PI);
    qDebug()<<"                                                                                              ";
    qDebug()<<"-------------------------------------------------------------------------------tvecs x "<<tvec[0]*1000;
    qDebug()<<"-------------------------------------------------------------------------------tvecs y "<<tvec[1]*1000;
    qDebug()<<"-------------------------------------------------------------------------------tvecs z "<<tvec[2]*1000;
*/

    //cv::imshow("board pose pose detected", imageCopy);
    //cv::waitKey(1);

    boardPose.translationLength = z;
    boardPose.rvec = rvec;
    boardPose.tvec = tvec;
    boardPose.imageDisplay = imageCopy;

    return boardPose;

}





/*!
 * \brief FieldDetect::computeProjectionMatrix
 * \param camMat
 * \param rotVec
 * \param transVec
 * \return
 */
cv::Mat FieldDetect::computeProjectionMatrix(cv::Mat camMat, cv::Vec3d rotVec, cv::Vec3d transVec)
{
    cv::Mat rotMat(3, 3, CV_64F), rotTransMat(3, 4, CV_64F); //Init.
    //Convert rotation vector into rotation matrix
    cv::Rodrigues(rotVec, rotMat);
    //Append translation vector to rotation matrix
    cv::hconcat(rotMat, transVec, rotTransMat);
    //Compute projection matrix by multiplying intrinsic parameter
    //matrix (A) with 3 x 4 rotation and translation pose matrix (RT).
    //Formula: Projection Matrix = A * RT;
    return (camMat * rotTransMat);
}





/*!
 * \brief FieldDetect::rotateImage
 * \param input cv::Mat
 * \param output cv::Mat
 * \param alpha double rotation x
 * \param beta  double rotation y
 * \param gamma double rotation z
 * \param dx double translation x
 * \param dy double translation y
 * \param dz double translation z
 * \param f focal point from camera cal
 */
void FieldDetect::rotateImage(const cv::Mat &input, cv::Mat &output, double alpha, double beta, double gamma, double dx, double dy, double dz, double f)
  {
    alpha = alpha * (CV_PI/180.);
    beta =  beta  * (CV_PI/180.);
    gamma = gamma * (CV_PI/180.);
    // get width and height for ease of use in matrices
    double w = (double)input.cols;
    double h = (double)input.rows;
    // Projection 2D -> 3D matrix
    cv::Mat A1 = (cv::Mat_<double>(4,3) <<
              1, 0, -w/2,
              0, 1, -h/2,
              0, 0,    0,
              0, 0,    1);
    // Rotation matrices around the X, Y, and Z axis
    cv::Mat RX = (cv::Mat_<double>(4, 4) <<
              1,          0,           0, 0,
              0, cos(alpha), -sin(alpha), 0,
              0, sin(alpha),  cos(alpha), 0,
              0,          0,           0, 1);
    cv::Mat RY = (cv::Mat_<double>(4, 4) <<
              cos(beta), 0, -sin(beta), 0,
              0, 1,          0, 0,
              sin(beta), 0,  cos(beta), 0,
              0, 0,          0, 1);
    cv::Mat RZ = (cv::Mat_<double>(4, 4) <<
              cos(gamma), -sin(gamma), 0, 0,
              sin(gamma),  cos(gamma), 0, 0,
              0,          0,           1, 0,
              0,          0,           0, 1);
    // Composed rotation matrix with (RX, RY, RZ)
    cv::Mat R = RX * RY * RZ;
    // Translation matrix
    cv::Mat T = (cv::Mat_<double>(4, 4) <<
             1, 0, 0, dx,
             0, 1, 0, dy,
             0, 0, 1, dz,
             0, 0, 0, 1);
    // 3D -> 2D matrix
    cv::Mat camera = (cv::Mat_<double>(3,4) <<
              f, 0, w/2, 0,
              0, f, h/2, 0,
              0, 0,   1, 0);
    // Final transformation matrix
    cv::Mat trans = camera * (T * (R * A1));
    // Apply matrix transformation
    cv::warpPerspective(input, output, trans, input.size(), cv::WARP_INVERSE_MAP ); //WARP_INVERSE_MAP
  }


/*!
 * \brief FieldDetect::computeC2MC1
 * \param R1
 * \param tvec1
 * \param R2
 * \param tvec2
 * \param R_1to2
 * \param tvec_1to2
 */
void FieldDetect::computeC2MC1(const cv::Mat &R1, const cv::Mat &tvec1, const cv::Mat &R2, const cv::Mat &tvec2,
                  cv::Mat &R_1to2, cv::Mat &tvec_1to2)
{
    //c2Mc1 = c2Mo * oMc1 = c2Mo * c1Mo.inv()
    R_1to2 = R2 * R1.t();
    tvec_1to2 = R2 * (-R1.t()*tvec1) + tvec2;
}





/*!
 * Loads the configuration file
 * \n error code = 0 = the config file was successfully loaded.
 * \n error code = -1 = the config file was not loaded.
 */
int FieldDetect::readConfigXML(std::string filename)
{

  cv::FileStorage fs;
  fs.open(filename, cv::FileStorage::READ);

  if (!fs.isOpened())
  {
      return -1;
      qDebug()<<"couldn't read config.xml";
  }


  fs["camera_matrix"] >> spin.cameraMatrix;
  fs["distortion_coefficients"] >> spin.distortionCoefficients;

  fs["light_level_low_threshold"] >> lightLevelLow;
  fs["light_level_high_threshold"] >> lightLevelHigh;

  qDebug()<<"low light level threshold: "<<lightLevelLow;
  qDebug()<<"high light level threshold: "<<lightLevelHigh;


  qDebug()<<"success reading config.xml";

  return 0;
}





/*!
 * Experimental function - not fully developed.  Using a custom Aruco cube centered on the couch top at isocenter, finds the gantry walkout and angle.
 */
void FieldDetect::findGantryWalkout(std::string videoFileName)
{

 cv::Mat* imageUndistorted;
 bool acquiring = true;
 bool haveTransform = false;
 cv::Point2d refCenter;
 double s=4;
 double os=500;

 int err = initializeCamera();
 qDebug()<<"mainWindow initializeCamera() error "<<err;

 cv::VideoCapture vidCap(videoFileName);        //comment out for live video
 cv::Mat H;


 while(acquiring)
 {
   cv::Mat frame;
   vidCap >> frame;                             //comment out for live video
   if(frame.empty()) break;                     //comment out for live video
   imageUndistorted = getImage(&frame);         //comment out for live video
   //imageUndistorted = getImage();             //UNcomment for LIVE video
   if(imageUndistorted->empty())
   {
     break;
   }

/*
   t_apRetStruct aprs;
   aprs = *arucoPerspective(imageUndistorted, false);

   qDebug()<< "gantry wo aprs error "<<aprs.error;

   cv::imshow("gantry wo flat", aprs.imageFlat);
   cv::waitKey(1);
*/
   std::vector<int> ids;
   std::vector<std::vector<cv::Point2f> > corners;
   params.cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
   cv::aruco::ArucoDetector detector(dictionary, params);
   detector.detectMarkers(*imageUndistorted, corners, ids);

   // if at least one marker detected
   if (ids.size() > 0) cv::aruco::drawDetectedMarkers(*imageUndistorted, corners, ids);


   std::vector<cv::Point2d> imagePts;
   std::vector<cv::Point3d> worldPts;


   for(size_t i=0; i<ids.size(); i++)
   {
     if(ids[i] == 1) imagePts.push_back(corners[i][0]);
     if(ids[i] == 1) imagePts.push_back(corners[i][1]);
     if(ids[i] == 1) imagePts.push_back(corners[i][2]);
     if(ids[i] == 1) imagePts.push_back(corners[i][3]);

     if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+10*s,os+10*s,0));
     if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+50*s,os+10*s,0));
     if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+50*s,os+50*s,0));
     if(ids[i] == 1) worldPts.push_back(cv::Point3d(os+10*s,os+50*s,0));

     if(ids[i] == 2) imagePts.push_back(corners[i][0]);
     if(ids[i] == 2) imagePts.push_back(corners[i][1]);
     if(ids[i] == 2) imagePts.push_back(corners[i][2]);
     if(ids[i] == 2) imagePts.push_back(corners[i][3]);

     if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+90*s,os+10*s,0));
     if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+130*s,os+10*s,0));
     if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+130*s,os+50*s,0));
     if(ids[i] == 2) worldPts.push_back(cv::Point3d(os+90*s,os+50*s,0));

     if(ids[i] == 3) imagePts.push_back(corners[i][0]);
     if(ids[i] == 3) imagePts.push_back(corners[i][1]);
     if(ids[i] == 3) imagePts.push_back(corners[i][2]);
     if(ids[i] == 3) imagePts.push_back(corners[i][3]);

     if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+90*s,os+90*s,0));
     if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+130*s,os+90*s,0));
     if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+130*s,os+130*s,0));
     if(ids[i] == 3) worldPts.push_back(cv::Point3d(os+90*s,os+130*s,0));


     if(ids[i] == 4) imagePts.push_back(corners[i][0]);
     if(ids[i] == 4) imagePts.push_back(corners[i][1]);
     if(ids[i] == 4) imagePts.push_back(corners[i][2]);
     if(ids[i] == 4) imagePts.push_back(corners[i][3]);

     if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+10*s,os+90*s,0));
     if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+50*s,os+90*s,0));
     if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+50*s,os+130*s,0));
     if(ids[i] == 4) worldPts.push_back(cv::Point3d(os+10*s,os+130*s,0));

   }


   //find the reference image transform and center of markers
   if(!haveTransform)
   {
     H = findHomography(imagePts, worldPts);
     cv::Mat imageRef;
     cv::warpPerspective(*imageUndistorted, imageRef, H, imageRef.size());
     std::vector<cv::Point2d> refPoints;
     refPoints = findMarkerPoints(&imageRef);
     refCenter = lineLineIntersection(refPoints[0], refPoints[10],refPoints[5],refPoints[15]);
     haveTransform = true;

     cv::circle(imageRef, refCenter, 10, cv::Scalar(0,255,0), 1, cv::LINE_AA);
     cv::imshow("imageRef", imageRef);
     cv::waitKey(0);
   }

   if(imagePts.size() ==16)
   {

     cv::Mat imageOut;
     if(!imageUndistorted->empty() && !H.empty())
     cv::warpPerspective(*imageUndistorted, imageOut, H, imageOut.size());
     std::vector<cv::Point2d> acqPoints;
     acqPoints = findMarkerPoints(&imageOut);


     cv::Rect ROI(0, 0, 2000, 2000);

     cv::Mat imageFlat = imageOut(ROI);


     if(acqPoints.size() == 16)
     {


       //put angle measurement HERE


       cv::Point2d walkoutPoint = lineLineIntersection(acqPoints[0], acqPoints[10],acqPoints[5],acqPoints[15]);

       double woVecLen =    sqrt( pow(refCenter.x - walkoutPoint.x,2) + pow(refCenter.y - walkoutPoint.y,2) );
       double markerWidth = sqrt( pow(acqPoints[0].x - acqPoints[1].x,2) + pow(acqPoints[0].y - acqPoints[1].y,2) );

       std::string woSt = std::to_string(woVecLen/s);
       std::string mwSt = std::to_string(markerWidth/s);

       cv::putText(imageFlat, woSt, cv::Point2d(1000,1000), 0, 1.5, cv::Scalar(255,0,0), 3, cv::LINE_8);
       cv::putText(imageFlat, mwSt, cv::Point2d(1000,1050), 0, 1.5, cv::Scalar(255,0,0), 3, cv::LINE_8);

       cv::circle(imageFlat, refCenter, 10, cv::Scalar(255,255,255), 1, cv::LINE_AA);
       cv::circle(imageFlat, walkoutPoint, 8, cv::Scalar(0,0,0), 1, cv::LINE_AA);
       cv::line(imageFlat, acqPoints[0], acqPoints[10], cv::Scalar(255,255,255), 1, cv::LINE_AA);
       cv::line(imageFlat, acqPoints[5], acqPoints[15], cv::Scalar(255,255,255), 1, cv::LINE_AA);
     }
     if(!imageOut.empty())
     {
       cv::imshow("out", imageFlat);
       cv::waitKey(1);
     }
   }
}

}





/*-------------------------------------------------------------------------------------------
 * findODI replaces findODIDistance
 *
 * error -1 = acquired image was empty
 * error -2 = aruco displacement wasn't 90,100 or 110
 * error -3 = there were not three ticks found above the crosshair
 * error -4 = there were not three ticks found below the crosshair
 * error -5 = no transform matrix
 * error -6 = there were not four markers
-------------------------------------------------------------------------------------------*/
FieldDetect::t_findODIDistanceReturnStruct FieldDetect::findODI()
{

  t_findODIDistanceReturnStruct fodirs;

  cv::Mat undistortedImage = *getImage();
  if(undistortedImage.empty()){
      fodirs.error = -1;
      return fodirs;
  }

  t_findArucoDisplacementReturnStruct adrs = findArucoDisplacement(&undistortedImage);
  qDebug()<<"aruco displacement z "<<adrs.disp.z << " aruco displacement error "<<adrs.error;


  //Perspective transform the image
  cv::Mat flatImage;
  cv::warpPerspective(undistortedImage, flatImage, referenceStruct.H, undistortedImage.size());

  //detect the aruco markers - there has to be four
  std::vector<std::vector<cv::Point2f>> corners, sortedCorners, rejectedCandidates;
  std::vector<int> ids;
  params.cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
  cv::aruco::ArucoDetector detector(dictionary, params);
  detector.detectMarkers(undistortedImage, corners, ids, rejectedCandidates);

  std::vector<cv::Point2d> imagePoints;

  //sort the markers by id number
  std::vector<std::pair<int, int>> markerIDCornerIndex;
  for(uint i=0; i<corners.size(); i++)
    markerIDCornerIndex.push_back(std::make_pair(ids.at(i),i));
  std::sort(markerIDCornerIndex.begin(), markerIDCornerIndex.end());

  if(ids.size() ==4)
  {
    for(uint i=0; i<markerIDCornerIndex.size(); i++)
    {
      imagePoints.push_back(corners[markerIDCornerIndex[i].second][0]);
      imagePoints.push_back(corners[markerIDCornerIndex[i].second][1]);
      imagePoints.push_back(corners[markerIDCornerIndex[i].second][2]);
      imagePoints.push_back(corners[markerIDCornerIndex[i].second][3]);
    }
  }
  else{
    fodirs.error = -6;
    return fodirs;
  }

  //perspective transform the points - this can't be done on a flat image because the top markers are not visible after being flattened
  std::vector<cv::Point2d> transformedImagePoints;
  if(!referenceStruct.H.empty())
    cv::perspectiveTransform(imagePoints, transformedImagePoints, referenceStruct.H);
  else{
    fodirs.error = -5;
    return fodirs;
  }


  for(uint j=0; j<imagePoints.size(); j++)
  {
    cv::circle(flatImage, transformedImagePoints[j], j+1, cv::Scalar(255,255,255),2, LINE_AA);
  }

  cv::Point2d arucoCenter = lineLineIntersection(transformedImagePoints[2],transformedImagePoints[8], transformedImagePoints[7], transformedImagePoints[13] );

  cv::Rect2d markerBoundsRect;
  double Y[6];  //number of y values
  double upperSpacingTest=0, lowerSpacingTest=0;

  if(adrs.disp.z > -105.0  && adrs.disp.z < -95.0){
    markerBoundsRect = Rect2d(arucoCenter.x-70, arucoCenter.y-320, 50, 525); //90
    Y[0]=84,Y[1]=86,Y[2]=88,Y[3]=92,Y[4]=94,Y[5]=96;
    upperSpacingTest = 45;
    lowerSpacingTest = 35;
    qDebug()<<"setup for 90 SSD measurement";
  }
  else
  if(adrs.disp.z > -5.0  && adrs.disp.z < 5.0){
    markerBoundsRect = Rect2d(arucoCenter.x-70, arucoCenter.y-170, 50, 290);//100
    Y[0]=94,Y[1]=96,Y[2]=98,Y[3]=102,Y[4]=104,Y[5]=106;
    upperSpacingTest = 25;
    lowerSpacingTest = 22;
    qDebug()<<"setup for 100 SSD measurement";
  }
  else
  if(adrs.disp.z > 95.0  && adrs.disp.z < 105.0){
    markerBoundsRect = Rect2d(arucoCenter.x-70, arucoCenter.y-110, 50, 185);//110
    Y[0]=104,Y[1]=106,Y[2]=108,Y[3]=112,Y[4]=114,Y[5]=116;
    upperSpacingTest = 15;
    lowerSpacingTest = 14;
    qDebug()<<"setup for 110 SSD measurement";
  }
  else{
    fodirs.error = -1;
    return fodirs;
  }



  cv::Rect2d crosshairROI(arucoCenter.x-150, arucoCenter.y-30, 30, 60);
  cv::Mat crossHairImage;
  flatImage.copyTo(crossHairImage);
  crossHairImage = crossHairImage(crosshairROI);
  cv::Point2d crosshairY = FindODI::getCrosshairY(crossHairImage, markerBoundsRect.y - crosshairROI.y);
  cv::Point2d processImageCenter(0, fabs(arucoCenter.y-markerBoundsRect.y)); //this is the center y of the roi image


  qDebug()<<"crosshair at " << crosshairY.y;

  cv::Point2d arucoCenterROI = cv::Point2d(arucoCenter.x - markerBoundsRect.tl().x , arucoCenter.y - markerBoundsRect.tl().y);
  cv::circle(flatImage, cv::Point(arucoCenter.x, arucoCenter.y), 5, cv::Scalar(255,255,255), 2, cv::LINE_AA);

  Mat displayImage;
  flatImage.copyTo(displayImage);
  cvtColor(displayImage, displayImage, COLOR_GRAY2RGB);

  cv::rectangle(displayImage, markerBoundsRect, cv::Scalar(127,127,127),2, LINE_AA);

  //cv::rectangle(displayImage, crosshairROI, cv::Scalar(0,0,0),2);
  cv::circle(displayImage, Point2d(crosshairY.x+markerBoundsRect.x, crosshairY.y+markerBoundsRect.y), 5, cv::Scalar(0,255,0), 2, cv::LINE_AA);

  cv::Mat processImage;
  flatImage(markerBoundsRect).copyTo(processImage);

  cv::Mat normalizedPeaks = FindODI::processImage(processImage);

  if(normalizedPeaks.empty()){
   fodirs.error = -1;
   return fodirs;
  }


  double X[6];  //number of x values
  std::vector<int> ticksAboveCrosshair = FindODI::findGoodTicks(normalizedPeaks, crosshairY, true, upperSpacingTest);
  if(ticksAboveCrosshair.size()==3)
  {
    X[0]=ticksAboveCrosshair[0],X[1]=ticksAboveCrosshair[1],X[2]=ticksAboveCrosshair[2];
  }
  else
  {
    fodirs.error = -3;
    return fodirs;
  }


  std::vector<int> ticksBelowCrosshair = FindODI::findGoodTicks(normalizedPeaks, crosshairY, false, lowerSpacingTest);
  if(ticksAboveCrosshair.size()==3)
  {
  X[3]=ticksBelowCrosshair[0],X[4]=ticksBelowCrosshair[1],X[5]=ticksBelowCrosshair[2];
  }
  else
  {
    fodirs.error = -4;
    return fodirs;
  }




  //development
  cv::circle(displayImage, crosshairY, 2, cv::Scalar(255,255,255), 2, cv::LINE_AA);

  for(uint i=0; i<ticksAboveCrosshair.size(); i++)
  {
    cv::circle(displayImage, cv::Point(markerBoundsRect.x+20, ticksAboveCrosshair[i]+markerBoundsRect.y), 1, cv::Scalar(0,0,255),3, LINE_AA);
    qDebug()<<"i "<< i << " peak index " << ticksAboveCrosshair[i];
  }


  for(uint i=0; i<ticksBelowCrosshair.size(); i++)
  {
    cv::circle(displayImage, cv::Point(markerBoundsRect.x+20, ticksBelowCrosshair[i]+markerBoundsRect.y), 1, cv::Scalar(0,0,255),3, LINE_AA);
    qDebug()<<"i "<< i << " peak index " << ticksBelowCrosshair[i];
  }

  double x = crosshairY.y;
  double C[4];  //number of coefficients (order +1)
  PolyFit<double> (X, Y, 6, 4, C);
  double y = C[3]*pow(x,3) + C[2]*pow(x,2) + C[1]*x + C[0];

  fodirs.distance = y;
  qDebug()<<"calculated ODI distance "<< y;




  fodirs.imageDisplay = displayImage;
  fodirs.error = 0;


  //cv::imshow("display image", processImage);
  //cv::imshow("flat image", flatImage);
  //cv::waitKey(1);

  return fodirs;
}
















