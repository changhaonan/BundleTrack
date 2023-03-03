#pragma once
#include <Eigen/Dense>
#include <zmq.h>
#include <cppzmq/zmq.hpp>
#include <cppzmq/zmq_addon.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/eigen.hpp>
#include <iostream>
#include <memory>
// Declare below

namespace pfh
{
    class DetectorClient
    {
    public:
        DetectorClient(const int port, const int fixed_width, const int fixed_height);
        ~DetectorClient();
        bool SetUp();
        void Detect(const cv::Mat &image, const Eigen::Vector4f &roi, const float rot_deg, std::vector<cv::KeyPoint> &keypoints, cv::Mat &descriptors);

    private:
        int fixed_width_;
        int fixed_height_;
        int port_;
        zmq::context_t context_;
        zmq::socket_t socket_;
    };

#ifdef DETECTOR_CLIENT_IMPLEMENTATION
    // Implement below
    DetectorClient::DetectorClient(const int port, const int fixed_width, const int fixed_height)
        : port_(port), fixed_width_(fixed_width), fixed_height_(fixed_height), context_(1),
          socket_(context_, ZMQ_REQ){};

    DetectorClient::~DetectorClient()
    {
        socket_.close();
        context_.close();
    };

    bool DetectorClient::SetUp()
    {
        socket_.connect("tcp://0.0.0.0:" + std::to_string(port_));
        std::cout << "Image connected to port " << port_ << ", Keypoint connected to port " << port_ << "..." << std::endl;
        return true;
    };

    void DetectorClient::Detect(const cv::Mat &image, const Eigen::Vector4f &roi, const float rot_deg, std::vector<cv::KeyPoint> &keypoints, cv::Mat &descriptors)
    {
        // Check if image is empty
        if (image.empty())
        {
            std::cout << "Image is empty" << std::endl;
            throw std::runtime_error("Image is empty!");
            return;
        }
        // Check if roi is valid
        if (roi(0) < 0 || roi(1) > image.cols || roi(2) < 0 || roi(3) > image.rows)
        {
            std::cout << "ROI is invalid" << std::endl;
            throw std::runtime_error("ROI is invalid!");
            return;
        }
        // Get the first 3 channels of image
        cv::Mat image_3ch;
        if (image.channels() == 4)
            cv::cvtColor(image, image_3ch, cv::COLOR_RGBA2RGB);
        else
            image_3ch = image;

        const int W = roi(1) - roi(0);
        const int H = roi(3) - roi(2);

        Eigen::Matrix3f forward_transform(Eigen::Matrix3f::Identity());
        Eigen::Matrix3f new_transform(Eigen::Matrix3f::Identity());

        int side = std::max(H, W);
        cv::Mat img = cv::Mat::zeros(side, side, CV_8UC3);
        for (int h = 0; h < H; h++)
        {
            for (int w = 0; w < W; w++)
            {
                img.at<cv::Vec3b>(h, w) = image_3ch.at<cv::Vec3b>(h + roi(2), w + roi(0));
            }
        }

        new_transform.setIdentity();
        new_transform(0, 2) = -roi(0);
        new_transform(1, 2) = -roi(2);
        forward_transform = new_transform * forward_transform;

        if (rot_deg != 0)
        {
            cv::Mat M = cv::getRotationMatrix2D(cv::Point2f(side / 2, side / 2), rot_deg, 1);
            cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), img.size(), rot_deg).boundingRect2f();
            M.at<double>(0, 2) += bbox.width / 2.0 - img.cols / 2.0;
            M.at<double>(1, 2) += bbox.height / 2.0 - img.rows / 2.0;
            side = std::max(bbox.width, bbox.height);
            cv::warpAffine(img, img, M, {side, side});
            Eigen::Matrix<float, 2, 3> tmp;
            cv::cv2eigen(M, tmp);
            new_transform.setIdentity();
            new_transform.block(0, 0, 2, 3) = tmp;
            forward_transform = new_transform * forward_transform;
        }

        const int H_input = fixed_height_;
        const int W_input = fixed_width_;

        cv::resize(img, img, {W_input, H_input});
        new_transform.setIdentity();
        new_transform(0, 0) = W_input / float(side);
        new_transform(1, 1) = H_input / float(side);
        forward_transform = new_transform * forward_transform;

        {
            zmq::message_t msg(2 * sizeof(int));
            std::vector<int> wh = {img.cols, img.rows};
            std::memcpy(msg.data(), wh.data(), 2 * sizeof(int));
            socket_.send(msg, zmq::send_flags::sndmore);
        }

        {
            cv::Mat flat = img.reshape(1, img.total() * img.channels());
            std::vector<unsigned char> vec = img.isContinuous() ? flat : flat.clone();
            zmq::message_t msg(vec.size() * sizeof(unsigned char));
            std::memcpy(msg.data(), vec.data(), vec.size() * sizeof(unsigned char));
            socket_.send(msg, zmq::send_flags::none);
        }

        printf("[detector client]: waiting for reply\n");
        std::vector<zmq::message_t> recv_msgs;
        auto result = zmq::recv_multipart(socket_, std::back_inserter(recv_msgs));
        printf("[detector client]: got reply\n");

        std::vector<int> info(2);
        std::memcpy(info.data(), recv_msgs[0].data(), info.size() * sizeof(int));
        const int num_feat = info[0];
        const int feat_dim = info[1];

        if (num_feat == 0)
        {
            std::cout << "[detector client]: no feature detected." << std::endl;
            return;
        }

        std::vector<float> kpts_array(num_feat * 2);
        std::memcpy(kpts_array.data(), recv_msgs[1].data(), kpts_array.size() * sizeof(float));

        std::vector<float> feat_array(num_feat * feat_dim);
        std::memcpy(feat_array.data(), recv_msgs[2].data(), feat_array.size() * sizeof(float));

        keypoints.resize(num_feat);
        descriptors = cv::Mat::zeros(num_feat, feat_dim, CV_32F);
        Eigen::Matrix3f backward_transform = forward_transform.inverse();
        for (auto i = 0; i < num_feat; i++)
        {
            Eigen::Vector3f p(kpts_array[2 * i], kpts_array[2 * i + 1], 1);
            p = backward_transform * p;
            cv::KeyPoint kpt({p(0), p(1)}, 1);
            keypoints[i] = kpt;

            for (int j = 0; j < feat_dim; j++)
            {
                descriptors.at<float>(i, j) = feat_array[i * feat_dim + j];
            }
        }
    };
#endif

}
