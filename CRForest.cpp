#include <boost/timer.hpp>
#include "CRForest.h"

void CRForest::learning(){
    // grow each tree
    // if you want to fix this program multi thread
    // you should change below
    for(int i = 0;i < conf.ntrees; ++i){
        growATree(i);
    } // end tree loop
}

void CRForest::growATree(const int treeNum){
    std::vector<CDataset> dataSets(0);
    cv::vector<cv::vector<cv::Mat *> > images;
    cv::vector<cv::vector<cv::Mat *> > features;
    std::vector<std::vector<CPatch> > vPatches;
    cv::vector<cv::Mat *> tempFeature;

    char buffer[256];

    // reserve memory
    //dataSet.reserve(conf.imagePerTree + 10);
    //images.reserve((conf.imagePerTree + 10) * 3);

    std::cout << "tree number " << treeNum << std::endl;
    // initialize random seed
    //std::cout << "time is " << time(NULL) << std::endl;
    boost::mt19937    gen( treeNum * static_cast<unsigned long>(time(NULL)) );

    boost::timer t;

    loadTrainFile(conf, dataSets);//, gen);
    std::cout << "dataset loaded" << std::endl;

    //for(int k = 0; k < dataSets.size(); ++k)
    //  dataSets.at(k).showDataset();
    
    // initialize class database
    classDatabase.clear();

    // create class database
    for(int p = 0;p < dataSets.size(); ++p){
        //      cv::Size tempSize;
        //      tempSize.width = dataSets.at(p).centerPoint.at(0).x * 2;
        //      tempSize.height = dataSets.at(p).centerPoint.at(0).y * 2;

        cv::Mat tempImage = cv::imread(dataSets.at(p).imageFilePath
                                       + dataSets.at(p).rgbImageName,3);

        cv::Size tempSize = cv::Size(tempImage.cols, tempImage.rows);

        classDatabase.add(dataSets.at(p).className,tempSize,0);
    }
    classDatabase.show();

    //create tree
    //vTrees.at(treeNum) = new CRTree(conf.min_sample, conf.max_depth, dataSets.at(0).centerPoint.size(),gen);

    CRTree *tree = new CRTree(conf.min_sample, conf.max_depth, dataSets.at(0).centerPoint.size(),gen);
    std::cout << "tree created" << std::endl;
    

    // load images to mamory
    loadImages(images, dataSets);

    std::cout << dataSets.at(0).centerPoint.size() << std::endl;

    std::cout << "extracting feature" << std::endl;

    features.resize(0);

    for(int j = 0; j < images.size(); ++j){
        //std::cout << "extruct " << j << std::endl;
        tempFeature.clear();

        // cv::namedWindow("test");
        // cv::imshow("test",*(images.at(j).at(0)));
        // cv::waitKey(0);
        // cv::destroyWindow("test");

        //extract features
        extractFeatureChannels(images.at(j).at(0), tempFeature);
        // add depth image to features
        tempFeature.push_back(images.at(j).at(1));
        features.push_back(tempFeature);
    }

    std::cout << "feature extructed!" << std::endl;

    for(int i = 0; i < images.size(); ++i){
        //std::cout << "releasing image number " << i << std::endl;
        delete images.at(i).at(0);
    }

    //images.clear();
    //std::cout << "images" << images.size() << std::endl;

    // extract patch from image
    //std::cout << "extruction patch from features" << std::endl;
    extractPatches(vPatches, dataSets, features, conf);
    //std::cout << "patch extracted!" << std::endl;

    std::vector<int> patchClassNum(classDatabase.vNode.size(), 0);

    for(int j = 0; j < vPatches.at(0).size(); ++j)
        patchClassNum.at(vPatches.at(0).at(j).classNum)++;

    //for(int c = 0; c < classDatabase.vNode.size(); ++c)
    //std::cout << patchClassNum.at(c) << std::endl;
    

    // grow tree
    //vTrees.at(treeNum)->growTree(vPatches, 0,0, (float)(vPatches.at(0).size()) / ((float)(vPatches.at(0).size()) + (float)(vPatches.at(1).size())), conf, gen, patchClassNum);
    tree->growTree(vPatches, 0,0, (float)(vPatches.at(0).size()) / ((float)(vPatches.at(0).size()) + (float)(vPatches.at(1).size())), conf, gen, patchClassNum);

    // save tree
    sprintf(buffer, "%s%03d.txt",
            conf.treepath.c_str(), treeNum + conf.off_tree);
    std::cout << "tree file name is " << buffer << std::endl;
    //vTrees.at(treeNum)->saveTree(buffer);
    tree->saveTree(buffer);

    // save class database
    sprintf(buffer, "%s%s%03d.txt",
            conf.treepath.c_str(),
            conf.classDatabaseName.c_str(), treeNum + conf.off_tree);
    std::cout << "write tree data" << std::endl;
    classDatabase.write(buffer);

    double time = t.elapsed();

    std::cout << "tree " << treeNum << " calicuration time is " << time << std::endl;

    sprintf(buffer, "%s%03d_timeResult.txt",conf.treepath.c_str(), treeNum + conf.off_tree);
    std::fstream lerningResult(buffer, std::ios::out);
    if(lerningResult.fail()){
        std::cout << "can't write result" << std::endl;
    }

    lerningResult << time << std::endl;

    lerningResult.close();

    delete tree;

    // std::vector<CDataset> dataSets(0);
    // cv::vector<cv::vector<cv::Mat> > images;
    // cv::vector<cv::vector<cv::Mat> > features;
    // std::vector<std::vector<CPatch> > vPatches;
    dataSets.clear();

    //vPatches.clear();
    //std::cout << "vPatches" << vPatches.size() << std::endl;




    for(int i = 0; i < features.size(); ++i){
        if(features.at(i).size() != 0){
            for(int j = 0; j < features.at(i).size(); ++j){
                //if(features.at(i).size != NULL)
                //std::cout << "feature keshiteru " << i << " "<< j << std::endl;
                delete features.at(i).at(j);
            }
        }
    }
    features.clear();
    std::cout << "features" << features.size() << std::endl;

    //delete vTrees.at(treeNum);
}

