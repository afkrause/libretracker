#pragma once
// simple opencv based imgui Button

class Button
{
private:

	bool is_inside(int x, int y, int w, int h, int mx, int my)
	{
		if ( mx >= x && my >= y && mx<= x+w && my <= y+h) { return true; }
		else { return false; }
		return false;
	}
	float was_inside = 0.0f;
	float was_triggered = 0.0f;
public:

	// returns true of button was triggered
	bool draw(cv::Mat& img, const int x, const int y, const int w, const int h, const int mx, const int my, bool& mouse_button_released, const std::string label)
	{
		using namespace cv;
		using namespace std;

		bool triggered = false;

		if (is_inside(x, y, w, h, mx, my))
		{
			//cout << "i";
			was_inside = 1.0f;
			if (mouse_button_released)
			{
				mouse_button_released = false; // consume the event
				triggered = true;
				was_triggered = 1.0f;
				cout << "button triggered" << endl;
			}
		}

		// fade color depending on states
		auto c = Scalar(0, 100+125*was_inside, 200 * was_triggered);


		const float fade_speed = 0.96;
		was_inside = fade_speed * was_inside;
		was_triggered = fade_speed * was_triggered;

		rectangle(img, Rect(x, y, w, h), c, CV_FILLED);
		const int fw = 15; // font width in pixel of FONT_HERSHEY_SIMPLEX
		const int fh = 15; // font width in pixel of FONT_HERSHEY_SIMPLEX
		int s = label.size();
		auto p = Point(x + 0.5*w - 0.5*s*fw, y + 0.5*h - 0.5*fh);
		putText(img, label, p, FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 4);


		return triggered;
	}
};