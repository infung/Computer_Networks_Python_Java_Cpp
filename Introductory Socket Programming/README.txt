CS456 A1 README

Instruction:
	The programs are written in Python. According to FAQ, we don't need makefile for this assignment.
	
	To execute the programs:
  start the server, run: ./server.sh <req_code>
	(**<req_code> is an integer. e.g. 13.)

  start the client, run: ./client.sh <server_address> <n_port> <req_code> <msg>
	(**<server_address> is single quoted string. e.g. '127.0.0.1'
	**<n_port> is an integer printed by the server. e.g. The server printed "SERVER_PORT=56720", then <n_port> = 56720
	**<req_code> is an integer should be the same as req_code passed to server.
	**<msg> is a string. use "" to include multiple words. e.g. " I will find an intern job! ")

Undergrad machines:
	The programs were built and tested on ubuntu1604-008 on the Linux student enviroment.

Version of make:
	No makefile needed

Complier:
	Python
