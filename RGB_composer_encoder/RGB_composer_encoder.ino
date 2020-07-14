/* ************************************************************************** */
/*                                                                            */
/*                                                                            */
/*   RGB Composer with encoder                                                */
/*                                                                            */
/*   By: djfg0 <djfg0@msn.com>                                                */
/*                                                                            */
/*                                                                            */
/*   Created: 2020/05/27                                                      */
/*                                                                            */
/* ************************************************************************** */

/*
**		This sketch is used to control the 3 channels of an RGB LED
**		and let the user mix the amount of each color in the LED,
**		using a rotary encoder.
**
**		It doesn't seem to be a complicated project, but I use
**		different fun programming techniques!
**		So if you're new to C, you will find a kind of
**		finite state machine, bit fields, a typdef, global variables,
**		a structure, and reccursive programming in my code!
**
**
**		The circuit will work this way:
**
**		- Your Arduino will wait untill you press the encoder's button (or
**		  another button if your encoder doesn't have an integrated one),
**		  a red signal LED will light to let you know you selected
**		  the red channel of the RGB LED.
**		- If you, then turn the encoder, the amount of red will increase or
**		  decrease depending if you turn it clockwise or counter clockwise.
**		- If you push the button again, the amount of red will be fixed in
**		  the RGB LED and you will then be able to modify the green channel.
**		  The red signal LED will go off and the green one will light up.
**		- Then again, if you turn the encoder, the amount of green will vary
**		  in the RGB LED, without changing the red amount! So now you're
**		  starting mixing colours!
**		- If you press the button again, you will be able to modify the blue
**		  channel of the RGB LED...
**		- Now, if you press the button once more, you will go back to the
**		  "standby" state. All signal LEDs are turned off and if you turn the
**		  encoder, nothing happens to the RGB LED. It's colour will stay the
**		  one you made.
**		- Pressing the button now will let you change the red colour again.
**		- And so on...
**
**		Required material:
**
**		- 1 x Arduino (I used a mega 2560 but an UNO works too!)
**		- 1 x breadboard
**		- breadboard wires
**		- 1 x common cathode RGB LED
**		- 1 x red LED
**		- 1 x green LED
**		- 1 x blue LED
**		- 6 x 220 ohm resistors
**		- 1 x rotary encoder (I used a KY 040 module with an EC11 encoder)
**		(- 1 x push button, if the encoder comes without)
*/


/*
**		Let's start defining names for the Arduino's pins we will use
**		in the following lines.
**		It's a good practice, so your code will contain text instead of
**		numbers and it will be easier to read!
*/

#define RSIG	13		// Pin for the red signal LED
#define GSIG	12		// Pin for the green signal LED
#define BSIG	11		// Pin for the blue signal LED

#define RLED	7		// Red channel of the RGB LED
#define GLED	6		// Green channel of the RGB LED
#define BLED	5		// Blue channel of the RGB LED

#define SW		48		// SW pin of the encoder module (or push button)
#define DT		50		// DT pin of the encoder module
#define CLK		52		// CLK pin of the encoder module

#define NONE	0		// "Standby" state of the finite state machine (FSM)
#define R		1		// Red channel state of the FSM
#define G		2		// Green channel state of the FSM
#define B		3		// Blue channel state of the FSM

/*
**		When we use push buttons, when you think pushing the button once,
**		it may "bounce" in reality and send more than one push to the board.
**		So, a simple solution to avoid going directly to the last channel
**		when you pressed the button only once, is to ask the board to wait
**		for an amount of time to avoid those bounces.
**		You can change the value of the following define.
**		(in mili seconds)
*/

#define DEBOUNCE	250	// Try to lower this value to see what happens...