// extract patch from images
void CRForest::extractPatches(std::vector<std::vector<CPatch> > &patches,const std::vector<CDataset> dataSet,const cv::vector<cv::vector<cv::Mat*> > &image,  CConfig conf){
    cv::Rect temp;
    CPatch tPatch;

    std::vector<CPatch> tPosPatch, posPatch, negPatch;
    std::vector<std::vector<CPatch> > patchPerClass(classDatabase.vNode.size());
    int pixNum;
    nCk nck;

    int classNum = 0;

    temp.width  = conf.p_width;
    temp.height = conf.p_height;

    patches.resize(0);

    tPosPatch.clear();
    posPatch.clear();
    negPatch.clear();

    std::cout << "image num is " << image.size();

    std::cout << "extracting patch from image" << std::endl;
    std::cout << image.at(0).size() << std::endl;
    for(int l = 0;l < image.size(); ++l){
        //tPosPatch.clear();
        for(int j = 0; j < image.at(l).at(0)->cols - conf.p_width; j += conf.stride){
            for(int k = 0; k < image.at(l).at(0)->rows - conf.p_height; k += conf.stride){
                //if(rand() < conf.patchRatio){
                //for(int i = 0;i < image.img.at(l).size(); ++i){// for every channel

                //std::cout << "x = " << j << " y = " << k << std::endl;
                temp.x = j;
                temp.y = k;

                pixNum = 0;

                //std::cout << image.img.at(l).at(1).cols << std::endl;
                //std::cout << image.img.at(l).at(1).rows << std::endl;

                // detect negative patch
                for(int m = j; m < j + conf.p_width; ++m){
                    for(int n = k; n < k + conf.p_height; ++n){
                        pixNum += (int)(image.at(l).at(image.at(l).size() - 1)->at<ushort>(n, m));
                    }
                }

                classNum = classDatabase.search(dataSet.at(l).className);



                //std::cout << classNum << std::endl;

                if(classNum == -1){
                    std::cout << "class not found!" << std::endl;
                    exit(-1);
                }

                // std::cout << "this is for debug" << std::endl;
                // std::cout << temp << std::endl;
                // std::cout << dataSet.at(l).centerPoint << std::endl;

                tPatch.setPatch(temp, image.at(l), dataSet.at(l), classNum);
                //for(int q = 0; q < tPosPatch.size(); ++q){
                // cv::namedWindow("test");
                // cv::imshow("test",(*(tPatch.patch.at(0)))(tPatch.patchRoi));
                // cv::waitKey(0);
                // cv::destroyWindow("test");

                //std::cout << pixNum << std::endl;
                //}
                //std::cout << pixNum << std::endl;
                if (pixNum > 0){
                    //if(pixNum > 500 * conf.p_height * conf.p_width * 0.2)
                    tPosPatch.push_back(tPatch);
                    patchPerClass.at(classNum).push_back(tPatch);
                    // std::cout << "this is for debug patch center point is :" << std::endl;
                    // std::cout << tPatch.center << std::endl;

                    //else
                    //negPatch.push_back(tPatch);
                } // if
                //}
                //}
            }//x
        }//y
        pBar(l,dataSet.size(), 50);
    }//allimages
    //int totalPatchNum = (int)(((double)(image.at(l).at(0).cols - conf.p_width) / (double)conf.stride) * ((double)(image.at(l).at(0).rows - conf.p_height) / (double)conf.stride));

    //std::cout << "total patch num is " << totalPatchNum << std::endl;
    //std::cout << tPosPatch.size() << std::endl;

    // for(int q = 0; q < tPosPatch.size(); ++q){
    //   cv::namedWindow("test");
    //   cv::imshow("test",(*(tPosPatch.at(q).patch.at(0)))(tPosPatch.at(q).patchRoi));
    //   cv::waitKey(0);
    //   cv::destroyWindow("test");
    // }

    for(int i = 0; i < patchPerClass.size(); ++i){
        if(patchPerClass.at(i).size() > conf.patchRatio){

            std::set<int> chosenPatch = nck.generate(patchPerClass.at(i).size(), conf.patchRatio);//totalPatchNum * conf.patchRatio);

            //std::cout << "keisan deketa" << std::endl;

            std::set<int>::iterator ite = chosenPatch.begin();

            //cv::namedWindow("test");
            //cv::imshow("test",image.at(l).at(0));
            //cv::waitKey(0);
            //cv::destroyWindow("test");w


            //std::cout << "patch torimasu" << std::endl;

            //std::cout << "tPosPatch num is " << tPosPatch.size() << std::endl;


            while(ite != chosenPatch.end()){
                //std::cout << "this is for debug ite is " << tPosPatch.at(*ite).center << std::endl;
                posPatch.push_back(patchPerClass.at(i).at(*ite));
                ite++;
            }

        }else{
            std::cout << "can't extruct enough patch" << std::endl;
        }
    }
    //std::cout << "kokomade kimashita" << std::endl;
    //tPosPatch.clear();
    //
    patches.push_back(posPatch);
    patches.push_back(negPatch);

    std::cout << std::endl;
}



