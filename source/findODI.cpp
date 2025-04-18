#include <findODI.h>
#include <QtCore>
#include <PeakFinder.h>
#include <fstream>
#include <cmath>
#include <algorithm>    // IWYU pragma: keep
#include <tuple>        // IWYU pragma: keep






/*-------------------------------------------------------------------------------------------------------
 *
 *
 *
 *
 *
 -------------------------------------------------------------------------------------------------------*/
Mat FindODI::processImage(Mat processImage)
{

  Scalar m;
  m = mean(processImage);
  double originalMean = m[0];
  qDebug()<<"ORIGINAL MEAN "<<m[0];
  Mat adjustedImage;
  uint subtractionValue=0;

  //imshow("ORIGINAL mean", processImage);
  //waitKey(1);

  //incrementally add or subtract 1 to the image until the mean is 130
  for(uint i=0; i<256; i++){
    if(originalMean < 130)
      adjustedImage = processImage + i;
    else
      adjustedImage = processImage - i;
    m = mean(adjustedImage);
    qDebug()<<"subtraction value " << i<< " Adjusted MEAN "<<m[0];
    if( m[0]> 129 && m[0] < 131){
        subtractionValue = i;
        break;
    }
 }

  qDebug()<<"FINAL ADJUSTMENT " << subtractionValue<< " Adjusted MEAN "<<m[0];

  //imshow("ADJUSTED mean", adjustedImage);
  //waitKey(1);

  adjustedImage = adjustedImage*1.25;  //amplify the image to help detect smaller tick marks

  Mat reducedImage;
  reduce(adjustedImage, reducedImage, 1, cv::REDUCE_SUM, CV_64FC1);
  //writeCSV("..//..//config//_reduced.csv", reducedImage);

  Mat normalized;
  normalize(reducedImage, normalized, 1.0, 0.0, cv::NORM_MINMAX);
  //writeCSV("..//..//config//normalized.csv", normalized);


  return normalized;

}




/*-------------------------------------------------------------------------------------------------------
 *
 *
 *
 *
 *
 -------------------------------------------------------------------------------------------------------*/
vector<int> FindODI::findGoodTicks(Mat normalizedImage, Point2d crosshairY, bool ticksAreAboveCrosshair, double tickSpacingTest)
{
  vector<float> peaks;
  vector<int> clippedPeaks,goodPeaks;
  Mat mask,clipped;
  Point2d roiUpperLeftCorner;
  Point2d roiLowerRightCorner;
  vector<int> noPeaks;

  if(ticksAreAboveCrosshair)
  {
    roiUpperLeftCorner = Point2d(0,0);
    roiLowerRightCorner = Point2d(crosshairY.x, crosshairY.y-10);   //mask out the crosshair by reducing the roi by 10
  }
  else
  {
    roiUpperLeftCorner = Point2d(crosshairY.x, crosshairY.y+10);    //mask out the crosshair by reducing the roi by 10
    roiLowerRightCorner = Point2d(0,normalizedImage.rows);
  }

  Mat fullImage = Mat::zeros(normalizedImage.size(), normalizedImage.type());
  Rect2d ROI(roiUpperLeftCorner, roiLowerRightCorner);
  Mat halfImage = normalizedImage(ROI);
  halfImage.copyTo(fullImage(ROI));

  string ul;
  if(ticksAreAboveCrosshair)  ul = "u";
  if(!ticksAreAboveCrosshair) ul = "l";



  Mat filteredTicks = getFilteredTicks(fullImage, ul);

  PeakFinder::findPeaks(filteredTicks, clippedPeaks, true);

  //make five passes to check for and erase a tick if it was detected at the crosshair (within 5 pixels of the CH)
  //TODO - figure out how to only do it for the number of close ticks
  for(uint i=0; i<5; i++)
  {
    for(size_t i=0; i<clippedPeaks.size(); i++)
    {
      if(fabs(crosshairY.y -  clippedPeaks[i]) < 5)
      {
        qDebug()<<"removing point "<< clippedPeaks[i]<< " near crosshair which is at "<<crosshairY.y;
        clippedPeaks.erase(clippedPeaks.begin()+i);
      }
    }
  }
  qDebug()<<"found peaks minus crosshair (if found) "<< clippedPeaks;

  if(clippedPeaks.size() > 2)
  {
    if(ticksAreAboveCrosshair)
      for(size_t i=clippedPeaks.size()-3; i<clippedPeaks.size(); i++)
        goodPeaks.push_back(clippedPeaks[i]);
    else
      for(int i=0; i<3; i++)
        goodPeaks.push_back(clippedPeaks[i]);

    double tickSpacing=0;
    if(ticksAreAboveCrosshair)
        tickSpacing = abs(crosshairY.y-goodPeaks[2]);
    else
        tickSpacing = abs(crosshairY.y-goodPeaks[0]);

    if(tickSpacing < tickSpacingTest) return noPeaks;


    qDebug()<<"tick spacing "<<tickSpacing;


    return goodPeaks;
  }


 return noPeaks;

}




