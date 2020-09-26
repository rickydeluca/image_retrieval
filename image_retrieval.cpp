#include "image_retrieval.hpp"

using namespace std;
using namespace cv;

/** @function showSimImages */
void showSimImages(vector<int>& sim_images_idx, double (&dist_array)[DATABASE_SIZE], const int num_sim_images) {
    printf("Show the most similars images\n\n");

    int img_idx;
    String sim_image_path;
    Size size(800, 600);

    for (int i = 0; i < num_sim_images; i++) {
        img_idx = sim_images_idx[i] + 1;
        sim_image_path = "";

        if (img_idx < 10) {
            printf("img_00%d.JPG --> Dist: %f\n", img_idx, dist_array[img_idx-1]);

            sim_image_path = "./image_database/img_00" + to_string(img_idx) + ".JPG";
            Mat sim_image = imread(sim_image_path, IMREAD_COLOR);
            resize(sim_image, sim_image, size);

            namedWindow("Display Image", WINDOW_AUTOSIZE);
    		imshow("Display Image", sim_image);
			waitKey(0);
        }

        else {
            printf("img_0%d.JPG --> Dist: %f\n", img_idx, dist_array[img_idx-1]);

			sim_image_path = "./image_database/img_0" + to_string(img_idx) + ".JPG";
			Mat sim_image = imread(sim_image_path, IMREAD_COLOR);
			resize(sim_image, sim_image, size);

			namedWindow("Display Image", WINDOW_AUTOSIZE);
    		imshow("Display Image", sim_image);
			waitKey(0);
        }
    }
}

/* @function findAvgDist */
double findAvgDist(vector<cv::DMatch>& matches, int num_matches) {
    double dist;

    for (int i = 0; i < 10; i++) {
        dist += matches[i].distance;
    }

    return dist / num_matches;
}

/* @function findNumInliers */
int findNumInliers(vector<cv::DMatch>& matches, vector<cv::KeyPoint>& query_kp, vector<cv::KeyPoint>& db_img_kp) {
    // If there aren't matches at all, it's useless to search inliers
    if (matches.size() == 0)
        return 0.0;

    // Extract location of good matches
    vector<Point2f> query_points, db_img_points;

    for (size_t i = 0; i < matches.size(); i++) {
        query_points.push_back(query_kp[matches[i].queryIdx].pt);
        db_img_points.push_back(db_img_kp[matches[i].trainIdx].pt);
    }

    // Find homograpy and its mask 
    vector<uchar> mask;   // mask is a Nx1 matrix of 1s and 0s
    Mat h = findHomography(query_points, db_img_points, mask, RANSAC, 3.0);

    // Find which matches are inliers
    int num_inliers  = 0;
    int num_outliers = 0;
    for (size_t i = 0; i < mask.size(); i++) {
        if (mask[i]) {    // If "1" the good match is also an inliers
            num_inliers++;
        } else {
            num_outliers++;
        }
    }

    return num_inliers;
}

/** @function compare_response */
bool compare_response(DMatch first, DMatch second) {
    if (first.distance < second.distance)
        return true;
    else 
        return false;
}

