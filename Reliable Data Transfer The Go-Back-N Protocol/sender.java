//CS456 Assignment 2
//Sender

//inputs:
//<host address of the network emulator>,
//<UDP port number used by the emulator to receive data from the sender>, 
//<UDP port number used by the sender to receive ACKs from the emulator>, 
//<name of the file to be transferred>

import java.io.*;
import java.net.*;
import java.util.*;

public class sender{
		static final int WindowSize = 10;
		static final int maxDataLength = 500;
		static final int modulo = 32;
		static PrintWriter seqnumLog;
		static PrintWriter ackLog; 
		static String emulatorHostAddress;
		static int emulatorRecPort;
		static int senderRecPort;
		static String fileName;
		static packet [] myPackets;
		static byte [][] bytesArray;

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
			//read data from the file and splite the data into multiple byte arrays and packets
							
			try {
				seqnumLog = new PrintWriter("seqnum.log", "UTF-8");
				ackLog = new PrintWriter("ack.log", "UTF-8");
			}catch (FileNotFoundException fe) {
				System.out.println("File not found");
			}	
			File file = new File(fileName);
			int PacketsNum = (int) (file.length() / maxDataLength);
			int LastPacket = (int) (file.length() % maxDataLength);
			byte [] allBytes = new byte[(int)file.length()];
		//System.out.println("file length is " + file.length() + "there is " + PacketsNum + ". the last packets is " + LastPacket + ". the content is " + Arrays.toString(allBytes));
			FileInputStream inputStream = new FileInputStream(file);
			inputStream.read(allBytes);
			int pluslast = (LastPacket != 0)? 1 : 0;
			myPackets = new packet[PacketsNum + pluslast];
			bytesArray = new byte[PacketsNum + pluslast][];
			for(int i = 0; i < PacketsNum; i++) {
				bytesArray[i] = new byte[maxDataLength];
				bytesArray[i] = Arrays.copyOfRange(allBytes, i*maxDataLength, (i+1)*maxDataLength);
				//System.out.println("for packet " + i + "the content is " + Arrays.toString(bytesArray[i]));

				myPackets[i] = packet.createPacket(i%modulo, new String(bytesArray[i], "UTF-8"));

			}
			if(LastPacket != 0) {
				bytesArray[PacketsNum] = new byte[LastPacket];
				bytesArray[PacketsNum] = Arrays.copyOfRange(allBytes, PacketsNum*maxDataLength, (int)file.length());
				myPackets[PacketsNum] = packet.createPacket(PacketsNum%modulo, new String(bytesArray[PacketsNum], "UTF-8"));
				PacketsNum++;
			}

			//sending
			int base = 0;
			int nextseqnum = 0;
			int timeout = 100;
			//Socket that sends packet to emulator
			DatagramSocket sendSocket = new DatagramSocket();
			InetAddress IPAddress = InetAddress.getByName(emulatorHostAddress);
			//Socket that receives packet from emualtor
			DatagramSocket receiveSocket = new DatagramSocket(senderRecPort);

		//System.out.println("beforing sending packets, total number of packets nedded to send is " + PacketsNum);
			while (base < PacketsNum) {
				//send packets when the window is not full
				while(nextseqnum < base + WindowSize && nextseqnum < PacketsNum) {
					byte [] data = myPackets[nextseqnum].getUDPdata();
					DatagramPacket sendPacket = new DatagramPacket(data, data.length, IPAddress, emulatorRecPort);
					sendSocket.send(sendPacket);
					//write seqnum into seqnum.log file
					seqnumLog.println(myPackets[nextseqnum].getSeqNum());
					nextseqnum++;
				}
				//System.out.println("the base sofar is " + base + " the next seqnum is " + nextseqnum);
				
				//according to https://stackoverflow.com/questions/4969760/setting-a-timeout-for-socket-operations
				//& https://stackoverflow.com/questions/12820874/what-is-the-functionality-of-setsotimeout-and-how-it-works

				receiveSocket.setSoTimeout(timeout);
				while (true) {
					try {
						//receive
						//receiveSocket.setSoTimeout(timeout);
						byte [] receiveData = new byte[512];
						DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
						receiveSocket.receive(receivePacket);
						packet recPacket = packet.parseUDPdata(receivePacket.getData());
						int ack = recPacket.getSeqNum();
		//System.out.println("ack number is " + ack);
						//write ack num into ack.log
						ackLog.println(ack);
						if((ack + 1) == (nextseqnum % modulo)) {
							base = nextseqnum;
							//stop_timer
							break;
						} else if (ack < 10 && (ack + modulo - base%modulo) < 10) {
							base += ack + modulo - base%modulo + 1;
						} else {
							base += ack - base%modulo + 1;
		//System.out.println("the base is " + base);
						}
					} catch (SocketTimeoutException se) {
						//resend
		//System.out.println("timeout!! resend N packets!");
						for(int k = base; k < nextseqnum; k++) {
							byte [] data = myPackets[k].getUDPdata();
							DatagramPacket sendPacket = new DatagramPacket(data, data.length, IPAddress, emulatorRecPort);
							sendSocket.send(sendPacket);
						}
						break;
					}
				}
				
			}
		//System.out.println("send eot to close connection");
			//all packets have been acked, send EOT packet
			packet eot = packet.createEOT(PacketsNum % modulo);
			byte[] dt = eot.getUDPdata();
			DatagramPacket eotP = new DatagramPacket(dt, dt.length, IPAddress, emulatorRecPort);
			sendSocket.send(eotP);
			//close its connection and exit only after it has received ACKs for all data packets
			//no need exception since eot never be lost
			while(true) {
				byte [] rec = new byte[512];
				DatagramPacket recP = new DatagramPacket(rec, rec.length);
				receiveSocket.receive(recP);
				packet recPkt = packet.parseUDPdata(recP.getData());
				int acknum = recPkt.getSeqNum();
				if(acknum == PacketsNum) {
					ackLog.close();
					seqnumLog.close();
					sendSocket.close();
					receiveSocket.close();
					return;
				}
			}
		} catch (Exception e) {
				e.printStackTrace();
				System.out.println("Exception");
		}
	}
}
