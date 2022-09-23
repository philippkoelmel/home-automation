import socket

UDP_IP = "192.168.178.77"
UDP_PORT = 9990
MESSAGE = bytearray("\x43\x3a\x30\x3a\x37\x33\x0d\x0a", 'utf-8')


print "UDP target IP:", UDP_IP
print "UDP target port:", UDP_PORT
print "message:", ''.join('{:02x}'.format(x) for x in MESSAGE)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))

MESSAGE = bytearray("\x43\x3a\x35\x3a\x37\x36\x0d\x0a", 'utf-8')
print "message:", ''.join('{:02x}'.format(x) for x in MESSAGE)

sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
