#include "bumper.h"

void init_bumper( struct Bumper* bumper ) {
	// Initialize all states to low/false
    bumper->flash_timer = clock();
    bumper->signal_timer = clock();
    // Bumper LED States
	bumper->outer_left = 0;
	bumper->inner_left = 0;
	bumper->inner_right = 0;
	bumper->outer_right = 0;
	// Brake related vars
    bumper->has_flashed = 0;
	bumper->brake_state = 0;
	bumper->flash_lock = 0;
	bumper->num_flashes = 0;
	// Turn signal vars
	bumper->need_to_signal = 0;
	bumper->left_lock = 0;
	bumper->right_lock = 0;
	bumper->signal = 0;
}


void brake( struct Bumper* bumper ) {
	if (bumper->brake_state) {
		if (!bumper->flash_lock) { bumper->inner_left  = bumper->inner_right = 1; }
		if (!bumper->left_lock)  { bumper->outer_left  = 1; }
		if (!bumper->right_lock) { bumper->outer_right = 1; }
	}
	else {
		bumper->inner_left = bumper->inner_right = 0;
		if (!bumper->left_lock)  { bumper->outer_left = 0; }
		if (!bumper->right_lock) { bumper->outer_right = 0; }
		bumper->num_flashes = 0;
	}
}


void brake_flash( struct Bumper* bumper ) {
    double diff = ((double) (clock() - bumper->flash_timer));
    if (diff > FLASH_TIME){
        bumper->flash_timer = clock();
        bumper->inner_left = bumper->inner_right = !(bumper->inner_left);
        bumper->num_flashes++;
        if (bumper->num_flashes > 4) {
            bumper->num_flashes = 0;
            bumper->flash_lock = 0;
            bumper->has_flashed = 1;
            bumper->flash_timer = clock();  // in case brakes need to flash again in < flash_timer
        }
    }
}


/*
 * 0000b = No Turn being signaled
 * 0001b = Left Turn to be Flashing
 * 0010b = Right turn to be Flashing
 * 0011b to 1101b = Reserved
 * 1110b = Error (to include both left and right selected simultaneously)
 * 1111b = Not available (do not change)
 */
void turn_signal_routine( struct Bumper* bumper ) {
    double diff = ((double) (clock() - bumper->signal_timer));
    if (diff > SIGNAL_TIME){
        if (bumper->signal == 0b1110){
            bumper->left_lock = bumper->right_lock = 1;
            bumper->outer_left = bumper->outer_right = !(bumper->outer_left);
            // Sync turn signals (left and right on reflect hazards)
            // This can unfortunately induce a 1 cycle delay on the left signal in the event
            // the right signal is already on, then left signal is turned on
        }
        else if (bumper->signal == 0b0001 ){
            bumper->left_lock = 1;
            bumper->right_lock = 0;
            bumper->outer_left = !(bumper->outer_left);
        }
        else if (bumper->signal == 0b0010){
            bumper->right_lock = 1;
            bumper->left_lock = 0;
            bumper->outer_right = !(bumper->outer_right);
        }
        else if (bumper->signal == 0b1111) {  }  // do nothing/continue behavior, node may be down
        else {
        	bumper->right_lock = bumper->left_lock = 0;
        }
        bumper->signal_timer = clock();
	}
}


void brake_routine( struct Bumper* bumper ) {
	if (bumper->flash_lock) {
		brake_flash(bumper);
	}
	brake(bumper);  // applies brakes based on state
	brake(bumper);  // applies brakes based on state
}
