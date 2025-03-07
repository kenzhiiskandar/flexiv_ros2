/**
 * @file external_tcp_wrench_broadcaster.cpp
 * @brief Controller to publish the estimated external wrench applied on TCP.
 * Adapted from ros2_controllers/force_torque_sensor_broadcaster
 * @copyright Copyright (C) 2016-2021 Flexiv Ltd. All Rights Reserved.
 * @author Flexiv
 */

#include "flexiv_controllers/external_tcp_wrench_broadcaster.hpp"

#include <memory>
#include <string>

namespace flexiv_controllers {

ExternalTcpWrenchBroadcaster::ExternalTcpWrenchBroadcaster()
: controller_interface::ControllerInterface()
{
}

controller_interface::InterfaceConfiguration
ExternalTcpWrenchBroadcaster::command_interface_configuration() const
{
    controller_interface::InterfaceConfiguration command_interfaces_config;
    command_interfaces_config.type
        = controller_interface::interface_configuration_type::NONE;
    return command_interfaces_config;
}

controller_interface::InterfaceConfiguration
ExternalTcpWrenchBroadcaster::state_interface_configuration() const
{
    controller_interface::InterfaceConfiguration state_interfaces_config;
    state_interfaces_config.type
        = controller_interface::interface_configuration_type::INDIVIDUAL;
    state_interfaces_config.names
        = force_torque_sensor_->get_state_interface_names();
    return state_interfaces_config;
}

controller_interface::return_type ExternalTcpWrenchBroadcaster::init(
    const std::string& controller_name)
{
    auto ret = ControllerInterface::init(controller_name);
    if (ret != controller_interface::return_type::OK) {
        return ret;
    }

    try {
        auto_declare<std::string>("sensor_name", "");
        auto_declare<std::string>("interface_names.force.x", "");
        auto_declare<std::string>("interface_names.force.y", "");
        auto_declare<std::string>("interface_names.force.z", "");
        auto_declare<std::string>("interface_names.torque.x", "");
        auto_declare<std::string>("interface_names.torque.y", "");
        auto_declare<std::string>("interface_names.torque.z", "");
        auto_declare<std::string>("frame_id", "");
        auto_declare<std::string>("topic_name", "");
    } catch (const std::exception& e) {
        fprintf(stderr,
            "Exception thrown during init stage with message: %s \n", e.what());
        return controller_interface::return_type::ERROR;
    }

    return controller_interface::return_type::OK;
}

CallbackReturn ExternalTcpWrenchBroadcaster::on_configure(
    const rclcpp_lifecycle::State& /*previous_state*/)
{
    sensor_name_ = node_->get_parameter("sensor_name").as_string();
    interface_names_[0]
        = node_->get_parameter("interface_names.force.x").as_string();
    interface_names_[1]
        = node_->get_parameter("interface_names.force.y").as_string();
    interface_names_[2]
        = node_->get_parameter("interface_names.force.z").as_string();
    interface_names_[3]
        = node_->get_parameter("interface_names.torque.x").as_string();
    interface_names_[4]
        = node_->get_parameter("interface_names.torque.y").as_string();
    interface_names_[5]
        = node_->get_parameter("interface_names.torque.z").as_string();

    const bool no_interface_names_defined
        = std::count(interface_names_.begin(), interface_names_.end(), "") == 6;

    if (sensor_name_.empty() && no_interface_names_defined) {
        RCLCPP_ERROR(node_->get_logger(),
            "'sensor_name' or at least one "
            "'interface_names.[force|torque].[x|y|z]' parameter has to be "
            "specified.");
        return CallbackReturn::ERROR;
    }

    if (!sensor_name_.empty() && !no_interface_names_defined) {
        RCLCPP_ERROR(node_->get_logger(),
            "both 'sensor_name' and "
            "'interface_names.[force|torque].[x|y|z]' parameters can not be "
            "specified together.");
        return CallbackReturn::ERROR;
    }

    frame_id_ = node_->get_parameter("frame_id").as_string();
    if (frame_id_.empty()) {
        RCLCPP_ERROR(
            node_->get_logger(), "'frame_id' parameter has to be provided.");
        return CallbackReturn::ERROR;
    }

    if (!sensor_name_.empty()) {
        force_torque_sensor_
            = std::make_unique<semantic_components::ForceTorqueSensor>(
                semantic_components::ForceTorqueSensor(sensor_name_));
    } else {
        force_torque_sensor_
            = std::make_unique<semantic_components::ForceTorqueSensor>(
                semantic_components::ForceTorqueSensor(interface_names_[0],
                    interface_names_[1], interface_names_[2],
                    interface_names_[3], interface_names_[4],
                    interface_names_[5]));
    }

    topic_name_ = node_->get_parameter("topic_name").as_string();
    if (topic_name_.empty()) {
        RCLCPP_ERROR(
            node_->get_logger(), "'topic_name' parameter has to be provided.");
        return CallbackReturn::ERROR;
    }

    try {
        // register ft sensor data publisher
        sensor_state_publisher_
            = node_->create_publisher<geometry_msgs::msg::WrenchStamped>(
                "~/" + topic_name_, rclcpp::SystemDefaultsQoS());
        realtime_publisher_
            = std::make_unique<StatePublisher>(sensor_state_publisher_);
    } catch (const std::exception& e) {
        fprintf(stderr,
            "Exception thrown during publisher creation at configure stage "
            "with message : %s \n",
            e.what());
        return CallbackReturn::ERROR;
    }

    realtime_publisher_->lock();
    realtime_publisher_->msg_.header.frame_id = frame_id_;
    realtime_publisher_->unlock();

    RCLCPP_DEBUG(node_->get_logger(), "configure successful");
    return CallbackReturn::SUCCESS;
}

controller_interface::return_type ExternalTcpWrenchBroadcaster::update()
{
    if (realtime_publisher_ && realtime_publisher_->trylock()) {
        realtime_publisher_->msg_.header.stamp = node_->now();
        force_torque_sensor_->get_values_as_message(
            realtime_publisher_->msg_.wrench);
        realtime_publisher_->unlockAndPublish();
    }

    return controller_interface::return_type::OK;
}

CallbackReturn ExternalTcpWrenchBroadcaster::on_activate(
    const rclcpp_lifecycle::State& /*previous_state*/)
{
    force_torque_sensor_->assign_loaned_state_interfaces(state_interfaces_);
    return CallbackReturn::SUCCESS;
}

CallbackReturn ExternalTcpWrenchBroadcaster::on_deactivate(
    const rclcpp_lifecycle::State& /*previous_state*/)
{
    force_torque_sensor_->release_interfaces();
    return CallbackReturn::SUCCESS;
}

} /* namespace flexiv_controllers */

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(flexiv_controllers::ExternalTcpWrenchBroadcaster,
    controller_interface::ControllerInterface)
