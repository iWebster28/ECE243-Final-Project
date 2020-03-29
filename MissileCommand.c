//ECE243 Final Project
//Missile Command
//Written in C for the DE1-SoC FPGA, or CPULATOR emulator.
//Use keyboard and mouse input to shoot down missiles, avoid being hit.
//Project Start: 3-27-20
//Project End: 	4-9-20, 9PM
//Eric Ivanov and Ian Webster


#define YMAX 239        //The max Y-coordinate of the screen.
#define XMAX 319        //The max X coordinate of the screen.


/*******Helper functions declared.******/

//Drawing functions.
void plot_pixel(int x, int y, short int line_color, volatile int pixel_buffer_address);
void draw_line(int x0, int y0, int x1, int y1, short int line_colo, 
                                volatile int pixel_buffer_address);
void clear_screen(volatile int pixel_buffer_address);
void wait_for_vsync(volatile int * pixel_ctrl_ptr);

//Miscallaneous functions.
void swap(int* first, int* second);
int abs_diff(int first, int second);


int main(void)
{
    //Pointers to various control registers for the De1-SoC Board.
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    volatile int * ps2_ctrl_ptr = (int *)0xFF200100;
    volatile int * led_ctrl_ptr = (int *)0xFF200000;

    

    //First, the Pixel Control Register pixel buffers are configured.

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


    //The main loop of the program.
    while (1)
    {
        //Poll keyboard input.
        

        


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