# scservo_linux_driver

```bash
ros2 run scservo_linux_driver scs_driver
```

## ROS I/F 

/feetech/cmd_pos : position in float32 rads.
/feetech/current : mA value. used to detect external force
/feetech/joint_states : joint state. it supports position, effort, velocity feedback. use this to check robot current pose.
/feetech/temperature : C
/feetech/voltage : V
