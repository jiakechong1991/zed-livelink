# -*- coding: utf-8 -*-

import time
from time import sleep
import socket
import json
from tools import euler2quat
ip = "192.168.1.2"
upd_port = 3001
JOINT_NUM = 38

true = True
false = False

data_frame = {
    "action_state": 0,
    "body_format": 2,
    "confidence": 95,
    "coordinate_system": 2,
    "coordinate_unit": 0, # 单位是0，位置缩放为0.1
    "frame_id": 6698,
    # # [ 6.92229218e-10  8.09762361e-01 -3.46352696e-01  4.73629315e-01]
    "global_root_orientation": euler2quat([0,0,0]), # x,y,z
    # """
    # 这里输入 --- 标定 ---UE中的结果    ： y,z轴是反向的
    # 0,0,90   0, 0, -90
    # 0,90,0   0, -90, 0
    # 90,0,0   90, 0, 0 
    # """
    # "global_root_orientation": {
    #     "w": -0.22990426421165466,
    #     "x": 0,
    #     "y": 0.8097623586654663,
    #     "z": 0.5398415923118591
    # },
    "global_root_posititon": {
        "x": 1000,  # 跟UE的单位是缩放0.1， 这里的10相当于1
        "y": 1000,
        "z": 10
    },
    "id": 1,
    "is_new": true,
    "keypoint": [
        {
            "x": 0,
            "y": 0,
            "z": 0
        }
    ]*JOINT_NUM,
    "keypoint_confidence": [
        100
    ]*JOINT_NUM,
    "local_orientation_per_joint": [
        {
            "w": 1,
            "x": 0,
            "y": 0,
            "z": 0
        }
    ]*JOINT_NUM,
    "local_position_per_joint": [
        {
            "x": 0,
            "y": 0,
            "z": 0
        }
    ]*JOINT_NUM,
    "nb_bodies": 1,
    "position": {
        "x": 1,
        "y": 2,
        "z": 3
    },
    "role": 2,
    "timestamp": 1736648403567450947,
    "tracking_state": 1
}

with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
    s.connect((ip, upd_port))
    while True:
        if True:
            #data_str = "hahaha:" + str(time.time())
            data_str = json.dumps(data_frame)
            print(data_str)
            s.sendall(data_str.encode())  # 真实的发送数据
        sleep(0.3)  # 粗糙的控制帧率

 

if __name__ == '__main__':
    print("udp client ")
    main()








