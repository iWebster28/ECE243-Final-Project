//ECE243 Final Project
//Missile Command
//Written in C for the DE1-SoC FPGA, or CPULATOR emulator.
//Use keyboard and mouse input to shoot down missiles, avoid being hit.
//Project Start: 3-27-20
//Project End: 	4-9-20, 9PM
//Eric Ivanov and Ian Webster

//Basic Audio Test
int main()
{
	unsigned int fifospace;
	volatile int * audio_ptr = (int *) 0xFF203040; // audio port
	
	while (1)
	{
		fifospace = *(audio_ptr+1); // read the audio port fifospace register
		if ((fifospace & 0x000000FF) > 0 &&		// Available sample right
			(fifospace & 0x00FF0000) > 0 &&		// Available write space right
			(fifospace & 0xFF000000) > 0)		// Available write space left
		{
			int sample = *(audio_ptr + 3);	// read right channel only
			*(audio_ptr + 2) = sample;		// Write to both channels
			*(audio_ptr + 3) = sample;
		}
	}
}
