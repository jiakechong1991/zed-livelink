///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2024, STEREOLABS.
//
// All rights reserved.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// OpenGL Viewer(!=0) or no viewer (0)
#define DISPLAY_OGL 1

// ZED include
#include "GLViewer.hpp"
#include "PracticalSocket.h"
#include "json.hpp"
#include "Util.h"
#include <sl/Camera.hpp>

#include <iostream>
#include <chrono>  // 用于时间单位的定义
#include <thread>

using namespace sl;

nlohmann::json toJSON(int frame_id, int serial_number, uint64_t timestamp, sl::Pose& cam_pose, sl::COORDINATE_SYSTEM coord_sys, sl::UNIT coord_unit);
nlohmann::json toJSON(int frame_id, uint64_t timestamp, sl::Bodies& bodies, int id, sl::BODY_FORMAT body_format, sl::COORDINATE_SYSTEM coord_sys, sl::UNIT coord_unit);

void print(string msg_prefix, ERROR_CODE err_code = ERROR_CODE::SUCCESS, string msg_suffix = "");

// Type of data send 
enum class ZEDLiveLinkRole
{
    Transform = 0,
    Camera,
    Animation
};

// Defines the Coordinate system and unit used in this sample
static const sl::COORDINATE_SYSTEM coord_sys = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
static const sl::UNIT coord_unit = sl::UNIT::MILLIMETER;

/// ----------------------------------------------------------------------------
/// ----------------------------------------------------------------------------
/// -------------------------------- MAIN LOOP ---------------------------------
/// ----------------------------------------------------------------------------
/// ----------------------------------------------------------------------------

