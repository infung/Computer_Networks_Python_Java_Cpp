
INSTRUCTION:
The programs are written in Java.

Open three different machines or open three putty windows that have different ssh hostname
I used:
ubuntu1604-002 for nEmulator
ubuntu1604-006 for receiver
ubuntu1604-008 for sender

To execute the programs:
Running command: make

On the host ubuntu1604-002 machine or putty window, Run:
 ./nEmulator-linux386 8991 ubuntu1604-006 8994 8993 ubuntu1604-008 8992 1 0.2 1

On the host ubuntu1604-006 machine or putty window, Run:
java receiver ubuntu1604-002 8993 8994 output.txt

On the host ubuntu1604-008 machine or putty window, Run:
java sender ubuntu1604-002 8991 8992 input.txt