/*-------------------------------------------------------------------------------------------------------
 *
 *
 *
 *
 *
 -------------------------------------------------------------------------------------------------------*/
Mat FindODI::getFilteredTicks(Mat zeroFilledHalfImage, string ul)
{
    vector<int> detectedPeaks;

    Mat normalized;
    normalize(zeroFilledHalfImage, normalized, 1.0, 0.0, cv::NORM_MINMAX);
    //writeCSV("..//..//config//_normalized.csv", normalized);


    vector<float> normalizedVector;
    normalized.col(0).copyTo(normalizedVector);

    // y[i] := ß * x[i] + (1-ß) * y[i-1]
    vector<float> filteredVector;
    float beta = 1.5;
    for(size_t i=1; i<normalizedVector.size(); i++)
    {
      float y=(normalizedVector[i] + (1-beta)) * normalizedVector[i-1];
      filteredVector.push_back(y);
    }
    //insert an element to compensate for filter phase shift
    if(filteredVector.size()>0)
      filteredVector.insert(filteredVector.begin(), 0);

    Mat filteredMat = Mat(Size(1, filteredVector.size()), CV_32FC1, (void*)&filteredVector[0]);

    Mat normalizedFiltered;
    normalize(filteredMat, normalizedFiltered, 1.0, 0.0, cv::NORM_MINMAX);

    //writeCSV("..//..//config//_normalized_filtered_mat_" + ul + ".csv", normalizedFiltered);


    Mat mask, clampedImage;
    inRange(normalizedFiltered,Scalar(0.4),Scalar(1.0),mask);
    normalizedFiltered.copyTo(clampedImage,mask);
    //writeCSV("..//..//config//_clamped_" + ul + ".csv", clampedImage);


    //return normalizedFiltered;
    return clampedImage;
}





/*-------------------------------------------------------------------------------------------------------
 *
 *  finds the crosshair in the roi and returns the y positon
 *
 *
 *
 -------------------------------------------------------------------------------------------------------*/
Point2d FindODI::getCrosshairY(Mat roiImage, double imageYMinusCrosshairY)
{

    cv::Mat reducedImage;
    vector<int> peak;

    roiImage = 255-roiImage;  //invert it so the crosshair is the maxima
    cv::reduce(roiImage, reducedImage, 1, cv::REDUCE_SUM, CV_32F);

    cv::Mat normalized;
    cv::normalize(reducedImage, normalized, 1.0, 0.0, cv::NORM_MINMAX);

    //cv::Mat mask,clipped;
    //inRange(normalized,cv::Scalar(0.50),cv::Scalar(1),mask);
    //normalized.copyTo(clipped,mask);

    PeakFinder::findPeaks(normalized, peak, false);

    int a = 0;
    for(uint i = 0; i<peak.size(); i++)
      a += peak[i];

    int averageYLocation = a/peak.size();

    //writeCSV("..//..//config//crosshair_normalized.csv", normalized);
    //writeCSV("..//..//config//crosshair_clipped.csv", clipped);
    //imshow("crosshair", roiImage);
    //waitKey(1);

  return Point(1, fabs(imageYMinusCrosshairY)+averageYLocation);
}





/*-------------------------------------------------------------------------------------------------------
 *
 *
 *
 *
 *
 -------------------------------------------------------------------------------------------------------*/
/*!
 *  The PolyFit() function uses Gaussian elimination with backward substitution.
 *  Code from Army Research Labs
 */
template<class T>void FindODI::PolyFit(//<=========FITS A POLYNOMIAL TO A SET OF POINTS
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





/*-------------------------------------------------------------------------------------------------------
 *  for development
 *
 *
 *
 *
 -------------------------------------------------------------------------------------------------------*/
void FindODI::writeCSV(std::string filename, cv::Mat m)
{
  std::ofstream myfile;
  myfile.open(filename.c_str());
  myfile<< cv::format(m, cv::Formatter::FMT_CSV) << std::endl;
  myfile.close();
}





