/*
**		The following lines are a little bit tricky, as I use
**		bit fields, in a typedef-ed structure at once...
**
**		As you can see, I use a typedef. It means I create and define
**		a new type of variable.
**		My new variable type is called t_state and is in fact
**		a structure called s_state which holds 4 unsigned int.
**		Well... in fact, the 4 variables are stored in only 1!
**		Thanks to bit fields I ask the compiler to only use 1
**		unsigned int that will be divided in 4 distinctive variables.
**		An unsigned int contains generally 32 bits.
**		By writing "unsigned int state : 2" I ask the compiler to
**		ony use 2 bits from the 32 of an unsigned int.
**		Then I ask 8 bits for the variable r.
**		8 for the variable g.
**		And 8 for the variable b.
**		So, 2 + 8 + 8 + 8 = 26 bits. They can hold in 1 unsigned int!
**		If we didn't use bit fields, we would have used 4 x 32 bits...
**		
**		Ok, now, why 2 bits and 3 x 8 bits?
**		Well, remember we have 4 states in our machine?
**		"Standby", red modification, green mod. and blue mod.
**		To count till 4 in binary, you only need two bits.
**		So the "state" variable from our typedef-ed structure
**		is intended to remember our FSM's state.
**		Now, to change the amount of red, green and blue, of the
**		RGB LED, we will in fact turn the LED on at its maximum
**		then shut it off for a certain amount of time, back on
**		and back off, and so on. But it will happen so fast that
**		we won't be able to see it blink.
**		But we will see it dim more or less!
**		To tell the board to dim the channels of the RGB LED, we need
**		to tell how long each channel needs to be turned on and off.
**		We will use the Pulse Width Modulation (PWM) capability
**		of the board to do that.
**		(SO, IF YOU'RE USING ANOTHER BOARD THAN A MEGA 2560, YOU
**		WILL HAVE TO USE THE PWM PINS OF YOUR BOARD!
**		Those pins are marked with a "~" on the board!
**		So just change the values of the above 3 defines RLED, GLED
**		and BLED to pin numbers with a "~" marked!)
**		The basic PWM takes a value between 0 and 255.
**		And to count till 255 in binary, we need exactly 8 bits!
**		That's why I use 3 x 8 bits! So we can choose the amount
**		of each color inside the RGB LED!
*/

typedef struct	s_state
{
	unsigned int	state : 2;
	unsigned int	r : 8;
	unsigned int	g : 8;
	unsigned int	b : 8;
}				t_state;

/*
**		Now the compiler knows about the sorcery behind a t_state type,
**		We still need to ask him to give us 1 variable of that kind
**		to work with!
**		That's what happens next.
**		I ask the compiler to make a t_state type variable called
**		g_state for us.
**		It's like you would write "int	age" to store an age in a
**		variable of type integer or
**		"char	c" to store a character in a variable of type character.
**		Here we use a variable called g_state of type t_state.
*/

t_state			g_state;

/*
**		I prefixed it with g_ to remember it's a global variable...
**		As it is not enclosed in a function (or more simply, between braces),
**		this variable is global!
**		Each function can access and make modifications to it.
**		Handy, so you don't need to pass a copy of it through
**		the functions below, or to send a pointer to it!
**
**		Now you know g_ means global, I can tell you the meanings
**		of the other prefixes...
**		s_ stands for structure and
**		t_ stands for typedef!
*/

/*
**		That's all for our coding setup!
**		Now you should directly jump to the setup function below
**		to see how to configure the pins of your board and follow
**		the loop function step by step!
*/





/*
**		After you turned the encoder, this function will be called.
**		I tried to implement the easiest way I could imagine
**		to detect if the encoder is turned!
**		It's not perfect, I observed a 1 bit error in some cases
**		if you're not turning the encoder straight to the next notch
**		but getting an error free reading of the encoder is not
**		important for this little project.
**		So, how does an encoder works?
**		Well, in fact, an encoder is nothing more than a double switch...
**		When you turn the shaft, the switches will turn off and back on
**		at every notch, but not exactly at the same time.
**		And, that's what we will wait for in this function!
**		We just check if one switch is on and the other off,
**		if so, that would mean that the shaft is spinning!
**		But in what direction? We just need to know which switch is on
**		and which is off.
**		If we are in one case we are rotating clockwise
**		otherwise in counterclockwise!
**		Then the only thing left is to update the value of red, green
**		or blue in our LED!
*/

