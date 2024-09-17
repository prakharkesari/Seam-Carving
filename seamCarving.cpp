#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

double computeEnergy(Mat &image, int x, int y){
    int height=image.rows;
    int width=image.cols;

    Vec3b left = image.at<Vec3b>(y,(x-1+width)%width);
    Vec3b right = image.at<Vec3b>(y,(x+1)%width);
    Vec3b up = image.at<Vec3b>((y-1+height)%height, x);
    Vec3b down = image.at<Vec3b>((y+1)%height, x);

    int deltaXRed = right[2]-left[2];
    int deltaXGreen = right[1]-left[1];
    int deltaXBlue = right[0]-left[0];
    double deltaXSquared = deltaXRed*deltaXRed + deltaXGreen*deltaXGreen + deltaXBlue*deltaXBlue;

    int deltaYRed = down[2]-up[2];
    int deltaYGreen = down[1]-up[1];
    int deltaYBlue = down[0]-up[0];
    double deltaYSquared = deltaYRed*deltaYRed + deltaYGreen*deltaYGreen + deltaYBlue*deltaYBlue;

    return sqrt(deltaXSquared + deltaYSquared);

}

Mat computeEnergyMatrix(Mat &image){
    int height= image.rows;
    int width=image.cols;

    Mat energy(height, width, CV_64F);

    for(int i=0;i<height;i++){
        for(int j=0;j<width; j++){
            energy.at<double>(i, j)= computeEnergy(image, j, i);
        }
    }

    return energy;
}

vector<int> findVerticalSeam(Mat &energy) {
    int height =energy.rows;
    int width =energy.cols;

    vector<vector<double>> dp(height, vector<double>(width, numeric_limits<double>::max()));
    vector<vector<int>> path(height, vector<int>(width, 0));

    for(int x = 0; x < width; x++){
        dp[0][x] =energy.at<double>(0, x);
    }

    for(int y = 1; y < height; y++){
        for(int x = 0; x < width; x++){
            double minEnergy =dp[y-1][x];
            int minIndex =x;

            if(x > 0 && dp[y-1][x-1] < minEnergy){
                minEnergy =dp[y-1][x-1];
                minIndex =x - 1;
            }
            if(x < width-1 && dp[y-1][x+1] <minEnergy){
                minEnergy = dp[y-1][x+1];
                minIndex =x+1;
            }

            dp[y][x] =energy.at<double>(y, x) +minEnergy;
            path[y][x] =minIndex;
        }
    }

    double minEnergy = dp[height-1][0];
    int minIndex = 0;
    for(int x = 1; x < width; ++x) {
        if(dp[height-1][x] < minEnergy) {
            minEnergy = dp[height-1][x];
            minIndex = x;
        }
    }

    vector<int> seam(height);
    seam[height-1] = minIndex;
    for(int y=height -2;y >= 0;y--){
        seam[y] = path[y+1][seam[y+1]];
    }

    return seam;
}

vector<int> findHorizontalSeam(Mat &energy) {
    int height = energy.rows;
    int width = energy.cols;

    vector<vector<double>> dp(height, vector<double>(width, numeric_limits<double>::max()));
    vector<vector<int>> path(height, vector<int>(width, 0));

    for(int y =0;y <height; y++) {
        dp[y][0] =energy.at<double>(y, 0);
    }

    for(int x =1; x <width; x++){
        for(int y =0;y <height;y++){
            double minEnergy = dp[y][x-1];
            int minIndex = y;

            if(y > 0 && dp[y-1][x-1] < minEnergy){
                minEnergy = dp[y-1][x-1];
                minIndex = y - 1;
            }
            if(y <height-1 && dp[y+1][x-1] <minEnergy){
                minEnergy = dp[y+1][x-1];
                minIndex = y+1;
            }

            dp[y][x] = energy.at<double>(y, x) + minEnergy;
            path[y][x] = minIndex;
        }
    }

    double minEnergy = dp[0][width-1];
    int minIndex = 0;
    for(int y =1;y <height; y++) {
        if(dp[y][width-1] < minEnergy) {
            minEnergy = dp[y][width-1];
            minIndex = y;
        }
    }

    vector<int> seam(width);
    seam[width-1] = minIndex;
    for(int x =width-2;x >= 0; x--){
        seam[x] = path[seam[x+1]][x+1];
    }

    return seam;
}

