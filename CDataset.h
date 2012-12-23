#ifndef __CDATASET__
#define __CDATASET__

#include <vector>
#include <iostream>
#include <string>

#include <opencv/highgui.h>

class CDataset {
 public:
  CDataset();
  std::string rgbImageName, depthImageName, maskImageName, className, imageFilePath;
  cv::Rect bBox;
  std::vector<cv::Point> centerPoint;

  double angle;

  void showDataset();
};

#endif
