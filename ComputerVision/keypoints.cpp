/**
 *	@author	Alan Meekins
 *	@date	9/8/09
**/

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/time.h>
#include <list>
#include <vector>

#define GetPixelPtrD8(i,r,c,chan) i->imageData[((r*i->widthStep)+(c*i->nChannels)+chan)]
#define GetPixelPtrD16(i,r,c,chan) i->imageData[((r*(i->widthStep/sizeof(uint16_t)))+(c*i->nChannels)+chan)]
#define GetPixelPtrD32(i,r,c,chan) ((float*)i->imageData)[((r*768)+(c*i->nChannels)+chan)]

#define STARTUP_DELAY 0
#define DELTA_PX 2

using namespace std;

CvPoint roiStart;
bool roiMod=false;
bool roiSlide=false;
bool updateNeeded=true;

char waitPause();
void printUsage(char*);
IplImage* getFrame(CvCapture *);


void roiHandler(int event, int x, int y, int flags, void* i);
bool varianceThreshold(IplImage* img, CvPoint point, float minVar, float maxVar);

enum MediaType {Unknown=-1, Video=1, Photo=2, Camera=3};

ofstream dataFile;


int main(int argc, char** argv){
	string fileName="";
	int camIndex=-1;
    char flag;
    char options[]="hp:m:c::";
	char inputMedia=Unknown;
    CvCapture* capture=NULL;
	IplImage* frame=NULL;
	IplImage* greyFrame=NULL;


    while((flag=getopt(argc, argv, options)) != -1){
		switch(flag){
			case 'h':
				printUsage(argv[0]);
				exit(EXIT_SUCCESS);
			case 'c':
				if(optarg == 0){
					if(optind < argc){
						camIndex=atoi(argv[optind]);
					}
				}
				else{
					camIndex=atoi(optarg);
				}
				inputMedia=Camera;
				break;
			case 'p':
				fileName=optarg;
				inputMedia=Photo;
				break;
			case 'm':
				fileName=optarg;
				inputMedia=Video;
				break;
			default:
				printUsage(argv[0]);
				exit(-1);
		}
    }

    if(inputMedia==Video){
		capture = cvCaptureFromAVI(fileName.c_str());
		if(!capture){
			cerr<<"Error: Could not open video "<<fileName<<endl;
			exit(-1);
		}
		cout<<"Reading video "<<fileName<<endl;
		frame=getFrame(capture);
    }

	if(inputMedia==Photo){
		frame=cvLoadImage(fileName.c_str());
		if(frame == NULL){
			cout<<"Error: Could not load photo "<<fileName<<endl;
			exit(-1);
		}
		cout<<"Loaded photo "<<fileName<<endl;
	}

    if(inputMedia==Camera){
		capture = cvCaptureFromCAM(camIndex);
		if(!capture){
			cerr<<"Error: Could not open camera "<<camIndex<<endl;
			exit(-1);
		}
		cout<<"Reading from camera "<<camIndex<<endl;
		frame=getFrame(capture);
    }

	if(inputMedia==Unknown){
		cout<<"Option error!"<<endl;
		printUsage(argv[0]);
		exit(-1);
	}

	cvNamedWindow("InputFrame", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("Sobel", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("Canny", CV_WINDOW_AUTOSIZE);

	bool isPaused=false;
	bool stepMode=false;
	char keyVal='p';
	float milSecs=0;
	uint32_t frameCount=0;

	IplImage* gray = NULL;
	IplImage* sobelImg = NULL;
	IplImage* sobelImg1 = NULL;
	IplImage* sobelImg2 = NULL;
	IplImage* cannyImg = NULL;
	//IplImage* color_dst = NULL;
	//CvMemStorage* storage = NULL;
	//CvSeq* lines = NULL;

	//dataFile.open("keypointData.txt");

	while(keyVal!='q'){
		if(keyVal=='r'){	//RESET
			updateNeeded=true;
		}
		if(keyVal=='s'){
			cout<<"Enter, step mode"<<endl;
			cout<<"\tn - Next frame"<<endl;
			cout<<"\tq - Quit"<<endl;
			cout<<"\ts - Exit step mode"<<endl;
			stepMode=true;
		}
		if(stepMode){
			while(1){
				keyVal=cvWaitKey(50);
				if(keyVal=='s'){		//Stop step mode
					stepMode=false;
					keyVal=0;
					cout<<"Exit, step mode"<<endl;
					break;
				}
				else if(keyVal=='q'){	//Exit program
					stepMode=false;
					cout<<"Exit program"<<endl;
					break;
				}
				else if(keyVal=='n'){	//Next frame
					cout<<"Step"<<endl;
					updateNeeded=true;
					break;
				}
			}
		}

		if(updateNeeded || !isPaused){
			if(inputMedia != Photo){
				frame=getFrame(capture);
			}

			if(frameCount < STARTUP_DELAY){
				cout<<"Starting up"<< STARTUP_DELAY-frameCount <<endl;
				frameCount++;
			}
			else{
				//Time how long it takes to process
				timeval timeTemp;
				timeval timeDelta;
				gettimeofday(&timeTemp, NULL);

				/********* PROCESS IMAGE *******/

				vector<CvPoint> points;

				if(gray==NULL){
					gray=cvCreateImage( cvGetSize(frame), 8, 1 );
				}
				if(sobelImg==NULL){
					sobelImg=cvCreateImage( cvGetSize(frame), 8, 1 );
				}
				if(sobelImg1==NULL){
					sobelImg1=cvCreateImage( cvGetSize(frame), 8, 1 );
				}
				if(sobelImg2==NULL){
					sobelImg2=cvCreateImage( cvGetSize(frame), 8, 1 );
				}
				if(cannyImg==NULL){
					cannyImg=cvCreateImage( cvSize(frame->width+1, frame->height+1), IPL_DEPTH_32S, 1 );
				}
				cvCvtColor( frame, gray, CV_BGR2GRAY );

				//cvCornerHarris( gray, cannyImg, 3 );
				//cvCanny( gray, cannyImg, 500, 300, 3);
				cvIntegral( gray, cannyImg);
				cvSobel( gray, sobelImg1, 0, 1, 1);
				cvSobel( gray, sobelImg2, 1, 0, 1);

				cvAdd(sobelImg1, sobelImg2, sobelImg);

				for(int r=0; r < gray->height; r++){
					for(int c=0; c < gray->width-2; c++){
						if(GetPixelPtrD8(sobelImg, r ,c ,0) < 50){
							continue;
						}
						CvPoint p=cvPoint(c,r);
						if(true/*varianceThreshold(gray, p, 4000, 10000)*/){
							points.push_back(p);
							//GetPixelPtrD8(img,point.y,i,0)
							cvLine(frame, cvPoint(p.x-2, p.y), cvPoint(p.x+2, p.y), cvScalar(0,255,0), 1);
							cvLine(frame, cvPoint(p.x, p.y-2), cvPoint(p.x, p.y+2), cvScalar(0,255,0), 1);
						}
					}
				}

				/********* END PROCESSING *******/

				gettimeofday(&timeDelta, NULL);
				timeDelta.tv_sec-=timeTemp.tv_sec;
				timeDelta.tv_usec-=timeTemp.tv_usec;
				milSecs=timeDelta.tv_sec*1000 + timeDelta.tv_usec/1000;
				cout<<"Found " << points.size() << " interest points";
				cout<<" - Processed frame in "<<milSecs<<"ms."<<endl;
			}

			cvShowImage("InputFrame", frame);
			cvShowImage("Sobel", sobelImg);
			cvShowImage("Canny", cannyImg);
			//cvShowImage("Edges", color_dst);
			updateNeeded=false;
		}

		if(keyVal=='p'){
			isPaused=!isPaused;
			if(isPaused){
				frameCount=0;
			}
		}
		if(keyVal=='q'){
			break;
		}
		keyVal=cvWaitKey(10);
	}

	//dataFile.close();

	printf("Cleaning up\n");

	cvDestroyAllWindows();

	if(inputMedia==Photo){
		cvReleaseImage(&frame);
	}
	else{
		cvReleaseCapture(&capture);
	}

	if(gray != NULL){
		cvReleaseImage(&gray);
	}

	if(sobelImg!=NULL){
		cvReleaseImage(&sobelImg);
	}
	if(sobelImg1!=NULL){
		cvReleaseImage(&sobelImg1);
	}
	if(sobelImg2!=NULL){
		cvReleaseImage(&sobelImg2);
	}
	if(cannyImg!=NULL){
		cvReleaseImage(&cannyImg);
	}
}



char waitPause(){
	char keyVal=0;
	while(keyVal!='p' && keyVal!='q'){
		keyVal=cvWaitKey(50);
		if(keyVal=='q'){
			return 'q';
		}
		if(updateNeeded){
			return 'p';
		}
	}
	return 0;
}

void roiHandler(int event, int x, int y, int flags, void* i){
	IplImage* image=(IplImage*)i;
	switch(event){
		case CV_EVENT_LBUTTONDOWN:
			roiStart.x=x;
			roiStart.y=y;
			roiMod=true;
			//printf("Start X=%i Y=%i\n", x, y);
			//updateNeeded=true;
			break;
		case CV_EVENT_LBUTTONUP:
			roiMod=false;
		case CV_EVENT_MOUSEMOVE:
			if(roiMod){
				int minX=(int)fmin(roiStart.x, x);
				int minY=(int)fmin(roiStart.y, y);
				int width=((int)fmax(roiStart.x, x)) - minX;
				int height=((int)fmax(roiStart.y, y)) - minY;
				//printf("minX=%i minY=%i width=%i height=%i\n", minX, minY, width, height);
				if(width > 1 && height > 1){
					cvSetImageROI(image, cvRect( minX, minY, width, height));
					//printf("ROI Set\n");
				}
				updateNeeded=true;
			}
			else if(roiSlide){
				CvRect imgROI=cvGetImageROI(image);
				if(imgROI.width > 1 && imgROI.height > 1){
					cvSetImageROI(image, cvRect( x, y, imgROI.width, imgROI.height));
				}
				updateNeeded=true;
			}
			break;
		case CV_EVENT_RBUTTONDOWN:
			roiSlide=true;
			roiMod=false;
			break;
		case CV_EVENT_RBUTTONUP:
			roiSlide=false;
			break;
	}
}

IplImage* getFrame(CvCapture *cvCpt){
    if(!cvGrabFrame(cvCpt)){
        cerr<<"Warning: Could not grab frame"<<endl;
        return NULL;
    }
    return cvRetrieveFrame(cvCpt);
}

void printUsage(char *name){
	cout << "Usage: " << name << " [options]" << endl << endl;
	cout << "\t-c\t[camId]\tRead from camera" << endl;
	cout << "\t-m\t[movie]\tRead video from file" << endl;
	cout << "\t-p\t[photo]\tRead photo from file" << endl;
}

bool varianceThreshold(IplImage* img, CvPoint point, float minVar, float maxVar){
	if(point.x+DELTA_PX > img->width || point.x-DELTA_PX < 0){ return false; }

	if(point.y+DELTA_PX > img->height || point.y-DELTA_PX < 0){ return false; }

	float minV=HUGE_VALF;
	float maxV=0;

	float avg=0;
	float var=0;

	//Average Columns
	for(int i=point.y - DELTA_PX; i < DELTA_PX+point.y; i++){
		avg+=GetPixelPtrD8(img,i,point.x,0);
	}
	avg=avg/((2*DELTA_PX) + 1);

	//Column Variance
	for(int i=point.y - DELTA_PX; i < DELTA_PX+point.y; i++){
		float delta=GetPixelPtrD8(img,i,point.x,0) - avg;
		var+=delta*delta;
	}
	var=var*(2*DELTA_PX);

	if(var<minV){minV=var;}
	if(var>maxV){maxV=var;}
	if(var < minVar || var > maxVar){ return false; }

	avg=0;
	var=0;

	//Average Rows
	for(int i=point.x - DELTA_PX; i < DELTA_PX+point.x; i++){
		avg+=GetPixelPtrD8(img,point.y,i,0);
	}
	avg=avg/((2*DELTA_PX) + 1);

	//Row Variance
	for(int i=point.x - DELTA_PX; i < DELTA_PX+point.x; i++){
		float delta=GetPixelPtrD8(img,point.y,i,0) - avg;
		var+=delta*delta;
	}
	var=var/(2*DELTA_PX);

	if(var<minVar){minV=var;}
	if(var>maxVar){maxV=var;}

	if(var < minVar || var > maxVar){ return false; }

	//dataFile<<maxV<<","<<minV<<endl;

	avg=0;
	var=0;
/*
	//Average Diagonal
	for(int i=-DELTA_PX; i < DELTA_PX; i++){
		avg+=GetPixelPtrD8(img,point.y+i ,point.x+i,0);
	}
	avg=avg/((2*DELTA_PX) + 1);

	//Diagonal Variance
	for(int i=-DELTA_PX; i < DELTA_PX; i++){
		float delta=GetPixelPtrD8(img,point.y+i ,point.x+i,0) - avg;
		var+=delta*delta;
	}
	var=var/(2*DELTA_PX);
	if(var < minVar || var > maxVar){ return false; }

	avg=0;
	var=0;


	//Average Diagonal
	for(int i=-DELTA_PX; i < DELTA_PX; i++){
		avg+=GetPixelPtrD8(img,point.y-i ,point.x+i,0);
	}
	avg=avg/((2*DELTA_PX) + 1);

	//Diagonal Variance
	for(int i=-DELTA_PX; i < DELTA_PX; i++){
		float delta=GetPixelPtrD8(img,point.y-i ,point.x+i,0) - avg;
		var+=delta*delta;
	}
	var=var/(2*DELTA_PX);
	if(var < minVar || var > maxVar){ return false; }
*/
	return true;
}