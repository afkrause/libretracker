#pragma once

#include <string>

#include <Eigen/Eigen>

#include <opencv2/opencv.hpp>

#include "cv_button.h"

// draw keyboard buttons	
class Speller_canvas
{
protected:
	cv::Scalar flicker_col_0{   0,  55,   0 }; // dark green
	cv::Scalar flicker_col_1{ 100, 255, 100 }; // bright green
	std::array<int, 2> flicker_code_01{ 0,1 };
	std::array<int, 3> flicker_code_02{ 0,0,1 };
	std::array<int, 4> flicker_code_03{ 0,0,1,1 };
	std::array<int, 5> flicker_code_04{ 0,0,0,1,1 };
	size_t flicker_counter = 0;
public:

	// button labels
	
	//Eigen::Matrix<Button, 4, 7> buttons;
	//Eigen::Matrix<char, 4, 7> labels;

	Eigen::Matrix<Button, 3, 10> buttons;
	Eigen::Matrix<char, 3, 10> labels;

	std::string speller_str = "";

	void setup()
	{
		/*
		// nach häufigkeit in der deutschen sprache sortiert
		labels <<
			'E', 'N', 'I', 'S', 'R', 'A', 'T',
			'D', 'H', 'U', 'L', 'C', 'G', 'M',
			'O', 'B', 'W', 'F', 'K', 'Z', 'P',
			'V', 'J', 'Y', 'X', 'Q', '_', '<';
		*/

		// standard german keyboard layout
		labels <<
			'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P',
			'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '!',
			'Y', 'X', 'C', 'V', 'B', 'N', 'M', '_', '_', '<';

	}

	void draw_keyboard(cv::Mat& img, int x, int y, int w, int h, int marker_size, int mx, int my, bool& button_released);
	void draw_keyboard_ssvep(cv::Mat& img, int x, int y, int w, int h, int marker_size, int mx, int my, bool& button_released);

	void draw()
	{
		//draw_keyboard(cv::Mat& img, int x, int y, int w, int h, int mx, int my, Eigen::Matrix<Button, 3, 10>& buttons, const Eigen::Matrix<char, 3, 10>& labels, string& speller_str)
	}
};
