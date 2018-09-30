#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <functional>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace std;

#define NBR_ROUTER 5 /* for simplicity we consider only 5 routers */

struct pkt_HELLO {
	unsigned int router_id; /* id of the router who sends the HELLO PDU */
	unsigned int link_id; /* id of the link through which it is sent */
};

struct pkt_LSPDU {
	unsigned int sender; /* sender of the LS PDU */
	unsigned int router_id; /* router id */
	unsigned int link_id; /* link id */
	unsigned int cost; /* cost of the link */
	unsigned int via; /* id of the link through which the LS PDU is sent */
};

struct pkt_INIT {
	unsigned int router_id; /* id of the router that send the INIT PDU */
};

struct link_cost {
	unsigned int link; /* link id */
	unsigned int cost; /* associated cost */
};

struct circuit_DB {
	unsigned int nbr_link; /* number of links attached to a router */
	struct link_cost linkcost[NBR_ROUTER];/* we assume that at most NBR_ROUTER links are attached to each router */
};

static struct circuit_DB send_INIT_recieve_CDB_send_HELLO(std::ofstream & outputLog, unsigned int r_id, int Socket, struct sockaddr_in * nsp_clientAddr, struct sockaddr_in * routerAddr) {
	//send init packet
	struct pkt_INIT init_packet = {r_id};
	sendto(Socket, (const void *)&init_packet, sizeof(init_packet), 0, (struct sockaddr *) nsp_clientAddr, sizeof(struct sockaddr_in));
	outputLog << "R" << r_id << " sends an INIT: router_id " << r_id << endl;

	//receive circuit DB
	struct circuit_DB c_db;
	recvfrom(Socket, &c_db, sizeof(c_db), 0, NULL, NULL);
	outputLog << "R" << r_id << " receives a CIRCUIT_DB: " << "nbr_link " << c_db.nbr_link << endl;
	for(unsigned int i = 0; i < c_db.nbr_link; i++) {
		outputLog << "                           link " << c_db.linkcost[i].link << ", cost " << c_db.linkcost[i].cost << endl;
	}

	//send hello packet to neighbours
	for(unsigned int i = 0; i < c_db.nbr_link; i++) {
		struct pkt_HELLO hello_packet = {r_id, c_db.linkcost[i].link};
		sendto(Socket, &hello_packet, sizeof(hello_packet), 0, (struct sockaddr *) nsp_clientAddr, sizeof(struct sockaddr_in));
		outputLog << "R" << r_id << " sends a HELLO: router_id "  << r_id << " link_id " << c_db.linkcost[i].link << endl;
	}
	
	return c_db; 
}


static void update_topology(struct pkt_LSPDU * lspdu, struct circuit_DB topology[]) {
	int num_of_link = topology[lspdu->router_id - 1].nbr_link;
	for(unsigned int i = 0; i < num_of_link; i++) {
		if(lspdu->link_id == topology[lspdu->router_id - 1].linkcost[i].link) {
			return;
		}
	}

	topology[lspdu->router_id - 1].linkcost[num_of_link].link = lspdu->link_id;
	topology[lspdu->router_id - 1].linkcost[num_of_link].cost = lspdu->cost;
	topology[lspdu->router_id - 1].nbr_link++;
}

//reference: https://www.geeksforgeeks.org/greedy-algorithms-set-6-dijkstras-shortest-path-algorithm/
static int * Dijskrta(int graph[NBR_ROUTER][NBR_ROUTER], unsigned int src) {
	int * rib = new int[NBR_ROUTER];
	bool PK[NBR_ROUTER];

	for(int i = 0; i < NBR_ROUTER; i++) {
		rib[i] = INT_MAX;
		PK[i] = false;
	}

	rib[src] = 0;

	int numtoloop = NBR_ROUTER - 1;
	while(numtoloop > 0) { 
		int min = INT_MAX;
		int minDis;
		for(int j = 0; j < NBR_ROUTER; j++) {
			if(!PK[j] && rib[j] <= min) {
				min = rib[j];
				minDis = j;
			}
		}
		PK[minDis] = true;

		for(int k = 0; k < NBR_ROUTER; k++) {
			if(!PK[k] && graph[minDis][k] && rib[minDis] < INT_MAX && rib[minDis] + graph[minDis][k] < rib[k]) {
				rib[k] = rib[minDis] + graph[minDis][k];
			}
		}
		numtoloop--;
	}
	/*for(int i = 0; i < NBR_ROUTER; i++) {
		cout << rib[i] << " " ;
	}
	cout << endl;*/
	return rib;
}
//reference: https://www.quora.com/What-is-the-most-simple-efficient-C++-code-for-Dijkstras-shortest-path-algorithm
/*static int * Dijskrta(vector<vector<pair<int, int>>> Graph, unsigned int src) {
	priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> PQ;
	int * rib = new int[NBR_ROUTER];
	for(int i = 0; i < NBR_ROUTER; i++) {
		rib[i] = INT_MAX;
	}
	rib[src] = 0;
	PQ.push({0, src});
	while(!PQ.empty()) {
		//extra min from PQ
		int r = PQ.top().second;
		PQ.pop();
		for(int j = 0; j < Graph[r].size(); j++) {
			int router = Graph[r][j].first;
			int cost = Graph[r][j].second;
			if(rib[router] > rib[r] + cost) {
				//decrement distance so far
				rib[router] = rib[r] + cost;
				PQ.push({rib[router], router});
			}
		}
	}
	return rib;
}*/

