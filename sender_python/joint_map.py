# -*- coding:utf-8 -*-


gvh2blender = {
    'root': 'root', 
    'bone_00':  'Pelvis', 
    'bone_01':  'L_Hip', 
    'bone_02':  'R_Hip', 
    'bone_03':  'Spine1', 
    'bone_04':  'L_Knee', 
    'bone_05':  'R_Knee', 
    'bone_06':  'Spine2', 
    'bone_07':  'L_Ankle', 
    'bone_08':  'R_Ankle', 
    'bone_09':  'Spine3', 
    'bone_10':  'L_Foot', 
    'bone_11':  'R_Foot', 
    'bone_12':  'Neck', 
    'bone_13':  'L_Collar', 
    'bone_14':  'R_Collar', 
    'bone_15':  'Head', 
    'bone_16':  'L_Shoulder', 
    'bone_17':  'R_Shoulder',
    'bone_18':  'L_Elbow', 
    'bone_19':  'R_Elbow', 
    'bone_20':  'L_Wrist', 
    'bone_21':  'R_Wrist',
    #'bone_22':  'L_Hand', 
    #'bone_23':  'R_Hand',
}

gvh2plugin = {
    'root': 'root', 
    'bone_00':  "PELVIS", #'Pelvis', 
    'bone_01':  "LEFT_HIP", # 'L_Hip', 
    'bone_02':  "RIGHT_HIP", # 'R_Hip', 
    'bone_03':  "SPINE_1", # 'Spine1', 
    'bone_04':  "LEFT_KNEE", # 'L_Knee', 
    'bone_05':  "RIGHT_KNEE", #'R_Knee', 
    'bone_06':  "SPINE_2", # 'Spine2', 
    'bone_07':  "LEFT_ANKLE", # 'L_Ankle', 
    'bone_08':  "RIGHT_ANKLE", # 'R_Ankle', 
    'bone_09':  "SPINE_3", #'Spine3', 
    'bone_10':  'L_Foot', 
    'bone_11':  'R_Foot', 
    'bone_12':  "NECK", # 'Neck', 
    'bone_13':  'L_Collar', 
    'bone_14':  'R_Collar', 
    'bone_15':  'Head', 
    'bone_16':  "LEFT_SHOULDER", #'L_Shoulder', 
    'bone_17':  "RIGHT_SHOULDER", #'R_Shoulder',
    'bone_18':  "LEFT_ELBOW", # 'L_Elbow', 
    'bone_19':  "RIGHT_ELBOW", #'R_Elbow', 
    'bone_20':  "LEFT_WRIST", #'L_Wrist', 
    'bone_21':  "RIGHT_WRIST", # 'R_Wrist',
    #'bone_22':  'L_Hand', 
    #'bone_23':  'R_Hand',
}

plugin2ue = {
	"PELVIS": "hips", # 髋关节
	"SPINE_1": "spine", # 脊柱1
	"SPINE_2": "spine1", # 脊柱2
	"SPINE_3": "spine2", # 脊柱3
	########面部
	"NECK": "Neck",  # 颈部
	"NOSE": None,  # 鼻子
	"LEFT_EYE": None,  # 左眼
	"RIGHT_EYE": None, # 右眼
	"LEFT_EAR": None, # 左耳
	"RIGHT_EAR": None, # 右耳

	"LEFT_CLAVICLE": "leftShoulder", # 左锁骨
	"RIGHT_CLAVICLE": "RightShoulder",
	"LEFT_SHOULDER": "leftArm", # 左肩
	"RIGHT_SHOULDER": "RightArm",
	"LEFT_ELBOW": "leftForeArm", # 左肘部
	"RIGHT_ELBOW": "RightForeArm", 
	"LEFT_WRIST": "leftHand",  # 左手腕
	"RIGHT_WRIST": "RightHand",
	"LEFT_HIP": "LeftUpLeg",  # 左髋部
	"RIGHT_HIP": "RightUpLeg",
	"LEFT_KNEE": "LeftLeg", # 左膝部
	"RIGHT_KNEE": "RightLeg",
	"LEFT_ANKLE": "LeftFoot", # 左脚踝(大圆骨头那里)
	"RIGHT_ANKLE": "RightFoot",
	"LEFT_BIG_TOE": "leftToeBase",  # 左脚 大拇脚趾(脚前端)
	"RIGHT_BIG_TOE": "RightToeBase",
	"LEFT_SMALL_TOE": None, # 左脚 小母脚趾(脚前端)
	"RIGHT_SMALL_TOE": None, 
	"LEFT_HEEL": None,  # 左 脚后跟
	"RIGHT_HEEL": None,
	#####手部
	"LEFT_HAND_THUMB_4": None, # 大拇指 1号
	"RIGHT_HAND_THUMB_4": None,
	"LEFT_HAND_INDEX_1": "LeftHandindex1",  # 食指 2号
	"RIGHT_HAND_INDEX_1": "RightHandindex1",
	"LEFT_HAND_MIDDLE_4": None,  #中指 3号
	"RIGHT_HAND_MIDDLE_4": None,
	"LEFT_HAND_PINKY_1": "LeftHandPinky1",  # 小拇指 5号
	"RIGHT_HAND_PINKY_1": "RightHandPinky1",
}














if __name__ == "__main__":
	psss






