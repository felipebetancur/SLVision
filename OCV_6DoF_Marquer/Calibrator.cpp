#include "Calibrator.h"
#include "Globals.h"

Calibrator::Calibrator(void)
{
	board_total = BOARD_W * BOARD_H;
	board_sz = cvSize( BOARD_W, BOARD_H );
	mapx = NULL;
	mapy = NULL;
	x_box = Globals::width/2;
	y_box = Globals::height/2;
	w_box = Globals::width/2;
	h_box = Globals::height/2;
}


Calibrator::~Calibrator(void)
{
}

void Calibrator::StartCalibration()
{
	/*mapx = cvCreateImage( cvGetSize(main_image), IPL_DEPTH_32F, 1 );
	mapy = cvCreateImage( cvGetSize(main_image), IPL_DEPTH_32F, 1 );
	cvInitUndistortMap(Globals::intrinsic,Globals::distortion,mapx,mapy);
	*/

	//if(mapx != NULL) cvReleaseImage(&mapx);
	//if(mapy != NULL) cvReleaseImage(&mapy);
	chessboard_finder = false;
}

void Calibrator::StartChessBoardFinder()
{
	image_points      = cvCreateMat(N_BOARDS*board_total,2,CV_32FC1);
	object_points     = cvCreateMat(N_BOARDS*board_total,3,CV_32FC1);
	point_counts      = cvCreateMat(N_BOARDS,1,CV_32SC1);

	cvReleaseMat(& Globals::intrinsic);
	cvReleaseMat(& Globals::distortion);
	Globals::intrinsic  = cvCreateMat(3,3,CV_32FC1);
	Globals::distortion = cvCreateMat(4,1,CV_32FC1);
	//Globals::intrinsic Matrix - 3x3 Lens Distorstion Matrix - 4x1
	// [fx 0 cx]              [k1 k2 p1 p2   k3(optional)]
	// [0 fy cy]
	// [0  0  1]
	corners = new CvPoint2D32f[ board_total ];
	successes = 0;
	step = 0;
	frame = 0;
	chessboard_finder = true; //is it really necessary?
}

