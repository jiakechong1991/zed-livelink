# -*- coding: utf-8 -*-

from time import sleep
import socket
 
 
def main():
    # udp 通信地址，IP+端口号
    udp_addr = ('127.0.0.1', 9999)
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # 绑定端口
    udp_socket.bind(udp_addr)
 
    # 等待接收对方发送的数据
    while True:
        recv_data = udp_socket.recvfrom(1024)  # 1024表示本次接收的最大字节数
        # 打印接收到的数据
        print("[From %s:%d]:%s" % (recv_data[1][0], recv_data[1][1], recv_data[0].decode("utf-8")))
 
if __name__ == '__main__':
    print("udp server ")
    main()
