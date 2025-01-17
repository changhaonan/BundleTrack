/*
Authors: Bowen Wen
Contact: wenbowenxjtu@gmail.com
Created in 2021

Copyright (c) Rutgers University, 2021 All rights reserved.

Bowen Wen and Kostas Bekris. "BundleTrack: 6D Pose Tracking for Novel Objects
 without Instance or Category-Level 3D Models."
 In IEEE/RSJ International Conference on Intelligent Robots and Systems (IROS). 2021.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Bowen Wen, Kostas Bekris, Rutgers University,
      nor the names of its contributors may be used to
      endorse or promote products derived from this software without
      specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Utils.h"
#include <vtkPolyLine.h>
#include <vtkImageReader2Factory.h>
#include <vtkImageReader2.h>
#include <vtkImageData.h>
#include <vtkImageFlip.h>

namespace Utils
{

  float rotationGeodesicDistance(const Eigen::Matrix3f &R1, const Eigen::Matrix3f &R2)
  {
    float tmp = ((R1 * R2.transpose()).trace() - 1) / 2.0;
    tmp = std::max(std::min(1.0f, tmp), -1.0f);
    return std::acos(tmp);
  }

  void readDepthImage(cv::Mat &depthImg, std::string path)
  {
    cv::Mat depthImgRaw = cv::imread(path, CV_16UC1);
    depthImg = cv::Mat::zeros(depthImgRaw.rows, depthImgRaw.cols, CV_32FC1);
    for (int u = 0; u < depthImgRaw.rows; u++)
      for (int v = 0; v < depthImgRaw.cols; v++)
      {
        unsigned short depthShort = depthImgRaw.at<unsigned short>(u, v);
        float depth = (float)depthShort * 0.001;
        if (depth < 0.1)
        {
          depthImg.at<float>(u, v) = 0.0;
        }
        else
        {
          depthImg.at<float>(u, v) = depth;
        }
      }
  }

  template <class PointT>
  void convert3dOrganizedRGB(cv::Mat &objDepth, cv::Mat &colImage, Eigen::Matrix3f &camIntrinsic, boost::shared_ptr<pcl::PointCloud<PointT>> objCloud)
  {
    const int imgWidth = objDepth.cols;
    const int imgHeight = objDepth.rows;

    objCloud->height = (uint32_t)imgHeight;
    objCloud->width = (uint32_t)imgWidth;
    objCloud->is_dense = false;
    objCloud->points.resize(objCloud->width * objCloud->height);

    const float bad_point = 0;

    for (int u = 0; u < imgHeight; u++)
      for (int v = 0; v < imgWidth; v++)
      {
        float depth = objDepth.at<float>(u, v);
        cv::Vec3b colour = colImage.at<cv::Vec3b>(u, v);
        if (depth > 0.1 && depth < 2.0)
        {
          (*objCloud)(v, u).x = (float)((v - camIntrinsic(0, 2)) * depth / camIntrinsic(0, 0));
          (*objCloud)(v, u).y = (float)((u - camIntrinsic(1, 2)) * depth / camIntrinsic(1, 1));
          (*objCloud)(v, u).z = depth;
          (*objCloud)(v, u).b = colour[0];
          (*objCloud)(v, u).g = colour[1];
          (*objCloud)(v, u).r = colour[2];
        }
        else
        {
          (*objCloud)(v, u).x = bad_point;
          (*objCloud)(v, u).y = bad_point;
          (*objCloud)(v, u).z = bad_point;
          (*objCloud)(v, u).b = 0;
          (*objCloud)(v, u).g = 0;
          (*objCloud)(v, u).r = 0;
        }
      }
  }
  template void convert3dOrganizedRGB(cv::Mat &objDepth, cv::Mat &colImage, Eigen::Matrix3f &camIntrinsic, boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> objCloud);
  template void convert3dOrganizedRGB(cv::Mat &objDepth, cv::Mat &colImage, Eigen::Matrix3f &camIntrinsic, boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGBNormal>> objCloud);

  void readDirectory(const std::string &name, std::vector<std::string> &v)
  {
    v.clear();
    DIR *dirp = opendir(name.c_str());
    if (dirp == NULL)
    {
      printf("Reading directory failed: %s\n", name.c_str());
    }
    struct dirent *dp;
    while ((dp = readdir(dirp)) != NULL)
    {
      if (std::string(dp->d_name) == "." || std::string(dp->d_name) == "..")
        continue;
      v.push_back(dp->d_name);
    }
    closedir(dirp);
    // Sort the files in number order
    std::sort(v.begin(), v.end(), [](const std::string &a, const std::string &b)
              { return std::stoi(a) < std::stoi(b); });
  }

  void readImagesInOrder(const std::string &name, std::vector<std::string> &v, const std::string &prefix, const std::string &suffix, int n_leading_zeros)
  {
    v.clear();
    // read all files in directory using std::filesystem
  }

  template <class PointT>
  void downsamplePointCloud(boost::shared_ptr<pcl::PointCloud<PointT>> cloud_in, boost::shared_ptr<pcl::PointCloud<PointT>> cloud_out, float vox_size)
  {
    pcl::VoxelGrid<PointT> vox;
    vox.setInputCloud(cloud_in);
    vox.setLeafSize(vox_size, vox_size, vox_size);
    vox.filter(*cloud_out);
  }
  template void downsamplePointCloud<pcl::PointXYZRGBNormal>(boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGBNormal>> cloud_in, boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGBNormal>> cloud_out, float vox_size);
  template void downsamplePointCloud<pcl::PointXYZRGB>(boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> cloud_in, boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> cloud_out, float vox_size);
  template void downsamplePointCloud<pcl::PointXYZ>(boost::shared_ptr<pcl::PointCloud<pcl::PointXYZ>> cloud_in, boost::shared_ptr<pcl::PointCloud<pcl::PointXYZ>> cloud_out, float vox_size);
  template void downsamplePointCloud<pcl::PointNormal>(boost::shared_ptr<pcl::PointCloud<pcl::PointNormal>> cloud_in, boost::shared_ptr<pcl::PointCloud<pcl::PointNormal>> cloud_out, float vox_size);
  template void downsamplePointCloud<pcl::PointSurfel>(boost::shared_ptr<pcl::PointCloud<pcl::PointSurfel>> cloud_in, boost::shared_ptr<pcl::PointCloud<pcl::PointSurfel>> cloud_out, float vox_size);

  void parsePoseTxt(std::string filename, Eigen::Matrix4f &out)
  {
    parseMatrixTxt<4, 4>(filename, out);
  }

  void normalizeRotationMatrix(Eigen::Matrix3f &R)
  {
    for (int col = 0; col < 3; col++)
    {
      R.col(col).normalize();
    }
  }

  void normalizeRotationMatrix(Eigen::Matrix4f &pose)
  {
    for (int col = 0; col < 3; col++)
    {
      pose.block(0, col, 3, 1).normalize();
    }
  }

  bool isPixelInsideImage(const int H, const int W, float u, float v)
  {
    u = std::round(u);
    v = std::round(v);
    if (u < 0 || u >= W || v < 0 || v >= H)
      return false;
    return true;
  }

  void solveRigidTransformBetweenPoints(const Eigen::MatrixXf &points1, const Eigen::MatrixXf &points2, Eigen::Matrix4f &pose)
  {
    assert(points1.cols() == 3 && points1.rows() >= 3 && points2.cols() == 3 && points2.rows() >= 3);
    pose.setIdentity();

    Eigen::Vector3f mean1 = points1.colwise().mean();
    Eigen::Vector3f mean2 = points2.colwise().mean();

    Eigen::MatrixXf P = points1.rowwise() - mean1.transpose();
    Eigen::MatrixXf Q = points2.rowwise() - mean2.transpose();
    Eigen::MatrixXf S = P.transpose() * Q;
    assert(S.rows() == 3 && S.cols() == 3);
    Eigen::JacobiSVD<Eigen::MatrixXf> svd(S, Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::Matrix3f R = svd.matrixV() * svd.matrixU().transpose();
    if (!((R.transpose() * R).isApprox(Eigen::Matrix3f::Identity())))
    {
      pose.setIdentity();
      return;
    }

    if (R.determinant() < 0)
    {
      auto V_new = svd.matrixV();
      V_new.col(2) = (-V_new.col(2)).eval();
      R = V_new * svd.matrixU().transpose();
    }
    pose.block(0, 0, 3, 3) = R;
    pose.block(0, 3, 3, 1) = mean2 - R * mean1;
    if (!isMatrixFinite(pose))
    {
      pose.setIdentity();
      return;
    }
  }

  void drawProjectPoints(PointCloudRGBNormal::Ptr cloud, const Eigen::Matrix3f &K, cv::Mat &out)
  {
    Utils::downsamplePointCloud(cloud, cloud, 0.01);
    for (const auto &pt : cloud->points)
    {
      int u = std::round(pt.x * K(0, 0) / pt.z + K(0, 2));
      int v = std::round(pt.y * K(1, 1) / pt.z + K(1, 2));
      if (u < 0 || u >= out.cols || v < 0 || v >= out.rows)
        continue;
      cv::circle(out, {u, v}, 1, {0, 255, 255}, -1);
    }
  }

  bool parseIntrinsicTxt(std::string filename, Eigen::Matrix3f &K)
  {
    std::ifstream file(filename);
    if (!file.is_open())
    {
      printf("Cannot open file: %s\n", filename.c_str());
      return false;
    }

    std::string line;
    std::vector<std::string> words;

    // fx
    std::getline(file, line);
    boost::split(words, line, boost::is_any_of(" "));
    K(0, 0) = std::stof(words[1]);

    // fy
    std::getline(file, line);
    boost::split(words, line, boost::is_any_of(" "));
    K(1, 1) = std::stof(words[1]);

    // cx
    std::getline(file, line);
    boost::split(words, line, boost::is_any_of(" "));
    K(0, 2) = std::stof(words[1]);

    // cy
    std::getline(file, line);
    boost::split(words, line, boost::is_any_of(" "));
    K(1, 2) = std::stof(words[1]);
  }

  std::pair<float, float> getProjectPointCoord3Dto2D(float x, float y, float z, const Eigen::Matrix3f &K, cv::Mat &out)
  {

    int u = std::round(x * K(0, 0) / z + K(0, 2));
    int v = std::round(y * K(1, 1) / z + K(1, 2));
    if (u < 0)
      u = 0;
    else if (u >= out.cols)
      u = (out.cols - 1);
    if (v < 0)
      v = 0;
    else if (v >= out.rows)
      v = (out.rows - 1);

    cv::circle(out, {u, v}, 1, {0, 255, 255}, -1);

    return std::pair<float, float>(u, v);
  }

  void drawLine(cv::Mat &out, int x1, int y1, int x2, int y2, cv::Scalar color)
  {
    cv::Point p1(x1, y1);
    cv::Point p2(x2, y2);
    cv::line(out, p1, p2, color, 2);
  }

  void drawBBox(float x, float y, float z, const Eigen::Matrix3f &K, cv::Mat &out, const Eigen::Matrix4f &pose)
  {
    // pcl::PointCloud<pcl::PointXYZ> bbox;
    Eigen::MatrixXf bbox = Eigen::MatrixXf::Random(4, 9);
    int pointIdx = 0;
    float x_ranges[2] = {-0.05, 0.05};
    float y_ranges[2] = {-0.05, 0.05};
    float z_ranges[2] = {-0.05, 0.05};
    for (auto y_range : y_ranges)
      for (auto x_range : x_ranges)
        for (auto z_range : z_ranges)
        {
          bbox(0, pointIdx) = x + x_range;
          bbox(1, pointIdx) = y + y_range;
          bbox(2, pointIdx) = z + z_range;
          bbox(3, pointIdx) = 1;
          pointIdx++;
        }

    bbox(0, 8) = x;
    bbox(1, 8) = y;
    bbox(2, 8) = z;

    Eigen::MatrixXf bboxTrans = pose * bbox;

    std::pair<float, float> p[9];
    for (int idx = 0; idx < 9; idx++)
      p[idx] = getProjectPointCoord3Dto2D(bboxTrans(0, idx), bboxTrans(1, idx), bboxTrans(2, idx), K, out);

    cv::circle(out, {p[8].first, p[8].second}, 4, {0, 0, 255}, -1);

    int lineStartingPoints[12] = {0, 0, 0, 1, 1, 2, 2, 3, 4, 4, 5, 6};
    int lineEndingPoints[12] = {1, 2, 4, 3, 5, 3, 6, 7, 5, 6, 7, 7};

    for (int i = 0; i < 12; i++)
      drawLine(out, p[lineStartingPoints[i]].first, p[lineStartingPoints[i]].second, p[lineEndingPoints[i]].first, p[lineEndingPoints[i]].second, cv::Scalar(0, 255, 0));
  }

} // namespace Utils
