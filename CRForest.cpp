#include "CRForest.h"

void CRForest::learning(){
  // init dataset and patch vector
  //dataSet.resize(conf.ntrees);
  //images.resize(conf.ntrees);
  //vPatches.resize(conf.ntrees);

  char buffer[256];

  //boost::mt19937    gen( conf.ntrees * static_cast<unsigned long>(time(0)) );

  // grow each tree
  // if you want to fix this program multi thread
  // you should change below
  for(int i=0;i < conf.ntrees; ++i){

    std::vector<CDataset> dataSets(0);
    std::vector<std::vector<cv::Mat> > images;
    std::vector<std::vector<cv::Mat> > features;
    std::vector<std::vector<CPatch> > vPatches;

    // reserve memory
    //dataSet.reserve(conf.imagePerTree + 10);
    //images.reserve((conf.imagePerTree + 10) * 3);

    std::cout << "tree number " << i << std::endl;
    // initialize random seed
    //std::cout << "time is " << time(NULL) << std::endl;
    boost::mt19937    gen( i * static_cast<unsigned long>(time(NULL)) );
    
    //load train image list and grand truth
    loadTrainFile(conf, dataSets, gen);

    //for(int p = 0;p < dataSets.size(); ++p)
    //dataSets.at(p).showDataset();

    //create tree
    vTrees.at(i) = new CRTree(conf.min_sample, conf.max_depth, dataSets.at(0).centerPoint.size(),gen);
    
    // load images to mamory
    loadImages(images, dataSets);

    // reserve memory
    // vPatches.at(i).reserve((int)((conf.imagePerTree + 10) * 3 * 
    // 				 (images.at(i).img.at(0).at(0).cols - conf.p_width) * 
    // 				 (images.at(i).img.at(0).at(0).rows - conf.p_height) / conf.stride));

    std::cout << "extracting feature" << std::endl;

    features.resize(0);
   
    for(int j = 0; j < images.size(); ++j){
      std::vector<cv::Mat> tempFeature;
      extractFeatureChannels(images.at(j).at(0), tempFeature);
      tempFeature.push_back(images.at(j).at(1));
      features.push_back(tempFeature);
    }
    std::cout << "allocate memory!" << std::endl;

    // extract patch from image
    extractPatches(vPatches, dataSets, features, gen, conf);
    std::cout << "patch extracted!" << std::endl;
    std::cout << vPatches.at(0).size() << " positive patches extracted" << std::endl;
    std::cout << vPatches.at(1).size() << " negative patches extracted" << std::endl;
    // grow tree
    vTrees.at(i)->growTree(vPatches, 0,0, (float)(vPatches.at(0).size()) / ((float)(vPatches.at(0).size()) + (float)(vPatches.at(1).size())), conf, gen);

    // save tree
    sprintf(buffer, "%s%03d.txt",conf.treepath.c_str(), i + conf.off_tree);
    vTrees.at(i)->saveTree(buffer);
  }
}

// extract patch from images
void CRForest::extractPatches(std::vector<std::vector<CPatch> > &patches,const std::vector<CDataset> dataSet,const std::vector<std::vector<cv::Mat> > &image, boost::mt19937 gen, CConfig conf){
  
  boost::uniform_real<> dst( 0, 1 );
  boost::variate_generator<boost::mt19937&, 
			   boost::uniform_real<> > rand( gen, dst );
  cv::Rect temp;
  CPatch tPatch;

  std::vector<CPatch> posPatch, negPatch;
  
  int pixNum;

  temp.width = conf.p_width;
  temp.height = conf.p_height;

  patches.resize(0);

  posPatch.clear();
  negPatch.clear();

  std::cout << "extracting patch from image" << std::endl;
  std::cout << image.at(0).size() << std::endl;
  for(int l = 0;l < image.size(); ++l){
    for(int j = 0; j < image.at(l).at(0).cols - conf.p_width; j += conf.stride){
      for(int k = 0; k < image.at(l).at(0).rows - conf.p_height; k += conf.stride){
	if(rand() < conf.patchRatio){
	  //for(int i = 0;i < image.img.at(l).size(); ++i){// for every channel	  
	    temp.x = j;
	    temp.y = k;

	    pixNum = 0;

	    //std::cout << image.img.at(l).at(1).cols << std::endl;
	    //std::cout << image.img.at(l).at(1).rows << std::endl;
	    for(int m = j; m < j + conf.p_width; ++m){
	      for(int n = k; n < k + conf.p_height; ++n){
		pixNum += (int)(image.at(l).at(image.at(l).size() - 1).at<ushort>(n, m));
		//std::cout << "depth " << image.at(l).at(32).at<ushort>(n, m) << std::endl;
		//std::cout << image.at(l).at(32) << std::endl;
	      }
	    }
	    //std::cout << "pixNum is " << pixNum << std::endl;
	    
	    //std::cout << image.img.at(l).size() << std::endl;

	    tPatch.setPatch(temp, image.at(l), dataSet.at(l).centerPoint);
	    //std::cout << pixNum << std::endl;
	    if (pixNum > 0){
	      if(pixNum > 500 * conf.p_height * conf.p_width * 0.2)
		posPatch.push_back(tPatch);
	      else
		negPatch.push_back(tPatch);
	    }
	    //}
	}
      }

    }
    pBar(l,dataSet.size(), 50);
  }
  patches.push_back(posPatch);
  patches.push_back(negPatch);
  std::cout << std::endl;
}

