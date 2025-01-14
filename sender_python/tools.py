# -*- coding:utf-8 -*-
from scipy.spatial.transform import Rotation as R
import numpy as np
from mathutils import Matrix, Vector, Quaternion, Euler


def euler2quat(euler_in):
    assert len(euler_in) == 3, u"euler_in是欧拉角表示，len必须是3"
    euler_angle = R.from_euler('XYZ', euler_in, degrees=True) 
    #euler_angle = R.from_euler('XYZ', [0,  0, 30], degrees=True) 
    res = euler_angle.as_quat()
    keys = ["x", "y", "z", "w"]
    # [ 6.92229218e-10  8.09762361e-01 -3.46352696e-01  4.73629315e-01]
    return dict(zip(keys, res))

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
    pose: size [1, 3维度欧拉角]。 这是一帧数据
    输出： 四元数
    """
    assert len(pose) == 3, u"pose是轴角表示，len必须是3"
    rod_rot = np.asarray(pose)
    qua_rots = Matrix(Rodrigues(rod_rot)).to_quaternion()
    this_qua:Quaternion = qua_rots
    res = {
        "x": this_qua.x,
        "y": this_qua.y,
        "z": this_qua.z,
        "w": this_qua.w
    }
    return res



if __name__ == "__main__":
    print(euler2quat(euler_in=[0,0,45]))
    euler_angle = R.from_euler('XYZ', [0,  0, 45], degrees=True) 
    print("===轴角表示===")
    print(euler_angle.as_rotvec())
    print(rodrigues2bshapes(euler_angle.as_rotvec()))










