cmake_minimum_required(VERSION 3.8)
project(scservo_linux_driver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)


set(hdrs include/SMS_STS.h include/SCSerial.h include/SCS.h include/SCSCL.h
         include/INST.h include/HLSCL.h include/ServoUtils.h
         include/SyncWriteBuffer.h include/ServoErrors.h include/SCServo.h)
set(srs src/SMS_STS.cpp src/SCSerial.cpp src/SCS.cpp src/SCSCL.cpp src/HLSCL.cpp)

add_library(${PROJECT_NAME} STATIC ${hdrs} ${srs})
# 2. Use ${PROJECT_NAME} here too
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# 3. Necessary for Position Independent Code (especially for ROS 2 libs)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# 4. Export Dependencies (Crucial for linking this lib to your Node later)
ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})


add_executable(
  sts_node
  src/sts_driver.cpp
)

ament_target_dependencies(sts_node
  rclcpp 
  std_msgs
  sensor_msgs
)

target_link_libraries(sts_node ${PROJECT_NAME})


# 5. Install Rules (So the library and headers move to the install folder)
install(
  DIRECTORY include/
  DESTINATION include
)

install(
  TARGETS sts_node
  DESTINATION lib/${PROJECT_NAME}
)

install(
  TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

ament_package()
