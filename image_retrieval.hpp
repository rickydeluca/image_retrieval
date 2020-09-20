#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <limits>

#include "opencv2/shape.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/core/utility.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/calib3d.hpp>

#define DATABASE_SIZE   56
#define N_SIM_IMGS      3

void showSimShapes(int (&idx_array)[N_SIM_IMGS]);

int retrieveShapes(cv::Mat query, double (&shape_dist_array)[DATABASE_SIZE]);

int retrieveColors(cv::Mat query, double (&color_dist_array)[DATABASE_SIZE]);

int retrieveOrbDescriptors(cv::Mat query, double (&desc_dist_array)[DATABASE_SIZE]);

int retrieveSiftDescriptors(cv::Mat query, double (&desc_dist_array)[DATABASE_SIZE]);

double ransacClean(std::vector<cv::DMatch>& matches, std::vector<cv::KeyPoint>& query_kp, std::vector<cv::KeyPoint>& db_img_kp);