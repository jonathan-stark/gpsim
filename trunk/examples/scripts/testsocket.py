# Sample client program to communicate with gpsim

import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("", 0x1234))
s.send("help\n")
print s.recv(20)

# helper functions
def ParseInt(buf):
    if buf[0:3] == "$03" :
        return int(buf[3:11])

    return 0

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
s.send("$"+"80")
print s.recv(20)

# Symbol Command 
s.send("$"+"97"+"0205misc1")
r = s.recv(20)
print "received:" + r + " decoded as " + str(ParseInt(r))

# Integer object
s.send("$0300000001")
print s.recv(20)

s.close()
