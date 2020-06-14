#pragma once

#include <vector>
#include <string>
#include <limits>
#include <algorithm>
#include <opencv2/opencv.hpp>

// Rainer: Wenn dieses Symbol gesetzt ist, wird das Histogramm aus dem Tiefenbild
// berechnet, und das Disparitätsbild ist aus dem ImageStack-Objekt gelöscht.
#define USE_ONLY_DEPTH

namespace SH {

	// Rectangle class with relative (e.g. to image size) values
	// for width, height, x_c and y_c
	class Rectangle
	{
	public:

		float x_c = 0.5f;
		float y_c = 0.5f;
		float width = 1.f;
		float height = 1.f;

		float getLeftBorder() {
			return x_c - width / 2.f;
		}

		float getRightBorder() {
			return x_c + width / 2.f;
		}

		float getTopBorder() {
			return y_c - height / 2.f;
		}

		float getBottomBorder() {
			return y_c + height / 2.f;
		}

		float getArea() {
			cropToImageSize();
			return width*height;
			//return (getRightBorder() - getLeftBorder()) * (getBottomBorder() - getTopBorder());
		}

		void setCenter(float x, float y) {
			x_c = x;
			if (x_c + width / 2.f > 1.f) {
				x_c = 1 - width / 2.f;
			}
			if (x_c - width / 2.f < 0.f) {
				x_c = width / 2.f;
			}
			y_c = y;
			if (y_c + height / 2.f > 1.f) {
				y_c = 1 - height / 2.f;
			}
			if (y_c - height / 2.f < 0.f) {
				y_c = height / 2.f;
			}
		}

		void setLeftBorder(float value, bool doCropToImageSize = true) {
			moveLeftBorder(value - x_c + width / 2.f, doCropToImageSize);
			cropToImageSize(doCropToImageSize);
		}

		void setRightBorder(float value, bool doCropToImageSize = true) {
			moveRightBorder(value - x_c - width / 2.f, doCropToImageSize);
			cropToImageSize(doCropToImageSize);
		}

		void setTopBorder(float value, bool doCropToImageSize = true) {
			moveTopBorder(value - y_c + height / 2.f, doCropToImageSize);
			cropToImageSize(doCropToImageSize);
		}

		void setBottomBorder(float value, bool doCropToImageSize = true) {
			moveBottomBorder(value - y_c - height / 2.f, doCropToImageSize);
			cropToImageSize(doCropToImageSize);
		}

		void moveLeftBorder(float value, bool doCropToImageSize = true) {
			if (value > width) {
				x_c = x_c + width / 2.f;
				width = 0.f;
			} else {
				x_c += value / 2.f;
				width -= value;
			}
			cropToImageSize(doCropToImageSize);
		}

		void moveRightBorder(float value, bool doCropToImageSize = true) {
			if (-value > width) {
				x_c = x_c - width / 2.f;
				width = 0.f;
			} else {
				x_c += value / 2.f;
				width += value;
			}
			cropToImageSize(doCropToImageSize);
		}

		void moveTopBorder(float value, bool doCropToImageSize = true) {
			if (value > height) {
				y_c = y_c + height / 2.f;
				height = 0.f;
			} else {
				y_c += value / 2.f;
				height -= value;
			}
			cropToImageSize(doCropToImageSize);
		}

		void moveBottomBorder(float value, bool doCropToImageSize = true) {
			if (-value > height) {
				y_c = y_c - height / 2.f;
				height = 0.f;
			} else {
				y_c += value / 2.f;
				height += value;
			}
			cropToImageSize(doCropToImageSize);
		}

		void cropToImageSize(bool doCropToImageSize = true) {
			if (!doCropToImageSize) {
				return;
			}

			if (getLeftBorder() < 0.f) {
				setLeftBorder(0.f, false);
			}
			if (getRightBorder() > 1.f) {
				setRightBorder(1.f, false);
			}
			if (getTopBorder() < 0.f) {
				setTopBorder(0.f, false);
			}
			if (getBottomBorder() > 1.f) {
				setBottomBorder(1.f, false);
			}
		}

		bool isAtBorder(float margin = 0.001f) {
			cropToImageSize();
			if (getLeftBorder() <= 0.f + margin ||
					getRightBorder() >= 1.f - margin ||
					getTopBorder() <= 0.f + margin ||
					getBottomBorder() >= 1.f - margin)
			{
				return true;
			}

			return false;
		}

