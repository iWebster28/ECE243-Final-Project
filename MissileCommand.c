//ECE243 Final Project
//Missile Command
//Written in C for the DE1-SoC FPGA, or CPULATOR emulator.
//Use keyboard and mouse input to shoot down missiles, avoid being hit.
//Project Start: 3-27-20
//Project End: 	4-9-20, 9PM
//Eric Ivanov and Ian Webster


#define YMAX 239        //The max Y-coordinate of the screen.
#define XMAX 319        //The max X coordinate of the screen.

//Different structs used to represent objects on the screen.


//The Cursor struct used to describe the cursor on the screen.
struct Cursor
{
    //The current coordinates of the centre of the cursor.
	int x;  
    int y;

    //The old coordinates of the centre of the cursor, from the previous frame.
    int xOld, yOld;
	
	//The direction the cursor is moving.
	int deltax;
	int deltay;
	
	//The colour of the cursor.
	short int colour;
};

typedef struct Cursor Cursor;



/*******Helper functions declared.******/

//Drawing functions.
void plot_pixel(int x, int y, short int line_color, volatile int pixel_buffer_address);
void draw_line(int x0, int y0, int x1, int y1, short int line_colo, 
                                volatile int pixel_buffer_address);
void draw_cursor(Cursor screenCursor, volatile int pixel_buffer_address);
void erase_old_cursor(Cursor screenCursor, volatile int pixel_buffer_address);
void clear_screen(volatile int pixel_buffer_address);
void wait_for_vsync(volatile int * pixel_ctrl_ptr);




//Input functions.
void mostRecentKeyboardInputs(volatile int * ps2_ctrl_ptr, unsigned char * readByte1, 
                                unsigned char * readByte2, unsigned char * readByte3);

//Game variable update functions.
void initializeScreenCursor(Cursor * screenCursorPtr);
void updateCursorMovementDirection(Cursor * screenCursorPtr, unsigned char readByte1,
                                    unsigned char readByte2, unsigned char readByte3);
void updateCursorPosition(Cursor * screenCursorPtr);

//Miscallaneous functions.
void swap(int* first, int* second);
int abs_diff(int first, int second);






