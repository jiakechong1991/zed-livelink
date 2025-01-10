import bpy
import os
# import sys
# from os.path import join
import math
import numpy as np
from mathutils import Matrix, Vector, Quaternion, Euler
import json
import pickle


#### Change the variables here
###############################
#smpl 人体模型
smpl_model = r"./basicModel_m_lbs_10_207_0_v1.0.2.fbx"
# 动捕数据，一共90秒
file = r'./hmr4d_results.pt.pkl'
# 人物模型和地面的高度差
high_from_floor = 1.5
##############################




# 读取动捕数据
with open(file, 'rb') as handle:
    results = pickle.load(handle)

# 骨骼映射
part_match_custom_less2 = {
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
    'bone_22':  'L_Hand', 
    'bone_23':  'R_Hand',
}



### INICIO --- Inseri para utilizar no WHAM
def Rodrigues(rotvec):
    """罗德里格斯公式： 旋转向量 to 旋转矩阵[3*3]"""
    theta = np.linalg.norm(rotvec)
    r = (rotvec/theta).reshape(3, 1) if theta > 0. else rotvec
    cost = np.cos(theta)
    mat = np.asarray([[0, -r[2], r[1]],
                    [r[2], 0, -r[0]],
                    [-r[1], r[0], 0]],dtype=object) #adicionei "",dtype=object" por que estava dando erro
    return(cost*np.eye(3) + (1-cost)*r.dot(r.T) + np.sin(theta)*mat)

def rodrigues2bshapes(pose):
    """
    pose: size [22个joint, 3维度欧拉角]。 这是一帧数据
    """
    rod_rots = np.asarray(pose).reshape(22, 3)
    mat_rots = [Rodrigues(rod_rot) for rod_rot in rod_rots]
    bshapes = np.concatenate([
        (mat_rot - np.eye(3)).ravel() for mat_rot in mat_rots[1:]
    ])
    return(mat_rots, bshapes)  # bshapes后面不用


############
def get_global_pose(global_pose, arm_ob, frame=None):

    arm_ob.pose.bones['m_avg_root'].rotation_quaternion.w = 0.0
    arm_ob.pose.bones['m_avg_root'].rotation_quaternion.x = -1.0


    bone = arm_ob.pose.bones['m_avg_Pelvis']
    # if frame is not None:
    #     bone.keyframe_insert('rotation_quaternion', frame=frame)

    root_orig = arm_ob.pose.bones['m_avg_root'].rotation_quaternion
    mw_orig = arm_ob.matrix_world.to_quaternion()
    pelvis_quat = Matrix(global_pose[0]).to_quaternion()

    bone.rotation_quaternion = pelvis_quat
    bone.keyframe_insert('rotation_quaternion', frame=frame)

    pelvis_applyied = arm_ob.pose.bones['m_avg_Pelvis'].rotation_quaternion
    bpy.context.view_layer.update()

    rot_world_orig = root_orig @ pelvis_applyied @ mw_orig #pegar a rotacao em relacao ao mundo

    return rot_world_orig


###############

# apply trans pose and shape to character
# def apply_trans_pose_shape(trans, body_pose, shape, ob, arm_ob, obname, scene, cam_ob, frame=None):
def apply_trans_pose_shape(trans, body_pose, arm_ob, obname, frame=None):
    """
    body_pose: (21+1)*3
    """

    # transform pose into rotation matrices (for pose) and pose blendshapes
#    if self.option in [2,3]: #para WHAM ou slahmr
#        mrots, bsh = rodrigues2bshapes(body_pose)
#    else: #para 4d humans
#        mrots = body_pose
    
    # mrots: [22个 旋转矩阵]
    mrots, bsh = rodrigues2bshapes(body_pose)
    part_bones  = part_match_custom_less2
    
    # 将root偏移量，在Y高度方向偏移
    trans = Vector((trans[0],trans[1]-high_from_floor,trans[2]))

    # print('frame in apply pose:', frame)
    arm_ob.pose.bones['m_avg_Pelvis'].location = trans
    arm_ob.pose.bones['m_avg_Pelvis'].keyframe_insert('location', frame=frame)
    
    arm_ob.pose.bones['m_avg_root'].rotation_quaternion.w = 0.0
    arm_ob.pose.bones['m_avg_root'].rotation_quaternion.x = -1.0
    
    for ibone, mrot in enumerate(mrots): # 0-21
         # ibone: index[0-21]
         # mrot: 旋转矩阵
         if ibone < 22: #incui essa parte por que no modelo que eu to usando nao tem bone para a mao
            # 用名称 从smpl骨骼上获取对应joint
            bone = arm_ob.pose.bones['m_avg_'+part_bones['bone_%02d' % ibone]]
            # 直接 设置该joint的旋转四元数
            bone.rotation_quaternion = Matrix(mrot).to_quaternion()
            
            if frame is not None:
                bone.keyframe_insert('rotation_quaternion', frame=frame)