void CRForest::extractAllPatches(const CDataset &dataSet, const cv::vector<cv::Mat*> &image, std::vector<CPatch> &patches) const{

    cv::Rect temp;
    CPatch tPatch;

    temp.width = conf.p_width;
    temp.height = conf.p_height;

    patches.clear();
    //std::cout << "extraction patches!" << std::endl;
    for(int j = 0; j < image.at(0)->cols - conf.p_width; j += conf.stride){
        for(int k = 0; k < image.at(0)->rows - conf.p_height; k += conf.stride){
            temp.x = j;
            temp.y = k;

            //std::cout << dataSet.className << std::endl;

            int classNum = classDatabase.search(dataSet.className);

            //std::cout << "class num is " << classNum << std::endl;
            //classDatabase.show();
            if(classNum == -1){
                //std::cout << "This tree not contain this class data" << std::endl;
                //exit(-1);
            }

            tPatch.setPatch(temp, image, dataSet, classNum);

            tPatch.setPosition(j,k);
            patches.push_back(tPatch);
        }
    }
}

void CRForest::loadForest(){
    char buffer[256];
    for(int i = 0; i < vTrees.size(); ++i){
        sprintf(buffer, "%s%03d.txt",conf.treepath.c_str(),i);
        vTrees[i] = new CRTree(buffer);
        sprintf(buffer, "%s%s%03d.txt", conf.treepath.c_str(), conf.classDatabaseName.c_str(), i);
        classDatabase.read(buffer);
    }
}

