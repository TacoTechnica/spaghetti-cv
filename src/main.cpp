#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui.hpp>

#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#define RESIZE_FX 0.5
#define RESIZE_FY 0.5

#define MIN_CONTOUR_AREA 100
#define MAX_CONTOUR_AREA 172000
#define MAX_RADIUS 150

using namespace std;
using namespace cv;

bool process_frame(Mat &frame, Mat cumulative, Point &lastPoint);

struct save_file_data {
    Mat image;
};

void *save_file_routine(void *data);

void webcam() {
    namedWindow("filter");
    namedWindow("webcam");
    cvSetWindowProperty("webcam", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    VideoCapture cap = VideoCapture(0);

    cap.set(CAP_PROP_FPS, 12);
    cap.set(CAP_PROP_FRAME_WIDTH, 1280 / 2);
    cap.set(CAP_PROP_FRAME_HEIGHT, 720 / 2);
    cap.set(4,1024);

    Mat first;
    cap >> first;

    Mat final;

    Mat cumulative(first.size(), CV_8UC3);
    cumulative = Scalar(0, 0, 0);
    cvtColor(cumulative, cumulative, CV_BGR2HSV);

    Point lastPoint(-1, -1);
    int framesSinceFound = 0;
    while (true) {
        Mat frame;
        cap >> frame;

        struct timeval tv;
        gettimeofday(&tv, NULL);
        int start = tv.tv_usec / 1000;
        bool found = process_frame(frame, cumulative, lastPoint);
        gettimeofday(&tv, NULL);
        int end = tv.tv_usec / 1000;
        cout << "Processing time: " << (end - start) << "ms" << endl;
        if (found)
            framesSinceFound = 0;
        else
            framesSinceFound++;

        if (framesSinceFound > 30) {
            lastPoint.x = -1;
            lastPoint.y = -1;
        }

        imshow("webcam", cumulative);
        imshow("filter", frame);

        final = cumulative.clone();
        int key = waitKey(20);
        if (key == ' ') {
            lastPoint.x = -1;
            lastPoint.y = -1;
            cumulative = Scalar(0, 0, 0);
        } else if (key == 'q') {
            break;
        } else if (key == 's') {
            pthread_t thread;
            pthread_create(&thread, NULL, save_file_routine, &final);
            cout << "Saving file" << endl;
        }
    }
}


int main(int argc, char **argv) {
    webcam();

    return 0;
}

int step = 0;

// returns whether or not it found the post-it
bool process_frame(Mat &frame, Mat cumulative, Point &lastPoint) {
    Mat out = frame.clone();

    Size size = Size(round(frame.size().width * RESIZE_FX), round(frame.size().height * RESIZE_FY));
    //resize(frame, out, size);

    //Mat original = out.clone();

    cvtColor(out, out, COLOR_BGR2HSV);
    inRange(out, Scalar(163, 101, 131), Scalar(180, 255, 255), out);

    Size ksize = Size(3, 3);
    Mat kernel(7, 7, CV_8UC1);

    GaussianBlur(out, out, ksize, 1, 1);
    erode(out, out, kernel, Point(-1, -1), 1);
    dilate(out, out, kernel, Point(-1, -1), 1);

    flip(out, out, 1);

    // doesn't help for some reason
    //Canny(out, out, 100, 1);

    out.copyTo(frame);
    cvtColor(frame, frame, CV_GRAY2BGR);

    vector< vector<Point> > contours;
    vector<Vec4i> h;
    findContours(out, contours, h, RETR_TREE, CHAIN_APPROX_SIMPLE);

    // drawContours(frame, contours, -1, Scalar(255, 0, 0), 3);

    int largestIndex = -1;
    double largestSize;
    for (int i = 0; i < contours.size(); i++) {
        vector<Point> contour = contours[i];
        double mySize = contourArea(contour);
        if (i == 0 || (mySize > MIN_CONTOUR_AREA && mySize < MAX_CONTOUR_AREA && mySize > largestSize)) {
            largestIndex = i;
            largestSize = mySize;
        }
    }
    if (largestIndex >= 0) {
        double area = contourArea(contours[largestIndex]);
        if (area < MIN_CONTOUR_AREA || area > MAX_CONTOUR_AREA)
            largestIndex = -1;
    }

    if (largestIndex >= 0) {
        drawContours(frame, contours, largestIndex, Scalar(0, 255, 0), 3);
        cout << "Contour size " << contourArea(contours[largestIndex]) << endl;

        Point2f center;
        float radius;
        minEnclosingCircle(contours[largestIndex], center, radius);

        if (radius < MAX_RADIUS) {
            //Point2f scaledCenter(center.x / RESIZE_FX, center.y / RESIZE_FY);
            //radius *= 1 / ((RESIZE_FX + RESIZE_FY) / 2);

            //circle(cumulative, scaledCenter, radius, Scalar(255, 0, 0), 3);

            if (lastPoint.x != -1 && lastPoint.y != -1) {
                int thickness = round(radius / 4);
                Mat hsv(1, 1, CV_8UC3);
                hsv = Scalar(step, 255, 255);
                Mat bgr;
                cvtColor(hsv, bgr, CV_HSV2BGR);
                Vec3b vec = bgr.at<Vec3b>(0,0);
                Scalar color(vec.val[0], vec.val[1], vec.val[2]);
                line(cumulative, lastPoint, center, color, thickness);
                step = (step + 5) % 180;
            }
            lastPoint.x = center.x;
            lastPoint.y = center.y;
        }

        return true;
    } else {
        return false;
    }

    //size = Size(round(frame.size().width * ), round(frame.size().height * RESIZE_FY));
    // resize(out, out, frame.size());
    // return out;
}

void *save_file_routine(void *data) {
    Mat image = *((Mat*) data);

    for (int i = 1; ; i++) {
        char *fname;
        asprintf(&fname, "save_%04d.png", i);
        if (access(fname, F_OK) != -1) {
            continue;
        } else {
            imwrite(fname, image);
            break;
        }
    }

    return NULL;
}
