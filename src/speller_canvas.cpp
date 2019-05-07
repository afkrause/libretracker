#include "speller_canvas.h"

// clip value x to range min..max
template<class T> inline T clip(T x, const T& min, const T& max)
{
	if (x < min)x = min;
	if (x > max)x = max;
	return x;
}

void Speller_canvas::draw_keyboard(cv::Mat& img, int x, int y, int w, int h, int marker_size, int mx, int my, bool& button_released)
{
	using namespace cv;
	const int s = 20; // spacing
	const int button_w = (w - s) / float(buttons.cols()) - s; // button width
	const int button_h = (h - s) / float(buttons.rows()) - s; // button height

	for (int i = 0; i < buttons.rows(); i++)
	{
		for (int k = 0; k < buttons.cols(); k++)
		{
			int button_x = x + s + k * (button_w + s);
			int button_y = y + s + i * (button_h + s);


			//if (buttons(i, k).draw(img_screen, mb + 10 + k * 110, mb + 10 + i * 110, 100, 100, mx_p, my_p, eye_button_up, std::string(1, labels(i, k))))
			bool pressed = buttons(i, k).draw(img, button_x, button_y, button_w, button_h, mx, my, button_released, std::string(1, labels(i, k)));
			if (pressed)
			{
				//if (labels(i, k) == '_') { speller_str += " "; }
				if (labels(i, k) == '<') { if (speller_str.size() > 0) { speller_str.pop_back(); } }
				else { speller_str += labels(i, k); }

			}
		}
	}

	cv::putText(img, speller_str.c_str(), Point(marker_size + 30, marker_size - 30), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 0, 255), 4);

	// put text close to gaze point !
	cv::putText(img, speller_str.c_str(), Point(mx - 20 * speller_str.size(), my + 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 100, 100), 3);
}


void Speller_canvas::draw_keyboard_ssvep(cv::Mat& img, int x, int y, int w, int h, int marker_size, int mx, int my, bool& button_released)
{
	
	using namespace cv;
	const int s = 20; // spacing
	const int button_w = (w - s) / float(buttons.cols()) - s; // button width
	const int button_h = (h - s) / float(buttons.rows()) - s; // button height


	auto flicker_color_01 = flicker_code_01[flicker_counter % flicker_code_01.size()] ? flicker_col_0 : flicker_col_1;
	auto flicker_color_02 = flicker_code_02[flicker_counter % flicker_code_02.size()] ? flicker_col_0 : flicker_col_1;
	auto flicker_color_03 = flicker_code_03[flicker_counter % flicker_code_03.size()] ? flicker_col_0 : flicker_col_1;
	auto flicker_color_04 = flicker_code_04[flicker_counter % flicker_code_04.size()] ? flicker_col_0 : flicker_col_1;

	flicker_counter++;
	

	// flicker block coordinates
	static int fx = -10, fy = -10;
	
	for (int i = 0; i < buttons.rows(); i++)
	{
		for (int k = 0; k < buttons.cols(); k++)
		{

			int button_x = x + s + k * (button_w + s);
			int button_y = y + s + i * (button_h + s);
			
			if (is_inside(button_x, button_y, button_w, button_h, mx, my))
			{
				// start coordinates of 2x2 flicker block
				fx = clip<int>(2 * floor(0.5f * k), 0, buttons.cols() - 2);
				fy = clip<int>(2 * floor(0.5f * i), 0, buttons.rows() - 2);
			}

			Scalar button_col1(0,100,0);
			if (k == fx + 0 && i == fy + 0) { button_col1 = flicker_color_01; }
			if (k == fx + 1 && i == fy + 0) { button_col1 = flicker_color_02; }
			if (k == fx + 1 && i == fy + 1) { button_col1 = flicker_color_03; }
			if (k == fx + 0 && i == fy + 1) { button_col1 = flicker_color_04; }

			// hoover col 
			Scalar button_col2 = Scalar_<uchar>(button_col1 * 1.5f);

			bool pressed = buttons(i, k).draw(img, button_x, button_y, button_w, button_h, mx, my, button_released, std::string(1, labels(i, k)), button_col1, button_col2);
			if (pressed)
			{
				//if (labels(i, k) == '_') { speller_str += " "; }
				if (labels(i, k) == '<') { if (speller_str.size() > 0) { speller_str.pop_back(); } }
				else { speller_str += labels(i, k); }

			}
		}
	}
	cv::putText(img, speller_str.c_str(), Point(marker_size + 30, marker_size - 30), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 0, 255), 4);

	// put text close to gaze point !
	cv::putText(img, speller_str.c_str(), Point(mx - 20 * speller_str.size(), my + 20), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 100, 100), 3);
}