static int * updtae_RIB(unsigned int r_id, struct circuit_DB topology[]) {
	//create a matrix used to pass to Dijskrta algorithm
	int graph[NBR_ROUTER][NBR_ROUTER];
	for(unsigned int i = 0; i < NBR_ROUTER; i++) {
		for(unsigned int j = 0; j < NBR_ROUTER; j++) {
			graph[i][j] = 0;
		}
	}

	vector<vector<pair<int, int>>> Graph;

	int temp[100] = {0}; 
	for(unsigned int i = 0; i < NBR_ROUTER; i++) {
		for(unsigned int j = 0; j < topology[i].nbr_link; j++) {
			int router = temp[topology[i].linkcost[j].link];
			if(router) {
				graph[i][router - 1] = graph[router - 1][i] = topology[i].linkcost[j].cost;
				//Graph[i].push_back({router-1, topology[i].linkcost[j].cost});
				//Graph[router-1].push_back({i, topology[i].linkcost[j].cost});
			} else {
				temp[topology[i].linkcost[j].link] = i + 1;
			}
		}
	}
/*
cout << "---------------------" << endl;
	for(unsigned int i = 0; i < NBR_ROUTER; i++) {
		for(unsigned int j = 0; j < NBR_ROUTER; j++) {
			cout << graph[i][j]  << " ";
		}
		cout << endl;
	}
	cout << "----------------------" <<endl;*/
	//Dijskrta algorithm returns the shortest path from current rounter to other routers.
	return Dijskrta(graph, r_id-1);
	//return Dijskrta(Graph, r_id-1);
}

static void receive_HELLO_or_LSPDU_update_TopologyDB(std::ofstream & outputLog, unsigned int r_id, int Socket, struct sockaddr_in * nsp_clientAddr, struct circuit_DB * c_db) {
	struct circuit_DB topology[NBR_ROUTER];
	for(unsigned int i = 0; i < NBR_ROUTER; i++) {
		topology[i].nbr_link = 0;
	}
	topology[r_id-1] = *c_db;

	vector<unsigned int> neighbors;

	while(true) {
		struct pkt_HELLO *hello_pkt;
		struct pkt_LSPDU *lspdu_pkt;
		char buffer[128] = {0};
		int len = recvfrom(Socket, &buffer, sizeof(buffer), 0, NULL, NULL);
		//receive a hello packet, send a set of LS PDUs to sender
		if (len == sizeof(struct pkt_HELLO)) {
			hello_pkt = (struct pkt_HELLO *) buffer;
			neighbors.push_back(hello_pkt->router_id);
			outputLog << "R" <<  r_id << " receives a HELLO: router_id " << hello_pkt->router_id << " link_id " << hello_pkt->link_id << endl;
			//send a set of LS PDUs to this neighbor
			for(unsigned int i = 0; i < c_db->nbr_link; i++) {
				struct pkt_LSPDU lspdu = {r_id, r_id, (c_db->linkcost)[i].link, (c_db->linkcost)[i].cost, hello_pkt->link_id};
				sendto(Socket, (const void *)&lspdu, sizeof(pkt_LSPDU), 0, (struct sockaddr *) nsp_clientAddr, sizeof(struct sockaddr_in));
				outputLog << "R" << lspdu.sender << " sends an LSPDU via " << lspdu.via << " that R" << lspdu.router_id << " has a link " << lspdu.link_id << " with cost " << lspdu.cost << endl; 
			}
		//receive a LS PDU packet, update topology database and send LS PDU to all neighbors
		} else if(len == sizeof(struct pkt_LSPDU)){
			lspdu_pkt = (struct pkt_LSPDU *) buffer;
			if(lspdu_pkt->router_id == r_id) continue;
			outputLog << "R" << r_id << " receives a LSPDU from R" << lspdu_pkt->sender << " via link " << lspdu_pkt->via << " that R" << lspdu_pkt->router_id << " has a link " << lspdu_pkt->link_id << " with cost " << lspdu_pkt->cost << endl; 
			//update topology database
			update_topology(lspdu_pkt,topology);
			//print topology database
			outputLog << endl << "# Topology database" << endl;
			for(unsigned int i = 0; i < NBR_ROUTER; i++) {
				if(topology[i].nbr_link == 0) continue;
				outputLog << "R" << r_id << " -> " << "R" << i + 1 << " nbr link " << topology[i].nbr_link << endl;
				for(unsigned int j = 0; j < topology[i].nbr_link; j++) {
					outputLog << "R" << r_id << " -> " << "R" << i + 1 << " link " << topology[i].linkcost[j].link << " cost " << topology[i].linkcost[j].cost << endl;
				}
			}
			outputLog << "----------------------------------------------" << endl;
			//update RIB using Dijkstra algorithm

			int * Rib = updtae_RIB(r_id, topology);
			//print RIB
			outputLog << endl << "# RIB" << endl;
			for(unsigned int j = 0; j < NBR_ROUTER; j++) {
				string r = (r_id == j+1)? "Local" : "R" + std::to_string(j+1);
				string n = (Rib[j] >= INT_MAX)? "INFINITY" : std::to_string(Rib[j]); 
				outputLog << "R" << r_id << " -> " << "R" << j + 1 << " -> "  << r << ", " << n << endl;
			}
			free(Rib);
			outputLog << "----------------------------------------------" << endl;
			//send changes to all neighbors
			for(unsigned int k = 0; k < neighbors.size(); k++) {
				if(lspdu_pkt->sender == neighbors[k]) continue;
				unsigned int via = 1;
				for(unsigned int i = 0; i < c_db->nbr_link; i++) {
					unsigned int currLink = c_db->linkcost[i].link;
					for(unsigned int j = 0; j < topology[neighbors[k] - 1].nbr_link; j++) {
						if(currLink == topology[neighbors[k] - 1].linkcost[j].link) {
							via = currLink;
						}
					}
				}
				struct pkt_LSPDU lspdu = {r_id, lspdu_pkt->router_id, lspdu_pkt->link_id, lspdu_pkt->cost, via};
				sendto(Socket, (const void *)&lspdu, sizeof(pkt_LSPDU), 0, (struct sockaddr *) nsp_clientAddr, sizeof(struct sockaddr_in));
				outputLog << "R" << lspdu.sender << " sends an LSPDU via " << lspdu.via << " that R" << lspdu.router_id << " has a link " << lspdu.link_id << " with cost " << lspdu.cost << endl; 
			}
		}
	}
}

