/*
	Daniel Gallardo Grassot
	daniel.gallardo@upf.edu
	Barcelona 2011

	Licensed to the Apache Software Foundation (ASF) under one
	or more contributor license agreements.  See the NOTICE file
	distributed with this work for additional information
	regarding copyright ownership.  The ASF licenses this file
	to you under the Apache License, Version 2.0 (the
	"License"); you may not use this file except in compliance
	with the License.  You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing,
	software distributed under the License is distributed on an
	"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
	KIND, either express or implied.  See the License for the
	specific language governing permissions and limitations
	under the License.
*/

#include "MarkerFinder.h"
#include "GlobalConfig.h"
#include "Globals.h"


MarkerFinder::MarkerFinder():
	FrameProcessor("MarkerFinder"),
	//enable_processor(datasaver::GlobalConfig::getRef("FrameProcessor:6DoF_fiducial_finder:enable",1)),
	use_adaptive_bar_value(datasaver::GlobalConfig::getRef("FrameProcessor:MarkerFinder:adaptive_threshold:enable",1)),
	block_size (datasaver::GlobalConfig::getRef("FrameProcessor:MarkerFinder:adaptive_threshold:blocksize",47)),
	threshold_C (datasaver::GlobalConfig::getRef("FrameProcessor:MarkerFinder:adaptive_threshold:threshold_C",2)),
	Threshold_value (datasaver::GlobalConfig::getRef("FrameProcessor:MarkerFinder:threshold:threshold_value",33)),
	finder (FiducialFinder(FIDUCIAL_WIN_SIZE))
{
	//init values
	if(use_adaptive_bar_value == 0) use_adaptive_threshold = false;
	else use_adaptive_threshold = true;

	if(enable_processor == 1) Enable(true);
	else Enable(false);

	//create gui
	BuildGui();

	//ssidGenerator = 1;
	//fiducial_finder = new FiducialFinder(FIDUCIAL_IMAGE_SIZE);
	//firstcontour=NULL;
	//polycontour=NULL;
	//InitFrames(Globals::screen);
	InitGeometry();
}

void MarkerFinder::InitGeometry()
{
	fiducial_image = cv::Mat(FIDUCIAL_WIN_SIZE,FIDUCIAL_WIN_SIZE,CV_8UC1);
	//std::cout << "size: " << fiducial_image->width << "  " << fiducial_image->height << std::endl;
	/*dst_pnt[0] = cvPoint2D32f (0, FIDUCIAL_IMAGE_SIZE);
	dst_pnt[1] = cvPoint2D32f (FIDUCIAL_IMAGE_SIZE, FIDUCIAL_IMAGE_SIZE);
	dst_pnt[2] = cvPoint2D32f (FIDUCIAL_IMAGE_SIZE, 0);
    dst_pnt[3] = cvPoint2D32f (0, 0);*/
	dst_pnt[2] = cvPoint2D32f (0, FIDUCIAL_WIN_SIZE);
	dst_pnt[3] = cvPoint2D32f (0, 0);
	dst_pnt[0] = cvPoint2D32f (FIDUCIAL_WIN_SIZE, 0);
    dst_pnt[1] = cvPoint2D32f (FIDUCIAL_WIN_SIZE, FIDUCIAL_WIN_SIZE);
	//map_matrix = cvCreateMat  (3, 3, CV_32FC1);
}

MarkerFinder::~MarkerFinder(void)
{

//	cvReleaseImage(&main_processed_image);	
//	cvReleaseImage(&main_processed_contour);
//	cvReleaseImage(&fiducial_image);	
//	cvReleaseMat (&map_matrix);
//	cvReleaseMemStorage(&main_storage_poligon);
//	cvReleaseMemStorage(&main_storage);
//	delete(fiducial_finder);
}

AliveList MarkerFinder::GetAlive()
{
	AliveList to_return;
//	if(IsEnabled())
//	{
//		for(FiducialMap::iterator it = fiducial_map.begin(); it!= fiducial_map.end(); it++)
//		{
//			to_return.push_back(it->first);
//			//std::cout << " " << it->first;
//		}
//		//std::cout << std::endl;
//	}
//	//std::vector<unsigned long>
	return to_return;
}