void CRForest::extractAllPatches(const CDataset &dataSet, const std::vector<cv::Mat> &image, std::vector<CPatch> &patches) const{

  cv::Rect temp;
  CPatch tPatch;

  temp.width = conf.p_width;
  temp.height = conf.p_height;

  patches.clear();
  //std::cout << "extraction patches!" << std::endl;
  for(int j = 0; j < image.at(0).cols - conf.p_width; j += conf.stride){
    for(int k = 0; k < image.at(0).rows - conf.p_height; k += conf.stride){
	temp.x = j;
	temp.y = k;
	
	tPatch.setPatch(temp, image, dataSet.centerPoint);
	patches.push_back(tPatch);
    }
  }
}

void CRForest::loadForest(){
  char buffer[256];
  for(int i = 0; i < vTrees.size(); ++i){
    sprintf(buffer, "%s%03d.txt",conf.treepath.c_str(),i);
    vTrees[i] = new CRTree(buffer);
  }
}

void CRForest::detection(const CDataset &dataSet, const std::vector<cv::Mat> &image, std::vector<cv::Mat> &vDetectImg) const{

  std::vector<CPatch> patches;
  std::vector<cv::Mat> scaledImage;
  
  std::vector<const LeafNode*> result;

  int xoffset = conf.p_width / 2;
  int yoffset = conf.p_height / 2;
  
  for(int i = 0; i < conf.scales.size(); ++i){
    scaledImage = convertScale(image, conf.scales.at(i));
    extractAllPatches(dataSet, scaledImage, patches);

    result.clear();

    for(int j = 0; j < patches.size(); ++j){
      this->regression(result, patches.at(j));

      // vote for all trees (leafs) 
      for(std::vector<const LeafNode*>::const_iterator itL = result.begin(); itL!=result.end(); ++itL) {

	// To speed up the voting, one can vote only for patches 
	// with a probability for foreground > 0.5
	// 
	// if((*itL)->pfg>0.5) {

	// voting weight for leaf 
	float w = (*itL)->pfg / float( (*itL)->vCenter.size() * result.size() );

	// vote for all points stored in the leaf
	for(std::vector<std::vector<cv::Point> >::const_iterator it = (*itL)->vCenter.begin(); it!=(*itL)->vCenter.end(); ++it) {

	  for(int c = 0; c < vDetectImg.size(); ++c) {
	    int x = int(xoffset - (*it)[0].x * conf.ratios[c] + 0.5);
	    int y = yoffset-(*it)[0].y;
	    if(y>=0 && y<vDetectImg.at(c).rows && x>=0 && x<vDetectImg[c].cols) {
	      
	      vDetectImg.at(c).at<uchar>(x,y) = w;
	      //*(ptDet[c]+x+y*stepDet) += w;
	    }
	  }
	}

	// } // end if

      }

    } // for every patch

    // smooth result image
    //for(int c=0; c<(int)vDetectImg.size(); ++c)
    //  cvSmooth( vDetectImg[c], vDetectImg[c], CV_GAUSSIAN, 3, 0, 0, 0);

  } // for every scale
}

// Regression 
void CRForest::regression(std::vector<const LeafNode*>& result, CPatch &patch) const{
  result.resize( vTrees.size() );
  for(int i=0; i<(int)vTrees.size(); ++i) {
    result[i] = vTrees[i]->regression(patch);
  }
}

void CRForest::loadImages(std::vector<std::vector<cv::Mat> > &img, std::vector<CDataset> dataSet){
  img.resize(0);

  cv::Mat rgb,depth, mask;
  std::vector<cv::Mat> planes;
  std::vector<cv::Mat> allImages;
  //std::vector<cv::Mat> rgbSplited;


  std::cout << dataSet.at(0).depthImageName << std::endl;
  
  for(int i = 0;i < dataSet.size(); ++i){
    // load Mask image
    mask = cv::imread(dataSet.at(0).imageFilePath
		      + dataSet.at(0).maskImageName,
		      CV_LOAD_IMAGE_ANYCOLOR);
    
    // load RGB image
    rgb = cv::imread(dataSet.at(0).imageFilePath
		     + dataSet.at(0).rgbImageName,
		     CV_LOAD_IMAGE_ANYCOLOR);

    // load Depth image
    depth = cv::imread(dataSet.at(0).imageFilePath
		       + dataSet.at(0).depthImageName,
		       CV_LOAD_IMAGE_ANYDEPTH);

    //std::cout << depth << std::endl;
    


    for(int k = 0;k < rgb.cols; ++k)
      for(int l = 0;l < rgb.rows; ++l){
    	//std::cout << depth.at<ushort>(l, k) << " " << std::endl;
    	if(!(bool)mask.at<char>(l, k))
    	  depth.at<ushort>(l, k) = 0;
    	// for(int j = 0;j < 3; ++j)
    	//   if(!(bool)mask.at<char>(l, k))
    	//     rgb.at<cv::Vec3b>(l, k)[j] = 0;
      }
    //rgbSplited.resize(rgb.channels());
    
    //cv::split(rgb, rgbSplited);
    
    allImages.clear();
    allImages.push_back(rgb);
    allImages.push_back(depth);
    img.push_back(allImages);
  }
}