int main(int argc, char **argv) {
	/////////////////////////////Initialization////////////////////////////////////////////////
	unsigned int r_id, nsp_port, r_port;
	string nsp_ip;

	if(argc != 5) {
		printf("Usage: router <router_id> <nse_host> <nse_port> <router_port>\n");
		exit(-1);
	}
	r_id = atoi(argv[1]);
	//reference: http://www.cplusplus.com/forum/articles/9742/
	hostent * nsp_host = gethostbyname(argv[2]);
	//int nsp_host_int;
	//memcpy(&nsp_host_int, nsp_host->h_addr_list[0], nsp_host->h_length);
	if(nsp_host == NULL) {
		printf("Usage: router <router_id> <nse_host> <nse_port> <router_port>\n");
		exit(-1);
	}
	in_addr * address = (in_addr *)nsp_host->h_addr;
	nsp_ip = inet_ntoa(* address);
	nsp_port = atoi(argv[3]);
	r_port = atoi(argv[4]);

	ofstream outputLog;
	string filename = "router" + std::string(argv[1]) + ".log";
	outputLog.open(filename);

//outputLog << "DEBUGGING: r_id: " << r_id << " ip: " << nsp_host_int << " port: " << nsp_port << " r_port: " << r_port << endl; 

	//////////////////////////SET UP UDP SOCKET////////////////////////////////////////////////////

	//reference: https://www.geeksforgeeks.org/socket-programming-cc/ 
	//https://stackoverflow.com/questions/31955514/simple-udp-socket-code-sending-and-receiving-messages
	int Socket;
	struct sockaddr_in serverAddr;

	//create socket
	if ((Socket = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    outputLog << "Router " << r_id << ": Socket created." << endl;

    //assign local value
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(r_port);

    //binds connection
    if(bind(Socket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
    	perror("bind failed");
        exit(EXIT_FAILURE);
    }

    outputLog << "Router " << r_id << ": Bind is successfule." << endl;

    //nsp
    struct sockaddr_in nsp_clientAddr;
    //memset(&nsp_clientAddr, 0, sizeof(struct sockaddr_in));
    nsp_clientAddr.sin_family = AF_INET;
    nsp_clientAddr.sin_addr.s_addr = inet_addr(nsp_ip.c_str());
    nsp_clientAddr.sin_port = htons(nsp_port);
    //memcpy(&nsp_clientAddr.sin_addr, &nsp_host_int, sizeof(int));

    ///////////////////////////ROUTERS///////////////////////////////////////////////////////
    struct circuit_DB c_db;
    c_db = send_INIT_recieve_CDB_send_HELLO(outputLog, r_id, Socket, &nsp_clientAddr, &serverAddr);
    receive_HELLO_or_LSPDU_update_TopologyDB(outputLog, r_id, Socket, &nsp_clientAddr, &c_db);
    
    outputLog.close();
    close(Socket);
    return 1;
}