int retrieveSiftDescriptors(Mat query, double (&desc_dist_array)[DATABASE_SIZE]) {
    Mat database_img;                       // Store the database image
    Mat query_desc, db_img_desc;            // Store descriptors
    vector<KeyPoint> query_kp, db_img_kp;   // Store keypoint
    vector<vector<DMatch>> matches;         // Store the matches between descriptors
    vector<DMatch> good_matches;            // Matches that pass the ratio test
    
    const float ratio       = 0.7;          // Value for the ratio test

    // Resize query and convert it to grayscale
    Size size(800,600);
    resize(query, query, size);
    cvtColor(query, query, COLOR_BGR2GRAY);
    
    // Init SIFT detector
    Ptr<xfeatures2d::SIFT> sift = xfeatures2d::SIFT::create();

    // Detect and compute keypoints and descriptors of the query
    query_kp.clear();
    sift->detectAndCompute(query, noArray(), query_kp, query_desc);

    // Create BFMatcher object with default params: NORM_L2, crossCheck = false
    Ptr<BFMatcher> bf = BFMatcher::create();

    // Scan the database
    VideoCapture cap("./image_database/img_%3d.JPG"); // %3d means 00x.JPG notation
	if (!cap.isOpened()) 							  // check if succeeded
		return -1;

    for (int i = 0; i < DATABASE_SIZE; i++) {
        // Read next image from database
        cap >> database_img;
        if (!database_img.data) {
            return -1;
        }

        // Resize database image and convert it to grayscale
        resize(database_img, database_img, size);
        cvtColor(database_img, database_img, COLOR_BGR2GRAY);
        
        // Detect and compute keypoints and descriptors of the database image
        db_img_kp.clear();
        sift->detectAndCompute(database_img, noArray(), db_img_kp, db_img_desc);

        // Match descriptors
        matches.clear();
        bf->knnMatch(query_desc, db_img_desc, matches, 2);

        // Apply ratio test
        good_matches.clear();
        for (int j = 0; j < matches.size(); j++) {
            if (matches[j][0].distance < ratio * matches[j][1].distance) {
                good_matches.push_back(matches[j][0]);
            }
        }

        // Refine using RANSAC
        int num_good_matches = findNumInliers(good_matches, query_kp, db_img_kp);

        // double num_good_matches = good_matches.size();
        // double dist = findAvgDist(good_matches, num_good_matches);

        printf("Num of SIFT matches with image %3d: %d\n", i+1, num_good_matches);
        desc_dist_array[i] = 1 / (double) num_good_matches;
    }

    return 0;
}

/** @function retrieveDescriptors */
int retrieveOrbDescriptors(Mat query, double (&desc_dist_array)[DATABASE_SIZE]) {
    Mat database_img;                       // Store the database image
    Mat query_desc, db_img_desc;            // Store descriptors
    Mat homography;                         // Store homography
    
    vector<KeyPoint> query_kp, db_img_kp;   // Store keypoint
    vector<DMatch> matches;                 // Store the matches between descriptors
    vector<DMatch> good_matches;
    
    const float GOOD_MATCH_PERCENT  = 1;

    // Resize the query and convert it to grayscale
    Size size(800,600);
    resize(query, query, size);
    cvtColor(query, query, COLOR_BGR2GRAY);

    // Create detector for ORB descriptors
    Ptr<ORB> orb = ORB::create();

    // Detect and compute keypoints and descriptors of the query
    query_kp.clear();
    orb->detectAndCompute(query, Mat(), query_kp, query_desc);

    // Create BFMatcher object
    Ptr<BFMatcher> bf = BFMatcher::create(NORM_HAMMING, true);

    // Scan the database
    VideoCapture cap("./image_database/img_%3d.JPG"); // %3d means 00x.JPG notation
	if (!cap.isOpened()) 							  // check if succeeded
		return -1;

    for (int i = 0; i < DATABASE_SIZE; i++) {
        // Read next image from database
        cap >> database_img;
        if (!database_img.data) {
            return -1;
        }

        // Resize database image and convert it to grayscale
        resize(database_img, database_img, size);
        cvtColor(database_img, database_img, COLOR_BGR2GRAY);

        // Detect and compute keypoints and descriptors of the database image
        db_img_kp.clear();
        orb->detectAndCompute(database_img, Mat(), db_img_kp, db_img_desc);
        
        // Match features
        matches.clear();
        bf->match(query_desc, db_img_desc, matches, Mat());

        // Sort matches by score
        std::sort(matches.begin(), matches.end(), compare_response);

        // Remove not so good matches
        int num_good_matches = matches.size() * GOOD_MATCH_PERCENT;
        matches.erase(matches.begin() + num_good_matches, matches.end());

        // Refine using RANSAC
        int num_good_matches_refined = findNumInliers(matches, query_kp, db_img_kp);
        
        printf("Num of ORB good matches with image %3d: %d\n", i+1, num_good_matches_refined);
        
        desc_dist_array[i] = 1 / (double) num_good_matches_refined;
    }

    return 0;
}


