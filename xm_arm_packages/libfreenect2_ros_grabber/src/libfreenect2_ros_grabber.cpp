/*
Copyright 2015, Giacomo Dabisias"
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
t (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
@Author 
Giacomo. Dabisias, PhD Student
PERCRO, (Laboratory of Perceptual Robotics)
Scuola Superiore Sant'Anna
via Luigi Alamanni 13D, San Giuliano Terme 56010 (PI), Italy

Based on libfreenect2pclgrabber test.cpp from https://github.com/cpaxton/libfreenect2pclgrabber/blob/master/test.cpp
build with some modification to disable 3D viewer and add ros publisher
*/
#include "k2g.h"
#include <pcl/visualization/cloud_viewer.h>
#include <chrono>

// extra headers for writing out ply file
#include <pcl/console/print.h>
#include <pcl/console/parse.h>
#include <pcl/console/time.h>
#include <pcl/conversions.h>
#include <pcl/io/ply_io.h>
#include <pcl/PCLPointCloud2.h>

// ros stuff
#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseArray.h>

// include to convert from messages to pointclouds and vice versa
#include <pcl_conversions/pcl_conversions.h>

#include <tf/transform_broadcaster.h>

using namespace pcl;
using namespace pcl::io;
using namespace pcl::console;

int main(int argc, char** argv)
{
  std::cout << "Syntax is: " << argv[0] << " [-processor 0|1|2] [output.ply]\n -processor options 0,1,2 correspond to CPU, OPENCL, and OPENGL respectively\n";
  processor freenectprocessor = OPENGL;
  std::vector<int> ply_file_indices;
  if(argc>1){
      int fnpInt;
      ply_file_indices = parse_file_extension_argument (argc, argv, ".ply");
      parse_argument (argc, argv, "-processor", fnpInt);
      freenectprocessor = static_cast<processor>(fnpInt);
  }
    
  boost::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> cloud;
  K2G k2g(freenectprocessor);
  std::cout << "getting cloud" << std::endl;
  cloud = k2g.getCloud();

  cloud->sensor_orientation_.w() = 0.0;
  cloud->sensor_orientation_.x() = 1.0;
  cloud->sensor_orientation_.y() = 0.0;
  cloud->sensor_orientation_.z() = 0.0;
  pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGB> rgb(cloud);
  
  //  Initialize ros node stuff
  // modify this topic name due to your need
  std::string POINTS_OUT("camera/depth_registered/points");
  // modify this frame_id due to the /robot_description param
  std::string frame_id("camera_link"); 
  ros::init(argc, argv, "libfreenect2_ros_grabber");
  ros::NodeHandle npc;
  ros::Publisher pc_pub = npc.advertise<sensor_msgs::PointCloud2>(POINTS_OUT,1000);

  tf::TransformBroadcaster br;
  int value=0;
  // because getting the PointCloud data real-time makes the cpu quitly rough, so we only need the data
  //  in the first 30 times
  while (ros::ok()&&value<30) {    
    cloud = k2g.updateCloud(cloud);
    pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGB> rgb(cloud);

    // br.sendTransform(
    //     tf::StampedTransform(
    //       tf::Transform(tf::Quaternion(0, 0, 0, 1), tf::Vector3(0.0, 0.0, 0.0)),
    //     ros::Time::now(), "/world", frame_id));

    sensor_msgs::PointCloud2 output_msg;
    toROSMsg(*cloud,output_msg);
    output_msg.header.frame_id = frame_id;
    pc_pub.publish(output_msg);
    std::cout<<value<<std::endl;
    value++;
  }
  k2g.shutDown();
  return 0;
}

