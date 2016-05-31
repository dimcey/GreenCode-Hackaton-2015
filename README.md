# GreenCode-Hackaton
Raspberry Pi receives a continuous UDP packets, process and modifies them, and sends everything on a server which has to evaluate the data

# The challenge and the evaluation
The subject for this year’s Green Code Lab Challenge was in the IoT domain, as there is huge expansion of the pervasive computing field and the number of connected devices in our everyday life is expected to have exponential growth. Therefore, an essential part of the development of such products is the optimization, mainly in order to minimize their impact on the energy consumption and data transmission, or with other words to make them as sustainable as possible. 

Hence, the entities that were provided to each team during the Challenge were a Raspberry Pi and a Server. The scenario of the subject is shown on the figure below.

![alt tag](https://github.com/dimcey/GreenCode-Hackaton-2015/blob/master/Task.png)

There is a injection server, simulating different sensors that are connected on the Raspberry Pi, which is sending continuously for 4 minutes random generated data on the Raspberry’s UDP port number 514. After that period of time the Raspberry has to process the data within 1 minute, optimize as much as possible and send it to the server. On the 5th minute, their own system is comparing the data which was initially sent to the Pi with the data that is received and stored on the server. The same process continues all over again after 10 minutes of idle period.

The evaluation criteria was divided into 5 pieces, each of them giving different amount of points. The main focus was the power efficiency, both on the server and on the Raspberry Pi, resulting with 500 points of the total 1000. During the final tests, the Evaluation team were constantly measuring the energy consumption on the Raspberry and on the server, and the less energy was consumed, the higher points we acquired. The next in line was the network efficiency, with 200 points. The measurements were based on the data exchanged between the Raspberry and the server, so the less we send, the more more points we acquired. The last 3 pieces of the evaluation criteria were based on the sharing the work that each team is doing, in a form of best practices, justifying the green techniques and using the social media for promotion. 

The following section will explain some of the techniques that we used in order to achieve the 5th overall place.

# Green practices used
○	 Using blocking I/O functions

Since we are using C++, we are using poll() and recvfrom() to see if we received something and to read them. Since they are blocking functions, i.e. the program will stop until something happens, we will not consume (a lot of) energy while we wait for the next packet. It is like the sleep() function, except we don’t sacrifice accuracy on the time-stamps for the incoming packets.
This way, we managed to reduce our energy consumption just be parametering the timeout of our poll().

○	 Aggregating messages

One problem that we faced was to encode the special characters that could be send to the Raspberry.Taking this into account makes aggregating messages a bit trickier. An initial solution would be to just concatenate the incoming datagrams, but any datagrams containing new-lines in the middle of the messages will have their newlines indistinguishable from the regular new-line characters ending the messages.
E.g. in one message of "Hello world!\nWelcome here!\n" the expected result is in the form of "Hello world!#012Welcome here!\n"

There are generally two solutions to this: adding some own custom code separating each message, avoiding aggregating messages altogether, or translating the ending new-lines into new 'magical number' characters. E.g. replacing the ending new-line '\n' with a value probably not used in any of the data (i.e. another special character mentioned but that isn't used). If you consult any ASCII character table (http://www.ascii-code.com/) you can see that the numbers 127, 129, 141, 143, 144, 157, etc. don't have any special use for representing regular characters.	
DO NOTE that this approach only works if you can estimate how the incoming data will look like, and may cause anomalies if you are using the *nix-based Unicode formatting scheme (using 1-4 bytes).

Actions performed on the Raspberry:
- Create time-stamp and prepend datagram contents.
- Replace ending '\n' (ending character @ (length - 1)) with non-used character of your choice.
- Append/aggregate with other prepared datagram entries.

Actions performed on the VM Server:
- Replace 'regular' special characters (new-line, horizontal tab), with their related codes, e.g. #012, #011
- Replace the character (chosen earlier) back to the '\n', hopefully producing a nice output file compliant with the syslog compatibility mentioned earlier 
- Iterate for the rest of the message, Print to file, etc.

○	 Character replacement for syslog compatibility 

Using the data straight away when writing will cause errors in comparison/expected data. New-lines, tabs, etc. need to be replaced into octal representations to save it properly. In order to change the special characters we will need to take into account additions of new characters.

Actions performed:
- Traverse/search/for-loop the String/character-sequence
- Each time you find a character with a value of 32 or below, prepare to replace it
- Move forward or copy temporarily the remainder of the string after the special character
- Create the code for the replacement and enter it where the special character was, e.g #010 for '\n' (newline)
- Append the remainder of the string if you copied it temporarily.

