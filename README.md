# Chatroom-Project

-- Server --



ls //search for the directory

cd desktop

gcc ServerHW2.c -o ServerHW2.out -lpthread

./ServerHW2.out 3030 //argv[1] is the port number




-- Client --


ls //search for the directory

cd desktop

gcc ClientHW2.c -o ClientHW2.out -lpthread

./ClientHW2.out 127.0.0.1 3030 //argv[1] is server's IP address. argv[2] is the port number