int main(int argc, char **argv) {

    ZEDConfig zed_config;
    std::string zed_config_file("../ZEDLiveLinkConfig.json"); // Default name and location.
    if (argc == 2)
    {
        zed_config_file = argv[1];
        std::cout << "Loading " << zed_config_file << " config file.";
    }
    else if (argc > 2)
    {
        std::cout << "Unexecpected arguments, exiting..." << std::endl;
        return EXIT_FAILURE;
    }
    else {
        std::cout << "Trying to load default config file 'ZEDLiveLinkConfig.json' " << std::endl;
    }
    readZEDConfig(zed_config_file, zed_config);
    std::cout << "Starting LiveLink sender" << endl;

    Camera zed;
    InitParameters init_parameters;
    init_parameters.camera_resolution = zed_config.resolution;
    init_parameters.camera_fps = zed_config.fps;
    init_parameters.depth_mode = zed_config.depth_mode;
    init_parameters.coordinate_system = coord_sys;
    init_parameters.coordinate_units = coord_unit;
    init_parameters.grab_compute_capping_fps = zed_config.grab_compute_capping_fps;

    init_parameters.input = zed_config.input;
    init_parameters.svo_real_time_mode = true;

    // // Open the camera 主动关闭
    // auto returned_state = zed.open(init_parameters);
    // if (returned_state != ERROR_CODE::SUCCESS) {
    //     print("Open Camera", returned_state, "\nExit program.");
    //     zed.close();
    //     return EXIT_FAILURE;
    // }

    // Enable Positional tracking (mandatory for body tracking) -------------------------------------------------------
    PositionalTrackingParameters positional_tracking_parameters;
    positional_tracking_parameters.set_floor_as_origin = zed_config.set_floor_as_origin;
    // If the camera is static, uncomment the following line to have better performance.
    positional_tracking_parameters.set_as_static = zed_config.set_as_static;
    positional_tracking_parameters.enable_pose_smoothing = zed_config.enable_pose_smoothing;
    positional_tracking_parameters.enable_area_memory = zed_config.enable_area_memory;
   
    // 主动关闭
    // returned_state = zed.enablePositionalTracking(positional_tracking_parameters);
    // if (returned_state != ERROR_CODE::SUCCESS) {
    //     print("enable Positional Tracking", returned_state, "\nExit program.");
    //     zed.close();
    //     return EXIT_FAILURE;
    // }


    BodyTrackingParameters body_tracking_params;
    if (zed_config.send_bodies)
    {
        // Enable the Body tracking module -------------------------------------------------------------------------------------

        body_tracking_params.enable_tracking = true; // track people across grabs
        body_tracking_params.enable_body_fitting = true; // smooth skeletons moves
        body_tracking_params.body_format = zed_config.body_format;
        body_tracking_params.detection_model = zed_config.detection_model;
        body_tracking_params.max_range = zed_config.max_range;
        // 主动关闭
        // returned_state = zed.enableBodyTracking(body_tracking_params);
        // if (returned_state != ERROR_CODE::SUCCESS) {
        //     print("enable Body Tracking", returned_state, "\nExit program.");
        //     zed.close();
        //     return EXIT_FAILURE;
        // }

    }

#if DISPLAY_OGL
    GLViewer viewer;
    viewer.init(argc, argv);
#endif

    Pose cam_pose;
    cam_pose.pose_data.setIdentity();

    // Configure body tracking runtime parameters
    BodyTrackingRuntimeParameters body_tracking_parameters_rt;
    body_tracking_parameters_rt.detection_confidence_threshold = zed_config.detection_confidence;
    body_tracking_parameters_rt.minimum_keypoints_threshold = zed_config.minimum_keypoints_threshold;
    body_tracking_parameters_rt.skeleton_smoothing = zed_config.skeleton_smoothing;

    // Create ZED Bodies filled in the main loop
    //////////////////////////bodies数据填充！！！！！！！！！
    Bodies bodies;
    bodies.is_new = true;
    // std::vector<sl::BodyData> body_list;
    //////////
    BodyData body_ins;
    body_ins.tracking_state = sl::OBJECT_TRACKING_STATE::OK; // int 1:正在追踪中
    body_ins.action_state = sl::OBJECT_ACTION_STATE::IDLE; // int 0: idel（角色正在idel） 1: 角色正在moving 
    body_ins.id = 1;  //int 代表人物编号
    // body 的 三维重心 位置
    body_ins.position = sl::float3(1.0f, 2.0f, 3.0f); // sl::float3 =等价= Vector3<float>
    body_ins.confidence = 95; // flaot, 范围：0~100，越低意味着定位置信度越低

    std::vector<float> keypoint_confidence_(38, 100.0f);
    body_ins.keypoint_confidence = keypoint_confidence_; // std::vector<float> 每个关键点的 置信度
    // 绝对位置
    std::vector<sl::float3> keypoint_ = {
        sl::float3(0.0f, 0.0f, 0.0f), // PELVIS
        sl::float3(0.0f, 1.0f, 0.0f), // SPINE_1
        sl::float3(0.0f, 2.0f, 0.0f), // SPINE_2
        sl::float3(0.0f, 3.0f, 0.0f), // SPINE_3
        sl::float3(0.0f, 4.0f, 0.0f), // NECK
        sl::float3(0.0f, 4.5f, 0.0f), // NOSE
        sl::float3(-0.1f, 4.5f, 0.1f), // LEFT_EYE
        sl::float3(0.1f, 4.5f, 0.1f), // RIGHT_EYE
        sl::float3(-0.2f, 4.0f, -0.1f), // LEFT_EAR
        sl::float3(0.2f, 4.0f, -0.1f), // RIGHT_EAR
        sl::float3(-0.5f, 3.5f, 0.0f), // LEFT_CLAVICLE
        sl::float3(0.5f, 3.5f, 0.0f), // RIGHT_CLAVICLE
        sl::float3(-1.0f, 3.5f, 0.0f), // LEFT_SHOULDER
        sl::float3(1.0f, 3.5f, 0.0f), // RIGHT_SHOULDER
        sl::float3(-1.5f, 3.0f, 0.0f), // LEFT_ELBOW
        sl::float3(1.5f, 3.0f, 0.0f), // RIGHT_ELBOW
        sl::float3(-2.0f, 2.5f, 0.0f), // LEFT_WRIST
        sl::float3(2.0f, 2.5f, 0.0f), // RIGHT_WRIST
        sl::float3(-0.5f, 0.0f, 0.0f), // LEFT_HIP
        sl::float3(0.5f, 0.0f, 0.0f), // RIGHT_HIP
        sl::float3(-0.5f, -1.0f, 0.0f), // LEFT_KNEE
        sl::float3(0.5f, -1.0f, 0.0f), // RIGHT_KNEE
        sl::float3(-0.5f, -2.0f, 0.0f), // LEFT_ANKLE
        sl::float3(0.5f, -2.0f, 0.0f), // RIGHT_ANKLE
        sl::float3(-0.5f, -2.5f, 0.0f), // LEFT_BIG_TOE
        sl::float3(0.5f, -2.5f, 0.0f), // RIGHT_BIG_TOE
        sl::float3(-0.5f, -2.5f, -0.1f), // LEFT_SMALL_TOE
        sl::float3(0.5f, -2.5f, -0.1f), // RIGHT_SMALL_TOE
        sl::float3(-0.5f, -2.0f, -0.1f), // LEFT_HEEL
        sl::float3(0.5f, -2.0f, -0.1f), // RIGHT_HEEL
        sl::float3(-2.0f, 2.5f, 0.1f), // LEFT_HAND_THUMB_4
        sl::float3(2.0f, 2.5f, 0.1f), // RIGHT_HAND_THUMB_4
        sl::float3(-2.0f, 2.5f, -0.1f), // LEFT_HAND_INDEX_1
        sl::float3(2.0f, 2.5f, -0.1f), // RIGHT_HAND_INDEX_1
        sl::float3(-2.0f, 2.5f, -0.2f), // LEFT_HAND_MIDDLE_4
        sl::float3(2.0f, 2.5f, -0.2f), // RIGHT_HAND_MIDDLE_4
        sl::float3(-2.0f, 2.5f, -0.3f), // LEFT_HAND_PINKY_1
        sl::float3(2.0f, 2.5f, -0.3f) // RIGHT_HAND_PINKY_1
    };
    body_ins.keypoint = keypoint_; // std::vector<sl::float3> 检测到的关键点 数组

    // 子关键点位置偏移 相对于 父关键点
    std::vector<sl::float3> local_position_per_joint_ = {
        sl::float3(0.0f, 0.0f, 0.0f), // PELVIS
        sl::float3(0.0f, 1.0f, 0.0f), // SPINE_1
        sl::float3(0.0f, 2.0f, 0.0f), // SPINE_2
        sl::float3(0.0f, 3.0f, 0.0f), // SPINE_3
        sl::float3(0.0f, 4.0f, 0.0f), // NECK
        sl::float3(0.0f, 4.5f, 0.0f), // NOSE
        sl::float3(-0.1f, 4.5f, 0.1f), // LEFT_EYE
        sl::float3(0.1f, 4.5f, 0.1f), // RIGHT_EYE
        sl::float3(-0.2f, 4.0f, -0.1f), // LEFT_EAR
        sl::float3(0.2f, 4.0f, -0.1f), // RIGHT_EAR
        sl::float3(-0.5f, 3.5f, 0.0f), // LEFT_CLAVICLE
        sl::float3(0.5f, 3.5f, 0.0f), // RIGHT_CLAVICLE
        sl::float3(-1.0f, 3.5f, 0.0f), // LEFT_SHOULDER
        sl::float3(1.0f, 3.5f, 0.0f), // RIGHT_SHOULDER
        sl::float3(-1.5f, 3.0f, 0.0f), // LEFT_ELBOW
        sl::float3(1.5f, 3.0f, 0.0f), // RIGHT_ELBOW
        sl::float3(-2.0f, 2.5f, 0.0f), // LEFT_WRIST
        sl::float3(2.0f, 2.5f, 0.0f), // RIGHT_WRIST
        sl::float3(-0.5f, 0.0f, 0.0f), // LEFT_HIP
        sl::float3(0.5f, 0.0f, 0.0f), // RIGHT_HIP
        sl::float3(-0.5f, -1.0f, 0.0f), // LEFT_KNEE
        sl::float3(0.5f, -1.0f, 0.0f), // RIGHT_KNEE
        sl::float3(-0.5f, -2.0f, 0.0f), // LEFT_ANKLE
        sl::float3(0.5f, -2.0f, 0.0f), // RIGHT_ANKLE
        sl::float3(-0.5f, -2.5f, 0.0f), // LEFT_BIG_TOE
        sl::float3(0.5f, -2.5f, 0.0f), // RIGHT_BIG_TOE
        sl::float3(-0.5f, -2.5f, -0.1f), // LEFT_SMALL_TOE
        sl::float3(0.5f, -2.5f, -0.1f), // RIGHT_SMALL_TOE
        sl::float3(-0.5f, -2.0f, -0.1f), // LEFT_HEEL
        sl::float3(0.5f, -2.0f, -0.1f), // RIGHT_HEEL
        sl::float3(-2.0f, 2.5f, 0.1f), // LEFT_HAND_THUMB_4
        sl::float3(2.0f, 2.5f, 0.1f), // RIGHT_HAND_THUMB_4
        sl::float3(-2.0f, 2.5f, -0.1f), // LEFT_HAND_INDEX_1
        sl::float3(2.0f, 2.5f, -0.1f), // RIGHT_HAND_INDEX_1
        sl::float3(-2.0f, 2.5f, -0.2f), // LEFT_HAND_MIDDLE_4
        sl::float3(2.0f, 2.5f, -0.2f), // RIGHT_HAND_MIDDLE_4
        sl::float3(-2.0f, 2.5f, -0.3f), // LEFT_HAND_PINKY_1
        sl::float3(2.0f, 2.5f, -0.3f) // RIGHT_HAND_PINKY_1
    };
    body_ins.local_position_per_joint = local_position_per_joint_;// std::vector<sl::float3>
    // 子关键点朝向向量(四元数)  相对一 父关键点 【目前我是少一个root 关节，我也不确定这样是否对】
    std::vector<sl::float4> local_orientation_per_joint_ = {
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // PELVIS
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // SPINE_1
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // SPINE_2
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // SPINE_3
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // NECK
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // NOSE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_EYE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_EYE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_EAR
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_EAR
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_CLAVICLE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_CLAVICLE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_SHOULDER
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_SHOULDER
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_ELBOW
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_ELBOW
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_WRIST
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_WRIST
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_HIP
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_HIP
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_KNEE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_KNEE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_ANKLE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_ANKLE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_BIG_TOE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_BIG_TOE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_SMALL_TOE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_SMALL_TOE
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_HEEL
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_HEEL
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_HAND_THUMB_4
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // RIGHT_HAND_THUMB_4
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_HAND_INDEX_1
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_HAND_INDEX_1
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_HAND_INDEX_1
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_HAND_INDEX_1
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f), // LEFT_HAND_INDEX_1
        sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f) // LEFT_HAND_INDEX_1
    };
    body_ins.local_orientation_per_joint = local_orientation_per_joint_; // std::vector<sl::float4>

    // root 节点的 朝向向量(四元数)
    body_ins.global_root_orientation =sl::float4(0.0f,  0.80976237f,  0.53984158f, -0.22990426f); // PELVIS
    // root 节点的位置，使用keypoint[0]的(x,y,z)代替
    // body_ins.global_root_posititon = // x,y,z
    
    //////////
    bodies.body_list = {body_ins};


    /////////////////////////////////////////////////
    bool run = true;

    // ----------------------------------
    // UDP ------------------------------
    // ----------------------------------
    std::string servAddress;
    unsigned short servPort;
    UDPSocket sock;

    if (zed_config.connection_type == CONNECTION_TYPE::MULTICAST) sock.setMulticastTTL(1);

    servAddress = zed_config.udp_ip;
    servPort = zed_config.udp_port;

    std::cout << "Sending data at " << servAddress << ":" << servPort << std::endl;

    // ----------------------------------
    // UDP ------------------------------
    // ----------------------------------

    RuntimeParameters rt_params = new RuntimeParameters();
    rt_params.measure3D_reference_frame = REFERENCE_FRAME::WORLD;
    int frame_id = 0;

    SetCtrlHandler();
    while (!exit_app)  // 主工作循环
    {
        // 获得当前时间
        auto start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        // 打开相机工作
        // auto err =  ERROR_CODE::SUCCESS; // zed.grab(rt_params);
        //std::cout << "FPS : " << zed.getCurrentFPS() << std::endl;
        std::cout << "新一轮循环开始---------------------------------" <<std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); //
        if (true)
        {
            auto now = std::chrono::high_resolution_clock::now(); // 获取当前时间点
            auto now_as_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(now); // 将时间点转换为纳秒
            auto value = now_as_ns.time_since_epoch(); // 获取自纪元以来的时间长度
            uint64_t ts = std::chrono::duration_cast<std::chrono::duration<uint64_t, std::nano>>(value).count();
            // uint64_t ts = zed.getTimestamp(sl::TIME_REFERENCE::IMAGE);
            std::cout << "当前timestamp in nanoseconds: " << ts << std::endl;
            if (zed_config.send_bodies)
            {   
                std::cout << "进入send bodies阶段" << std::endl;
                // Retrieve Detected Human Bodies
                // 将追踪数据赋值到bodies中
                // zed.retrieveBodies(bodies, body_tracking_parameters_rt);
#if DISPLAY_OGL
                //Update GL View
                viewer.updateData(bodies, cam_pose.pose_data);
#endif

                if (true) // bodies.is_new
                {
                    try
                    {
                        // send body data one at a time instead of as one single packet.
                        // 可能探测到多个身体，对每个身体body进行 数据发送
                        for (int i = 0; i < bodies.body_list.size(); i++)  
                        {
                            std::string data_to_send = toJSON(frame_id, ts, bodies, i, body_tracking_params.body_format, coord_sys, coord_unit).dump();
                            // 借助标准的UDP协议发送数据
                            sock.sendTo(data_to_send.data(), data_to_send.size(), servAddress, servPort);
                            std::cout << data_to_send << std::endl;
                            std::cout << "body数据已经发送" << std::endl;
                        }
                    }
                    catch (SocketException& e)
                    {
                        cerr << e.what() << endl;
                        //exit(1);
                    }
                }
            }

            if (true || zed_config.send_camera_pose)
            {
                std::cout << "进入send camera阶段" << std::endl;
                // 获得相机位姿
                // zed.getPosition(cam_pose);
                // 发送camera位姿数据
                //std::string data_to_send = toJSON(frame_id, zed.getCameraInformation().serial_number, ts, cam_pose, coord_sys, coord_unit).dump();
                std::string data_to_send = toJSON(frame_id, 8289, ts, cam_pose, coord_sys, coord_unit).dump();
                std::cout << data_to_send << std::endl;
                // 借助标准的UDP接口发送数据
                sock.sendTo(data_to_send.data(), data_to_send.size(), servAddress, servPort);
            }

            frame_id++;
        }
        // else if (err == sl::ERROR_CODE::END_OF_SVOFILE_REACHED)
        // {
        //     frame_id = 0;
        //     zed.setSVOPosition(0);
        // }
        // else
        // {
        //     print("error grab， Exit program.");
        // }

