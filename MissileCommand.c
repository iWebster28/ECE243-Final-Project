//ECE243 Final Project
//Missile Command
//Written in C for the DE1-SoC FPGA, or CPULATOR emulator.
//Use keyboard and mouse input to shoot down missiles, avoid being hit.
//Project Start: 3-27-20
//Project End: 	4-9-20, 9PM
//Eric Ivanov and Ian Webster

#include <stdlib.h>
#include <stdbool.h>
#include <time.h> //for rand
#include <stdio.h>
#include <math.h>

//Global
volatile int * led_ctrl_ptr = (int *)0xFF200000;

#define FPS 60 //Framerate

#define YMAX 239        //The max Y-coordinate of the screen.
#define XMAX 319        //The max X coordinate of the screen.

#define PS2INPUTBYTES 12    //The number of bytes from the PS2 input processed at a time.
                            //Stored PS2 input bytes are refreshed each game cycle.
                            //If the number of input bytes exceeds this value, the extra
                            //bytes are discarded.

#define N 10 //MAX NUM MISSILES ON SCREEN ALLOWED AT A TIME (WITHIN XMAX AND YMAX)
#define SIZE_MISSILE 4
//Colours
const short int black = 0x0000;
const short int red = 0xF800;
const short int blue = 0x081F;
const short int green = 0x07E0;

time_t t; //for rand

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

struct Missile {
    double x_vel; //Velocity
    double y_vel; 
    double x_pos; //Current position to be drawn
    double y_pos;
    double x_old; //Previous position
    double y_old;
    double x_old2;
    double y_old2;
    short int colour; //Colour of missile
    int in_bound; //0 if not in bounds; 1 if in bounds; 2 if JUST went out of bounds (for clear_missiles)
    //Note if set to bool, then graphics work fine. when set to 2, = true;
    //But when int, then frame flickers on bound.
};

typedef struct Missile Missile;

//The AntiAirRocket struct used to describe the missiles fired by the player to 
//defend the cities.
struct AntiAirRocket
{
    //The current coordinates of the anti-air rocket.
    int x;
    int y;

    //The old coordinates of the anti-air rocket.
    int xOld, yOld;

    //The final coordinates of the anti-air rocket, that is, where it is moving towards
    //after being fired from the missile platform.
    int xFinal;
    int yFinal;

    //The colour of the anti-air missile.
    short int colour;


};

typedef struct AntiAirRocket AntiAirRocket;



/*******Helper functions declared.*******/

//Drawing functions.
void plot_pixel(int x, int y, short int line_color, volatile int pixel_buffer_address);
void draw_line(int x0, int y0, int x1, int y1, short int line_color, volatile int pixel_buffer_address);
void draw_cursor(Cursor screenCursor, volatile int pixel_buffer_address);
void erase_old_cursor(Cursor screenCursor, volatile int pixel_buffer_address);
void clear_screen(volatile int pixel_buffer_address);
void wait_for_vsync(volatile int * pixel_ctrl_ptr);

//Missile Functions
void compute_missiles(Missile *missile_array, int num_missiles, double x_target, double y_target, int x_vel_max, int y_vel_max, short int colour, volatile int pixel_buffer_address, bool adding_missiles);
void draw_enemy_missile(int x0, int y0, short int colour, volatile int pixel_buffer_address);
void draw_missiles_and_update(Missile *missile_array, int num_missiles, volatile int pixel_buffer_address);
void clear_missiles(int num_missiles, Missile *missile_array, short int colour, volatile int pixel_buffer_address);

int determine_num_missiles(int round_num, int *spawn_threshold); //Returns max num_missiles allowed on screen at a time per round#. ALSO updates spawn threshold.
void add_missiles(int num_missiles, Missile *missile_array, short int *colour, volatile int pixel_buffer_address); //Add more missiles to the missile_array for the current round. Make sure to check entries only with in_bound = 2. Then set to 1 after!
bool missiles_on_screen_check(int *num_missiles, Missile *missile_array, int *spawn_threshold); //Check how many missiles are on screen right now. Return true if within threshold to draw more. Else, (i.e. still a lot on screen), return false.

