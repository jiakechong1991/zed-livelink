# -*- coding:utf-8 -*-
from tools import euler2quat


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

udf_map = {
	"global_root_orientation": euler2quat([0,0,0]), # y, z,x
	"global_root_posititon": {
        "x": 1000,  # 跟UE的单位是缩放0.1， 这里的10相当于1
        "y": 1000,
        "z": 10
    },
	"local_orientation_per_joint": [
		[0, "PELVIS", euler2quat([0,0,0])],
		[1, "SPINE_1", euler2quat([0,0,45])], # y,z,x
		[2, "SPINE_2", euler2quat([0,0,0])],
		[3, "SPINE_3", euler2quat([0,0,0])],
		[4, "NECK", euler2quat([0,0,0])],
		[5, "NOSE", euler2quat([0,0,0])],
		[6, "LEFT_EYE", euler2quat([0,0,0])],
		[7, "RIGHT_EYE", euler2quat([0,0,0])],
		[8, "LEFT_EAR", euler2quat([0,0,0])],
		[9, "RIGHT_EAR", euler2quat([0,0,0])],
		[10, "LEFT_CLAVICLE", euler2quat([0,0,0])],
		[11, "RIGHT_CLAVICLE", euler2quat([0,0,0])],
		[12, "LEFT_SHOULDER", euler2quat([0,0,0])],
		[13, "RIGHT_SHOULDER", euler2quat([0,0,0])],
		[14, "LEFT_ELBOW", euler2quat([0,0,0])],  # y,z,x
		[15, "RIGHT_ELBOW", euler2quat([0,0,0])],
		[16, "LEFT_WRIST", euler2quat([0,0,0])],
		[17, "RIGHT_WRIST", euler2quat([0,0,0])],
		[18, "LEFT_HIP", euler2quat([0,0,0])],
		[19, "RIGHT_HIP", euler2quat([0,0,0])],
		[20, "LEFT_KNEE", euler2quat([0,0,0])],
		[21, "RIGHT_KNEE", euler2quat([0,0,0])],
		[22, "LEFT_ANKLE", euler2quat([0,0,0])],
		[23, "RIGHT_ANKLE", euler2quat([0,0,0])],
		[24, "LEFT_BIG_TOE", euler2quat([0,0,0])],
		[25, "RIGHT_BIG_TOE", euler2quat([0,0,0])],
		[26, "LEFT_SMALL_TOE", euler2quat([0,0,0])],
		[27, "RIGHT_SMALL_TOE", euler2quat([0,0,0])],
		[28, "LEFT_HEEL", euler2quat([0,0,0])],
		[29, "RIGHT_HEEL", euler2quat([0,0,0])],
		[30, "LEFT_HAND_THUMB_4", euler2quat([0,0,0])],
		[31, "RIGHT_HAND_THUMB_4", euler2quat([0,0,0])],
		[32, "LEFT_HAND_INDEX_1", euler2quat([0,0,0])],
		[33, "RIGHT_HAND_INDEX_1", euler2quat([0,0,0])],
		[34, "LEFT_HAND_MIDDLE_4", euler2quat([0,0,0])],
		[35, "RIGHT_HAND_MIDDLE_4", euler2quat([0,0,0])],
		[36, "LEFT_HAND_PINKY_1", euler2quat([0,0,0])],
		[37, "RIGHT_HAND_PINKY_1", euler2quat([0,0,0])]
	]
}

JOINT_NUM = len(udf_map["local_orientation_per_joint"])



