void markVerticalSeam(Mat &image, vector<int> &seam) {
    for (int y = 0; y < seam.size(); y++) {
        image.at<Vec3b>(y, seam[y]) = Vec3b(0, 0, 255);
    }
}

void markHorizontalSeam(Mat &image, vector<int> &seam) {
    for (int x = 0; x < seam.size(); x++) {
        image.at<Vec3b>(seam[x], x) = Vec3b(0, 0, 255);
    }
}

Mat removeVerticalSeam(Mat &image, vector<int> &seam) {
    int height = image.rows;
    int width = image.cols;

    Mat output(height, width-1, CV_8UC3);

    for(int y =0;y <height;y++){
        for(int x =0;x <seam[y]; x++){
            output.at<Vec3b>(y, x) = image.at<Vec3b>(y, x);
        }
        for(int x =seam[y]+1; x <width; x++){
            output.at<Vec3b>(y, x-1) = image.at<Vec3b>(y, x);
        }
    }

    return output;
}

Mat removeHorizontalSeam(Mat &image, vector<int> &seam) {
    int height = image.rows;
    int width = image.cols;

    Mat output(height-1, width, CV_8UC3);

    for(int x =0;x <width; x++){
        for(int y =0;y <seam[x]; y++){
            output.at<Vec3b>(y, x) =image.at<Vec3b>(y, x);
        }
        for(int y =seam[x]+1;y <height;y++){
            output.at<Vec3b>(y - 1, x) = image.at<Vec3b>(y, x);
        }
    }

    return output;
}

Mat seamCarving(Mat &image, int width, int height){
    Mat carvedImage = image.clone();
    namedWindow("Seam Carving is in progress", WINDOW_NORMAL);

    while(carvedImage.cols > width || carvedImage.rows > height){
        if(carvedImage.cols > width){
            Mat energy=computeEnergyMatrix(carvedImage);
            vector<int> seam= findVerticalSeam(energy);
            markVerticalSeam(carvedImage, seam);
            imshow("Seam Carving Progress", carvedImage);
            waitKey(50); 
            carvedImage= removeVerticalSeam(carvedImage, seam);
        }

        if(carvedImage.rows > height){
            Mat energy= computeEnergyMatrix(carvedImage);
            vector<int> seam= findHorizontalSeam(carvedImage);
            markHorizontalSeam(carvedImage, seam);
            imshow("Seam Carving Progress", carvedImage);
            waitKey(50);
            carvedImage = removeHorizontalSeam(carvedImage, seam);
        }
    }

    return carvedImage;
}

string extractDirectory(const string& filepath) {
    size_t pos = filepath.find_last_of("/\\");
    return (pos == string::npos) ? "" : filepath.substr(0, pos + 1);
}

int main(int argc, char** argv){

        if (argc != 4){
        cerr << "Usage: " << argv[0] << " <image_path> <new_width> <new_height>" << endl;
        return -1;
    }

    Mat image = imread(argv[1]);

    if (image.empty()) {
        cerr << "Error: Unable to load image at " << argv[1] << endl;
        return -1;
    }

    int width = stoi(argv[2]);
    int height = stoi(argv[3]);

        if (width <= 0 || height <= 0) {
        cerr << "Error: Width and height must be positive integers." << endl;
        return -1;
    }

    Mat result= seamCarving(image, width, height);

    string inputImagePath = argv[1];
    string outputDirectory = extractDirectory(inputImagePath);
    string outputImagePath = outputDirectory + "/carved_image.jpeg";
    imwrite(outputImagePath, result);

    namedWindow("Seam Carving Result", WINDOW_NORMAL);
    imshow("Seam Carving Result", result);
    waitKey(0);

    return 0;
}