// name   : detect function
// input  : image and dataset
// output : classification result and detect picture
void CRForest::detection(const CDataset &dataSet,
                         const cv::vector<cv::Mat*> &image) const{//,
    //std::vector<double> &detectionResult,
    //int &detectClass) const{

    // 変数の宣言
    //contain class number
    int classNum = classDatabase.vNode.size();
    std::vector<CPatch> patches;
    cv::vector<cv::Mat*> features;
    std::vector<const LeafNode*> result;
    //std::vector<double> classification_result(classNum, 0);

    cv::vector<cv::Mat> outputImage(classNum);//, image.at(0)->clone());
    cv::vector<cv::Mat> outputImageColorOnly(classNum);//,cv::Mat::zeros(image.at(0)->rows,image.at(0)->cols,CV_8UC3));

    for(int i = 0; i < classNum; ++i){
        outputImage.at(i) = image.at(0)->clone();
        outputImageColorOnly.at(i) = cv::Mat::zeros(image.at(0)->rows,image.at(0)->cols,CV_8UC1);
        //        outImage.at(i) = image.at(0)->clone();
        //        outputImageColorOnly.at(i) = new cv::Mat::zeros(image.at(0)->rows,image.at(0)->cols,CV_8UC3);
    }

    // set offset of patchcv::waitKey(0);
    int xoffset = conf.p_width / 2;
    int yoffset = conf.p_height / 2;

    // extract feature from test image
    features.clear();
    extractFeatureChannels(image.at(0), features);

    // add depth image to features
    features.push_back(image.at(1));
    //delete image.at(0);

    // extract patches from features
    extractAllPatches(dataSet, features, patches);

    std::cout << "patch num: " << patches.size() << std::endl;

    boost::timer t;

    // regression for every patch
    for(int j = 0; j < patches.size(); ++j){
        int maxClass = 0;
        std::vector<float> detectionScore(classNum,0);

        result.clear();
        this->regression(result, patches.at(j));



        // vote for all trees (leafs)
        for(std::vector<const LeafNode*>::const_iterator itL = result.begin();
            itL!=result.end(); ++itL) {

            for(int l = 0;l < (*itL)->pfg.size(); ++l){

                // voting weight for leaf
                if((*itL)->pfg.at(l) > 0.5)
                    detectionScore.at(l) += (*itL)->pfg.at(l) * (*itL)->vCenter.at(l).size();

                //classification_result.at(maxClass) += (double) w;
            }

            for(int c = 0; c < classNum; c++){
                //std::cout << "class: " << c << " " << (*itL)->vCenter.at(c).size() << std::endl;
                if(!(*itL)->vCenter.at(c).empty()){
                    if((*itL)->pfg.at(c) > 0.9  ){
                        float weight =  (*itL)->pfg.at(c) / float((*itL)->vCenter.at(c).size() * result.size());

                        for(int l = 0; l < (*itL)->vCenter.at(c).size(); ++l){
                            cv::Point patchSize(conf.p_height/2,conf.p_width/2);
                            //std::cout << weight << std::endl;

                            cv::Point pos = patches.at(j).position + patchSize +  (*itL)->vCenter.at(c).at(l);

                            if(pos.x > 0 && pos.y > 0 &&
                                    pos.x < outputImage.at(c).cols && pos.y < outputImage.at(c).rows &&
                                    (outputImage.at(c).at<uchar>(pos.y,pos.x) + weight * 100) < 254){

                                outputImage.at(c).at<cv::Vec3b>(pos.y,pos.x)[2] += ((*itL)->pfg.at(c) - 0.9) * 100;//+= weight * 500;
                                outputImageColorOnly.at(c).at<uchar>(pos.y,pos.x) += ((*itL)->pfg.at(c) - 0.9) * 100;//weight * 500;

                            }
                        }
                    }
                }
            }


        } // for every leaf
    } // for every patch28512

    double time = t.elapsed();
    std::cout << time << "sec" << std::endl;

    std::cout << 1 / (time / classNum) << "Hz" << std::endl;

    for(int c = 0; c < classNum; ++c){
        cv::namedWindow("test");
        cv::imshow("test",outputImage.at(c));

        cv::namedWindow("test2");
        cv::imshow("test2",outputImageColorOnly.at(c));

        cv::waitKey(0);
        cv::destroyWindow("test");

        std::stringstream cToString;

        cToString << c;

        std::string outputName = "output" + cToString.str() + ".png";
        std::string outputName2 = "outputColorOnly" + cToString.str() + ".png";

        cv::imwrite(outputName.c_str(),outputImage.at(c));
        cv::imwrite(outputName2.c_str(),outputImageColorOnly.at(c));
    }

    std::cout << "detection result outputed" << std::endl;


    //    cv::cvtColor(outputImageColorOnly.at(0),outputImageC1,CV_8UC1);

    //    cv::namedWindow("test");
    //    cv::imshow("test",outputImageC1);
    //    cv::waitKey(0);
    //    cv::destroyWindow("test");

    for(int c = 0; c < classNum; ++c){

        double min,max;

        cv::Point minLoc,maxLoc;

        cv::minMaxLoc(outputImageColorOnly.at(c),&min,&max,&minLoc,&maxLoc);

        //cv::circle(outputImage.at(c),maxLoc,20,cv::Scalar(200,0,0),3);

        cv::Size tempSize = classDatabase.vNode.at(c).classSize;
        cv::Rect_<int> outRect(maxLoc.x - tempSize.width / 2,maxLoc.y - tempSize.height / 2 , tempSize.width,tempSize.height);
        cv::rectangle(outputImage.at(c),outRect,cv::Scalar(0,200,0),3);


        cv::namedWindow("test");
        cv::imshow("test",outputImage.at(c));
        cv::waitKey(0);
        cv::destroyWindow("test");

        std::string outputName = "detectionResult" + classDatabase.vNode.at(c).name + ".png";

        cv::imwrite(outputName.c_str(),outputImage.at(c));

        std::cout << maxLoc << std::endl;

    }

    for(int i = 0; i < features.size() - 1; ++i){
        delete features.at(i);
    }

    //patches.clear();
    features.clear();
}

