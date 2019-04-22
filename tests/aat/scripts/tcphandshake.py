import errno
import IN
import socket
import sys
import time

intfs = [sys.argv[3], sys.argv[4]]
s = []
error = False
try:
    for i in range(2):
        s.append(socket.socket(socket.AF_INET, socket.SOCK_STREAM))
        s[i].setsockopt(socket.SOL_SOCKET, IN.SO_BINDTODEVICE, intfs[i])
    s[0].bind((sys.argv[1], int(sys.argv[2])))
    s[0].listen(1)
    s[0].settimeout(0)
    s[1].connect((sys.argv[1], int(sys.argv[2])))
    for i in range(10):
        try:
            conn, addr = s[0].accept()
            break;
        except:
            time.sleep(0.1)
    error = True if i == 10 else False
except:
    error = True

for i in s:
    i.close()
sys.exit(-1 if error else 0)
