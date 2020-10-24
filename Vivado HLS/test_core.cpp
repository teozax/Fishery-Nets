#include <hls_opencv.h>
#include "core.h"

#define INPUT_IMAGE "C:\\Users\\x\\Pictures\\MATLAB\\1.tif"
#define OUTPUT_IMAGE "C:\\Users\\x\\Pictures\\MATLAB\\2.tif"

int main (int argc, char** argv)
{
	my_AXI_STREAM frame_axi, dst_axi;

//	  const char* lpFileName = "C:\\Users\\theoz\\Documents\\MATLAB\\my.mp4";
//    CvCapture* capture = NULL;
//    capture = cvCaptureFromFile(lpFileName);

  IplImage* source = cvCreateImage(cvSize(MAX_WIDTH, MAX_HEIGHT), IPL_DEPTH_8U, 3);
  IplImage* destination = cvCreateImage(cvSize(MAX_WIDTH, MAX_HEIGHT), IPL_DEPTH_8U, 3);


//	  source = cvQueryFrame(capture);
  source = cvLoadImage(INPUT_IMAGE);
//	while(source != NULL);
//    {
//        cvResize(source, frame);

        // Convert OpenCV format to AXI4 Stream format
       IplImage2AXIvideo(source, frame_axi);
        // Call the function to be synthesized
       net_holes_detection(frame_axi, dst_axi);
        // Convert the AXI4 Stream data to OpenCV format
       AXIvideo2IplImage(dst_axi, destination);

//       cvResize(destination, final);
       cvSaveImage(OUTPUT_IMAGE, destination);
//        source = cvQueryFrame(capture);
//    }


//	cvReleaseImage(&source);
//	cvReleaseImage(&destination);

	return 0;
}
