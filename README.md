# Multithreaded Traffic Signal System with BeagleBone Black

G00765199: Alan Boyce 
G01339210: Sai Srikar Chivukula

Hardware Description

	•	BeagleBone Black
	•	Breadboard
	•	6 --- 100 ohms resistors
	•	6 --- LED lights
	•	2 --- push buttons
	•	2 ---  1000 ohms resistors for the buttons
	•	Male-Male jumper wires

	•	We connected one of the grounds (P8-02) from BBB to breadboard (-) right side row.
	•	We connected one of the grounds (P9-02) from BBB to breadboard (-) left side row.
	•	We connected the negative pin of the first red light to the left side ground and the positive pin to the resistor which is connected to GPIO69
	•	We connected the negative pin of the first yellow light to the left side ground and the positive pin to the resistor which is connected to GPIO45
	•	We connected the negative pin of the first green light to the left side ground and the positive pin to the resistor which is connected to GPIO66
	•	We connected the negative pin of the second red light to the right-side ground and the positive pin to the resistor which is connected to GPIO44
	•	We connected the negative pin of the second yellow light to the right-side ground and the positive pin to the resistor which is connected to GPIO68
	•	We connected the negative pin of the second green light to the right-side ground and the positive pin to the resistor which is connected to GPIO67
	•	We have connected 3.3v from the Pin (3) P9 to breadboard (+) left side row.
	•	For Push Button-1, We have connected one end of the button to the left side 3.3v of breadboard. The other end of the button to GPIO26, for taking input from the button and sending it to Beaglebone black.
	•	For Push Button-2, We have connected one end of the button to the left side 3.3v of breadboard. The other end of the button to GPIO47, for taking input from the button and sending it to Beaglebone black.
	•	

Code Check General Reflection
For the Code Check, we were able to reduce the number of violations by creating #defines, modularizing the code and creating global variables as well as using types throughout the codebase. Some of the code check violations were redundant such as the discarded return values for the main function and other void functions. Other violations were unclear how to fix such as the “has external linkage but no visible declaration” so those were left as is. 


