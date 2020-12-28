
import socket
import sys
from datetime import datetime

from termcolor import colored, cprint

print_yellow = lambda x: cprint(x, 'yellow')
print_red = lambda x: cprint(x, 'red')
print_green = lambda x: cprint(x, 'green')
print_blue = lambda x: cprint(x, 'cyan')
print_magenta = lambda x: cprint(x, 'magenta')
print_on_white = lambda x: cprint(x, 'magenta','on_white',attrs=['bold'])

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
    print_green("%s Message: %s from %s"%(current_time, data.decode("utf-8"), addr))

