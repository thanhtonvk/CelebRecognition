#include "face_emb.h"

#include <string.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "cpu.h"
#include <opencv2/opencv.hpp>

FaceEmb::FaceEmb() {
}

int FaceEmb::load(AAssetManager *mgr) {

    modelEmb.clear();

    ncnn::set_cpu_powersave(2);
    ncnn::set_omp_num_threads(ncnn::get_big_cpu_count());

    modelEmb.opt = ncnn::Option();


    modelEmb.opt.num_threads = ncnn::get_big_cpu_count();

    char parampath[256];
    char modelpath[256];
    sprintf(parampath, "w600k_mbf.param");
    sprintf(modelpath, "w600k_mbf.bin");

    modelEmb.load_param(mgr, parampath);
    modelEmb.load_model(mgr, modelpath);

    return 0;
}

cv::Mat computeAffineMatrix(const std::vector<cv::Point2f> &src_points,
                            const std::vector<cv::Point2f> &dst_points) {
    CV_Assert(src_points.size() == 5 && dst_points.size() == 5);

    // Construct matrices A and B for solving the linear system
    cv::Mat A(10, 6, CV_32F, cv::Scalar(0));
    cv::Mat B(10, 1, CV_32F);

    for (int i = 0; i < 5; i++) {
        A.at<float>(i * 2, 0) = src_points[i].x;
        A.at<float>(i * 2, 1) = src_points[i].y;
        A.at<float>(i * 2, 2) = 1;
        A.at<float>(i * 2, 3) = 0;
        A.at<float>(i * 2, 4) = 0;
        A.at<float>(i * 2, 5) = 0;

        A.at<float>(i * 2 + 1, 0) = 0;
        A.at<float>(i * 2 + 1, 1) = 0;
        A.at<float>(i * 2 + 1, 2) = 0;
        A.at<float>(i * 2 + 1, 3) = src_points[i].x;
        A.at<float>(i * 2 + 1, 4) = src_points[i].y;
        A.at<float>(i * 2 + 1, 5) = 1;

        B.at<float>(i * 2, 0) = dst_points[i].x;
        B.at<float>(i * 2 + 1, 0) = dst_points[i].y;
    }

    // Solve for the affine transformation matrix parameters
    cv::Mat affine_params;
    cv::solve(A, B, affine_params, cv::DECOMP_SVD);

    // Construct the 2x3 affine transformation matrix
    cv::Mat affine_transform(2, 3, CV_32F);
    affine_transform.at<float>(0, 0) = affine_params.at<float>(0, 0);
    affine_transform.at<float>(0, 1) = affine_params.at<float>(1, 0);
    affine_transform.at<float>(0, 2) = affine_params.at<float>(2, 0);
    affine_transform.at<float>(1, 0) = affine_params.at<float>(3, 0);
    affine_transform.at<float>(1, 1) = affine_params.at<float>(4, 0);
    affine_transform.at<float>(1, 2) = affine_params.at<float>(5, 0);

    return affine_transform;
}

cv::Mat align_face(const cv::Mat &image, const std::vector<cv::Point2f> &detected_landmarks) {
    // Reference points (5 points for left eye, right eye, nose, left mouth corner, right mouth corner)
    std::vector<cv::Point2f> ref_points = {
            cv::Point2f(30.2946f, 51.6963f), // Left eye
            cv::Point2f(65.5318f, 51.5014f), // Right eye
            cv::Point2f(48.0252f, 71.7366f), // Nose
            cv::Point2f(33.5493f, 92.3655f), // Left mouth corner
            cv::Point2f(62.7299f, 92.2041f)  // Right mouth corner
    };

    // Compute the affine transform manually using 5 points
    cv::Mat affine_transform = computeAffineMatrix(detected_landmarks, ref_points);

    // Determine the output size (e.g., a typical aligned face size)
    cv::Size output_size(112, 112); // Example: typical aligned face size

    // Apply the affine transformation to the image
    cv::Mat aligned_image;
    cv::warpAffine(image, aligned_image, affine_transform, output_size);

    return aligned_image;
}

int FaceEmb::getEmbeding(cv::Mat src, cv::Point2f landmark[5], std::vector<float> &result,
                         cv::Mat &faceAligned) {

    std::vector<cv::Point2f> landmarks;
    for (int i = 0; i < 5; i++) {
        cv::Point2f p1 = cv::Point(landmark[i].x, landmark[i].y);
        landmarks.push_back(p1);
    }
//    aligner.AlignFace(src,landmarks,&faceAligned);
    faceAligned = align_face(src, landmarks);
    ncnn::Mat in_net = ncnn::Mat::from_pixels_resize(faceAligned.clone().data,
                                                     ncnn::Mat::PIXEL_BGR2RGB, faceAligned.cols,
                                                     faceAligned.rows,
                                                     112, 112);

    float norm[3] = {1 / 127.5f, 1 / 127.5f, 1 / 127.5f};
    float mean[3] = {127.5f, 127.5f, 127.5f};
    in_net.substract_mean_normalize(mean, norm);
    ncnn::Extractor extractor = modelEmb.create_extractor();
    extractor.input("input.1", in_net);
    ncnn::Mat outBlob;
    extractor.extract("516", outBlob);
    for (int i = 0; i < outBlob.w; i++) {
        float test = outBlob.row(0)[i];

        result.push_back(test);
    }
    return 0;
}

