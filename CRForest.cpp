#include "CRForest.h"

void CRForest::learning(){
  char buffer[256];

  

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


    for(int p = 0;p < dataSets.size(); ++p){
      //makedataSets.at(p).showDataset();
      classDatabase.add(dataSets.at(p).className);
    }
    classDatabase.show();
    classDatabase.write("test.dat");

    //std::cout << classDatabase.search("bottle") << std::endl;

    //create tree
    vTrees.at(i) = new CRTree(conf.min_sample, conf.max_depth, dataSets.at(0).centerPoint.size(),gen);
    
    // load images to mamory
    loadImages(images, dataSets);

    // reserve memory
    // vPatches.at(i).reserve((int)((conf.imagePerTree + 10) * 3 * 
    // 				 (images.at(i).img.at(0).at(0).cols - conf.p_width) * 
    // 				 (images.at(i).img.at(0).at(0).ros - conf.p_height) / conf.stride));

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

    std::vector<int> patchClassNum(classDatabase.vNode.size(), 0);

    for(int j = 0; j < vPatches.at(0).size(); ++j){
      patchClassNum.at(vPatches.at(0).at(j).classNum)++;
      //std::cout << vPatches.at(0).at(j).classNum << std::endl;
    }

    for(int c = 0; c < classDatabase.vNode.size(); ++c)
      std::cout << patchClassNum.at(c) << std::endl;
    

    // grow tree
    vTrees.at(i)->growTree(vPatches, 0,0, (float)(vPatches.at(0).size()) / ((float)(vPatches.at(0).size()) + (float)(vPatches.at(1).size())), conf, gen, patchClassNum);

    // save tree
    sprintf(buffer, "%s%03d.txt",conf.treepath.c_str(), i + conf.off_tree);
    vTrees.at(i)->saveTree(buffer);
    sprintf(buffer, "%s%03d.txt",conf.classDatabaseName.c_str(), i + conf.off_tree);
    classDatabase.write(buffer);
  } // end tree loop
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

  temp.width  = conf.p_width;
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

	  int classNum = classDatabase.search(dataSet.at(l).className);
	  //std::cout << classNum << std::endl;

	  if(classNum == -1){
	    std::cout << "class not found!" << std::endl;
	    exit(-1);
	  }

	  tPatch.setPatch(temp, image.at(l), dataSet.at(l).centerPoint, classNum);
	  //std::cout << pixNum << std::endl;
	  if (pixNum > 0){
	    //if(pixNum > 500 * conf.p_height * conf.p_width * 0.2)
	      posPatch.push_back(tPatch);
	      //else
	      //negPatch.push_back(tPatch);
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
	
	int classNum = classDatabase.search(dataSet.className);
	//classDatabase.show();
	if(classNum == -1){
	  std::cout << "This tree not contain this class data" << std::endl;
	  //exit(-1);
	}
	
	tPatch.setPatch(temp, image, dataSet.centerPoint, classNum);
	patches.push_back(tPatch);
    }
  }
}

void CRForest::loadForest(){
  char buffer[256];
  for(int i = 0; i < vTrees.size(); ++i){
    sprintf(buffer, "%s%03d.txt",conf.treepath.c_str(),i);
    vTrees[i] = new CRTree(buffer);
    sprintf(buffer, "%s%03d.txt", conf.classDatabaseName.c_str(), i);
    classDatabase.read(buffer);
  }
}

void CRForest::detection(const CDataset &dataSet, const std::vector<cv::Mat> &image) const{
  std::vector<cv::Mat> scaledImage;
  std::vector<CPatch> patches;
  std::vector<cv::Mat> features;
  std::vector<const LeafNode*> result;
  
  std::vector<int> classSum(classDatabase.vNode.size(),0);
  std::vector<double> classification_result(classDatabase.vNode.size(), 0);
  //classSum.resize(classDatabase.vNode.size());

  int xoffset = conf.p_width / 2;
  int yoffset = conf.p_height / 2;

  // cv::namedWindow("test");
  // cv::imshow("test",image.at(0));
  // cv::waitKey(0);
  // cv::destroyWindow("test");

  //this is for debug
  std::vector<int> leafPerClass(classDatabase.vNode.size(), 0);
  std::vector<int> patchPerClass(classDatabase.vNode.size(), 0);

  
  
  for(int i = 0; i < conf.scales.size(); ++i){
    scaledImage = convertScale(image, conf.scales.at(i));
    
    features.resize(0);
    extractFeatureChannels(image.at(0), features);
    
    features.push_back(image.at(1));

    extractAllPatches(dataSet, features, patches);

    
    
    result.clear();

    std::cout << patches.size() << std::endl;
    for(int j = 0; j < patches.size(); ++j){
      this->regression(result, patches.at(j));
      //std::cout << "kokomade" << std::endl;
      // vote for all trees (leafs) 
      for(std::vector<const LeafNode*>::const_iterator itL = result.begin(); itL!=result.end(); ++itL) {

	for(int k = 0; k < classDatabase.vNode.size(); ++k)
	  classSum.at(k) = 0;

	// To speed up the voting, one can vote only for patches 
	// with a probability for foreground > 0.5
	// 
	// if((*itL)->pfg>0.5) {
	
	for(int l = 0; l < (*itL)->pfg.size(); ++l){

	  // vote for all points stored in the leaf
	  for(int k = 0; k < (*itL)->vCenter.size(); ++k){  
	    classSum.at((*itL)->vClass.at(k))++;
	    
	  }

	  
	  for(int c = 0; c < classDatabase.vNode.size(); c++)
	    patchPerClass.at(c) += classSum.at(c);

	  //leafPerClass.at((*itL)->vClass.at(0))++;
 
	  // voting weight for leaf 
	  float w = (*itL)->pfg.at(l) / (float)((float)(*itL)->vCenter.size()/ (*itL)->pfg.size() * result.size() );

	  
	  for(int c = 0; c < classDatabase.vNode.size(); ++c){
	    if(classSum.at(c) != 0)
	      classification_result.at(c) += (double) w;// * ((double)classSum.at(c) / (double)(*itL)->vClass.size());
	  }
	  // } // end if
	}
      } // for every leaf
    } // for every patch

  } // for every scale

  std::cout << dataSet.className << std::endl;
  //std::cout << "result" << std::endl;
  for(int i = 0; i < classSum.size(); ++i){
    std::cout << classDatabase.vNode.at(i).name << " : " << classification_result.at(i) << std::endl;
  }
  std::cout << std::endl;

  // this is for debug
  for(int c = 0; c < classSum.size(); ++c){
    std::cout << "class1: ";
    std::cout << leafPerClass.at(c) << " ";
    std::cout << (float)classification_result.at(c) /  (float)patchPerClass.at(c) << std::endl;
  }
}

// Regression 
void CRForest::regression(std::vector<const LeafNode*>& result, CPatch &patch) const{
  result.resize( vTrees.size() );
  //std::cout << "enter regression" << std::endl;
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

