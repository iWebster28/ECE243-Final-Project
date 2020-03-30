//Basic Audio Test

//Base Address: 0xFF203040
//Base Address + BITS:
//0: Control register - clear FIFOs and interrupt control/status
//4: Fifospace register - returns how much space in each of the 4 FIFOs
//8: Left Data: reads from MIC FIFO - writes to LINEOUT FIFO
//12: Right Data: reads from MIC FIFO - writes to LINEOUT FIFO

//Control Register:
//0: Read interrupt enable
//1: Write Interrupt enable
//2: Clear left/right read FIFOs
//3: Clear left and right write FIFOs
//8: Indicates read interrupt pending
//9: Indicates write interrupt pending

//FIFOSPACE Register - 32-bits wide - each FIFO 128 entries
//7:0 Read data avail in R CH
//15:8 Read data avail in L CH
//23:16 Write space in R CH
//31:24 Write space in L CH
int main()
{
	unsigned int fifospace;
	volatile int * audio_ptr = (int *) 0xFF203040; //Base address for audio port
	
	int apple = 0;

	while (1) //Loop forever
	{
		fifospace = *(audio_ptr+1); //Read the FIFOSPACE register (holds read AND write data)
		if ((fifospace & 0x000000FF) > 0 &&	//Make sure the read space is free.
			(fifospace & 0x00FF0000) > 0 &&	//Make sure R write space is free
			(fifospace & 0xFF000000) > 0) //Make sure L write space is free
		{
			//apple = !apple;
			//Need delay...
			int sample = //*(audio_ptr + 3);	//Get sample data from R CH (MIC)
			*(audio_ptr + 2) = sample; //Write to L and R CH
			*(audio_ptr + 3) = sample;
		}
	}
}
