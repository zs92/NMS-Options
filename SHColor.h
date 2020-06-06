#pragma once

#include <QString>

#include <opencv2/opencv.hpp>

namespace SH {

	typedef struct {
		float r; // [0..1]
		float g; // [0..1]
		float b; // [0..1]
	} RGB;

	typedef struct {
		float h; // angle in degrees
		float s; // [0..1]
		float v; // [0..1]
	} HSV;

	static HSV rgb2hsv(RGB in) {
		HSV out;
		float min, max, delta;
		min = in.r < in.g ? in.r : in.g;
		min = min < in.b ? min : in.b;
		max = in.r > in.g ? in.r : in.g;
		max = max > in.b ? max : in.b;
		out.v = max;
		delta = max - min;
		if (delta < 0.00001) {
			out.s = 0;
			out.h = 0; // undefined, maybe nan?
			return out;
		}
		if (max > 0.0) { // NOTE: if Max is == 0, this divide would cause a crash
			out.s = (delta / max); // s
		} else {
			// if max is 0, then r = g = b = 0
			// s = 0, v is undefined
			out.s = 0.0;
			out.h = NAN; // its now undefined
			return out;
		}
		if (in.r >= max) { // > is bogus, just keeps compilor happy
			out.h = (in.g - in.b) / delta; // between yellow & magenta
		} else {
			if (in.g >= max) {
				out.h = 2.0 + (in.b - in.r) / delta; // between cyan & yellow
			} else {
				out.h = 4.0 + (in.r - in.g) / delta; // between magenta & cyan
			}
		}

		out.h *= 60.0; // degrees

		if (out.h < 0.0) {
			out.h += 360.0;
		}

		return out;
	}


	static RGB hsv2rgb(HSV in) {
		RGB out;
		float hh, p, q, t, ff;
		long i;

		if (in.s <= 0.0) { // < is bogus, just keeps compilor happy
			out.r = in.v;
			out.g = in.v;
			out.b = in.v;
			return out;
		}

		hh = in.h;
		if (hh >= 360.0) hh = 0.0;
		hh /= 60.0;
		i = (long)hh;
		ff = hh - i;
		p = in.v * (1.0 - in.s);
		q = in.v * (1.0 - (in.s * ff));
		t = in.v * (1.0 - (in.s * (1.0 - ff)));

		switch (i) {
			case 0: {
				out.r = in.v;
				out.g = t;
				out.b = p;
			} break;
			case 1: {
				out.r = q;
				out.g = in.v;
				out.b = p;
			} break;
			case 2: {
				out.r = p;
				out.g = in.v;
				out.b = t;
			} break;
			case 3: {
				out.r = p;
				out.g = q;
				out.b = in.v;
			} break;
			case 4: {
				out.r = t;
				out.g = p;
				out.b = in.v;
			} break;
			case 5:
			default: {
				out.r = in.v;
				out.g = p;
				out.b = q;
			} break;
		}
		return out;
	}


	static cv::Scalar makeColor(int num, float saturation = 1.0, float value = 1.0, float scale = 255.f) {
		float colorHue = std::abs(std::fmod<float>(num * 65, 360.0));
		HSV hsvColor{ colorHue, saturation, value };
		RGB rgbColor = hsv2rgb(hsvColor);
		cv::Scalar result(scale * rgbColor.r, scale * rgbColor.g, scale * rgbColor.b, scale);
		return result;
	}


	static cv::Scalar makeColor(QString name, float saturation = 1.0, float value = 1.0, float scale = 255.f) {
		int number = 0; // Generate an integer number from name
		for (auto character : name.toLocal8Bit()) {
			number += (int)character;
		}
		return makeColor(number, saturation, value, scale);
	}

} // namespace SH
