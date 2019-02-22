#pragma once

#include <string>

#include <Eigen/Eigen>

#include "cv_button.h"

// draw keyboard buttons	
class Speller_canvas
{
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

	void draw_keyboard(cv::Mat& img, int x, int y, int w, int h, int marker_size, int mx, int my, bool& button_released)
	{
		using namespace cv;
		const int s = 20; // spacing
		const int button_w = (w - s) / float(buttons.cols()) - s; // button width
		const int button_h = (h - s) / float(buttons.rows()) - s; // button height

		for (int i = 0; i < buttons.rows(); i++)
		{
			for (int k = 0; k < buttons.cols(); k++)
			{
				//if (buttons(i, k).draw(img_screen, mb + 10 + k * 110, mb + 10 + i * 110, 100, 100, mx_p, my_p, eye_button_up, std::string(1, labels(i, k))))
				if (buttons(i, k).draw(img, x + s + k * (button_w + s), y + s + i * (button_h + s), button_w, button_h, mx, my, button_released, std::string(1, labels(i, k))))
				{
					//if (labels(i, k) == '_') { speller_str += " "; }
					if (labels(i, k) == '<') { if (speller_str.size() > 0) { speller_str.pop_back(); } }
					else { speller_str += labels(i, k); }

				}
			}
		}
		putText(img, speller_str.c_str(), Point(marker_size + 30, marker_size - 30), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 0, 255), 4);

		// put text close to gaze point !
		putText(img, speller_str.c_str(), Point(mx - 20 * speller_str.size(), my + 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 100, 100), 3);
	}

	void draw()
	{
		//draw_keyboard(cv::Mat& img, int x, int y, int w, int h, int mx, int my, Eigen::Matrix<Button, 3, 10>& buttons, const Eigen::Matrix<char, 3, 10>& labels, string& speller_str)
	}
};
