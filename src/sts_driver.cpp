#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "sensor_msgs/msg/temperature.hpp"
#include "std_msgs/msg/int32.hpp"
#include "std_msgs/msg/float32.hpp"

// Include the Feetech SDK
#include "SCServo.h"

using namespace std::chrono_literals;

class FeetechDriverNode : public rclcpp::Node
{
public:
    FeetechDriverNode()
    : Node("feetech_driver_node")
    {
        // 1. Declare Parameters
        this->declare_parameter("serial_port", "/dev/HWT9075");
        this->declare_parameter("baud_rate", 1000000);
        this->declare_parameter("servo_id", 1);
        this->declare_parameter("default_speed", 2400); // Steps/sec
        this->declare_parameter("default_accel", 50);   // Acceleration

        // 2. Get Parameters
        std::string port = this->get_parameter("serial_port").as_string();
        int baud = this->get_parameter("baud_rate").as_int();
        servo_id_ = this->get_parameter("servo_id").as_int();
        speed_ = this->get_parameter("default_speed").as_int();
        accel_ = this->get_parameter("default_accel").as_int();

        // 3. Initialize Servo Connection
        RCLCPP_INFO(this->get_logger(), "Connecting to STS Servo on %s at %d baud...", port.c_str(), baud);
        if(!sm_st_.begin(baud, port.c_str())){
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize SMS/STS motor! Check connections/permissions.");
            rclcpp::shutdown();
        }

        // 4. Initialize Publishers
        joint_pub_ = this->create_publisher<sensor_msgs::msg::JointState>("feetech/joint_states", 10);
        temp_pub_ = this->create_publisher<sensor_msgs::msg::Temperature>("feetech/temperature", 10);
        volt_pub_ = this->create_publisher<std_msgs::msg::Float32>("feetech/voltage", 10);
        current_pub_ = this->create_publisher<std_msgs::msg::Int32>("feetech/current", 10);

        // 5. Initialize Subscriber (Position Control)
        // Topic expects a raw integer (0-4095)
        cmd_sub_ = this->create_subscription<std_msgs::msg::Float32>(
            "feetech/cmd_pos", 
            10, 
            std::bind(&FeetechDriverNode::command_callback, this, std::placeholders::_1)
        );

        // 6. Create Feedback Timer (20Hz = 50ms)
        timer_ = this->create_wall_timer(
            50ms, std::bind(&FeetechDriverNode::timer_callback, this)
        );

        RCLCPP_INFO(this->get_logger(), "Feetech Driver Started. Listening for ID: %d", servo_id_);
    }

    ~FeetechDriverNode() {
        sm_st_.end();
    }

private:
    // SDK Object
    SMS_STS sm_st_;
    
    // Config
    int servo_id_;
    int speed_;
    int accel_;

    // ROS Handles
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_pub_;
    rclcpp::Publisher<sensor_msgs::msg::Temperature>::SharedPtr temp_pub_;
    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr current_pub_;
    rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr volt_pub_;
    rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr cmd_sub_;
    rclcpp::TimerBase::SharedPtr timer_;

    // --- Command Callback ---
    void command_callback(const std_msgs::msg::Float32::SharedPtr msg)
    {
        // Clamp values to safe STS range (0 - 4095)

        int steps = static_cast<int>(msg->data*4095.0/(2 * M_PI));

        int target_pos = std::max(0, std::min(4095, steps));
        
        // Write Position using parameters for speed/accel
        sm_st_.WritePosEx(servo_id_, target_pos, speed_, accel_);
        
        // Logging (Debug)
        // RCLCPP_INFO(this->get_logger(), "Moving ID %d to %d", servo_id_, target_pos);
    }

    // --- Feedback Loop ---
    void timer_callback()
    {
        // Call FeedBack to fetch all parameters at once
        // This returns -1 if communication fails
        if(sm_st_.FeedBack(servo_id_) != -1)
        {
            // 1. Read Cached Data (-1 argument reads the buffer from FeedBack call)
            int pos = sm_st_.ReadPos(-1);
            int speed = sm_st_.ReadSpeed(-1);
            int load = sm_st_.ReadLoad(-1);
            int voltage = sm_st_.ReadVoltage(-1);
            int temper = sm_st_.ReadTemper(-1);
            int current = sm_st_.ReadCurrent(-1); // Optional

            // torque is max 30kg cm = 2.94 Nm. rated torque is 10kgcm

            

            // 2. Publish JointState
            auto joint_msg = sensor_msgs::msg::JointState();
            joint_msg.header.stamp = this->now();
            joint_msg.name.push_back("servo_" + std::to_string(servo_id_));
            

            float pos_turn = static_cast<float>(pos)/4095.0;
            float speed_turn = static_cast<float>(speed)/4095.0; 
            float pos_rad = pos_turn * 2 * M_PI;
            float speed_rad = speed_turn * 2 * M_PI;
            float load_nm = static_cast<float>(load* 2.94)/1000.0;

            // Note: STS3125 Raw Units. You might want to convert to Rad/s later.
            joint_msg.position.push_back(pos_rad); 
            joint_msg.velocity.push_back(speed_rad);
            joint_msg.effort.push_back(load_nm); 
            
            joint_pub_->publish(joint_msg);

            // 3. Publish Voltage (Raw is usually in 0.1V steps, check manual)
            // Typically Feetech returns voltage * 10. E.g. 120 = 12.0V
            auto volt_msg = std_msgs::msg::Float32();
            volt_msg.data = voltage / 10.0f; 
            volt_pub_->publish(volt_msg);


            auto current_msg = std_msgs::msg::Int32();
            current_msg.data = current;
            current_pub_->publish(current_msg);
            // 4. Publish Temperature
            auto temp_msg = sensor_msgs::msg::Temperature();
            temp_msg.header.stamp = this->now();
            temp_msg.temperature = static_cast<double>(temper);
            temp_pub_->publish(temp_msg);
        }
        else
        {
            RCLCPP_WARN(this->get_logger(), "Servo %d Feedback Timeout/Error", servo_id_);
        }
    }
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<FeetechDriverNode>());
    rclcpp::shutdown();
    return 0;
}