void			f_enc(void)
{
	Serial.println("In f_enc");
	if (!(digitalRead(DT)) && digitalRead(CLK))				//	If spinning clockwise
	{
		if ((g_state.state == R) && (g_state.r < 255))			// If machine's state is "R" and g_state.r is lower than 255 (as we only can count untill 255 on 8 bits)
			g_state.r++;											// Add + 1 to g_state.r (the value of the red channel from our RGB LED)
		else if ((g_state.state == G) && (g_state.g < 255))		// If machine's state is "G" and g_state.g is lower than 255
			g_state.g++;											// Add + 1 to g_state.g
		else if ((g_state.state == B) && (g_state.b < 255))		// ...
			g_state.b++;											// ...
	}
	else if (digitalRead(DT) && !(digitalRead(CLK)))		//	If spinning counter clockwise
	{
		if ((g_state.state == R) && (g_state.r > 0))			// If machine's state is "R" and g_state.r is higher than 0 (as we can't handle negative values with an unsigned int)
			g_state.r--;											// Remove - 1 from g_state.r
		else if ((g_state.state == G) && (g_state.g > 0))		// If machine's state is "G" and g_state.g is higher than 0
			g_state.g--;											// Remove - 1 from g_state.g
		else if ((g_state.state == B) && (g_state.b > 0))		//	...
			g_state.b--;											// ...
	}
	Serial.print("R: ");
	Serial.print(g_state.r);
	Serial.print("\tG: ");
	Serial.print(g_state.g);
	Serial.print("\tB: ");
	Serial.println(g_state.b);
	analogWrite(RLED, g_state.r);							// Fade the red channel of the RGB LED using PWM to the value of g_state.r
	analogWrite(GLED, g_state.g);							// Fade the green channel of the RGB LED using PWM to the value of g_state.g
	analogWrite(BLED, g_state.b);							// Fade the blue channel of the RGB LED using PWM to the value of g_state.b
}

/*
**		You pressed the button, the main function brought
**		you to function "f", who brought you here!
**
**		Here, we just check the state of the machine and
**		turn the corresponding signal LEDs on or off.
*/
void			f_sig(void)
{
	if (g_state.state == R)			// If state is R
	{
		digitalWrite(RSIG, HIGH);		// Turn red signal LED ON
		digitalWrite(GSIG, LOW);		// Turn green signal LED OFF
		digitalWrite(BSIG, LOW);		// Turn blue signal LED OFF
	}
	else if (g_state.state == G)	// If state is G
	{
		digitalWrite(RSIG, LOW);		// Turn red signal LED OFF
		digitalWrite(GSIG, HIGH);		// Turn green signal LED ON
		digitalWrite(BSIG, LOW);		// Turn blue signal LED OFF
	}
	else if (g_state.state == B)	// If state is B
	{
		digitalWrite(RSIG, LOW);		// ...
		digitalWrite(GSIG, LOW);
		digitalWrite(BSIG, HIGH);
	}
	else if (g_state.state == NONE)	// If state is NONE
	{
		digitalWrite(RSIG, LOW);		// ...
		digitalWrite(GSIG, LOW);
		digitalWrite(BSIG, LOW);
	}
}