// #if DISPLAY_OGL
//         run = viewer.isAvailable();
// #endif

       
        auto stop = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::this_thread::sleep_for(std::chrono::milliseconds(25)); //
        std::cout << stop - start << " ms" << std::endl;
    }

#if DISPLAY_OGL
    viewer.exit();
#endif

    // Release Bodies
    // bodies.body_list.clear();

    // 关闭设备 Disable modules
    //zed.disableBodyTracking();
    //zed.disablePositionalTracking();
    //zed.close();

    return EXIT_SUCCESS;
}



/// ----------------------------------------------------------------------------
/// ----------------------------------------------------------------------------
/// ----------------------------- DATA FORMATTING ------------------------------
/// ----------------------------------------------------------------------------
/// ----------------------------------------------------------------------------


void print(string msg_prefix, ERROR_CODE err_code, string msg_suffix) {
    cout << "[Sample]";
    if (err_code != ERROR_CODE::SUCCESS)
        cout << "[Error]";
    cout << " " << msg_prefix << " ";
    if (err_code != ERROR_CODE::SUCCESS) {
        cout << " | " << toString(err_code) << " : ";
        cout << toVerbose(err_code);
    }
    if (!msg_suffix.empty())
        cout << " " << msg_suffix;
    cout << endl;
}