//Diagnostic Functions
bool inBounds(int x, int y); //Returns bool
void inFunction(volatile int pixel_buffer_address, int led_to_light);
void notInFunction(volatile int pixel_buffer_address, int led_to_light);


//Input functions.
void mostRecentKeyboardInputs(volatile int * ps2_ctrl_ptr, unsigned char readBytes[]);

//Game variable update functions.
void initializeScreenCursor(Cursor * screenCursorPtr);
void updateCursorMovementDirection(Cursor * screenCursorPtr, unsigned char readBytes[]);
void updateCursorPosition(Cursor * screenCursorPtr);

//Miscallaneous functions.
void swap(int* first, int* second);
int abs_diff(int first, int second);
double randDouble(double min, double max);




int main(void)
{
    //Pointers to various control registers for the De1-SoC Board.
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    volatile int * ps2_ctrl_ptr = (int *)0xFF200100;
    volatile int * led_ctrl_ptr = (int *)0xFF200000;

    //Variables to store the data read from the PS/2 input.
    
    //The array holding the bytes read from the PS2 input each cycle.
    //The byte with index "0" is the one most recently read.
    unsigned char readBytes[PS2INPUTBYTES];


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


    //GENERATE AND STORE MISSILES
    int round_num = 0; //intial round is 0
    int spawn_threshold = 0; //Upper bound on how many on-screen missiles are left, which will cause more to be spawned for the current round.
    int num_missiles = 5; //num_missiles will be determined based on round number, which changed in add_missiles
    Missile missile_array[N]; //Declare array of missiles. FIXED MAXIMUM OF N MISSILES ON SCREEN AT A TIME.

    //These parameters could be changed for every round/stage. Testing purposes currently.!!!!!!!!!!!!!
    double x_target = 160;
    double y_target = 239;
    //Max velocities in pixels/sec (Framerate already taken into account in code.)
    //Note: these will just be computed as one max velocity.
    int x_vel_max = 250; //pos or neg. direction
    int y_vel_max = 250; 
    short int colour = red; //temp
    
    //Round 0: determine how many missiles on screen allowed at a time.
    num_missiles = determine_num_missiles(round_num, &spawn_threshold); 
    //Compute N missiles for the intial round.
    bool add_more_missiles = false; //not addding extra missiles! Computing the starting wave, therefore set = false
    compute_missiles(missile_array, num_missiles, x_target, y_target, x_vel_max, y_vel_max, colour, pixel_buffer_address, add_more_missiles);
    
    //The main loop of the program.
    while (1)
    {
        //Clear previous graphics.

        //Erase the old position of all the missiles - FIX.
        clear_missiles(num_missiles, missile_array, blue, pixel_buffer_address);

        //ROUND LOGIC------------------------------------------
        //CHECK IF ROUND OVER (i.e. #of collisions with player > some_max)
        //If yes, reset to round 0 (round_num = 0;

        //else, see if we can change to NEXT ROUND:
        //check if certain time has passed? or maybe if add_missiles has been called enough times. 
        //Then: round_num++; 
        //num_missiles = determine_num_missiles(int round_num); //udpate num missiles for this round
        //then call compute_missiles(num_missiles as determined by round_num)
        //compute_missiles should be called at the start of every round. Will overwrite/create an entirely new batch of missiles. 
        //(for loop for num_missiles times.)
        
        //DETERMINE IF WE NEED TO ADD MORE MISSILES TO THE SCREEN - for the CURRENT ROUND.
        //Check how many missiles are on screen. 
        add_more_missiles = missiles_on_screen_check(&num_missiles, missile_array, &spawn_threshold);
        if (num_missiles != 5) printf("num_missiles: %d\n", num_missiles);
        
        if (add_more_missiles) printf("Adding more missiles!\n");
        if (add_more_missiles) {
            num_missiles = determine_num_missiles(round_num, &spawn_threshold);
            compute_missiles(missile_array, num_missiles, x_target, y_target, x_vel_max, y_vel_max, colour, pixel_buffer_address, add_more_missiles);
            //add_missiles(num_missiles, missile_array, colour, pixel_buffer_address); 
            //This function should be called whenever missiles_on_screen_check returns true. (checks if num onscreen missiles is == certain 
            //threshold for spawning more)
            //This function should overwrite the missile entries that went out of bounds, for the current num_missiles allowed on screen for the curent round.

            //Depending on the round, some or all of the slots in the missile_array will be used.
            add_more_missiles = false; //redundant
        }
    
        //-------------------------------------

        //Erase the cursor from the previous frame.
        erase_old_cursor(screenCursor, pixel_buffer_address);

        //Poll keyboard input.
        mostRecentKeyboardInputs(ps2_ctrl_ptr, readBytes);

        //Process the input to update game variables.
        updateCursorMovementDirection(&screenCursor, readBytes);
        updateCursorPosition(&screenCursor);

        //Draw new graphics.
        draw_cursor(screenCursor, pixel_buffer_address);

        draw_missiles_and_update(missile_array, num_missiles, pixel_buffer_address); //pass in arrays

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
void mostRecentKeyboardInputs(volatile int * ps2_ctrl_ptr, unsigned char readBytes[]) 
{
    //Keyboard input processing. Keyboard input is read one byte at a time.
    //Every time a read is performed of the PS/2 control register, it discards the 
    //last byte. On a frame cycle of the game, only the most recent 3 inputs from
    //that frame are considered.

    int ps2_data;
    char validRead;

    //Clear previously stored input.
    //Note that the number of bytes processed each cycle is identified by PS2INPUTBYTES.
    //Thus, the size of the arrya holding the input need not be explicitly passed to this 
    //function and others that use it.
    for (int i = 0; i < PS2INPUTBYTES; i++)
        readBytes[i] = 0;
    

    ps2_data = *(ps2_ctrl_ptr);
    validRead = ( (ps2_data & 0x8000) != 0);

    //So long as there is still valid input to process, keep filling the array storing 
    //the bytes of input from the bottom up.
    //If the array becomes full, extra input is discarded.
    int i = PS2INPUTBYTES - 1;  //Index for the bottom of the array.
    while (validRead)
    {
        //Add bytes to the array containing input so long as it is not full.
        if (i >= 0)
        {
            readBytes[i] = (ps2_data & 0xFF);
            i--;
        }

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
void updateCursorMovementDirection(Cursor * screenCursorPtr, unsigned char readBytes[])
{
    //The array of keyboard input is examined in overlapping three-byte packets.
    //The location of these packets in the array is kept track of with the index i.
    //3 byte overlapping sequences are considered from the highest index of the
    //input array to the lowest (from the oldest input to the most recent).
    for (int i = PS2INPUTBYTES - 1; i > 1; i--)
    {
        //First check to see if an arrow key sent the input byte.
        //Information from arrow keys starts with the hex value E0.
        if (readBytes[i] == 0xE0)
        {
            //Now, check and see if the second byte indicates a key release.
            //If the arrow key was released, it should have the value F0.
            if (readBytes[i - 1] == 0xF0)
            {
                //Now that it is known an arrow key was released, stop movement
                //according to the key that was pressed (inspect the third byte).
                if (readBytes[i - 2] == 0x6B)      //Left arrow key released.
                    screenCursorPtr->deltax = 0;

                if (readBytes[i - 2] == 0x74)      //Right arrow key released.
                    screenCursorPtr->deltax = 0;

                if (readBytes[i - 2] == 0x75)      //Up arrow key released.
                    screenCursorPtr->deltay = 0;

                if (readBytes[i - 2] == 0x72)      //Down arrow key released.
                    screenCursorPtr->deltay = 0;
            }

            //If the second byte did not indicate a release, it may indicate a press instead.
            //Check the value of the second byte to see which key press it represents, and
            //update movement accordingly.
            if (readBytes[i - 1] == 0x6B)      //Left arrow key.
                screenCursorPtr->deltax = -1;

            if (readBytes[i - 1] == 0x74)      //Right arrow key.
                screenCursorPtr->deltax = 1;

            if (readBytes[i - 1] == 0x75)      //Up arrow key.
                screenCursorPtr->deltay = -1;

            if (readBytes[i - 1] == 0x72)      //Down arrow key.
                screenCursorPtr->deltay = 1;
        }
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

//This function creates multiple missiles according to the parameters it is fed.
void compute_missiles(Missile *missile_array, int num_missiles, double x_target, double y_target, int x_vel_max, int y_vel_max, short int colour, volatile int pixel_buffer_address, bool adding_missiles) {
    int y_vel_min = 100;

    //Colours array
    //short int colours[3] = {red, blue, green}; //{0xFFFF, 0xF800, 0x07E0, 0x001F, 0x5890, 0x1240, 0x2510, 0xFFAB};

    //not using x and y target yet. Soon, use eqn of line.
    //Random numbers
    srand((unsigned) time(&t)); //Generate random numbers

    int x_width_spawn = 300; //assumed spwan width centered at top of screen.
    int y_height_spawn = 5; //How far from top of screen can missiles spawn.
    //int x0 = 0, y0 = 0, x_vel = 0, y_vel = 0;
    //((rand() % 2)*2) - 1;
    for (int i = 0; i < num_missiles; i++) {

        //if adding_missielse == true, then we just want to create new missiles at array indices at which
        //the missiles have gone out of bounds!
        //If adding_missiles == false - we are creating new missiles at EVERY indice in the array because a new wave has started.
        
        if (adding_missiles == false || (adding_missiles == true && missile_array[i].in_bound == 2)) {
            //Initialize all previous position data to 0
            missile_array[i].in_bound = 1; //1 means in bound
            missile_array[i].x_old = 0;
            missile_array[i].y_old = 0;
            missile_array[i].x_old2 = 0;
            missile_array[i].y_old2 = 0;
            
            //Random Position
            missile_array[i].x_pos = (rand() % x_width_spawn) + (0.5*XMAX - 0.5*x_width_spawn) /*center*/;
            missile_array[i].y_pos = (rand() % y_height_spawn);
            
            //Random Velocity
            //missile_array[i].x_vel = ((rand() % 2)*2) - 1; //(rand() % x_vel_max) - x_vel_max; //between - and + x_vel_max
            //Trying to generate a random double for y-velocity.
            //srand(time(NULL));
            //missile_array[i].y_vel = randDouble(0, 2);
            //printf ( "%f\n", missile_array[i].y_vel);
            //if (missile_array[i].y_vel == 0) missile_array[i].y_vel = (rand() % (y_vel_max - 1)) + 1;

            //Try to target a specific location by precomputing the velocity to reach the target
            double path_time = 1*FPS; //in 1/60th of second intervals. Therfore (1/60th sec)*60 = 1 second from start to end position.
            double dx = ((double) (x_target - missile_array[i].x_pos));
            double dy = ((double) (y_target - missile_array[i].y_pos));

            //double vel_max = sqrt(x_vel_max*x_vel_max + y_vel_max*y_vel_max);

            double rand_x_vel = ((rand() % (x_vel_max - 1)) - x_vel_max + 1); //Used to determine velocities to reach the final position.
            double rand_y_vel = (rand() % (y_vel_max - 1 - y_vel_min)) + 1 + y_vel_min;

            double path_time_x = (dx / rand_x_vel); //Store the time it would take (in 60ths of a sec) to go from start to end XPOSITION, based on the user-defined velocity.
            double path_time_y = (dy / rand_y_vel); //Store the time (in 60ths of a second) to go from start to end YPOSITION, based on the user-defined a random yvelocity.
            
            path_time = FPS*sqrt(path_time_x*path_time_x + path_time_y*path_time_y); //Path time it will take
            //to go from start x,y to end x,y - at constrained x,y random velocities - with maximums set by the user in pixels/sec.

            missile_array[i].x_vel = dx / path_time;
            missile_array[i].y_vel = dy / path_time;

            missile_array[i].colour = colour;
            //draw_enemy_missile(x0, y0, colour, pixel_buffer_address);
        }
    }

}

//Main draw function
void draw_missiles_and_update(Missile *missile_array, int num_missiles, volatile int pixel_buffer_address) {
    //Draw all the missiles in the missile_array
    for (int i = 0; i < num_missiles; i++) {
        //missile_array[i].x_pos - SIZE_MISSILE, missile_array[i].y_pos - SIZE_MISSILE)
        if (inBounds(missile_array[i].x_pos, missile_array[i].y_pos)) {
            draw_enemy_missile(missile_array[i].x_pos, missile_array[i].y_pos, missile_array[i].colour, pixel_buffer_address);
            
            // //Update positions
            missile_array[i].x_old2 = missile_array[i].x_old;
            missile_array[i].x_old = missile_array[i].x_pos; //Set as previous position
            missile_array[i].x_pos += missile_array[i].x_vel; //Update positions correspondingly

            missile_array[i].y_old2 = missile_array[i].y_old;
            missile_array[i].y_old = missile_array[i].y_pos;
            missile_array[i].y_pos += missile_array[i].y_vel;

            //Bounds-clamping - for newly computed positions.
            if (missile_array[i].x_pos <= 0) {
                missile_array[i].x_pos == -1;
                missile_array[i].in_bound = 0; //to notify clear_missiles on next while loop iteration in main that the missile is now out of bounds
            }
            if (missile_array[i].x_pos >= XMAX) {
                missile_array[i].x_pos == XMAX + 1;
                missile_array[i].in_bound = 0;
            }
            if (missile_array[i].y_pos >= YMAX) {
                missile_array[i].y_pos == YMAX + 1;
                missile_array[i].in_bound = 0;
            }
        } else { //not in bounds.
            //missile_array[i].in_bound = 0;
        }
    }
    
    return;

}


//Function to draw the incoming enemy nukes from the top of the screen.
void draw_enemy_missile(int x0, int y0, short int colour, volatile int pixel_buffer_address) {
    for (int x = x0; (x < x0 + SIZE_MISSILE); x++) {
        for (int y = y0; (y < y0 + SIZE_MISSILE); y++) {
            if (x <= 0|| x >= XMAX || y <= 0 || y >= YMAX) { //was <= >= <= >=
                //if out of bounds, don't draw pixel. // WILL HAVE TO CHANGE CORRESPONDING TO BOX SIZE.
            } else {
                plot_pixel(x, y, colour, pixel_buffer_address);
            } 
        }
    }

}

//Remove missiles from 2 frames ago! - THIS SHOULD NOT BE DRAWING AFTER A MISSILE LEAVES THE SCREEN...
void clear_missiles(int num_missiles, Missile *missile_array, short int colour, volatile int pixel_buffer_address) {
    short int colour_array[3] = {red, blue, green};
    //short int trail_colour_1 = blue; //colour_array[rand() % 3];
    //short int trail_colour_2 = green;

    short int colour_to_paint = black;
    for (int i = 0; i < num_missiles; i++) {
        if (missile_array[i].in_bound == 2) { //Don't draw anything for this missile - it went out of bounds.
            //notInFunction(pixel_buffer_address, 1);
        }
        else if (missile_array[i].in_bound == 0) { //If at the boundary:
            if (inBounds(missile_array[i].x_old2, missile_array[i].y_old2)) { //clear 2nd last frame
                //inFunction(pixel_buffer_address, 1); //DIAG
                printf("At boundary\n");
                draw_enemy_missile(missile_array[i].x_old2, missile_array[i].y_old2, colour_to_paint, pixel_buffer_address);
            }
            if (inBounds(missile_array[i].x_old, missile_array[i].y_old)) { //clear last frame
                draw_enemy_missile(missile_array[i].x_old, missile_array[i].y_old, colour_to_paint, pixel_buffer_address);
            }
            missile_array[i].in_bound = 2; //prevent drawing again.

        } else if (missile_array[i].in_bound == 1) { //if in bounds
            if (inBounds(missile_array[i].x_old2, missile_array[i].y_old2)) { //clear 2nd last frame
                draw_enemy_missile(missile_array[i].x_old2, missile_array[i].y_old2, colour_to_paint, pixel_buffer_address);
            }
            if (inBounds(missile_array[i].x_old, missile_array[i].y_old)) { //clear last frame
                draw_enemy_missile(missile_array[i].x_old, missile_array[i].y_old, colour_to_paint, pixel_buffer_address);
            }
        }
    }
    
    /*
    for (int i = 0; i < num_missiles; i++) {
        if (missile_array[i].x_pos == XMAX || missile_array[i].x_pos == 0 || missile_array[i].y_pos == YMAX) { //missile_array[i].in_bound == 0
            //Clear current pos.
            draw_enemy_missile(missile_array[i].x_pos, missile_array[i].y_pos, trail_colour_1, pixel_buffer_address);

            if (inBounds(missile_array[i].x_old2, missile_array[i].y_old2)) { //clear 2nd last frame
                inFunction(pixel_buffer_address, 1); //DIAG
                draw_enemy_missile(missile_array[i].x_old2, missile_array[i].y_old2, trail_colour_1, pixel_buffer_address);
            }
            if (inBounds(missile_array[i].x_old, missile_array[i].y_old)) { //clear last frame
                inFunction(pixel_buffer_address, 1); //DIAG
                draw_enemy_missile(missile_array[i].x_old, missile_array[i].y_old, trail_colour_1, pixel_buffer_address);
            }
            missile_array[i].in_bound = 2; //2 means "went out of bounds, and need to clear an extra frame. (the "current position")"
            missile_array[i].x_pos = XMAX + 1; //to make out of bounds. Therefore, draw_missiles_and_update won't update position, and this 'if' won't draw again.
            missile_array[i].y_pos = YMAX + 1;

        } else if (inBounds(missile_array[i].x_pos, missile_array[i].y_pos)) { //if still in bounds of screen.
            if (inBounds(missile_array[i].x_old2, missile_array[i].y_old2)) { //clear 2nd last frame
                inFunction(pixel_buffer_address, 2); //DIAG
                draw_enemy_missile(missile_array[i].x_old2, missile_array[i].y_old2, trail_colour_2, pixel_buffer_address);
            }
            if (inBounds(missile_array[i].x_old, missile_array[i].y_old)) { //clear last frame
                //inFunction(pixel_buffer_address, 2); //DIAG
                draw_enemy_missile(missile_array[i].x_old, missile_array[i].y_old, trail_colour_2, pixel_buffer_address);
            }
        //Prevent the IFs from drawing after the last 2 positions were cleared, and the missile is off-screen.
        //if (inBounds(missile_array[i].x_pos, missile_array[i].y_pos) == false)  //If x and y not on screen, then don't draw!
        //    missile_array[i].in_bound = false;
        } else {
            notInFunction(pixel_buffer_address, 1); //DIAG
            notInFunction(pixel_buffer_address, 2);
        }
        
        //missile_array[i].in_bound = 2;
       
    }*/
}


//Currently 5 levels
int determine_num_missiles(int round_num, int *spawn_threshold) { //Returns max num_missiles allowed on screen at a time per round#.
    int num_missiles = 1;
    switch (round_num) {
        case 0:
            num_missiles = 5;
            *spawn_threshold = 3; //when there's 3 missiles left, can spawn more!
            break;
        case 1:
            num_missiles = 6;
            *spawn_threshold = 3;
            break;
        case 2:
            num_missiles = 7;
            *spawn_threshold = 3;
            break;
        case 3:
            num_missiles = 8;
            *spawn_threshold = 3;
            break;
        case 4:
            num_missiles = 9;
            *spawn_threshold = 3;
            break;
        case 5:
            num_missiles = 10;
            *spawn_threshold = 3;
            break;
        default:
            printf("Invalid level.\n");
            break;
    }
    //printf("Round: %d with num_missiles (on screen at a time): %d\n", round_num, num_missiles);
    //printf("spwan_threshold (if <= to this # missiles left on screen, then spwan more.): %d\n", *spawn_threshold);
    return num_missiles;
}


//This is moved to compute_missiles. - REDUNDANT
void add_missiles(int num_missiles, Missile *missile_array, short int *colour, volatile int pixel_buffer_address){ 
    //Add more missiles to the missile_array for the current round. Make sure to check entries only with in_bound = 2. 
    //Then set to 1 after!

    //This function should be called whenever missiles_on_screen_check returns true. (checks if num onscreen missiles is == certain 
    //threshold for spawning more)
    //This function should overwrite the missile entries that went out of bounds, for the current num_missiles allowed on screen for the curent round.

    //Depending on the round, some or all of the slots in the missile_array will be used.

    for (int i = 0; i < num_missiles; i++) {
        //check which missiles are not in bounds
        //replace these missiles with new ones! 
        //Use code from compute_missiles again. 
        //Could maybe make a subfunction to minimize redunadant code
        if (missile_array[i].in_bound == 2) {
            //Generate a new one!
        }

    }

}


bool missiles_on_screen_check(int *num_missiles, Missile *missile_array, int *spawn_threshold) { 
    //Check how many missiles are on screen right now. 
    //Return true if within threshold to draw more. Else, (i.e. still a lot on screen), return false.
    
    //Check how many missiles are on screen. 
        //If curr_missiles_screen < spawn_threshold then spawn more (i.e. call add_missiles)
        //to check curr_missiles_screen -> for loop to see which missiles have in_bound = 2.
    int curr_missiles_screen = *num_missiles; 
    for (int i = 0; i < *num_missiles; i++) {
        //Search the missile array for missiles that are off-screen (i.e. in_bound = 2)
        if (missile_array[i].in_bound == 2) {
            curr_missiles_screen -= 1; //subtract the ones off-screen
        }
    }

    //If there are few enough missiles on-screen - then spawn more
    if (curr_missiles_screen <= *spawn_threshold) {
        return true;
    } else {
        return false;
    }


}


bool inBounds(int x, int y) {
    if ((x >= 0 && x <= XMAX) && (y >= 0 && y <= YMAX))
        return true;
    else 
        return false;
}

//Diagnostic pixel in top left corner: flashes if in function you put this in. Like cout to say if you're in a certain function - visual debugging
void inFunction(volatile int pixel_buffer_address, int led_to_light) {
    short int colour[3] = {red, blue, green};
    draw_enemy_missile(0, 0, colour[rand() % 3], pixel_buffer_address); //keep flashing pixel until out of function.

    *led_ctrl_ptr = *led_ctrl_ptr | led_to_light; //Light up led_to_light - leave others alone
}

void notInFunction(volatile int pixel_buffer_address, int led_to_light) {
    draw_enemy_missile(0, 0, black, pixel_buffer_address); //black out

    *led_ctrl_ptr = 0; //Turn off ALL (ignore led_to_light)
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

double randDouble(double min, double max) {
    //srand (time(NULL));
    double range = max - min;
    double divisor = RAND_MAX / range; //
    return min + (rand() / divisor); 

}