		// imWidth and imHeight are absulte pixel values
		cv::Rect getCvRect(int imWidth, int imHeight) {
			cv::Rect result;
			result.x = getLeftBorder() * imWidth;
			result.width = width * imWidth;
			result.y = getTopBorder() * imHeight;
			result.height = height * imHeight;
			if (!(0 <= result.x)) {
				result.x = 0;
			}
			if (!(0 <= result.width)) {
				result.width = 0;
			}
			if (!(result.x <= imWidth)) {
				result.x = imWidth;
			}
			if (!(result.x + result.width <= imWidth)) {
				result.width = imWidth - result.x;
			}
			if (!(0 <= result.y)) {
				result.y = 0;
			}
			if (!(0 <= result.height)) {
				result.height = 0;
			}
			if (!(result.y <= imHeight)) {
				result.y = imHeight;
			}
			if (!(result.y + result.height <= imHeight)) {
				result.height = imHeight - result.y;
			}
			return result;
		}

		cv::Rect getCvRect(const cv::Size& imSize) {
			return getCvRect(imSize.width, imSize.height);
		}

		cv::Mat getCvRoi(const cv::Mat& input) {
			return input(getCvRect(input.size()));
		}

		void scale(float scale_width, float scale_height) {
			width *= scale_width;
			height *= scale_height;
			cropToImageSize();
		}

		Rectangle getScaledRect(float scale_width, float scale_height) {
			Rectangle result = *this;
			result.scale(scale_width, scale_height);
			return result;
		}

		void setFromCvRect(const cv::Rect& rect, const cv::Size& imSize) {
			width = (float)rect.width / (float)imSize.width;
			height = (float)rect.height / (float)imSize.height;
			x_c = (float)(rect.x + rect.width / 2) / (float)imSize.width;
			y_c = (float)(rect.y + rect.height / 2) / (float)imSize.height;
			cropToImageSize();
		}

		//@Verify(epsilon might be required, see isRectSimilarTo)
		bool Rectangle::operator==(const Rectangle& rhs) {
			return this->x_c == rhs.x_c
				&& this->y_c == rhs.y_c
				&& this->width == rhs.width
				&& this->height == rhs.height;
		}

		bool Rectangle::operator!=(const Rectangle& rhs) {
			return !(*this == rhs);
		}

	};


	static float intersectionArea(Rectangle& d1, Rectangle& d2) {
		float left = (std::max)(d1.getLeftBorder(), d2.getLeftBorder());
		float right = (std::min)(d1.getRightBorder(), d2.getRightBorder());
		float top = (std::max)(d1.getTopBorder(), d2.getTopBorder());
		float bottom = (std::min)(d1.getBottomBorder(), d2.getBottomBorder());
		return (std::max)((right - left), 0.f) * (std::max)((bottom - top), 0.f);
	}


	static float unionArea(Rectangle& d1, Rectangle& d2) {
		return d1.getArea() + d2.getArea() - intersectionArea(d1, d2);
	}


	static float intersectionOverUnion(Rectangle& d1, Rectangle& d2) {
		float intersection_area = intersectionArea(d1, d2);
		float union_area = d1.getArea() + d2.getArea() - intersection_area;
		return intersection_area / union_area;
	}

	float distanceCenPoint(Rectangle& d1, Rectangle& d2){
		return pow((d1.x_c - d2.x_c) , 2) + pow((d1.y_c - d2.y_c) , 2);
	}

	float diagonal(Rectangle& d1, Rectangle& d2){
		float right = (std::max)(d1.getLeftBorder(), d2.getLeftBorder());
		float left = (std::min)(d1.getRightBorder(), d2.getRightBorder());
		float bottom = (std::max)(d1.getTopBorder(), d2.getTopBorder());
		float top = (std::min)(d1.getBottomBorder(), d2.getBottomBorder());
		float result = pow((left - right), 2) + pow((bottom - top), 2);
		return result;
	}

	float NonMaxSupp(Rectangle& d1, Rectangle& d2){
		result = intersectionOverUnion(d1, d2) - (distanceCenPoint(d1, d2)/diagonal(d1, d2));//Penalty function: R(diou).
		return result;
	}

	class DetectionRectangle : public Rectangle
	{
	public:

		QString className;
		float prob = 0.f;
	};


	bool isSameObj(DetectionRectangle& d1, DetectionRectangle& d2, float threshold){
		if(d1.className != d2.className){
			return false;
		}
		float NMS = SH::NonMaxSupp(d1, d2);
		if(NMS <= threshold){
			return true;
		}else{
			return false;
		}

	}	

} // namespace SH