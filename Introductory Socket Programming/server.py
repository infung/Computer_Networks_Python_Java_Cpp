#!/usr/bin/env python

#server
#input: <req_code>

from socket import*
import sys, string

def transactionUDPserver(serverUDPSocket):
	message, clientAddress = serverUDPSocket.recvfrom(2048)
	reversrMsg = (message.decode())[::-1]
	serverUDPSocket.sendto(reversrMsg.encode(), clientAddress)
	serverUDPSocket.close() #close current serverUDPsocket to listen for other reversing requests from the client

def negotiationTCPserver(serverTCPSocket, requestCode):
	serverTCPSocket.listen(1)
#	print "The server is ready to receive"
	while True:
		connectionSocket, addr = serverTCPSocket.accept()
		sentence = connectionSocket.recv(1024).decode()
		request_code = int(sentence)
		if request_code == requestCode:
			while True:
				serverUDPSocket = socket(AF_INET, SOCK_DGRAM)
				serverUDPSocket.bind(('', 0))  #let the system chooses the port
				random_port = serverUDPSocket.getsockname()[1]
				if random_port < 1024: #Port id should be greater than 1024 since 0-1023 are already reserved
					serverUDPSocket.close()
					continue
				else:
					connectionSocket.send(str(random_port).encode())
					break
			connectionSocket.close()
			break
		else:
			connectionSocket.close()
	transactionUDPserver(serverUDPSocket)

def  main():
	if len(sys.argv) == 2:
#		print "request code: " sys.argv[0]

		req_code = int(sys.argv[1])
		serverTCPSocket = socket(AF_INET, SOCK_STREAM)
		try:
			serverTCPSocket.bind(('', 0))  #let the system chooses the port
			print "SERVER_PORT=" + str(serverTCPSocket.getsockname()[1])
			print str(serverTCPSocket.getsockname()[0])
		except Exception as e:
			print "Port is in use"
			serverTCPSocket.close()
			quit()

		while True: #server should continue listening to its n_port for subsequent client requests
			negotiationTCPserver(serverTCPSocket, req_code)

	else:
		print "Error: incorrect arguments. Please enter <req_code>"
		quit()

	serverTCPSocket.close()
main()