void MarkerFinder::Process(cv::Mat&	main_image)
{
	/******************************************************
	* Redraw GUI if needed
	*******************************************************/
	if(use_adaptive_bar_value == 0 && use_adaptive_threshold == true)
	{
		use_adaptive_threshold = false;
		BuildGui(true);
	}
	else if (use_adaptive_bar_value == 1 && use_adaptive_threshold == false)
	{
		use_adaptive_threshold = true;
		BuildGui(true);
	}
	/******************************************************
	* Convert image to graycsale
	*******************************************************/
	if ( main_image.type() ==CV_8UC3 )   cv::cvtColor ( main_image,grey,CV_BGR2GRAY );
	else     grey=main_image;
	/******************************************************
	* Apply threshold
	*******************************************************/
	if(use_adaptive_threshold)
	{
		if (block_size<3) block_size=3;
		if (block_size%2!=1) block_size++;
		cv::adaptiveThreshold ( grey,thres,255,cv::ADAPTIVE_THRESH_GAUSSIAN_C,cv::THRESH_BINARY_INV,block_size,threshold_C );
	}
	else 
		cv::threshold(grey,thres,Threshold_value,255,cv::THRESH_BINARY_INV);
	/******************************************************
	* Find contours
	*******************************************************/
	thres2 = thres.clone();
	double _minSize=0.04;
	double _maxSize=0.5;
	int minSize=_minSize*std::max(thres.cols,thres.rows)*4;
	int maxSize=_maxSize*std::max(thres.cols,thres.rows)*4;
	cv::findContours ( thres2 , contours, hierarchy,CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE );
	cv::vector<cv::Point>  approxCurve;
	/******************************************************
	* Find Squares
	*******************************************************/
	std::vector<candidate> SquareCanditates;
	int idx = 0;
    for( ; idx >= 0; idx = hierarchy[idx][0] )
    {
		//80's effect
		//if ( minSize< contours[idx].size() &&contours[idx].size()<maxSize  )
		//{
        //cv::Scalar color( rand()&255, rand()&255, rand()&255 );
        //drawContours( main_image, contours, idx, color, CV_FILLED, 8, hierarchy );
		//}
		if ( minSize< contours[idx].size() &&contours[idx].size()<maxSize  )
		{
			approxPolyDP (  contours[idx]  ,approxCurve , double ( contours[idx].size() ) *0.05, true );
			if ( approxCurve.size() ==4 )
			{
				if ( cv::isContourConvex ( cv::Mat ( approxCurve ) ) )
				{
					//ensure that the   distace between consecutive points is large enough
					float minDist=1e10;
					for ( int j=0;j<4;j++ )
					{
						float d= std::sqrt ( ( float ) ( approxCurve[j].x-approxCurve[ ( j+1 ) %4].x ) * ( approxCurve[j].x-approxCurve[ ( j+1 ) %4].x ) +
											 ( approxCurve[j].y-approxCurve[ ( j+1 ) %4].y ) * ( approxCurve[j].y-approxCurve[ ( j+1 ) %4].y ) );
						if ( d<minDist ) minDist=d;
					}
					//check that distance is not very small
					if ( minDist>10 )
					{
						//calculates the area
						double area = cv::contourArea(contours[idx]);
						//calculates the centroid
						cv::Moments fiducial_blob_moments = cv::moments(contours[idx],true);
						double x = (float)(fiducial_blob_moments.m10 / fiducial_blob_moments.m00);
						double y = (float)(fiducial_blob_moments.m01 / fiducial_blob_moments.m00);
						//add the points
						SquareCanditates.push_back ( candidate(area,x,y,std::vector<cv::Point2f>()) );
						//MarkerCanditates.back().idx=i;
						for ( int j=0;j<4;j++ )
						{
							SquareCanditates.back().points.push_back ( cv::Point2f ( approxCurve[j].x,approxCurve[j].y ) );
						}
					}
				}
			}
		}
	}
	/******************************************************
	* Find Markers
	*******************************************************/
	std::vector<candidate> Squares;
	SquareDetector(SquareCanditates, Squares);
	for (size_t i=0;i<Squares.size();i++) 
	{
		
		///Marker identification
		for(int k = 0; k < 4; k++)
			tmp_pnt[k] = cv::Point2f(Squares[i].points[k]);
			//check for the exsiting ones matching !!!!!!
		//Zoom the square to detect the fiducial
		mapmatrix = cv::getPerspectiveTransform(tmp_pnt,dst_pnt);
		cv::warpPerspective(thres,fiducial_image,mapmatrix,cv::Size(FIDUCIAL_WIN_SIZE,FIDUCIAL_WIN_SIZE));
		cv::resize(fiducial_image, fiducial_image_zoomed, cv::Size(FIDUCIAL_WIN_SIZE*2,FIDUCIAL_WIN_SIZE*2));
		/******************************************************
		* Decode Markers
		*******************************************************/
		Fiducial temp = Fiducial();
		temp.Update(Squares[i].x,Squares[i].y,Squares[i].points[0],Squares[i].points[1],Squares[i].points[2],Squares[i].points[3],Squares[i].area,0);
		int tmp_id = finder.DecodeFiducial(fiducial_image, temp);
		if(tmp_id >= 0)
		{
			if(Globals::is_view_enabled)
			{
				/******************************************************
				* Draw vertices
				*******************************************************/
				cv::putText(main_image, "a", temp.GetCorner(0), cv::FONT_HERSHEY_SIMPLEX, 0.5f, cv::Scalar(0,255,0));
				cv::putText(main_image, "b", temp.GetCorner(1), cv::FONT_HERSHEY_SIMPLEX, 0.5f, cv::Scalar(0,255,0));
				cv::putText(main_image, "c", temp.GetCorner(2), cv::FONT_HERSHEY_SIMPLEX, 0.5f, cv::Scalar(0,255,0));
				cv::putText(main_image, "d", temp.GetCorner(3), cv::FONT_HERSHEY_SIMPLEX, 0.5f, cv::Scalar(0,255,0));
				///show perimeter
				for(int k = 0; k < 4; k++)
				{
					if (k!= 3)
						cv::line(main_image,Squares[i].points[k],Squares[i].points[k+1],cv::Scalar(0,0,255,255),1,CV_AA);
					else
						cv::line(main_image,Squares[i].points[k],Squares[i].points[0],cv::Scalar(0,0,255,255),1,CV_AA);
					tmp_pnt[k] = cv::Point2f(Squares[i].points[k]);
				}
				/******************************************************
				* Draw 3Daxis
				*******************************************************/

				/*************************************
				* Draw pose params  cube
				**************************************/
				cv::Mat objectPoints (8,3,CV_32FC1);
				objectPoints.at<float>(0,0)=0;
				objectPoints.at<float>(0,1)=temp.GetSize();
				objectPoints.at<float>(0,2)=0;

				objectPoints.at<float>(1,0)=temp.GetSize();
				objectPoints.at<float>(1,1)=temp.GetSize();
				objectPoints.at<float>(1,2)=0;

				objectPoints.at<float>(2,0)=temp.GetSize();
				objectPoints.at<float>(2,1)=0;
				objectPoints.at<float>(2,2)=0;

				objectPoints.at<float>(3,0)=0;
				objectPoints.at<float>(3,1)=0;
				objectPoints.at<float>(3,2)=0;

				objectPoints.at<float>(4,0)=0;
				objectPoints.at<float>(4,1)=temp.GetSize();
				objectPoints.at<float>(4,2)=temp.GetSize();

				objectPoints.at<float>(5,0)=temp.GetSize();
				objectPoints.at<float>(5,1)=temp.GetSize();
				objectPoints.at<float>(5,2)=temp.GetSize();

				objectPoints.at<float>(6,0)=temp.GetSize();
				objectPoints.at<float>(6,1)=0;
				objectPoints.at<float>(6,2)=temp.GetSize();

				objectPoints.at<float>(7,0)=0;
				objectPoints.at<float>(7,1)=0;
				objectPoints.at<float>(7,2)=temp.GetSize();

				cv::vector<cv::Point2f> imagePoints;
				cv::Mat Tvec = temp.GetTranslationVector();
				cv::Mat Rvec = temp.GetRotationVector();
				if(!(Tvec.rows == 0 || Tvec.cols == 0 || Rvec.rows == 0 || Rvec.cols == 0))
				{
					cv::projectPoints(objectPoints,temp.GetRotationVector(), temp.GetTranslationVector(),Globals::CameraMatrix,Globals::Distortion,imagePoints);
			
					//draw lines of different colours
					for (int i=0;i<4;i++)
						cv::line(main_image,imagePoints[i],imagePoints[(i+1)%4],cv::Scalar(0,255,255,255),1,CV_AA);

					for (int i=0;i<4;i++)
						cv::line(main_image,imagePoints[i+4],imagePoints[4+(i+1)%4],cv::Scalar(255,0,255,255),1,CV_AA);

					for (int i=0;i<4;i++)
						cv::line(main_image,imagePoints[i],imagePoints[i+4],cv::Scalar(255,255,0,255),1,CV_AA);
				}

				/*************************************
				* Draw pose params  axis
				**************************************/
				float size=temp.GetSize()*3;
				float halfSize=temp.GetSize()/2;
				objectPoints= cv::Mat(4,3,CV_32FC1);
				objectPoints.at<float>(0,0)=halfSize;
				objectPoints.at<float>(0,1)=halfSize;
				objectPoints.at<float>(0,2)=0;
				objectPoints.at<float>(1,0)=size;
				objectPoints.at<float>(1,1)=halfSize;
				objectPoints.at<float>(1,2)=0;
				objectPoints.at<float>(2,0)=halfSize;
				objectPoints.at<float>(2,1)=size;
				objectPoints.at<float>(2,2)=0;
				objectPoints.at<float>(3,0)=halfSize;
				objectPoints.at<float>(3,1)=halfSize;
				objectPoints.at<float>(3,2)=size;
				imagePoints.clear();
				cv::projectPoints( objectPoints, Rvec,Tvec, Globals::CameraMatrix,Globals::Distortion, imagePoints);
				//draw lines of different colours
				cv::line(main_image,imagePoints[0],imagePoints[1],cv::Scalar(0,0,255,255),2,CV_AA);
				cv::putText(main_image, "X", imagePoints[1], cv::FONT_HERSHEY_SIMPLEX, 1.0f, cv::Scalar(0,0,255));
				cv::line(main_image,imagePoints[0],imagePoints[2],cv::Scalar(0,255,0,255),2,CV_AA);
				cv::putText(main_image, "Y", imagePoints[2], cv::FONT_HERSHEY_SIMPLEX, 1.0f, cv::Scalar(0,255,0));
				cv::line(main_image,imagePoints[0],imagePoints[3],cv::Scalar(255,0,0,255),2,CV_AA);
				cv::putText(main_image, "Z", imagePoints[3], cv::FONT_HERSHEY_SIMPLEX, 1.0f, cv::Scalar(255,0,0));
			}
		}
	}
	
	

	cv::putText(main_image, "hola don pepito", cv::Point(50,50), cv::FONT_HERSHEY_SIMPLEX, 0.5f, cv::Scalar(0,255,0));
	/******************************************************
	* Show thresholded Image
	*******************************************************/
	if (this->show_screen)
	{
		cv::imshow("fidfinder",fiducial_image_zoomed);
		cv::imshow(this->name,thres);
	}
}



