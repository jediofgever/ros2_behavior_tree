/*
 *  Copyright (c) 2015, Nagoya University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Autoware nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ros2_behavior_tree/pure_pursuit_controller.hpp"

#include <memory>
#include <sstream>

#include "rclcpp/rclcpp.hpp"

namespace ros2_behavior_tree
{

PurePursuitController::PurePursuitController(std::shared_ptr<rclcpp::Node> node, bool linear_interpolate_mode)
: node_(node),
  RADIUS_MAX_(9e10),
  KAPPA_MIN_(1/RADIUS_MAX_),
  linear_interpolate_(linear_interpolate_mode)
{
}

PurePursuitController::~PurePursuitController()
{
}

geometry_msgs::msg::Point
PurePursuitController::get_pose_of_next_waypoint() const
{
  return current_waypoints_.getWaypointPosition(num_of_next_waypoint_);
}

geometry_msgs::msg::Point
PurePursuitController::get_pose_of_next_target() const
{
  return position_of_next_target_;
}

geometry_msgs::msg::Pose
PurePursuitController::get_current_pose() const
{
  return current_pose_.pose;
}

double
PurePursuitController::get_lookahead_distance() const
{
  return lookahead_distance_;
}

void
PurePursuitController::callback_from_current_pose(const geometry_msgs::msg::PoseStamped::SharedPtr & msg)
{
  current_pose_.header = msg->header;
  current_pose_.pose = msg->pose;
  pose_set_ = true;
}

void
PurePursuitController::callback_from_current_velocity(const geometry_msgs::msg::TwistStamped::SharedPtr & msg)
{
  current_velocity_ = *msg;
  velocity_set_ = true;
}

void
PurePursuitController::callback_from_waypoints(const ros2_behavior_tree_msgs::msg::Lane::SharedPtr & msg)
{
  current_waypoints_.setPath(*msg);
  waypoint_set_ = true;
}

double
PurePursuitController::get_cmd_velocity(int waypoint) const
{
  if (current_waypoints_.isEmpty()) {
    RCLCPP_WARN(node_->get_logger(), "PurePursuitController: waypoints not loaded");
    return 0;
  }

  double velocity = current_waypoints_.getWaypointVelocityMPS(waypoint);

  std::stringstream ss;
  ss << "waypoint : " << mps2kmph(velocity) << " km/h (" << velocity << "m/s)";
  RCLCPP_INFO(node_->get_logger(), ss.str().c_str());

  return velocity;
}

void
PurePursuitController::calc_lookahead_distance(int waypoint)
{
  double current_velocity_mps = current_velocity_.twist.linear.x;
  double maximum_lookahead_distance =  current_velocity_mps * 10;
  double ld = current_velocity_mps * lookahead_distance_calc_ratio_;

  lookahead_distance_ = ld < minimum_lookahead_distance_ ? minimum_lookahead_distance_
                      : ld > maximum_lookahead_distance ? maximum_lookahead_distance
                      : ld ;

  RCLCPP_INFO(node_->get_logger(), "lookahead distance: %f", lookahead_distance_);
}

double
PurePursuitController::calc_curvature(geometry_msgs::msg::Point target) const
{
  double kappa;
  double denominator = pow(getPlaneDistance(target, current_pose_.pose.position), 2);
  double numerator = 2 * calcRelativeCoordinate(target, current_pose_.pose).y;

  if (denominator != 0) {
    kappa = numerator / denominator;
  } else {
    if (numerator > 0) {
      kappa = KAPPA_MIN_;
    } else {
      kappa = -KAPPA_MIN_;
    }
  }

  RCLCPP_INFO(node_->get_logger(), "kappa: f", kappa);
  return kappa;
}

// linear interpolation of next target
bool
PurePursuitController::interpolate_next_target(int next_waypoint, geometry_msgs::msg::Point * next_target) const
{
  constexpr double ERROR = pow(10, -5);  // 0.00001

  int path_size = static_cast<int>(current_waypoints_.getSize());

  if (next_waypoint == path_size - 1) {
    *next_target = current_waypoints_.getWaypointPosition(next_waypoint);
    return true;
  }

  double search_radius = lookahead_distance_;

  geometry_msgs::msg::Point zero_p;
  geometry_msgs::msg::Point end = current_waypoints_.getWaypointPosition(next_waypoint);
  geometry_msgs::msg::Point start = current_waypoints_.getWaypointPosition(next_waypoint - 1);

  // Let the linear equation be "ax + by + c = 0"
  // If there are two points (x1,y1) , (x2,y2), a = "y2-y1, b = "(-1) * x2 - x1" ,c = "(-1) * (y2-y1)x1 + (x2-x1)y1"
  double a = 0;
  double b = 0;
  double c = 0;

  double get_linear_flag = getLinearEquation(start, end, &a, &b, &c);
  if (!get_linear_flag) {
    return false;
  }

  // Let the center of circle be "(x0,y0)", in my code , the center of circle is vehicle position
  // the distance  "d" between the foot of a perpendicular line and the center of circle is ...
  //    | a * x0 + b * y0 + c |
  // d = -------------------------------
  //          √( a~2 + b~2)
  double d = getDistanceBetweenLineAndPoint(current_pose_.pose.position, a, b, c);

  // ROS_INFO("a : %lf ", a);
  // ROS_INFO("b : %lf ", b);
  // ROS_INFO("c : %lf ", c);
  // ROS_INFO("distance : %lf ", d);

  if (d > search_radius) {
    return false;
  }

  // unit vector of point 'start' to point 'end'
  tf2::Vector3 v((end.x - start.x), (end.y - start.y), 0);
  tf2::Vector3 unit_v = v.normalize();

  // normal unit vectors of v
  tf2::Vector3 unit_w1 = rotateUnitVector(unit_v, 90);   // rotate to counter clockwise 90 degree
  tf2::Vector3 unit_w2 = rotateUnitVector(unit_v, -90);  // rotate to counter clockwise 90 degree

  // the foot of a perpendicular line
  geometry_msgs::msg::Point h1;
  h1.x = current_pose_.pose.position.x + d * unit_w1.getX();
  h1.y = current_pose_.pose.position.y + d * unit_w1.getY();
  h1.z = current_pose_.pose.position.z;

  geometry_msgs::msg::Point h2;
  h2.x = current_pose_.pose.position.x + d * unit_w2.getX();
  h2.y = current_pose_.pose.position.y + d * unit_w2.getY();
  h2.z = current_pose_.pose.position.z;

  // ROS_INFO("error : %lf", error);
  // ROS_INFO("whether h1 on line : %lf", h1.y - (slope * h1.x + intercept));
  // ROS_INFO("whether h2 on line : %lf", h2.y - (slope * h2.x + intercept));

  // check which of two foot of a perpendicular line is on the line equation
  geometry_msgs::msg::Point h;
  if (fabs(a * h1.x + b * h1.y + c) < ERROR) {
    h = h1;
    //   ROS_INFO("use h1");
  } else if (fabs(a * h2.x + b * h2.y + c) < ERROR) {
    //   ROS_INFO("use h2");
    h = h2;
  } else {
    return false;
  }

  // get intersection[s]
  // if there is a intersection
  if (d == search_radius) {
    *next_target = h;
    return true;
  } else {
    // if there are two intersection
    // get intersection in front of vehicle
    double s = sqrt(pow(search_radius, 2) - pow(d, 2));
    geometry_msgs::msg::Point target1;
    target1.x = h.x + s * unit_v.getX();
    target1.y = h.y + s * unit_v.getY();
    target1.z = current_pose_.pose.position.z;

    geometry_msgs::msg::Point target2;
    target2.x = h.x - s * unit_v.getX();
    target2.y = h.y - s * unit_v.getY();
    target2.z = current_pose_.pose.position.z;

    // ROS_INFO("target1 : ( %lf , %lf , %lf)", target1.x, target1.y, target1.z);
    // ROS_INFO("target2 : ( %lf , %lf , %lf)", target2.x, target2.y, target2.z);
    // displayLinePoint(a, b, c, target1, target2, h);  // debug tool

    // check intersection is between end and start
    double interval = getPlaneDistance(end, start);
    if (getPlaneDistance(target1, end) < interval) {
      // ROS_INFO("result : target1");
      *next_target = target1;
      return true;
    } else if (getPlaneDistance(target2, end) < interval) {
      // ROS_INFO("result : target2");
      *next_target = target2;
      return true;
    } else {
      // ROS_INFO("result : false ");
      return false;
    }
  }
}

bool
PurePursuitController::verify_following() const
{
  double a = 0;
  double b = 0;
  double c = 0;

  // Use next 2 waypts from current pose to make a line seg to check
  int next_wp = closest_waypoint_idx_ + 1;
  int next_next_wp = closest_waypoint_idx_ + 2;

  getLinearEquation(current_waypoints_.getWaypointPosition(next_wp),
                    current_waypoints_.getWaypointPosition(next_next_wp),
                    &a, &b, &c);

  double displacement = getDistanceBetweenLineAndPoint(current_pose_.pose.position, a, b, c);

  // Use angle from current pose to next waypoint to check
  double relative_angle = getRelativeAngle(current_waypoints_.getWaypointPose(next_wp),
                                           current_pose_.pose);

  // ROS_ERROR("side diff : %lf , angle diff : %lf",displacement,relative_angle);
  if (displacement < displacement_threshold_ && relative_angle < relative_angle_threshold_) {
    // ROS_INFO("Following : True");
    return true;
  } else {
    // ROS_INFO("Following : False");
    return false;
  }
}

geometry_msgs::msg::Twist
PurePursuitController::calc_twist(double curvature, double cmd_velocity) const
{
  // verify whether vehicle is following the path
  bool following_flag = verify_following();
  static double prev_angular_velocity = 0;

  geometry_msgs::msg::Twist twist;
  twist.linear.x = cmd_velocity;
  if (!following_flag) {
    // ROS_ERROR_STREAM("Not following");
    twist.angular.z = current_velocity_.twist.linear.x * curvature;
  } else {
    twist.angular.z = prev_angular_velocity;
  }

  prev_angular_velocity = twist.angular.z;
  return twist;
}


// Search for closest waypt from current pose
void
PurePursuitController::get_closest_waypoint()
{
  int path_size = static_cast<int>(current_waypoints_.getSize());

  // If waypoints are not given, do nothing
  if (path_size == 0) {
    closest_waypoint_idx_ = -1;
    return;
  }

  // Initialize distance to first waypoint
  int closest_wp = 0;
  double dist = getPlaneDistance(current_waypoints_.getWaypointPosition(0),
                                 current_pose_.pose.position);

  // Search for a closer waypoint
  for (int i = 1; i < path_size; i++) {
    double t_dist = getPlaneDistance(current_waypoints_.getWaypointPosition(i),
                                     current_pose_.pose.position);
    if (t_dist < dist) {
      // Found a closer waypoint, store it and keep searching
      dist = t_dist;
      closest_wp = i;
    }
  }

  // Store closest waypoint as output
  closest_waypoint_idx_ = closest_wp;
  // ROS_ERROR_STREAM("closest wp = " << closest_waypoint_idx_ << " dist = " << dist);
}

void
PurePursuitController::get_next_waypoint()
{
  int path_size = static_cast<int>(current_waypoints_.getSize());

  // if waypoints are not given, do nothing.
  if (path_size == 0) {
    num_of_next_waypoint_ = -1;
    return;
  }

  // look for the next waypoint, starting from closest waypoint to prevent
  // searching behind the car
  for (int i = closest_waypoint_idx_; i < path_size; i++) {
    // if search waypoint is the last
    if (i == (path_size - 1)) {
      // ROS_INFO("search waypoint is the last");
      num_of_next_waypoint_ = i;
      return;
    }

    // if there exists an effective waypoint
    if (getPlaneDistance(current_waypoints_.getWaypointPosition(i), current_pose_.pose.position) > lookahead_distance_) {
      num_of_next_waypoint_ = i;
      // ROS_ERROR_STREAM("wp = " << i << " dist = " << getPlaneDistance(current_waypoints_.getWaypointPosition(i), current_pose_.pose.position) );
      return;
    }
  }

  // if this program reaches here , it means we lost the waypoint!
  num_of_next_waypoint_ = -1;
  return;
}

geometry_msgs::msg::TwistStamped
PurePursuitController::output_zero() const
{
  geometry_msgs::msg::TwistStamped twist;
  twist.twist.linear.x = 0;
  twist.twist.angular.z = 0;
  twist.header.stamp = node_->now();
  return twist;
}

geometry_msgs::msg::TwistStamped
PurePursuitController::output_twist(geometry_msgs::msg::Twist t) const
{
  double g_lateral_accel_limit = 5.0;
  double ERROR = 1e-8;

  geometry_msgs::msg::TwistStamped twist;
  twist.twist = t;
  twist.header.stamp = node_->now();

  double v = t.linear.x;
  double omega = t.angular.z;
  double omega_abs = fabs(omega);

  if (omega_abs < ERROR) {
    return twist;
  }

  double max_v = g_lateral_accel_limit / omega_abs;
  double a = v * omega_abs;
  // ROS_INFO("lateral accel = %lf", a);

  twist.twist.linear.x = fabs(a) > g_lateral_accel_limit ? max_v : v;
  twist.twist.angular.z = omega;

  return twist;
}

geometry_msgs::msg::TwistStamped
PurePursuitController::go()
{
  if (!pose_set_ || !waypoint_set_ || !velocity_set_) {
    if (!pose_set_) {
       // ROS_WARN("position is missing");
     }

     if (!waypoint_set_) {
       // ROS_WARN("waypoint is missing");
     }

     if (!velocity_set_) {
       // ROS_WARN("velocity is missing");
    }

    return output_zero();
  }

  bool interpolate_flag = false;

  calc_lookahead_distance(1);

  // Search for closest waypoint to current pose
  get_closest_waypoint();

  // search next waypoint
  get_next_waypoint();
  if (num_of_next_waypoint_ == -1) {
    // ROS_WARN("lost next waypoint");
    return output_zero();
  }
  // ROS_ERROR_STREAM("next waypoint = " <<  num_of_next_waypoint_);

  // if g_linear_interpolate_mode is false or next waypoint is first or last
  if (!linear_interpolate_ || num_of_next_waypoint_ == 0 ||
      num_of_next_waypoint_ == (static_cast<int>(current_waypoints_.getSize() - 1))) {
    position_of_next_target_ = current_waypoints_.getWaypointPosition(num_of_next_waypoint_);

    return output_twist(calc_twist(calc_curvature(position_of_next_target_),
                                 get_cmd_velocity(closest_waypoint_idx_)));
  }

  // linear interpolation and calculate angular velocity
  interpolate_flag = interpolate_next_target(num_of_next_waypoint_, &position_of_next_target_);

  if (!interpolate_flag) {
    // ROS_ERROR_STREAM("lost target! ");
    return output_zero();
  }

  // ROS_INFO("next_target : ( %lf , %lf , %lf)", next_target.x, next_target.y,next_target.z);

  return output_twist(calc_twist(calc_curvature(position_of_next_target_),
                               get_cmd_velocity(closest_waypoint_idx_)));

// ROS_INFO("linear : %lf, angular : %lf",twist.twist.linear.x,twist.twist.angular.z);

#ifdef LOG
  std::ofstream ofs("/tmp/pure_pursuit.log", std::ios::app);
  ofs << _current_waypoints.getWaypointPosition(next_waypoint).x << " "
      << _current_waypoints.getWaypointPosition(next_waypoint).y << " " << next_target.x << " " << next_target.y
      << std::endl;
#endif
}

#if 0

int main(int argc, char **argv)
{
  ros::init(argc, argv, "pure_pursuit");

  ros::NodeHandle nh;

  bool linear_interpolate_mode = true;

  waypoint_follower::PurePursuitController pp(linear_interpolate_mode);

  ros::Publisher cmd_vel_pub = nh.advertise<geometry_msgs::TwistStamped>("twist_cmd", 1);

  ros::Subscriber waypoint_sub =
      nh.subscribe("final_waypoints", 1, &PurePursuitController::callback_from_waypoints, &pp);

  ros::Subscriber ndt_sub =
      nh.subscribe("current_pose", 1, &PurePursuitController::callback_from_current_pose, &pp);

  ros::Subscriber twist_sub =
      nh.subscribe("current_velocity", 1, &PurePursuitController::callback_from_current_velocity, &pp);

  constexpr int LOOP_RATE = 30;  // Processing frequency
  ros::Rate loop_rate(LOOP_RATE);

  while (ros::ok()) {
    ros::spinOnce();
    cmd_vel_pub.publish(pp.go());
    loop_rate.sleep();
  }

  return 0;
}
#endif

}  // namespace ros2_behavior_tree
