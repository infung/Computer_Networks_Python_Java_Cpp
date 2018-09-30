//CS456 Assignment 2
//Receiver

//inputs:
//<hostname for the network emulator>,
//<UDP port number used by the link emulator to receive ACKs from the receiver>,
//<UDP port number used by the receiver to receive data from the emulator>,
//<name of the file into which the received data is written>

import java.io.*;
import java.net.*;
import java.util.*;

public class receiver{
	static final int modulo = 32;
	static PrintWriter arrivalLog;
	static PrintWriter writeToFile;
	static String emulatorHostAddress;
	static int emulatorRecPort;
	static int senderRecPort;
	static String fileName;
	public static void main(String[] args) throws Exception {
		try {
			//extract host address of emulator, port number of receving from emulator, port number of sending to emulator and file name from the arguments.
			if(args.length != 4) {
				System.out.println("Please enter: <host address of the network emulator>, <UDP port number used by the emulator to receive data from the sender>, <UDP port number used by the sender to receive ACKs from the emulator>, <name of the file to be transferred>");
				System.exit(1);
			} else {
				emulatorHostAddress = args[0];
				emulatorRecPort = Integer.valueOf(args[1]);
				senderRecPort = Integer.valueOf(args[2]);
				fileName = args[3];
			}
			try {
			arrivalLog = new PrintWriter("arrival.log", "UTF-8");
			writeToFile = new PrintWriter(fileName, "UTF-8");
			} catch(FileNotFoundException fe){
				System.out.println("File not found");
			}

			int expectedSeqnum = 0;
			//Socket that sends packet to emulator
			DatagramSocket sendSocket = new DatagramSocket();
			InetAddress IPAddress = InetAddress.getByName(emulatorHostAddress);
			//Socket that receives packet from emualtor
			DatagramSocket receiveSocket = new DatagramSocket(senderRecPort);
//System.out.println("start receiving!!");
			while(true) {
				byte [] receiveData = new byte[512];
				DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
				receiveSocket.receive(receivePacket);
				packet recPacket = packet.parseUDPdata(receivePacket.getData());
	//System.out.println("received! the seq num just received is " + recPacket.getSeqNum());
				int seqNum = recPacket.getSeqNum();
				arrivalLog.println(seqNum);
				int sendingSeqNum = 31;
				if(seqNum == expectedSeqnum) {
					//if packet is a eot packet, send an eot packet and then close connection.
					if (recPacket.getType() == 2) {
						packet eot = packet.createEOT(recPacket.getSeqNum());
						byte[] dt = eot.getUDPdata();
						DatagramPacket eotP = new DatagramPacket(dt, dt.length, IPAddress, emulatorRecPort);
						sendSocket.send(eotP);
						writeToFile.close();
						arrivalLog.close();
						sendSocket.close();
						receiveSocket.close();
						return;
					}
					//if packet is not eot write it to the file without newline followed.
					if(recPacket.getType() == 1) {
						writeToFile.print(new String(recPacket.getData()));
					}
					sendingSeqNum = seqNum;
					expectedSeqnum = (expectedSeqnum + 1) % modulo;
				} else if(expectedSeqnum == 0) {
					//if the first packet is out of order. the receiver just discard the packet without ack
					continue;
				} else {
					sendingSeqNum = (modulo + (expectedSeqnum - 1))%modulo;
				}
				packet ack = packet.createACK(sendingSeqNum);
				byte [] data = ack.getUDPdata();
				DatagramPacket sendPacket = new DatagramPacket(data, data.length, IPAddress, emulatorRecPort);
				sendSocket.send(sendPacket);
			}
		} catch (Exception e) {
				e.printStackTrace();
				System.out.println("Exception");
		}
	}

}