// Create the json sent to the clients
nlohmann::json toJSON(int frame_id, int serial_number,uint64_t timestamp, sl::Pose& cam_pose, sl::COORDINATE_SYSTEM coord_sys, sl::UNIT coord_unit)
{
    nlohmann::json j;

    j["serial_number"] = serial_number;
    j["frame_id"] = frame_id;
    j["timestamp"] = timestamp;
    j["role"] = ZEDLiveLinkRole::Camera;
    j["coordinate_system"] = coord_sys;
    j["coordinate_unit"] = coord_unit;

    j["camera_position"] = nlohmann::json::object();
    j["camera_position"]["x"] = isnan(cam_pose.getTranslation().x) ? 0 : cam_pose.getTranslation().x;
    j["camera_position"]["y"] = isnan(cam_pose.getTranslation().y) ? 0 : cam_pose.getTranslation().y;
    j["camera_position"]["z"] = isnan(cam_pose.getTranslation().z) ? 0 : cam_pose.getTranslation().z;

    j["camera_orientation"] = nlohmann::json::object();
    j["camera_orientation"]["x"] = isnan(cam_pose.getOrientation().x) ? 0 : cam_pose.getOrientation().x;
    j["camera_orientation"]["y"] = isnan(cam_pose.getOrientation().x) ? 0 : cam_pose.getOrientation().y;
    j["camera_orientation"]["z"] = isnan(cam_pose.getOrientation().x) ? 0 : cam_pose.getOrientation().z;
    j["camera_orientation"]["w"] = isnan(cam_pose.getOrientation().x) ? 0 : cam_pose.getOrientation().w;

    return j;
}