/** @function retrieveShapes */
int retrieveShapes(Mat query, double (&shape_dist_array)[DATABASE_SIZE]) {
    int i = 0;              // Iter
    double dist = 0.0;      // Store the distance between shapes
    Mat database_img;       // Store the images read from the database
    Mat thresh_query, thresh_db_img;

    // Convert to grayscale
    cvtColor(query, query, COLOR_BGR2GRAY);

    // Resize
    Size size(800,600);
    resize(query, query, size);

    // Canny(query, query, 100, 200, 3);
    // adaptiveThreshold(query, query, 255, ADAPTIVE_THRESH_MEAN_C, 
    //                       THRESH_BINARY_INV, 11, 2);
    threshold(query, query, 0, 255, THRESH_BINARY_INV + THRESH_OTSU);

    namedWindow("Display Query", WINDOW_AUTOSIZE);
    imshow("Display Query", query);
    waitKey(0);

    // Read images from database
    VideoCapture cap("./image_database/img_%3d.JPG"); // %3d means 00x.JPG notation
	if (!cap.isOpened()) 							  // check if succeeded
		return -1;

    // Scan the database
    for (i = 0; i < DATABASE_SIZE; i++) {
        // Read next image from database
        cap >> database_img;
        if (!database_img.data) {
            return -1;
        }
        
        // Convert to grayscale and resize
		cvtColor(database_img, database_img, COLOR_BGR2GRAY);
        resize(database_img, database_img, size);

        // Binarize the image using thresholding        
        // Canny(database_img, database_img, 100, 200, 3);
        // adaptiveThreshold(database_img, database_img, 255, ADAPTIVE_THRESH_MEAN_C, 
        //                  THRESH_BINARY_INV, 11, 2);
        
        threshold(database_img, database_img, 0, 255, THRESH_BINARY_INV + THRESH_OTSU);

        namedWindow("Display DB Img", WINDOW_AUTOSIZE);
        imshow("Display DB Img", database_img);
        waitKey(2);

        // Shape matching
        dist = matchShapes(query, database_img, CONTOURS_MATCH_I2, 0.0);
        printf("Shape distance from image %d: %f\n", i+1, dist);
        
        // Insert distances in the array
        shape_dist_array[i] = dist;
    }

    return 0;
}


/** @function retrieveColors */
int retrieveColors(Mat query, double (&color_dist_array)[DATABASE_SIZE]) {
    int i = 0;          // Iter
    Mat database_img;   // Store the images read from the database

    // Resize the image
    Size size(800,600);
    resize(query, query, size);

    // Convert to HSV colorspace
    cvtColor(query, query, COLOR_BGR2HSV);

    // Split query channels
	vector<Mat> channels;
	split(query, channels);
	Mat h0 = channels[0];
	Mat s0 = channels[1];
	Mat v0 = channels[2];

    // Read images from database
	VideoCapture cap("image_database/img_%3d.JPG"); // %3d means 00x.JPG notation
	if (!cap.isOpened()) 							// check if succeeded
		return -1;

    // Declare vars to store the database image channels
    vector<Mat> database_channels;
	Mat h1, s1, v1;			

    // Store color spaces distances
    double dh, ds, dv;  // Single color space distance
    double dist = 0.0;  // global color spaces distance

    for (i = 0; i < DATABASE_SIZE; i++) {
		// Read next image from database
        cap >> database_img;
        if (!database_img.data) {
            return -1;
        }
        // Resize image
        resize(database_img, database_img, size);
        
        // Convert to HSV color space
		cvtColor(database_img, database_img, COLOR_BGR2HSV);

		// Split database_img channels
		split(database_img, database_channels);
		h1 = database_channels[0];
		s1 = database_channels[1];
		v1 = database_channels[2];

        // Compute distance for each channel
		dh = norm(abs(h1 - h0), NORM_L2);
		ds = norm(abs(s1 - s0), NORM_L2);
		dv = norm(abs(v1 - v0), NORM_L2);

        // Get global color space distance
		dist = norm(dh + ds + dv, NORM_L2);
		printf("Color distance from %3d: %f\n", i+1, dist);

		// insert found distance in distance_array
		color_dist_array[i] = dist;
	}

    return 0;
}