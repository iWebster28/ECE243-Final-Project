int main(void)
{
    //Pointers to various control registers for the De1-SoC Board.
    volatile int * ps2_ctrl_ptr = (int *)0xFF200100;
    volatile int * led_ctrl_ptr = (int *)0xFF200000;

    //Variables to store the data read from the PS/2 input.
    int ps2_data;
    char validRead;

    //The last 3 bytes read (byte1 is the most recent).
    unsigned char byte1, byte2, byte3;
    byte1 = 0;
    byte2 = 0;
    byte3 = 0;

    //The main loop of the program, which polls the PS/2 input and
    //illuminates LEDS depending on which keys are pressed.
    while (1)
    {
        //Keyboard input processing. Keyboard input is read one byte at a time.
        //Every time a read is performed of the PS/2 control register, it discards the 
        //last byte.
        ps2_data = *(ps2_ctrl_ptr);
        validRead = ( (ps2_data & 0x8000) != 0);

        if (validRead)
        {
            //Update the last 3 bytes read to reflect the current read.
            byte3 = byte2;
            byte2 = byte1;
            byte1 = (ps2_data & 0xFF);

            //Show the most recently read byte on the LEDs.
            *led_ctrl_ptr = byte1;

        }
    }

    return 0;
}