# -*- coding: utf-8 -*-

import time
from time import sleep
import socket
import json
from tools import euler2quat
from importlib import reload
from copy import deepcopy
import temp_pose


ip = "192.168.1.10"
upd_port = 3001
JOINT_NUM = 38


camera_frame = {
    #
    # 这里输入 --- 标定 ---UE中的结果    ： y,z轴是反向的
    # 0,0,90   0, 0, -90
    # 0,90,0   0, -90, 0
    # 90,0,0   90, 0, 0 
    # """
    "camera_orientation": euler2quat([0,0,0]),
    "camera_position": {
        "x": 0,   # 跟UE的单位是缩放0.1， 这里的10相当于1
        "y": 0,
        "z": 0
    },
    "coordinate_system": 2,
    "coordinate_unit": 0, # 单位是0，位置缩放为0.1
    "frame_id": 2002,
    "role": 1,
    "serial_number": 8289,
    "timestamp": 1734958710410477382
}



def merge_pose_data(frame_id):
    reload(temp_pose)
    temp_data_frame = deepcopy(camera_frame)
    temp_data_frame["camera_orientation"] = temp_pose.camera_frame["camera_orientation"]
    temp_data_frame["camera_position"] = temp_pose.camera_frame["camera_position"]

    temp_data_frame["timestamp"] = time.time_ns()
    temp_data_frame["frame_id"] = frame_id
    return temp_data_frame





with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
    s.connect((ip, upd_port))
    frame_id = 1
    while True:
        if True:
            #data_str = "hahaha:" + str(time.time())
            data_str = json.dumps(merge_pose_data(frame_id))
            frame_id += 1
            print(data_str)
            print("\n\n")
            s.sendall(data_str.encode())  # 真实的发送数据
        sleep(0.3)  # 粗糙的控制帧率

 

if __name__ == '__main__':
    print("udp client ")
    main()