void Calibrator::ProcessFrame(IplImage*	main_image) //gray scale image as input?
{
	if(chessboard_finder)
	{
		if(successes < N_BOARDS)
		{
			int found = cvFindChessboardCorners(main_image, board_sz, corners,&corner_count, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS );

			cvFindCornerSubPix(main_image, corners, corner_count, cvSize(11,11),cvSize(-1,-1), cvTermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ));
			cvDrawChessboardCorners(Globals::screen, board_sz, corners, corner_count, found); 

			if(corner_count == board_total) 
			{
				step = successes*board_total;
				for( int i=step, j=0; j<board_total; ++i,++j ) 
				{
					CV_MAT_ELEM(*image_points, float,i,0) = corners[j].x;
					CV_MAT_ELEM(*image_points, float,i,1) = corners[j].y;
					CV_MAT_ELEM(*object_points,float,i,0) = (float) j/BOARD_W;
					CV_MAT_ELEM(*object_points,float,i,1) = (float) (j%BOARD_W);
					CV_MAT_ELEM(*object_points,float,i,2) = 0.0f;
				}
				CV_MAT_ELEM(*point_counts, int,successes,0) = board_total;    
				successes++;
				printf("\r%d successful Snapshots out of %d collected.",successes,N_BOARDS);
			}

			if (successes >= N_BOARDS)
			{
				chessboard_finder = false;
				printf("\n\n *** Calbrating the camera now...\n");
  
				//Allocate matrices according to successful number of captures
				object_points2  = cvCreateMat(successes*board_total,3,CV_32FC1);
				image_points2   = cvCreateMat(successes*board_total,2,CV_32FC1);
				point_counts2   = cvCreateMat(successes,1,CV_32SC1);
	
				for(int i = 0; i<successes*board_total; ++i)
				{
					CV_MAT_ELEM( *image_points2, float, i, 0) = CV_MAT_ELEM( *image_points,  float, i, 0);
					CV_MAT_ELEM( *image_points2, float,i,1)   = CV_MAT_ELEM( *image_points,  float, i, 1);
					CV_MAT_ELEM(*object_points2, float, i, 0) = CV_MAT_ELEM( *object_points, float, i, 0) ;
					CV_MAT_ELEM( *object_points2, float, i, 1)= CV_MAT_ELEM( *object_points, float, i, 1) ;
					CV_MAT_ELEM( *object_points2, float, i, 2)= CV_MAT_ELEM( *object_points, float, i, 2) ;
				}

				for(int i=0; i<successes; ++i)
				{ 
					CV_MAT_ELEM( *point_counts2, int, i, 0) = CV_MAT_ELEM( *point_counts, int, i, 0); //These are all the same number
				}

				cvReleaseMat(& object_points);
				cvReleaseMat(& image_points);
				cvReleaseMat(& point_counts);

				// Initialize the Globals::intrinsic matrix with both the two focal lengths in a ratio of 1.0
				CV_MAT_ELEM( *Globals::intrinsic, float, 0, 0 ) = 1.0f;
				CV_MAT_ELEM( *Globals::intrinsic, float, 1, 1 ) = 1.0f;
			
				//Calibrate the camera
				cvCalibrateCamera2(object_points2, image_points2, point_counts2, cvGetSize( main_image ), Globals::intrinsic, Globals::distortion, NULL, NULL,0 );

				//printf(" *** Calibration Done!\n\n");
				//printf("Storing Intrinsics.xml and Distortions.xml files...\n");
				cvSave(M_PATH_INTRINSIC,Globals::intrinsic);
				cvSave(M_PATH_DISTORTION,Globals::distortion);
				//printf("Files saved.\n\n");

				//release data
				cvReleaseMat(& object_points2);
				cvReleaseMat(& image_points2);
				cvReleaseMat(& point_counts2);

				if(mapx != NULL) cvReleaseImage(&mapx);
				if(mapy != NULL) cvReleaseImage(&mapy);
				mapx = cvCreateImage( cvGetSize(main_image), IPL_DEPTH_32F, 1 );
				mapy = cvCreateImage( cvGetSize(main_image), IPL_DEPTH_32F, 1 );
				cvInitUndistortMap(Globals::intrinsic,Globals::distortion,mapx,mapy);

				//create new window
				cvNamedWindow( "Undistort" );
			}
		}
	}
	else
	{
		if(mapx == NULL || mapy == NULL)
		{
			mapx = cvCreateImage( cvGetSize(main_image), IPL_DEPTH_32F, 1 );
			mapy = cvCreateImage( cvGetSize(main_image), IPL_DEPTH_32F, 1 );
			cvInitUndistortMap(Globals::intrinsic,Globals::distortion,mapx,mapy);
		}
		IplImage *t = cvCloneImage(main_image);
		cvRemap( main_image, t, mapx, mapy );  // Undistort image

		cvLine(t, cvPoint( int(x_box - ceil(w_box/2.0)) , int(y_box - ceil(h_box/2.0)) ), cvPoint( int(x_box + ceil(w_box/2.0)) , int(y_box - ceil(h_box/2.0)) ), CV_RGB(0,255,0),2);
		cvLine(t, cvPoint( int(x_box - ceil(w_box/2.0)) , int(y_box + ceil(h_box/2.0)) ), cvPoint( int(x_box + ceil(w_box/2.0)) , int(y_box + ceil(h_box/2.0)) ), CV_RGB(0,255,0),2);
		cvLine(t, cvPoint( int(x_box - ceil(w_box/2.0)) , int(y_box - ceil(h_box/2.0)) ), cvPoint( int(x_box - ceil(w_box/2.0)) , int(y_box + ceil(h_box/2.0)) ), CV_RGB(0,255,0),2);
		cvLine(t, cvPoint( int(x_box + ceil(w_box/2.0)) , int(y_box - ceil(h_box/2.0)) ), cvPoint( int(x_box + ceil(w_box/2.0)) , int(y_box + ceil(h_box/2.0)) ), CV_RGB(0,255,0),2);
		
		//show options:
		cvRectangle(t, cvPoint(10,5), cvPoint(5+600,25), CV_RGB(100,150,100), CV_FILLED);
		cvRectangle(t, cvPoint(10,25), cvPoint(5+200,45), CV_RGB(100,150,100), CV_FILLED);
		Globals::Font::Write(t," 'g' chessboard calibration; 'z' reset z max and min; 'c' to exit calibration",cvPoint(10,20),FONT_HELP,0,0,0);
		Globals::Font::Write(t," 'r' reset;",cvPoint(10,40),FONT_HELP,0,0,0);
		cvShowImage("Undistort", t);
	}
}

void Calibrator::EndCalibration()
{
	cvDestroyWindow("Undistort");
}

void Calibrator::ProcessKey(char key)
{
	switch(key)
	{
	case KEY_CALIBRATION_GRID:
		StartChessBoardFinder();
		break;
	case KEY_RESET_Z:
		Globals::ResetZValues();
		break;
	case KEY_RESET:
		Globals::LoadDefaultDistortionMatrix();
		if(mapx != NULL) cvReleaseImage(&mapx);
		if(mapy != NULL) cvReleaseImage(&mapy);
		break;
	}		
}