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
	std::string ImgPath = "C:/Data/NMS_test/000000000063.jpg";
	QString AnnotationsPath ="C:/Data/NMS_test/000000000063.annotations" ;
	//File:000000000785, 000000004395,000000006471,000000000063
	// Example 1: Load Annotations
	//auto detections = readZBSAnnotationsFile("C:/Data/CocoAnnotations/test2017/000000000016.annotations"); //@Zhe: Path needs to be adjusted
	auto detections = readZBSAnnotationsFile(AnnotationsPath);
	std::vector<SH::DetectionRectangle> newdetections, comparedetections;
	// Example 2: Write Annotations
	//writeZBSAnnotationsFile(detections, "D:/test.annotations"); //@Zhe: Path needs to be adjusted
	int flag = 2;
	float NMS_Threshlod = 0.1;
	int flagOfNMS = 3;
	float k1 = 1.35;
	MergeBBox(detections, newdetections, NMS_Threshlod, flag, flagOfNMS, k1);
	MergeBBox(detections, comparedetections, NMS_Threshlod, 1, flagOfNMS, k1);

	// Example 3: Show image with annotations
	//cv::Mat image = cv::imread("C:/Data/CocoAnnotations/test2017/000000000016.jpg"); //@Zhe: Path needs to be adjusted
	writeZBSAnnotationsFile(newdetections, "C:/Data/NMS_test/test.annotations"); //@Zhe: Path needs to be adjusted

	cv::Mat image = cv::imread(ImgPath); 
	cv::Mat compareimage = cv::imread(ImgPath);
	cv::Mat newimage = cv::imread(ImgPath);

	drawKeypoints(image, detections);
	drawKeypoints(compareimage, comparedetections);
	drawKeypoints(newimage, newdetections);


	cv::Mat honcat, output;
	cv::hconcat(image, compareimage, output);
	cv::hconcat(output, newimage, honcat);

	cv::imshow("honcat", honcat);

	cv::waitKey(0);
}
