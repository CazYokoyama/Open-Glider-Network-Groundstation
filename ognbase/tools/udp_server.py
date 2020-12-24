#!usr/bin/python3

import socket
import sys
from datetime import datetime

if len(sys.argv) > 1:
    UDP_PORT_NO = int(sys.argv[1])
else:
    print("usage: python3 udp_server.py <port>")
    sys.exit(1)


UDP_IP_ADDRESS = "10.0.0.200"

serverSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
serverSock.bind((UDP_IP_ADDRESS, UDP_PORT_NO))

while True:
    data, addr = serverSock.recvfrom(512)
    now = datetime.now()
    current_time = now.strftime("%H:%M:%S")
    print("%s Message: %s from %s"%(current_time, data.decode("utf-8"), addr))
