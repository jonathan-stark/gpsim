# Sample client program to communicate with gpsim

import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("", 0x1234))
s.send("help\n")
print s.recv(20)

# Object container
s.send("$0105")
print s.recv(20)

# String object
s.send("$0205HELLO")
print s.recv(20)

# Integer object
s.send("$03DEADBEEF")
print s.recv(20)


# Command object
s.send("$0401")
print s.recv(20)

s.close()