// send one skeleton at a time
nlohmann::json toJSON(int frame_id, uint64_t timestamp, sl::Bodies& bodies, int id, sl::BODY_FORMAT body_format, sl::COORDINATE_SYSTEM coord_sys, sl::UNIT coord_unit)
{
    nlohmann::json j;

    j["frame_id"] = frame_id;
    j["timestamp"] = timestamp;
    j["role"] = ZEDLiveLinkRole::Animation;
    j["body_format"] = body_format;
    j["is_new"] = (bool)bodies.is_new;
    j["coordinate_system"] = coord_sys;
    j["coordinate_unit"] = coord_unit;  // // int 描述使用什么坐标系：opengl坐标系，ros坐标系，ue坐标系[
    j["nb_bodies"] = bodies.body_list.size();

    if (id < bodies.body_list.size())
    {
        auto body = bodies.body_list[id];

        j["tracking_state"] = (int)body.tracking_state;
        j["action_state"] = (int)body.action_state;
        j["id"] = body.id;
        j["position"] = nlohmann::json::object();
        j["position"]["x"] = isnan(body.position.x) ? 0 : body.position.x;
        j["position"]["y"] = isnan(body.position.y) ? 0 : body.position.y;
        j["position"]["z"] = isnan(body.position.z) ? 0 : body.position.z;

        j["confidence"] = isnan(body.confidence) ? 0 : body.confidence;

        j["keypoint_confidence"] = nlohmann::json::array();
        for (auto& i : body.keypoint_confidence)
        {
            j["keypoint_confidence"].push_back(isnan(i) ? 0 : i);
        }

        j["keypoint"] = nlohmann::json::array();
        for (auto& i : body.keypoint)
        {
            nlohmann::json e;
            e["x"] = isnan(i.x) ? 0 : i.x;
            e["y"] = isnan(i.y) ? 0 : i.y;
            e["z"] = isnan(i.z) ? 0 : i.z;
            j["keypoint"].push_back(e);
        }
        j["local_position_per_joint"] = nlohmann::json::array();
        for (auto& i : body.local_position_per_joint)
        {
            nlohmann::json e;
            e["x"] = isnan(i.x) ? 0 : i.x;
            e["y"] = isnan(i.y) ? 0 : i.y;
            e["z"] = isnan(i.z) ? 0 : i.z;
            j["local_position_per_joint"].push_back(e);
        }
        j["local_orientation_per_joint"] = nlohmann::json::array();
        for (auto& i : body.local_orientation_per_joint)
        {
            nlohmann::json e;
            e["x"] = isnan(i.x) ? 0 : i.x;
            e["y"] = isnan(i.y) ? 0 : i.y;
            e["z"] = isnan(i.z) ? 0 : i.z;
            e["w"] = isnan(i.w) ? 0 : i.w;
            j["local_orientation_per_joint"].push_back(e);
        }

        j["global_root_posititon"] = nlohmann::json::object();
        j["global_root_posititon"]["x"] = isnan(body.keypoint[0].x) ? 0 : body.keypoint[0].x;
        j["global_root_posititon"]["y"] = isnan(body.keypoint[0].y) ? 0 : body.keypoint[0].y;
        j["global_root_posititon"]["z"] = isnan(body.keypoint[0].z) ? 0 : body.keypoint[0].z;

        j["global_root_orientation"] = nlohmann::json::object();
        j["global_root_orientation"]["x"] = isnan(body.global_root_orientation.x) ? 0 : body.global_root_orientation.x;
        j["global_root_orientation"]["y"] = isnan(body.global_root_orientation.y) ? 0 : body.global_root_orientation.y;
        j["global_root_orientation"]["z"] = isnan(body.global_root_orientation.z) ? 0 : body.global_root_orientation.z;
        j["global_root_orientation"]["w"] = isnan(body.global_root_orientation.w) ? 0 : body.global_root_orientation.w;
    }

    return j;
}
