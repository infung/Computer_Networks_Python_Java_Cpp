#!/usr/bin/env python
#Client
#inputs: <server_address> , <n_port>, <req_code>, <msg>

from socket import *
import sys, string

def negotiationTCP(server_address, n_port, req_code):
	clientTCPSocket = socket(AF_INET, SOCK_STREAM)
	clientTCPSocket.connect((server_address, n_port))
	clientTCPSocket.send(str(req_code).encode())
	r_port = int(clientTCPSocket.recv(1024).decode())
	clientTCPSocket.close()
	return r_port

def transactionUDP(server_address, r_port, msg):
	clientUDPSocket = socket(AF_INET, SOCK_DGRAM)
	clientUDPSocket.sendto(msg.encode(), (server_address, r_port))
	reverse_msg, serverAddress = clientUDPSocket.recvfrom(2048)
	clientUDPSocket.close()
	return reverse_msg.decode()

def main():
	if len(sys.argv) == 5:
#		print "server address: ", sys.argv[0]
#		print "negotiation port: " sys.argv[1]
#		print "request code: " sys.argv[2]
#		print "message: " sys.argv[3]

		server_address = sys.argv[1]
		n_port = int(sys.argv[2])
		req_code = int(sys.argv[3])
		msg = sys.argv[4]

		try:
			r_port = negotiationTCP(server_address, n_port, req_code)
		except Exception as e:
			print "Negotiation failed!"
			quit()
		
		try:
			reverse_msg = transactionUDP(server_address, r_port, msg)
			print reverse_msg
		except Exception as e:
			print "Transaction failed!"
			quit()

	else:
		print "Error: incorrect arguments. Please enter <server_address> , <n_port>, <req_code>, <msg>"
		quit()
main()
