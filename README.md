You may have to capture the keys by first selecting the “UP_ARROW” from the drop down in CPULator, selecting send for “Make”, then send for “Break.” Then, clicking the “Type Here” box

Overview:
____________Graphics___________

-----------------Main Player-----------------
- 5 missile firing locations (5 cities at bottom of screen)
- Fires defense bullets/missiles
- WASD to move cursor to fire missiles
- Spacebar to fire missile
- Defensive missiles create an explosion animation at the location of impact (mouse cursor location)

------------Enemies and Missiles----------
- Enemies - randomizes spawn locations at the top of the screen
- Can set target location and maximum velocities in the code
- Graphics: on collision, random coloured growing circles/"explosions" appear

----------Missile Animation------
- Random velocities - maybe slower velocities for earlier/easier Rounds
- Maybe aim these velocities such that they will hit the player/come close to hitting the player
- Can use the equation of a line (be aware that +y is down, -y is up. screen limits...etc...)


____________Gameplay Logic____________
- Generate random enemy missiles with random velocities/locations at top of screen, in general direction of cities (one of the 5 cities at the bottom of the screen)
- The maximum enemy missile velocity increases every 5 times a wave of enemy missiles is spawned (waves are spawned when a spawn_threshold is reached. In this case, we have the code set so that when at most 3 missiles or less are remaining on the screen, then more missiles will spawn - with a maximum of 5 missiles on-screen at a time in the current version of code.

--------Collision Detection------
- Use the explosion animation to check if a collision occurs (i.e. read the colours of pixels at the location of an enemy missile. If the defense missile hits
- Be checking for collisions constantly - or as often as possible?

--------------Rounds----------------
- The maximum velocity of the missiles increases after every 5 waves, as explained above

--------Scoring System and Reset-----
- Increment score when hit an enemy missile

___________Audio______________

---------Sound Effects-----------
- On Collision, triggers a saw wave, which comes from the MIC FIFO (which is not actually reading from a microphone - CPULator is playing back a sawwave)