/*
**		So, you just pressed the button for the first time
**		and the main function (loop) brought you here.
**		The first thing that happens here is calling
**		function "f_sig", to light the correct signal LED.
**		->	Go read function f_sig above now and come back
**			after that.
**		
**		Now the signal LED is on, the board will wait for your
**		instructions!
**		Imagine, you press the button again...
**		g_state.state will get + 1. So g_state.state will be equal
**		to 2 (or G).
**		Then on the next line, we call the function "f" again.
**		But we already are in "f", so, the program will pause
**		itself and run a second function "f"!
**		So, once again, we will first run function "f_sig"
**		to turn the wright signal LED on, only using the machine's
**		state.
**		When it's done, we're back to "f" and wait again for
**		user input.
**		If you press the button again, this function "f" will
**		pause too. And so on, till the machine's state is "NONE"
**		again. Then we will go back to the last paused function
**		and as you can see, after calling function "f", there's
**		nothing remaining left to do. So, this paused function
**		will stop and we will go back to the previous paused
**		function. And so on till all the paused functions are
**		resumed.
**
**		Calling a function inside herself is called recursive
**		programming.
**		It is interesting, because, ou program basically does
**		3 times the same thing... So, why, writing 3 times the same
**		code as we can run it again!
**
**		Ok, now, imagine... You pressed the button once.
**		The machine's state is changed "R".
**		"f" is called, who calls "f_sig".
**		Now, instead of pushing the button again, we will turn
**		the encoder.
**		If you do so, function "f_enc" will be called!
**		->	Let's see above what happens!
**
**		And that's pretty it...
**		If you turn the shaft, you know what happens and so if
**		you press the button!
*/

void			f(void)
{
	Serial.println("In f");
	Serial.print("  state: ");
	Serial.println(g_state.state);
	f_sig();
	if (g_state.state != NONE)
	{
		while ((digitalRead(SW)))
			f_enc();
		delay(DEBOUNCE);
		g_state.state++;
		f();
	}
}

/*
**	SETUP function
**
**		What we do here, is just telling the board what pins are inputs,
**		which one are outputs and the starting values of our variables.
**		Then we turn off all the 6 LEDs to be sure they will not light
**		at the beginning of our program.
**
**		The "Serial.begin(9600)" line is to tell the board to "talk" to
**		the computer. So you will be able to see the values you are
**		choosing on screen! But it totally works without!
*/

void			setup()
{
	pinMode(RSIG, OUTPUT);
	pinMode(GSIG, OUTPUT);
	pinMode(BSIG, OUTPUT);
	pinMode(RLED, OUTPUT);
	pinMode(GLED, OUTPUT);
	pinMode(BLED, OUTPUT);
	pinMode(SW, INPUT_PULLUP);
	pinMode(DT, INPUT);
	pinMode(CLK, INPUT);
	digitalWrite(RSIG, LOW);
	digitalWrite(GSIG, LOW);
	digitalWrite(BSIG, LOW);
	digitalWrite(RLED, LOW);
	digitalWrite(GLED, LOW);
	digitalWrite(BLED, LOW);
	Serial.begin(9600);
	g_state.state = NONE;
	g_state.r = 0;
	g_state.g = 0;
	g_state.b = 0;
}

/*
**	LOOP function
**
**		Here is the heart of our program!
**		Everything starts here!
**		First, the board waits for you to press the button. If you
**		turn the encoder, nothing happens.
**		When you press the button, the machine will change to the next
**		state. That's happening by adding one to g_state.state.
**		So previous state was "NONE" AKA 0 thanks to our defines above
**		And state is now 1 AKA "R".
**		Now the state has changed, lets call function f. 
*/

void			loop()
{
	Serial.println("In loop");
	if (!digitalRead(SW))
	{
		delay(DEBOUNCE);
		g_state.state++;
		f();
	}
}

/*
**	Going further
**
**		Now you know how all this works, you maybe looked at
**		the serial monitor too and saw the values of each channel...
**		
**		But did you know the serial monitor (and your board too) could
**		also receive data you type?
**		Can you imagine changing a few things in this code to set the
**		values of g_state.r, .g and .b from your computer's keyboard live?
**
**		Or by connecting a keypad to your board?
**
**		Or adding an LCD display to monitor the values without a computer?
**
**		Or mixing everything together?
**
**		Imagine what else you could add, change, remove...
**		And TRY! LEARN!
**		Let your creativity run your world!
*/

/*
**		Thanks! <3
**		FG
*/
