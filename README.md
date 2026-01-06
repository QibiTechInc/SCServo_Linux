# scservo_linux_driver

```bash
git clone https://github.com/QibiTechInc/SCServo_Linux -b feature/ros2 scservo_linux_driver
```

```bash
colcon build --packages-select scservo_linux_driver
ros2 run scservo_linux_driver scs_driver
```

### ROS Interface  

| topic name | Category | Type | Explanation |  
| ---- | ---- | ---- | ---- |
| /feetech/cmd_pos | Subscriber | std_msgs/Float32 | Commanding the rotation of the motor in radians. This does not know the position of where the linear shaft is due to mechanical reasons(nonlinearty) |
| /feetech/current | Publisher | std_msgs/Float32 | Current value in mA. It is used to know external force, but due to the current mechanical reasons(nonlinear damper), we cannot know external force.(The motor itself knows but not with the mechanism) |
| /feetech/joint_states | Publisher | sensor_msgs/JointState | Joint state. it supports position, effort, velocity feedback. use this to check robot current pose and output torque | 
| /feetech/temperature | Publisher | std_msgs/Float32 | Temperature in celcius | 
| /feetech/voltage | Publisher | std_msgs/Float32 | V | 

### ROS Paramaters  
| Parameter name | Type | Default Value | Explanation | 
| ---- | ---- | ---- | ---- |
| serial_port | string | /dev/HWT9075 | serial port. It uses the same usb serial IC chip as the IMU/CO2 sensor, so it'll collide with the imu.(TODO fix) |
| baud_rate| int | 1000000 | don't change this |
| servo_id | int | 1 | don't change this unless controlling multiple motor on motor driver |  
| default_speed | int | 2400 | unit is steps/s. 1 turn = 4095 steps |  
| default_accel | int | 50 | unit is steps/s^2 |  