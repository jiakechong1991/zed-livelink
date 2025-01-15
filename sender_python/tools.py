# -*- coding:utf-8 -*-
from scipy.spatial.transform import Rotation as R
import numpy as np
from mathutils import Matrix, Vector, Quaternion, Euler


def Rodrigues(rotvec):
    """罗德里格斯公式： 旋转向量 to 旋转矩阵[3*3]"""
    theta = np.linalg.norm(rotvec)
    r = (rotvec/theta).reshape(3, 1) if theta > 0. else rotvec
    cost = np.cos(theta)
    mat = np.asarray([[0, -r[2], r[1]],
                    [r[2], 0, -r[0]],
                    [-r[1], r[0], 0]],dtype=object) #adicionei "",dtype=object" por que estava dando erro
    return(cost*np.eye(3) + (1-cost)*r.dot(r.T) + np.sin(theta)*mat)

def rodrigues2bshapes(pose):  # 输入旋转向量，输出四元数
    """
    pose: size [1,3]。 这是一旋转矢量
    输出： 四元数
    """
    rod_rots = np.asarray(pose).reshape(1, 3)
    qua_rots = [Matrix(Rodrigues(rod_rot)).to_quaternion() for rod_rot in rod_rots]
    this_rot = qua_rots[0]
    res = {
        "x": this_rot.x,
        "y": this_rot.y,
        "z": this_rot.z,
        "w": this_rot.w
    }

    return res
def euler2quat(euler_in): # 输入角度，获得四元数
    assert len(euler_in) == 3, u"euler_in是欧拉角表示，len必须是3"
    euler_angle = R.from_euler('XYZ', euler_in, degrees=True) 
    #euler_angle = R.from_euler('XYZ', [0,  0, 30], degrees=True) 
    res = euler_angle.as_quat()
    keys = ["x", "y", "z", "w"]
    # [ 6.92229218e-10  8.09762361e-01 -3.46352696e-01  4.73629315e-01]
    return dict(zip(keys, res))





if __name__ == "__main__":
    print(euler2quat(euler_in=[0,0,45]))
    rot_vec = R.from_rotvec(np.pi/2 * np.array([0, 0, 0.5]))  # 旋转矢量
    print(rot_vec.as_euler('XYZ', degrees=True))
    print(rot_vec.as_quat())
    print(rodrigues2bshapes(np.pi/2 * np.array([0, 0, 0.5])))

    print(euler2quat(euler_in=[0,0,0]))
    euler_angle = R.from_euler('XYZ', [0,  0, 0], degrees=True) 
    print("===轴角表示===")
    print(euler_angle.as_rotvec())
    print(rodrigues2bshapes(euler_angle.as_rotvec()))