// Regression 
void CRForest::regression(std::vector<const LeafNode*>& result, CPatch &patch) const{
    result.resize( vTrees.size() );
    //std::cout << "enter regression" << std::endl;
    for(int i=0; i<(int)vTrees.size(); ++i) {
        result[i] = vTrees[i]->regression(patch);
    }
}

void CRForest::loadImages(cv::vector<cv::vector<cv::Mat *> > &img, std::vector<CDataset> &dataSet){
    img.resize(0);

    cv::Mat* rgb,*depth, *mask;
    cv::vector<cv::Mat*> planes;
    cv::vector<cv::Mat*> allImages;
    //cv::vector<cv::Mat> rgbSplited;

    for(int i = 0;i < dataSet.size(); ++i){
        rgb = new cv::Mat();
        depth = new cv::Mat();
        mask = new cv::Mat();

        // load Mask image

        *mask = cv::imread(dataSet.at(i).imageFilePath
                           + dataSet.at(i).maskImageName,3).clone();

        // load RGB image
        *rgb = cv::imread(dataSet.at(i).imageFilePath
                          + dataSet.at(i).rgbImageName,3).clone();

        //std::cout << dataSet.at(i).rgbImageName << " " << rgb->channels() << std::endl;
        // load Depth image
        *depth = cv::imread(dataSet.at(i).imageFilePath
                            + dataSet.at(i).depthImageName,
                            CV_LOAD_IMAGE_ANYDEPTH).clone();
        cv::Point tempPoint;
        tempPoint.x = (*rgb).cols / 2;
        tempPoint.y = (*rgb).rows / 2;

        dataSet.at(i).centerPoint.push_back(tempPoint);

        //cv::namedWindow("test");
        //cv::imshow("test",*rgb);
        //cv::waitKey(0);
        //cv::destroyWindow("test");



        //std::cout << depth << std::endl;



        for(int k = 0;k < rgb->cols; ++k)
            for(int l = 0;l < rgb->rows; ++l){
                //std::cout << depth.at<ushort>(l, k) << " " << std::endl;
                //if(!(bool)mask->at<char>(l, k))
                //depth->at<ushort>(l, k) = 0;
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

        delete mask;
    }

    
}

