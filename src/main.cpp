#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui.hpp>


using namespace std;
using namespace cv;

int main(int argc, char **argv) {
    namedWindow("original");
    namedWindow("after");
    Mat input = imread("tomatoes.jpg");
    Mat original = input.clone();

    cvtColor(input, input, COLOR_BGR2HSV);

    Mat out;
    inRange(input, Scalar(20, 0, 40), Scalar(40, 255, 255), out);

    erode(out, out, Mat(3, 3, CV_8UC1), Point(-1, -1), 1);
    dilate(out, out, Mat(3, 3, CV_8UC1), Point(-1, -1), 1);

    GaussianBlur(out, out, Size(3, 3), 0, 0);

    imshow("original", original);
    imshow("after", out);
    waitKey(0);

    return 0;
}