//
//void MarkerFinder::InitFrames(IplImage*	main_image)
//{
//	blob_moments = (CvMoments*)malloc( sizeof(CvMoments) );
//	main_processed_image = cvCreateImage(cvGetSize(main_image),IPL_DEPTH_8U,1);		//allocates 1-channel memory frame for the image
//	main_processed_contour = cvCreateImage(cvGetSize(main_image),IPL_DEPTH_8U,1);	//allocates 1-channel memory frame for the image
//	//contour data allocation
//	main_storage = cvCreateMemStorage (0);											//Creates a memory main_storage and returns pointer to it /param:/Size of the main_storage blocks in bytes. If it is 0, the block size is set to default value - currently it is 64K.
//	main_storage_poligon = cvCreateMemStorage (0);
//	//fiducial frame allocation
//	fiducial_image = cvCreateImage(cvSize(FIDUCIAL_IMAGE_SIZE,FIDUCIAL_IMAGE_SIZE),IPL_DEPTH_8U,1);
//	fiducial_image_zoomed = cvCreateImage(cvSize(fiducial_image->width*2,fiducial_image->height*2),IPL_DEPTH_8U,1);
//}
//
//
//IplImage* MarkerFinder::Process(IplImage*	main_image)
//{
//	
//	cvClearMemStorage(main_storage);
//	cvClearMemStorage(main_storage_poligon);
//
//	if(use_adaptive_threshold)
//		cvAdaptiveThreshold(main_image,main_processed_image,255,CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY,adaptive_block_size,2);//CV_ADAPTIVE_THRESH_MEAN_C
//	else 
//		cvThreshold(main_image,main_processed_image,threshold_value,255, CV_THRESH_BINARY);
//
//	cvNot(main_processed_image,main_processed_image); //invert colors
//
//	cvCopy(main_processed_image,main_processed_contour);	
//	cvFindContours (main_processed_contour, main_storage, &firstcontour, sizeof (CvContour), CV_RETR_CCOMP);
//	//find squares
//	polycontour=cvApproxPoly(firstcontour,sizeof(CvContour),main_storage_poligon,CV_POLY_APPROX_DP,3,1);
//	
//	for(CvSeq* c=polycontour;c!=NULL;c=c->h_next)
//	{
//		if((cvContourPerimeter(c)<2000)&&(cvContourPerimeter(c)>60)&&(c->total==4))
//		{
//			//if(Globals::is_view_enabled)cvDrawContours(Globals::screen,c,CV_RGB(255,0,0),CV_RGB(200,255,255),0);
//			if(c->v_next!=NULL && (c->v_next->total==4))
//			{
//				CvSeq* c_vnext=c->v_next;
//				cvMoments( c, blob_moments );
//				//inside square processing
//				float xlist[4];
//				float ylist[4];
//
//				float exlist[4];
//				float eylist[4];
//				for(int n=0;n<4;n++)
//				{
//					CvPoint* p=CV_GET_SEQ_ELEM(CvPoint,c->v_next,n);
//					tmp_pnt[n].x=(float)p->x;
//					tmp_pnt[n].y=(float)p->y;	
//					xlist[n]=(float)p->x;
//					ylist[n]=(float)p->y;
//
//					CvPoint* ep = CV_GET_SEQ_ELEM(CvPoint,c,n);
//					exlist[n]=(float)ep->x;
//					eylist[n]=(float)ep->y;
//				}
//
//				
//
//
//				temporal.clear();
//				//float area = fabs(cvContourArea( c, CV_WHOLE_SEQ ));
//				//x = (float)(fiducial_blob_moments->m10 / fiducial_blob_moments->m00);
//				//y = (float)(fiducial_blob_moments->m01 / fiducial_blob_moments->m00);
//				temporal.Update((float)(blob_moments->m10 / blob_moments->m00),(float)(blob_moments->m01 / blob_moments->m00),
//								cvPoint((int)xlist[0],(int)ylist[0]),
//								cvPoint((int)xlist[1],(int)ylist[1]),
//								cvPoint((int)xlist[2],(int)ylist[2]),
//								cvPoint((int)xlist[3],(int)ylist[3]),
//								(float)fabs(cvContourArea( c, CV_WHOLE_SEQ )),0 );
//
//				to_process = true;
//				tmp_ssid = 0;
//				minimum_distance = 99999999.0f;
//				for(FiducialMap::iterator it = fiducial_map.begin(); it!= fiducial_map.end(); it++)
//				{
//					if( it->second->CanUpdate(temporal,minimum_distance) )
//					{
//						tmp_ssid = it->first;
//					}
//					else if( it->second->Is_inside(temporal) )
//					{
//						to_process = false;
//						break;
//					}
//				}
//
//				if(to_process)
//				{
//					if(Globals::is_view_enabled)cvDrawContours(Globals::screen,c,CV_RGB(255,255,0),CV_RGB(200,255,255),0);
//					if(Globals::is_view_enabled)cvDrawContours(Globals::screen,c_vnext,CV_RGB(255,0,0),CV_RGB(0,255,255),0);
//					
//					cvGetPerspectiveTransform (tmp_pnt, dst_pnt, map_matrix);
//					cvWarpPerspective (main_processed_image, fiducial_image, map_matrix, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cvScalarAll (0));
//
//					int maxCount=0;
//					int markerDirection=0;
//					cvResize(fiducial_image,fiducial_image_zoomed);
//
//					fiducial_finder->DecodeFiducial(fiducial_image, temporal);
//#ifdef SHOWDEBUG
//					if(temporal.GetFiducialID() != -1)
//					{
//						cvNamedWindow ("fiducial_screen", CV_WINDOW_AUTOSIZE);
//						cvShowImage("fiducial_screen",fiducial_image);
//					}
//#endif
//
//					if(tmp_ssid == 0)
//					{
//						if(temporal.GetFiducialID() == -1) break;
//						//add fiducial
//						fiducial_map[Globals::ssidGenerator++] = new Fiducial(temporal);
//						tmp_ssid = Globals::ssidGenerator-1;
//					}
//					else if( tmp_ssid > 0)
//					{
//						fiducial_map[tmp_ssid]->Update(temporal);
//					}
//#ifdef ENABLEPOSE
//					markerDirection = fiducial_map[tmp_ssid]->GetOrientation();
//					if(markerDirection==0)
//					{
//						src_pnt[0].x = xlist[0];		
//						src_pnt[1].x = xlist[1];
//						src_pnt[2].x = xlist[2];
//						src_pnt[3].x = xlist[3];
//
//						src_pnt[0].y = ylist[0];
//						src_pnt[1].y = ylist[1];
//						src_pnt[2].y = ylist[2];
//						src_pnt[3].y = ylist[3];
//#ifdef USEEIGHTPOINTS
//						src_pnt[4].x = exlist[0];		
//						src_pnt[5].x = exlist[3];
//						src_pnt[6].x = exlist[2];
//						src_pnt[7].x = exlist[1];
//
//						src_pnt[4].y = eylist[0];
//						src_pnt[5].y = eylist[3];
//						src_pnt[6].y = eylist[2];
//						src_pnt[7].y = eylist[1];
//#endif
//					}
//					else if(markerDirection==1)//90
//					{
//						src_pnt[0].x = xlist[3];		
//						src_pnt[1].x = xlist[0];
//						src_pnt[2].x = xlist[1];
//						src_pnt[3].x = xlist[2];
//
//						src_pnt[0].y = ylist[3];
//						src_pnt[1].y = ylist[0];
//						src_pnt[2].y = ylist[1];
//						src_pnt[3].y = ylist[2];
//#ifdef USEEIGHTPOINTS
//						src_pnt[4].x = exlist[1];		
//						src_pnt[5].x = exlist[0];
//						src_pnt[6].x = exlist[3];
//						src_pnt[7].x = exlist[2];
//
//						src_pnt[4].y = eylist[1];
//						src_pnt[5].y = eylist[0];
//						src_pnt[6].y = eylist[3];
//						src_pnt[7].y = eylist[2];
//#endif
//					}
//					else if(markerDirection==3)//180
//					{
//						src_pnt[0].x = xlist[2];		
//						src_pnt[1].x = xlist[3];
//						src_pnt[2].x = xlist[0];
//						src_pnt[3].x = xlist[1];
//
//						src_pnt[0].y = ylist[2];
//						src_pnt[1].y = ylist[3];
//						src_pnt[2].y = ylist[0];
//						src_pnt[3].y = ylist[1];
//#ifdef USEEIGHTPOINTS
//						src_pnt[4].x = exlist[2];		
//						src_pnt[5].x = exlist[1];
//						src_pnt[6].x = exlist[0];
//						src_pnt[7].x = exlist[3];
//
//						src_pnt[4].y = eylist[2];
//						src_pnt[5].y = eylist[1];
//						src_pnt[6].y = eylist[0];
//						src_pnt[7].y = eylist[3];
//#endif
//					}
//					else if(markerDirection==2)//270
//					{
//						src_pnt[0].x = xlist[1];		
//						src_pnt[1].x = xlist[2];
//						src_pnt[2].x = xlist[3];
//						src_pnt[3].x = xlist[0];
//
//						src_pnt[0].y = ylist[1];
//						src_pnt[1].y = ylist[2];
//						src_pnt[2].y = ylist[3];
//						src_pnt[3].y = ylist[0];
//#ifdef USEEIGHTPOINTS
//						src_pnt[4].x = exlist[3];		
//						src_pnt[5].x = exlist[2];
//						src_pnt[6].x = exlist[1];
//						src_pnt[7].x = exlist[0];
//
//						src_pnt[4].y = eylist[3];
//						src_pnt[5].y = eylist[2];
//						src_pnt[6].y = eylist[1];
//						src_pnt[7].y = eylist[0];
//#endif
//					}				
//					
//					cvInitMatHeader (&image_points, 4, 1, CV_32FC2, src_pnt);
//					cvInitMatHeader (&object_points, 4, 3, CV_32FC1, baseMarkerPoints);
//
//					CvPoint a;
//
//#ifdef SHOWDEBUG
//					std::ostringstream convert;
//					convert << "   --o: " << markerDirection;
//					a.x = src_pnt[0].x; a.y = src_pnt[0].y;
//					Globals::Font::Write(Globals::screen,convert.str().c_str(),a,FONT_AXIS,255,0,0);
//
//					a.x = src_pnt[0].x; a.y = src_pnt[0].y;
//					Globals::Font::Write(Globals::screen,"a",a,FONT_AXIS,255,0,0);
//					a.x = src_pnt[1].x; a.y = src_pnt[1].y;
//					Globals::Font::Write(Globals::screen,"b",a,FONT_AXIS,255,0,0);
//					a.x = src_pnt[2].x; a.y = src_pnt[2].y;
//					Globals::Font::Write(Globals::screen,"c",a,FONT_AXIS,255,0,0);
//					a.x = src_pnt[3].x; a.y = src_pnt[3].y;
//					Globals::Font::Write(Globals::screen,"d",a,FONT_AXIS,255,0,0);
//
//#ifdef USEEIGHTPOINTS
//					a.x = src_pnt[4].x; a.y = src_pnt[4].y;
//					Globals::Font::Write(Globals::screen,"a",a,FONT_AXIS,255,0,255);
//					a.x = src_pnt[5].x; a.y = src_pnt[5].y;
//					Globals::Font::Write(Globals::screen,"b",a,FONT_AXIS,255,0,255);
//					a.x = src_pnt[6].x; a.y = src_pnt[6].y;
//					Globals::Font::Write(Globals::screen,"c",a,FONT_AXIS,255,0,255);
//					a.x = src_pnt[7].x; a.y = src_pnt[7].y;
//					Globals::Font::Write(Globals::screen,"d",a,FONT_AXIS,255,0,255);
//#endif
//#endif
//					/*************************************
//					* Find extrinsic fiducial params
//					**************************************/
//					int fidsize = 5;
//					double halfSize=fidsize;///2.;
//
//#ifdef USEEIGHTPOINTS
//					cv::Mat ObjPoints(8,3,CV_32FC1);
//#else
//					cv::Mat ObjPoints(4,3,CV_32FC1);
//#endif
//					ObjPoints.at<float>(0,0)=0;
//					ObjPoints.at<float>(0,1)=halfSize;
//					ObjPoints.at<float>(0,2)=0;
//					ObjPoints.at<float>(1,0)=halfSize;
//					ObjPoints.at<float>(1,1)=halfSize;
//					ObjPoints.at<float>(1,2)=0;
//					ObjPoints.at<float>(2,0)=halfSize;
//					ObjPoints.at<float>(2,1)=0;
//					ObjPoints.at<float>(2,2)=0;
//					ObjPoints.at<float>(3,0)=0;
//					ObjPoints.at<float>(3,1)=0;
//					ObjPoints.at<float>(3,2)=0;
//#ifdef USEEIGHTPOINTS
//					double inc = halfSize/3.0;
//					ObjPoints.at<float>(4,0)=-inc;
//					ObjPoints.at<float>(4,1)=halfSize+inc;
//					ObjPoints.at<float>(4,2)=0;
//					ObjPoints.at<float>(5,0)=halfSize+inc;
//					ObjPoints.at<float>(5,1)=halfSize+inc;
//					ObjPoints.at<float>(5,2)=0;
//					ObjPoints.at<float>(6,0)=halfSize+inc;
//					ObjPoints.at<float>(6,1)=-inc;
//					ObjPoints.at<float>(6,2)=0;
//					ObjPoints.at<float>(7,0)=-inc;
//					ObjPoints.at<float>(7,1)=-inc;
//					ObjPoints.at<float>(7,2)=0;
//#endif
//
//#ifdef USEEIGHTPOINTS
//					cv::Mat ImagePoints(8,2,CV_32FC1);
//					int numpoints = 8;
//#else
//					cv::Mat ImagePoints(4,2,CV_32FC1);
//					int numpoints = 4;
//#endif
//					for (int c=0;c<numpoints;c++)
//					{
//						ImagePoints.at<float>(c,0)=(src_pnt[c].x);
//						ImagePoints.at<float>(c,1)=(src_pnt[c].y);
//					}
//
//					cv::Mat raux,taux;
//					cv::solvePnP(ObjPoints, ImagePoints, Globals::CameraMatrix, Globals::Distortion,raux,taux);
//					raux.convertTo(Rvec,CV_32F);
//					taux.convertTo(Tvec ,CV_32F);
//#endif
//#ifdef ENABLEPOSE
//					/*************************************
//					* Prepare parameters
//					**************************************/
//					cv::Mat R(3,3,CV_32F);
//					Rodrigues(Rvec, R);
//					fiducial_map[tmp_ssid]->yaw = atan2(-R.ptr<float>(2)[0],sqrt( R.ptr<float>(2)[1]*R.ptr<float>(2)[1] + R.ptr<float>(2)[2]*R.ptr<float>(2)[2]));
//					fiducial_map[tmp_ssid]->pitch = atan2(R.ptr<float>(2)[1],R.ptr<float>(2)[2]);
//					fiducial_map[tmp_ssid]->roll = (2.0f*3.141592654f)-atan2(R.ptr<float>(1)[0],R.ptr<float>(0)[0]);
//
//					fiducial_map[tmp_ssid]->xpos = Tvec.ptr<float>(0)[0];
//					fiducial_map[tmp_ssid]->ypos = Tvec.ptr<float>(0)[1];
//					fiducial_map[tmp_ssid]->zpos = Tvec.ptr<float>(0)[2];
//					//std::cout << fiducial_map[tmp_ssid]->xpos << "\t" << fiducial_map[tmp_ssid]->ypos << "\t" << fiducial_map[tmp_ssid]->zpos << std::endl;
//
//
//					//Rotate XAxis
//					cv::Mat Rx= cv::Mat::eye(3,3,CV_32F);
//					float angle = M_PI;
//					Rx.at<float>(1,1) = cos(angle);
//					Rx.at<float>(1,2) = -sin(angle);
//					Rx.at<float>(2,1) = sin(angle);
//					Rx.at<float>(2,2) = cos(angle);
//					R = R*Rx;
//
//					//Rotate YAxis
//					/*cv::Mat Ry= cv::Mat::eye(3,3,CV_32F);
//					Ry.at<float>(1,1) = cos(angle);
//					Ry.at<float>(1,2) = -sin(angle);
//					Ry.at<float>(2,1) = sin(angle);
//					Ry.at<float>(2,2) = cos(angle);
//					R = R*Ry;*/
//
//
//					fiducial_map[tmp_ssid]->r11 = -R.ptr<float>(0)[0];
//					fiducial_map[tmp_ssid]->r12 = R.ptr<float>(0)[1];
//					fiducial_map[tmp_ssid]->r13 = -R.ptr<float>(0)[2];
//					fiducial_map[tmp_ssid]->r21 = R.ptr<float>(1)[0];
//					fiducial_map[tmp_ssid]->r22 = -R.ptr<float>(1)[1];
//					fiducial_map[tmp_ssid]->r23 = -R.ptr<float>(1)[2];
//					fiducial_map[tmp_ssid]->r31 = -R.ptr<float>(2)[0];
//					fiducial_map[tmp_ssid]->r32 = -R.ptr<float>(2)[1];
//					fiducial_map[tmp_ssid]->r33 = R.ptr<float>(2)[2];
//
//#ifdef SHOWDEBUG
//					std::ostringstream fidinfo;
//					fidinfo << "--------------------------------------------------------------------------------" << std::endl;
//					fidinfo << "Fiducial info;" << std::endl;
//					fidinfo << "\tID: " <<  fiducial_map[tmp_ssid]->GetFiducialID()  <<std::endl;
//					fidinfo << "\tgX: " <<  fiducial_map[tmp_ssid]->GetX() << "\tgY: " <<  fiducial_map[tmp_ssid]->GetY()  <<std::endl;
//					fidinfo << "\tX: " <<  fiducial_map[tmp_ssid]->xpos  <<std::endl;
//					fidinfo << "\tY: " <<  fiducial_map[tmp_ssid]->ypos  <<std::endl;
//					fidinfo << "\tZ: " <<  fiducial_map[tmp_ssid]->zpos  <<std::endl;
//					fidinfo << "\tyaw: " <<  fiducial_map[tmp_ssid]->yaw <<"\t" <<fiducial_map[tmp_ssid]->pitch << "\t" << fiducial_map[tmp_ssid]->roll  <<std::endl;
//					std::cout << fidinfo.str() << std::endl;
//#endif
//#endif
//					if(Globals::is_view_enabled)
//					{
//#ifdef DRAWPOSE
//					/*************************************
//					* Draw pose params  cube
//					**************************************/
//					cv::Mat objectPoints (8,3,CV_32FC1);
//					halfSize=fidsize/*/2*/;
//					objectPoints.at<float>(0,0)=0;
//					objectPoints.at<float>(0,1)=halfSize;
//					objectPoints.at<float>(0,2)=0;
//
//					objectPoints.at<float>(1,0)=halfSize;
//					objectPoints.at<float>(1,1)=halfSize;
//					objectPoints.at<float>(1,2)=0;
//
//					objectPoints.at<float>(2,0)=halfSize;
//					objectPoints.at<float>(2,1)=0;
//					objectPoints.at<float>(2,2)=0;
//
//					objectPoints.at<float>(3,0)=0;
//					objectPoints.at<float>(3,1)=0;
//					objectPoints.at<float>(3,2)=0;
//
//					objectPoints.at<float>(4,0)=0;
//					objectPoints.at<float>(4,1)=halfSize;
//					objectPoints.at<float>(4,2)=halfSize;
//
//					objectPoints.at<float>(5,0)=halfSize;
//					objectPoints.at<float>(5,1)=halfSize;
//					objectPoints.at<float>(5,2)=halfSize;
//
//					objectPoints.at<float>(6,0)=halfSize;
//					objectPoints.at<float>(6,1)=0;
//					objectPoints.at<float>(6,2)=halfSize;
//
//					objectPoints.at<float>(7,0)=0;
//					objectPoints.at<float>(7,1)=0;
//					objectPoints.at<float>(7,2)=halfSize;
//
//					cv::vector<cv::Point2f> imagePoints;
//
//					projectPoints( objectPoints, Rvec, Tvec,  Globals::CameraMatrix,Globals::Distortion,   imagePoints);
//					
//					//draw lines of different colours
//					for (int i=0;i<4;i++)
//						cvLine(Globals::screen,imagePoints[i],imagePoints[(i+1)%4],CV_RGB(255,255,0),1,8,0);
//
//					for (int i=0;i<4;i++)
//						cvLine(Globals::screen,imagePoints[i+4],imagePoints[4+(i+1)%4],CV_RGB(255,0,255),1,8,0);
//
//					for (int i=0;i<4;i++)
//						cvLine(Globals::screen,imagePoints[i],imagePoints[i+4],CV_RGB(0,255,255),1,8,0);
//					/*************************************
//					* Draw pose params  axis
//					**************************************/
//					
//					float size=fidsize*3;
//					halfSize=fidsize/2;
//					objectPoints= cv::Mat(4,3,CV_32FC1);
//					objectPoints.at<float>(0,0)=halfSize;
//					objectPoints.at<float>(0,1)=halfSize;
//					objectPoints.at<float>(0,2)=0;
//					objectPoints.at<float>(1,0)=size;
//					objectPoints.at<float>(1,1)=halfSize;
//					objectPoints.at<float>(1,2)=0;
//					objectPoints.at<float>(2,0)=halfSize;
//					objectPoints.at<float>(2,1)=size;
//					objectPoints.at<float>(2,2)=0;
//					objectPoints.at<float>(3,0)=halfSize;
//					objectPoints.at<float>(3,1)=halfSize;
//					objectPoints.at<float>(3,2)=size;
//
//					//cv::vector<cv::Point2f> imagePoints;
//					imagePoints.clear();
//					cv::projectPoints( objectPoints, Rvec,Tvec, Globals::CameraMatrix,Globals::Distortion, imagePoints);
//					//draw lines of different colours
//					cvLine(Globals::screen,imagePoints[0],imagePoints[1],CV_RGB(0,0,255),2,8,0);
//					Globals::Font::Write(Globals::screen,"X",imagePoints[1],FONT_AXIS,0,0,255);
//					cvLine(Globals::screen,imagePoints[0],imagePoints[2],CV_RGB(0,255,0),2,8,0);
//					Globals::Font::Write(Globals::screen,"Y",imagePoints[2],FONT_AXIS,0,255,0);
//					cvLine(Globals::screen,imagePoints[0],imagePoints[3],CV_RGB(255,0,0),2,8,0);
//					Globals::Font::Write(Globals::screen,"Z",imagePoints[3],FONT_AXIS,255,0,0);
//					
//					#endif
//					}
//					
//				}
//			}
//		}
//	}
//	return main_processed_image;
//}
//
//AliveList MarkerFinder::GetAlive()
//{
//	AliveList to_return;
//	if(IsEnabled())
//	{
//		for(FiducialMap::iterator it = fiducial_map.begin(); it!= fiducial_map.end(); it++)
//		{
//			to_return.push_back(it->first);
//			//std::cout << " " << it->first;
//		}
//		//std::cout << std::endl;
//	}
//	//std::vector<unsigned long>
//	return to_return;
//}
//
//void MarkerFinder::KeyInput(char key)
//{
//}
//
//void MarkerFinder::RepportOSC()
//{
//	if(!this->IsEnabled())return;
//	to_remove.clear();
//	for(FiducialMap::iterator it = fiducial_map.begin(); it!= fiducial_map.end(); it++)
//	{
//		if(it->second->IsUpdated())
//		{
//			//send tuio message!!!!
//			TuioServer::Instance().Add3DObjectMessage(
//				it->first,
//				0,
//				it->second->GetFiducialID(),
//				it->second->GetX()/Globals::width,//Globals::GetX(it->second->xpos),//it->second->xpos,
//				it->second->GetY()/Globals::height,//Globals::GetY(it->second->ypos),//it->second->ypos,
//				it->second->zpos,//Globals::GetZValue(it->second->zpos),
//				it->second->yaw,
//				it->second->pitch,
//				it->second->roll,
//				it->second->r11,
//				it->second->r12,
//				it->second->r13,
//				it->second->r21,
//				it->second->r22,
//				it->second->r23,
//				it->second->r31,
//				it->second->r32,
//				it->second->r33
//				);
//		}
//		else
//		{
//			to_remove.push_back(it->first);
//		}
//		if(Globals::is_view_enabled)
//		{
//			sprintf_s(text,"%i , %i",it->second->GetFiducialID(), it->first); 
//			Globals::Font::Write(Globals::screen,text,cvPoint((int)it->second->GetX(), (int)it->second->GetY()),FONT_HELP,0,255,0);
//		}
//	}
//
//	for(std::vector<unsigned int>::iterator it = to_remove.begin(); it != to_remove.end(); it++)
//	{
//		fiducial_map.erase(*it);
//	}
//}


