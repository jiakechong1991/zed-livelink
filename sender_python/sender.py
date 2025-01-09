# -*- coding: utf-8 -*-

import time
from time import sleep
import socket
ip = "127.0.0.1"
upd_port = 9999

with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
    s.connect((ip, upd_port))
    while True:
        if True:
            data_str = "hahaha:" + str(time.time())
            print(data_str)
            s.sendall(data_str.encode())  # 真实的发送数据
        sleep(1)  # 粗糙的控制帧率

 

if __name__ == '__main__':
    print("udp client ")
    main()








