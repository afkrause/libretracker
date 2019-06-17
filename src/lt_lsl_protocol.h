#pragma once

// labstreaming layer sends data as arrays of e.g. floats or doubles. 
// for example, an EEG headset might send 20 doubles ( = number of EEG channels ) per frame.

// data structure for one eye. because the timestamp must be a double, the other values are coded as doubles as well, even if floats would be enough.
// currently, only monocular tracking
enum enum_eye_data
{
	LT_TIMESTAMP,
	LT_PUPIL_X, // pupil centre
	LT_PUPIL_Y,
	LT_GAZE_X, // gaze coordinates in scene camera koordinate system
	LT_GAZE_Y,
	LT_SCREEN_X, // gaze coordinates transformed to screen space using AR Markers
	LT_SCREEN_Y,  // set to NaN if AR-Marker Tracking is not working
	//LT_SCREEN_X_FILTERED, // jitter filter gaze coordinates in screen space
	//LT_SCREEN_Y_FILTERED, 
	LT_N_EYE_DATA
};


// TODO marker streaming

// TODO video streaming