void MarkerFinder::SquareDetector(std::vector<candidate>& MarkerCanditates,std::vector<candidate>& dest)
{
	dest.clear();
	///sort the points in anti-clockwise order
	std::valarray<bool> swapped(false,MarkerCanditates.size());//used later
	for ( unsigned int i=0;i<MarkerCanditates.size();i++ )
	{
		//trace a line between the first and second point.
		//if the thrid point is at the right side, then the points are anti-clockwise
		double dx1 = MarkerCanditates[i].points[1].x - MarkerCanditates[i].points[0].x;
		double dy1 =  MarkerCanditates[i].points[1].y - MarkerCanditates[i].points[0].y;
		double dx2 = MarkerCanditates[i].points[2].x - MarkerCanditates[i].points[0].x;
		double dy2 = MarkerCanditates[i].points[2].y - MarkerCanditates[i].points[0].y;
		double o = ( dx1*dy2 )- ( dy1*dx2 );
		if ( o  < 0.0 )		 //if the third point is in the left side, then sort in anti-clockwise order
		{
			std::swap ( MarkerCanditates[i].points[1],MarkerCanditates[i].points[3] );
			swapped[i]=true;
			//sort the contour points
	//  	    reverse(MarkerCanditates[i].contour.begin(),MarkerCanditates[i].contour.end());//????
		}
	}
	/// remove these elements whise corners are too close to each other
	//first detect candidates
	std::vector<std::pair<int,int>  > TooNearCandidates;
	for ( unsigned int i=0;i<MarkerCanditates.size();i++ )
	{
		// 	cout<<"Marker i="<<i<<MarkerCanditates[i]<<endl;
		//calculate the average distance of each corner to the nearest corner of the other marker candidate
		for ( unsigned int j=i+1;j<MarkerCanditates.size();j++ )
		{
			float dist=0;
			for ( int c=0;c<4;c++ )
				dist+= sqrt (	( MarkerCanditates[i].points[c].x-MarkerCanditates[j].points[c].x ) * 
								( MarkerCanditates[i].points[c].x-MarkerCanditates[j].points[c].x ) + 
								( MarkerCanditates[i].points[c].y-MarkerCanditates[j].points[c].y ) * 
								( MarkerCanditates[i].points[c].y-MarkerCanditates[j].points[c].y ) );
			dist/=4;
			//if distance is too small
			if ( dist< 10 )
			{
				TooNearCandidates.push_back ( std::pair<int,int> ( i,j ) );
			}
		}
	}
	//mark for removal the element of  the pair with smaller perimeter
	std::valarray<bool> toRemove ( false,MarkerCanditates.size() );
	for ( unsigned int i=0;i<TooNearCandidates.size();i++ )
	{
		if ( perimeter ( MarkerCanditates[TooNearCandidates[i].first ].points ) > perimeter ( MarkerCanditates[ TooNearCandidates[i].second].points ) )
			toRemove[TooNearCandidates[i].second]=true;
		else toRemove[TooNearCandidates[i].first]=true;
	}
	for (size_t i=0;i<MarkerCanditates.size();i++) 
	{
		if (!toRemove[i]) 
		{
			dest.push_back(MarkerCanditates[i]);
		}
	}
}

int MarkerFinder::perimeter ( std::vector<cv::Point2f> &a )
{
    int sum=0;
    for ( unsigned int i=0;i<a.size();i++ )
    {
        int i2= ( i+1 ) %a.size();
        sum+= sqrt ( ( a[i].x-a[i2].x ) * ( a[i].x-a[i2].x ) + ( a[i].y-a[i2].y ) * ( a[i].y-a[i2].y ) ) ;
    }
    return sum;
}

void MarkerFinder::BuildGui(bool force)
{
	if(force)
	{
		cv::destroyWindow(name);
		cv::namedWindow(name,CV_WINDOW_AUTOSIZE);
	}
	cv::createTrackbar("Enable", name,&enable_processor, 1, NULL);
	cv::createTrackbar("Use Adaptive", name,&use_adaptive_bar_value, 1, NULL);
	if(use_adaptive_bar_value == 1)
	{
		cv::createTrackbar("block_size", name,&block_size, 50, NULL);
		cv::createTrackbar("C", name,&threshold_C, 10, NULL);
	}
	else
	{
		cv::createTrackbar("th.value", name,&Threshold_value, 255, NULL);
	}
}