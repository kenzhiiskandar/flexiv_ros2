/**
 * @file flexiv_hardware_interface.hpp
 * @brief Hardware interface to Flexiv robots for ROS 2 control. Adapted from
 * ros2_control_demos/ros2_control_demo_hardware/include/ros2_control_demo_hardware/rrbot_system_multi_interface.hpp
 * @copyright Copyright (C) 2016-2021 Flexiv Ltd. All Rights Reserved.
 * @author Flexiv
 */

#ifndef FLEXIV_HARDWARE__FLEXIV_HARDWARE_INTERFACE_HPP_
#define FLEXIV_HARDWARE__FLEXIV_HARDWARE_INTERFACE_HPP_

#include <memory>
#include <string>
#include <vector>

// ROS
#include <rclcpp/clock.hpp>
#include <rclcpp/duration.hpp>
#include <rclcpp/macros.hpp>
#include <rclcpp/logger.hpp>
#include <rclcpp/time.hpp>

// ros2_control hardware_interface
#include <hardware_interface/hardware_info.hpp>
#include <hardware_interface/base_interface.hpp>
#include <hardware_interface/system_interface.hpp>
#include <hardware_interface/types/hardware_interface_return_values.hpp>
#include <hardware_interface/types/hardware_interface_status_values.hpp>

#include "flexiv_hardware/visibility_control.h"

// Flexiv
#include "flexiv/Robot.hpp"

using hardware_interface::return_type;

namespace flexiv_hardware {

enum StoppingInterface
{
    NONE,
    STOP_POSITION,
    STOP_VELOCITY,
    STOP_EFFORT
};

class FlexivHardwareInterface
: public hardware_interface::BaseInterface<hardware_interface::SystemInterface>
{
public:
    RCLCPP_SHARED_PTR_DEFINITIONS(FlexivHardwareInterface)

    FLEXIV_HARDWARE_PUBLIC
    return_type configure(
        const hardware_interface::HardwareInfo& info) override;

    FLEXIV_HARDWARE_PUBLIC
    std::vector<hardware_interface::StateInterface>
    export_state_interfaces() override;

    FLEXIV_HARDWARE_PUBLIC
    std::vector<hardware_interface::CommandInterface>
    export_command_interfaces() override;

    FLEXIV_HARDWARE_PUBLIC
    return_type prepare_command_mode_switch(
        const std::vector<std::string>& start_interfaces,
        const std::vector<std::string>& stop_interfaces) override;

    FLEXIV_HARDWARE_PUBLIC
    return_type perform_command_mode_switch(
        const std::vector<std::string>& start_interfaces,
        const std::vector<std::string>& stop_interfaces) override;

    FLEXIV_HARDWARE_PUBLIC
    return_type start() override;

    FLEXIV_HARDWARE_PUBLIC
    return_type stop() override;

    FLEXIV_HARDWARE_PUBLIC
    return_type read() override;

    FLEXIV_HARDWARE_PUBLIC
    return_type write() override;

    static const size_t n_joints = 7;

private:
    // Flexiv RDK
    std::unique_ptr<flexiv::Robot> robot_;

    // Joint commands
    std::vector<double> hw_commands_joint_positions_;
    std::vector<double> hw_commands_joint_velocities_;
    std::vector<double> hw_commands_joint_efforts_;
    std::vector<double> internal_commands_joint_positions_;

    // Joint states
    std::vector<double> hw_states_joint_positions_;
    std::vector<double> hw_states_joint_velocities_;
    std::vector<double> hw_states_joint_efforts_;

    // Force-torque (FT) sensor raw reading in flange frame. The value is 0 if
    // no FT sensor is installed.
    std::vector<double> hw_states_force_torque_sensor_;

    // Estimated external wrench applied on TCP and expressed in base frame.
    std::vector<double> hw_states_external_wrench_in_base_;

    // Estimated external wrench applied on TCP and expressed in TCP frame.
    std::vector<double> hw_states_external_wrench_in_tcp_;

    // Measured TCP pose expressed in base frame [x, y, z, qx, qy, qz, qw].
    std::vector<double> hw_states_tcp_pose_;

    static rclcpp::Logger getLogger();

    // control modes
    bool controllers_initialized_;
    std::vector<uint> stop_modes_;
    std::vector<std::string> start_modes_;
    bool position_controller_running_;
    bool velocity_controller_running_;
    bool torque_controller_running_;

    // Store time between update loops
    rclcpp::Clock clock_;
    rclcpp::Time last_timestamp_;
    rclcpp::Time current_timestamp_;
};

} /* namespace flexiv_hardware */
#endif /* FLEXIV_HARDWARE__FLEXIV_HARDWARE_INTERFACE_HPP_ */