int main(void)
{
    //Pointers to various control registers for the De1-SoC Board.
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    volatile int * ps2_ctrl_ptr = (int *)0xFF200100;
    volatile int * led_ctrl_ptr = (int *)0xFF200000;

    //Variables to store the data read from the PS/2 input.
    
    //The last 3 bytes read (byte1 is the most recent).
    unsigned char readByte1, readByte2, readByte3;


    //The Pixel Control Register pixel buffers are configured.

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer

    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync(pixel_ctrl_ptr);

    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;

    //Now, the front and back pixel buffers are cleared for later drawing.

    //pixel_buffer_address points to the front pixel buffer.
    volatile int pixel_buffer_address = *pixel_ctrl_ptr; 
	clear_screen(pixel_buffer_address); 
    
    //Set pixel_buffer_address to the back buffer, which will be drawed on.
    pixel_buffer_address = *(pixel_ctrl_ptr + 1);
    clear_screen(pixel_buffer_address); 


    //Game variables, such as objects on the screen, are declared.
    Cursor screenCursor;
    initializeScreenCursor(&screenCursor);
    

    //The main loop of the program.
    while (1)
    {
        //Clear previous graphics.

        //Erase the cursor from the previous frame.
        erase_old_cursor(screenCursor, pixel_buffer_address);

        //Poll keyboard input.
        mostRecentKeyboardInputs(ps2_ctrl_ptr, &readByte1, &readByte2, &readByte3);

        //Process the input to update game variables.
        updateCursorMovementDirection(&screenCursor, readByte1, readByte2, readByte3);
        updateCursorPosition(&screenCursor);

        //Draw new graphics.
        draw_cursor(screenCursor, pixel_buffer_address);


        //Synchronize with the VGA display and swap the front and back pixel buffers.
        wait_for_vsync(pixel_ctrl_ptr); // swap front and back buffers on VGA vertical sync
        pixel_buffer_address = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}



/******Helper functions implemented.******/

//Drawing functions.

//Plots a single pixel of specified colour at an (x, y) coordinate on the screen.
void plot_pixel(int x, int y, short int line_color, volatile int pixel_buffer_address)
{
    *(short int *)(pixel_buffer_address + (y << 10) + (x << 1)) = line_color;
}

//Draws a coloured line between two points using Bresenham's Algorithm.
void draw_line(int x0, int y0, int x1, int y1, short int line_color, 
                                volatile int pixel_buffer_address)
{
	//char is used as a substitute for boolean to avoid dependency on std::bool.h.
	char is_steep = abs_diff(y1, y0) > abs_diff(x1, x0);
	if (is_steep)
	{
		//The x coordinates of the line are swapped with the y coordinates.
		swap(&x0, &y0);
		swap(&x1, &y1);
	}
	
	//If the starting point is further right than the ending point, swap the points so the line is always drawn from smaller x values
	//to larger x values.
	if (x0 > x1)
	{
		swap(&x0, &x1);
		swap(&y0, &y1);
	}
	
	//Calculate deltax, deltay, and error.
	int deltax = x1 - x0;
	int deltay = abs_diff(y1, y0);
	int error = -(deltax / 2);
	
	//y_step depends on if the change in y from left to right is positive or negative.
	int y_step = (y0 < y1) ? 1 : -1;
	
	//Each pixel of the line is drawn, from small x values to large x values.
	int y = y0;
	for (int x = x0; x <= x1; x++)
	{
		if (is_steep)
			plot_pixel(y, x, line_color, pixel_buffer_address);
		else
			plot_pixel(x, y, line_color, pixel_buffer_address);
		
		//If error becomes positive, the value of y is adjusted.
		error += deltay;
		if (error >= 0)
		{
			y = y + y_step;
			error = error - deltax;
		}
	}
	
	return;
}


//Draws the cursor on the screen.
void draw_cursor(Cursor screenCursor, volatile int pixel_buffer_address)
{
    //The cursor consists of an intersecting vertical and horizontal line, both 5 pixels long.

    //Draw the horizontal part of the cursor.
    draw_line(screenCursor.x - 2, screenCursor.y, screenCursor.x + 2, screenCursor.y,
                screenCursor.colour, pixel_buffer_address);

    //Draw the vertical part of the cursor.
    draw_line(screenCursor.x, screenCursor.y - 2, screenCursor.x, screenCursor.y + 2,
                screenCursor.colour, pixel_buffer_address);

    return;
}

//Erases the old cursor from the previous frame.
void erase_old_cursor(Cursor screenCursor, volatile int pixel_buffer_address)
{
    //To erase the old cursor, draw over its previous position with black.
    //Do not need to erase its current position due to frame buffering, as this
    //will be erased the next frame cycle (it becomes the new "current position").

    //Erase the horizontal part of the old cursor.
    draw_line(screenCursor.xOld - 2, screenCursor.yOld, screenCursor.xOld + 2, 
                screenCursor.yOld, 0, pixel_buffer_address);

    //Erase the vertical part of the old cursor.
    draw_line(screenCursor.xOld, screenCursor.yOld - 2, screenCursor.xOld, 
                screenCursor.yOld + 2, 0, pixel_buffer_address);

    return;
}

//Writes a black pixel to every pixel on the screen, essentially "clearing" it.
void clear_screen(volatile int pixel_buffer_address)
{
	for (int y = 0; y <= YMAX; y++)
		for (int x = 0; x <= XMAX; x++)
			plot_pixel(x, y, 0, pixel_buffer_address);
	
	return;
}

//Does not return until the VGA controller indicates a VSync has occurred. Swaps the front
//and back pixel buffers.
void wait_for_vsync(volatile int * pixel_ctrl_ptr)
{
	//Request a buffer swap.
	*(pixel_ctrl_ptr) = 1;
	
	//A sync has occurred if the value of the S bit in the Status register of the pixel 
    //controller is set to 0.
	register char syncOccurred = !( *(pixel_ctrl_ptr + 3) & 0b1 );
	
	//Wait for the sync to occur.
	while (!syncOccurred)
	{
		syncOccurred = !( *(pixel_ctrl_ptr + 3) & 0b1 );
	}
	
	return;
}



//Input Functions.

//Gets the 3 most recent bytes of keyboard input and stores them at the addresses of the
//parameters. The most recent input is stored in the address pointed to by readByte1.
void mostRecentKeyboardInputs(volatile int * ps2_ctrl_ptr, unsigned char * readByte1, 
                                unsigned char * readByte2, unsigned char * readByte3)
{
    //Keyboard input processing. Keyboard input is read one byte at a time.
    //Every time a read is performed of the PS/2 control register, it discards the 
    //last byte. On a frame cycle of the game, only the most recent 3 inputs from
    //that frame are considered.

    int ps2_data;
    char validRead;

    //Clear previously stored input.
    *readByte1 = 0;
    *readByte2 = 0;
    *readByte3 = 0;

    ps2_data = *(ps2_ctrl_ptr);
    validRead = ( (ps2_data & 0x8000) != 0);

    //So long as there is still valid input to process, keep updating the variables storing 
    //the most recent bytes of input.
    while (validRead)
    {
        //Update the last 3 bytes read to reflect the current read.
        *readByte3 = *readByte2;
        *readByte2 = *readByte1;
        *readByte1 = (ps2_data & 0xFF);

        //Attempt to read from the PS/2 Controller again and check if the read is valid.
        ps2_data = *(ps2_ctrl_ptr);
        validRead = ( (ps2_data & 0x8000) != 0);
    }

    return;
}



//Game variable update functions.

//Sets the initial value of the fields of the Cursor object.
void initializeScreenCursor(Cursor * screenCursorPtr)
{
    //The cursor starts in the centre of the screen.
    screenCursorPtr->x = XMAX / 2;
    screenCursorPtr->y = YMAX / 2;

    //The old locations of the screenCursor are set to be the same as its starting position.
    screenCursorPtr->xOld = screenCursorPtr->x;
    screenCursorPtr->yOld = screenCursorPtr->y;

    //The colour of the cursor, in this case green.
    screenCursorPtr->colour = 0x07E0;

    return;
}

//The dirction the cursor is moving is updated by the arrow keys.
void updateCursorMovementDirection(Cursor * screenCursorPtr, unsigned char readByte1,
                                    unsigned char readByte2, unsigned char readByte3)
{
    //Only one cursor direction can be affected by input at a time.

    //First, set all cursor movement to 0.
    screenCursorPtr->deltax = 0;
    screenCursorPtr->deltay = 0;

    //Now, check to see if arrow keys were pressed, and update movement direction accordingly.
    if (readByte2 == 0xE0)          //This byte is always present for an arrow key.
    {
        if (readByte1 == 0x6B)      //Left arrow key.
            screenCursorPtr->deltax = -1;

        if (readByte1 == 0x74)      //Right arrow key.
            screenCursorPtr->deltax = 1;

        if (readByte1 == 0x75)      //Up arrow key.
            screenCursorPtr->deltay = -1;

        if (readByte1 == 0x72)      //Down arrow key.
            screenCursorPtr->deltay = 1;
    }

    return;
}

//The position of the cursor (as well as the value of its "old position" fields) is updated
//based off of its movement direction and its position relative to the screen bounds.
void updateCursorPosition(Cursor * screenCursorPtr)
{
    //Update the fields reflecting the old position with the current position.
    screenCursorPtr->xOld = screenCursorPtr->x;
    screenCursorPtr->yOld = screenCursorPtr->y;

    //Update the current position of the cursor.
    if (screenCursorPtr->deltax == -1)      //Moving left.
    {
        //Only move left if the cursor, which is 5 pixels wide and tall and centred on (x,y)
        //will not move past the left bound of the screen.
        if (screenCursorPtr->x > 2)
            screenCursorPtr->x = screenCursorPtr->x - 1;
    }

    if (screenCursorPtr->deltax == 1)       //Moving right.
    {
        //Only move right if the cursor will not move past the right edge of the screen.
        if (screenCursorPtr->x < XMAX - 2)
            screenCursorPtr->x = screenCursorPtr->x + 1;
    }

    if (screenCursorPtr->deltay == -1)      //Moving up.
    {
        //Only move up if the cursor will not move past the top of the screen.
        if (screenCursorPtr->y > 2)
            screenCursorPtr->y = screenCursorPtr->y - 1;
    }

    if (screenCursorPtr->deltay == 1)       //Moving down.
    {
        //Only move down if the cursor will not move past the bottom of the screen.
        if (screenCursorPtr->y < YMAX - 2)
            screenCursorPtr->y = screenCursorPtr->y + 1;
    }

    return;
}









//Miscallaneous functions.

//A swap function, which swaps the value stored by two integer variables.
void swap(int* first, int* second)
{
	int temp = *first;
	*first = *second;
	*second = temp;
	return;
}
	
//An absolute difference function, which returns the positive numerical distance between 
//two values;
int abs_diff(int first, int second)
{
	if (first >= second)
		return first - second;
	else
		return second - first;
}