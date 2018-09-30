JCC = javac

default: sender.class receiver.class
sender.class:
	$(JCC) sender.java packet.java

receiver.class:
	$(JCC) receiver.java packet.java

clean:
	$(RM) *.class
