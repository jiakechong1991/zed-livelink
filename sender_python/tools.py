# -*- coding:utf-8 -*-
from scipy.spatial.transform import Rotation as R
import numpy as np

def euler2quat(euler_in):
    euler_angle = R.from_euler('XYZ', euler_in, degrees=True) 
    #euler_angle = R.from_euler('XYZ', [0,  0, 30], degrees=True) 
    res = euler_angle.as_quat()
    keys = ["x", "y", "z", "w"]
    # [ 6.92229218e-10  8.09762361e-01 -3.46352696e-01  4.73629315e-01]
    return dict(zip(keys, res))


if __name__ == "__main__":
    print(euler2quat(euler_in=[0,0,0]))








