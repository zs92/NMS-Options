//#include <iostream>

//#include <QtCore>
//#include <QString>

//#include "SHRectangle.h"
//#include "SHColor.h"
#include "SortByClass.h"
#include <math.h>

SH::DetectionRectangle readZBSAnnotationLine(const QString& annotationLine) {
	SH::DetectionRectangle result;

	QRegExp rx("\\<(.*)\\>");
	int pos = rx.indexIn(annotationLine);
	QString className = rx.capturedTexts().back();
	QString values = annotationLine.split(QRegExp("\\<.*\\>"), QString::SkipEmptyParts).back();
	QStringList valuesSplit = values.split(QRegExp("\\s+"), QString::SkipEmptyParts);

	if (rx.capturedTexts().size() != 2 ||
			annotationLine.split(QRegExp("\\<.*\\>"), QString::SkipEmptyParts).size() != 1 ||
			valuesSplit.size() < 4)
	{
		std::cout << __func__ << ": Error parsing annotationLine" << std::endl;
		return result;
	}

	result.className = className;
	result.x_c = valuesSplit.at(0).toFloat();
	result.y_c = valuesSplit.at(1).toFloat();
	result.width = valuesSplit.at(2).toFloat();
	result.height = valuesSplit.at(3).toFloat();
	if (valuesSplit.size() > 4) {
		result.prob = valuesSplit.at(4).toFloat();
	}

	return result;
}


std::vector<SH::DetectionRectangle> readZBSAnnotationsFile(QString ZBSAnnotationsFilePath) {
	std::vector<SH::DetectionRectangle> result;

	QFile ZBSAnnotationsFile(ZBSAnnotationsFilePath);
	if (!ZBSAnnotationsFile.open(QIODevice::ReadOnly)) {
		return result;
	}

	QTextStream inStream(&ZBSAnnotationsFile);
	while (!inStream.atEnd()) {
		QString annotationLine = inStream.readLine();
		SH::DetectionRectangle detection = readZBSAnnotationLine(annotationLine);
		if (!detection.className.isEmpty()) {
			result.push_back(detection);
		}
	}
	return result;
}


void writeZBSAnnotationsFile(std::vector<SH::DetectionRectangle> detections, QString ZBSAnnotationsFilePath) {
	if (ZBSAnnotationsFilePath.isEmpty()) {
		return;
	}

	QFile annotationFile(ZBSAnnotationsFilePath);
	if (!annotationFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return;
	}

	QTextStream zbsAnnotationsOutStream(&annotationFile);
	for (auto detection : detections) {
		zbsAnnotationsOutStream << "<" << detection.className << "> "
			<< detection.x_c << " "
			<< detection.y_c << " "
			<< detection.width << " "
			<< detection.height << " "
			<< detection.prob << "\n";
	}
}


void cvDrawGradientLine(cv::Mat& img, const cv::Point& start, const cv::Point& end,
												const cv::Scalar& c1, const cv::Scalar& c2, int thickness = 1)
{
	cv::LineIterator iter(img, start, end, cv::LINE_8);
	for (int i = 0; i < iter.count; i++, iter++) {
		if (iter.pos().x - thickness >= 0 &&
				iter.pos().y - thickness >= 0 &&
				iter.pos().x + thickness < img.cols &&
				iter.pos().y + thickness < img.rows)
		{
			double alpha = double(i) / iter.count;
			img(cv::Rect(iter.pos(), cv::Size(thickness, thickness))) = c1 * (1.0 - alpha) + c2 * alpha;
		}
	}
}


void drawKeypoints(cv::Mat& img_draw, std::vector<SH::DetectionRectangle> detections) {
	{ // Draw Rectangles
		cv::Scalar color;
		color = cv::Scalar(195, 195, 195, 255);

		// Draw all rectangles in vector
		for (auto rect : detections) {
			QString firstClassName = rect.className;
			if (firstClassName.contains("/")) { // Keypoint detection -- Draw "/" limb
				QStringList partsList = firstClassName.split('/');
				if (partsList.size() == 2) {
					cvDrawGradientLine(img_draw,
														 cv::Point(rect.getLeftBorder() * img_draw.cols, rect.getBottomBorder() * img_draw.rows),
														 cv::Point(rect.getRightBorder() * img_draw.cols, rect.getTopBorder() * img_draw.rows),
														 SH::makeColor(partsList[0].toInt()), SH::makeColor(partsList[1].toInt()), 2);
				}
			} else if (rect.className.contains("\\")) { // Keypoint detection -- Draw "\" limb
				QStringList partsList = firstClassName.split('\\');
				if (partsList.size() == 2) {
					cvDrawGradientLine(img_draw,
														 cv::Point(rect.getLeftBorder() * img_draw.cols, rect.getTopBorder() * img_draw.rows),
														 cv::Point(rect.getRightBorder() * img_draw.cols, rect.getBottomBorder() * img_draw.rows),
														 SH::makeColor(partsList[0].toInt()), SH::makeColor(partsList[1].toInt()), 2);
				}
			} else { // Draw bounding box
				cv::rectangle(img_draw,
											cv::Point(rect.getLeftBorder() * img_draw.cols, rect.getTopBorder() * img_draw.rows),
											cv::Point(rect.getRightBorder() * img_draw.cols, rect.getBottomBorder() * img_draw.rows),
											SH::makeColor(rect.className));
			}
		}
	}
}




int main() {
	// Example 1: Load Annotations
	auto detections = readZBSAnnotationsFile("E:/HiTD_SEFUGA-Data/Coco/test2017/000000000063.annotations"); //@Zhe: Path needs to be adjusted

	MergeBBox(detections, newdetections, 0.5);

	// Example 2: Write Annotations
	writeZBSAnnotationsFile(newdetections, "D:/test.annotations"); //@Zhe: Path needs to be adjusted

	// Example 3: Show image with annotations
	cv::Mat image = cv::imread("E:/HiTD_SEFUGA-Data/Coco/test2017/000000000063.jpg"); //@Zhe: Path needs to be adjusted
	drawKeypoints(image, detections);
	cv::Mat newimage = cv::imread("E:/HiTD_SEFUGA-Data/Coco/test2017/000000000063.jpg");
	drawKeypoints(newimage, newdetections);

	cv::imshow("image", image);
	cv::imshow("newimage", newimage);
	cv::waitKey(0);
}