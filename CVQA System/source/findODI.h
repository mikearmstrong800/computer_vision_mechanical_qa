#ifndef FINDODI_H
#define FINDODI_H



#include <opencv2/opencv.hpp>

using namespace  std;
using namespace  cv;


namespace FindODI{

    Mat processImage(Mat processImage);
    Mat getFilteredTicks(Mat zeroFilledHalfImage, string ul);
    Point2d getCrosshairY(Mat roiImage, double imageYMinusCrosshairY);
    void writeCSV(std::string filename, cv::Mat m);
    vector<int> findGoodTicks(Mat normalizedImage, Point2d crosshairY, bool ticksAreAboveCrosshair, double tickSpacingTest);
    template<class T>void PolyFit(const T*X, const T*Y, int n, int m, T*C);
}



#endif // FINDODI_H