def init_scene(scene, params, gender='male', angle=0):
    """导入SMPL模型,并初始化场景"""
    path_fbx = smpl_model
    bpy.ops.import_scene.fbx(filepath=path_fbx, axis_forward='-Y', axis_up='-Z', global_scale=100)#, automatic_bone_orientation=True)
    # 它就是场景中的 骨骼动画对象（Armature object）
    arm_ob = bpy.context.selected_objects[0]
    
    obj_gender = 'm'
    obname = '%s_avg' % obj_gender
    ob = bpy.data.objects[obname]
    # arm_obj = 'Armature'
#    bpy.context.scene.source = arm_obj

    print('success load')
    
    # ob.data.use_auto_smooth = False  # autosmooth creates artifacts
    bpy.ops.object.select_all(action='DESELECT')  # 取消 选择多有对象
    bpy.ops.object.select_all(action='DESELECT')
    cam_ob = ''
    # ob.data.shape_keys.animation_data_clear()
    # arm_ob = bpy.data.objects[arm_obj]
#    arm_ob = context.scene.source
    arm_ob.animation_data_clear() # 清除所有动画数据
    
    return(ob, obname, arm_ob, cam_ob)


###开始

params = []
object_name = 'm_avg'
obj_gender = 'm'
scene = bpy.data.scenes['Scene']  # 从blender中获取名叫"Scene"的场景
ob, obname, arm_ob, cam_ob= init_scene(scene, params, obj_gender)

# 总帧数
qtd_frames = len(results['smpl_params_global']['transl'])

print('qtd frames: ',qtd_frames)
# shape = results[character]['betas'].tolist()
for fframe in range(0,qtd_frames):
    bpy.context.scene.frame_set(fframe)
    #print('data',data)
    # trans = [0.0, 0.0, 1.521]
    # root的平移
    trans = results['smpl_params_global']['transl'][fframe]
    # shape = data[1]['smpl'][character]['betas']
    # root的朝向
    global_orient = results['smpl_params_global']['global_orient'][fframe]
    # pelvis = fixed_pelvis_quat[fframe]
    # global_orient = np.array(Quaternion(pelvis).to_matrix()).reshape(1,3,3)
    #
    ##o trtamento abaixo nao deu certo
    # rotation_x = Matrix.Rotation(math.radians(180.0),3,'X') #rodar ao redor de X
    # rotation_y = Matrix.Rotation(math.radians(90.0),3,'Y') #rodar ao redor de X
    # global_orient = global_orient @ rotation_x @rotation_y
    # joint的旋转pose数据
    body_pose = results['smpl_params_global']['body_pose'][fframe]
    body_pose_fim = body_pose.reshape(int(len(body_pose)/3), 3)  # 21*3
    # 将root和其他joint的旋转数据拼接到一起，获得全身数据 22*3
    final_body_pose = np.vstack([global_orient, body_pose_fim])
    # apply_trans_pose_shape(Vector(trans), final_body_pose, shape, obj,arm_ob, obname, scene, cam_ob, fframe)
    apply_trans_pose_shape(
        Vector(trans), # 平移
        final_body_pose, # 全身 joint-pose旋转数据
        arm_ob, # 动画句柄
        obname, 
        fframe
    )
    bpy.context.view_layer.update() # 更新整个视图view



arm_ob.pose.bones['m_avg_root'].rotation_quaternion.w = 1.0
arm_ob.pose.bones['m_avg_root'].rotation_quaternion.x = 0.0
arm_ob.pose.bones['m_avg_root'].rotation_quaternion.y = 0.0
arm_ob.pose.bones['m_avg_root'].rotation_quaternion.z = 0.0


arm_ob.pose.bones['m_avg_Pelvis'].constraints.new('COPY_LOCATION')
# arm_ob.pose.bones["m_avg_Pelvis"].constraints["Copy Location"].target = armature_ref
arm_ob.pose.bones["m_avg_Pelvis"].constraints[0].target = arm_ob
arm_ob.pose.bones["m_avg_Pelvis"].constraints[0].subtarget = "m_avg_Pelvis"
# arm_ob.pose.bones["m_avg_Pelvis"].constraints["Copy Location"].subtarget = "m_avg_Pelvis"


arm_ob.pose.bones['m_avg_Pelvis'].constraints.new('COPY_ROTATION')
# arm_ob.pose.bones["m_avg_Pelvis"].constraints["Copy Rotation"].target = armature_ref
arm_ob.pose.bones["m_avg_Pelvis"].constraints[1].target = arm_ob
# arm_ob.pose.bones["m_avg_Pelvis"].constraints["Copy Rotation"].subtarget = "m_avg_Pelvis"
arm_ob.pose.bones["m_avg_Pelvis"].constraints[1].subtarget = "m_avg_Pelvis"